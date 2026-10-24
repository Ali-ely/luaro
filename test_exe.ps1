param(
    [string]$ExePath = (Join-Path $PSScriptRoot 'build/simworld.exe'),
    [string]$ReportDirectory = (Join-Path $PSScriptRoot 'build/exe-tests'),
    [ValidateRange(1, 120)][int]$TimeoutSeconds = 10
)

$ErrorActionPreference = 'Stop'
if (!(Test-Path -LiteralPath $ExePath -PathType Leaf)) {
    throw "Executable not found: $ExePath. Run build_m10.ps1 first, or supply -ExePath."
}
$resolvedExe = (Resolve-Path -LiteralPath $ExePath).Path
New-Item -ItemType Directory -Path $ReportDirectory -Force | Out-Null
$resolvedReports = (Resolve-Path -LiteralPath $ReportDirectory).Path
$previousPath = $env:PATH
$compilerCommand = Get-Command g++ -ErrorAction SilentlyContinue
if ($compilerCommand) {
    $env:PATH = (Split-Path -Parent $compilerCommand.Source) + ';' + $previousPath
} elseif (Test-Path -LiteralPath 'C:\Qt\Tools\mingw1310_64\bin') {
    $env:PATH = 'C:\Qt\Tools\mingw1310_64\bin;' + $previousPath
}

function Test-ExecutableCase {
    param(
        [string]$Name,
        [string[]]$InputLines = @(),
        [string]$Arguments = '',
        [int]$ExpectedExit = 0,
        [string[]]$Expected = @(),
        [string[]]$Absent = @(),
        [string]$ExpectedError = ''
    )
    $inputText = ''
    if ($InputLines.Count -gt 0) { $inputText = ($InputLines -join "`n") + "`n" }
    $inputText | Set-Content -LiteralPath (Join-Path $resolvedReports "$Name.input.txt") -Encoding UTF8
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo.FileName = $resolvedExe
    $process.StartInfo.Arguments = $Arguments
    $process.StartInfo.WorkingDirectory = Split-Path -Parent $resolvedExe
    $process.StartInfo.UseShellExecute = $false
    $process.StartInfo.CreateNoWindow = $true
    $process.StartInfo.RedirectStandardInput = $true
    $process.StartInfo.RedirectStandardOutput = $true
    $process.StartInfo.RedirectStandardError = $true
    try {
        if (!$process.Start()) { throw 'Could not start executable.' }
        $stdoutTask = $process.StandardOutput.ReadToEndAsync()
        $stderrTask = $process.StandardError.ReadToEndAsync()
        try {
            $process.StandardInput.Write($inputText)
        } catch [System.IO.IOException] {
        } finally {
            $process.StandardInput.Close()
        }
        if (!$process.WaitForExit($TimeoutSeconds * 1000)) {
            $process.Kill()
            $process.WaitForExit()
            throw "Timed out after $TimeoutSeconds seconds."
        }
        $stdout = $stdoutTask.GetAwaiter().GetResult()
        $stderr = $stderrTask.GetAwaiter().GetResult()
        $stdout | Set-Content -LiteralPath (Join-Path $resolvedReports "$Name.stdout.txt") -Encoding UTF8
        $stderr | Set-Content -LiteralPath (Join-Path $resolvedReports "$Name.stderr.txt") -Encoding UTF8
        if ($process.ExitCode -ne $ExpectedExit) {
            throw "Exit code $($process.ExitCode); expected $ExpectedExit. See $Name.stderr.txt."
        }
        if ($ExpectedError) {
            if (!$stderr.Contains($ExpectedError)) { throw "Missing error: $ExpectedError" }
        } elseif ($stderr.Length -gt 0) {
            throw "Unexpected standard error. See $Name.stderr.txt."
        }
        foreach ($text in $Expected) {
            if (!$stdout.Contains($text)) { throw "Missing output: $text" }
        }
        foreach ($text in $Absent) {
            if ($stdout.Contains($text)) { throw "Unexpected output: $text" }
        }
    } finally {
        $process.Dispose()
    }
}

$cases = @(
    @{ Name = 'startup-and-quit'; InputLines = @('1', '0'); Expected = @(
        'Next phase: Night | Turns: 0 | Rounds: 0', 'Undoable edits: 0',
        'ID 1 | Clara | Witch', 'ID 6 | Farah | AlphaWolf', 'ID 8 | Hana | Villager', 'Goodbye.') },
    @{ Name = 'invalid-input'; InputLines = @('abc', '15', '-1', '1 2', '2147483648', '0');
       Expected = @('Enter one whole number from 0 to 14.', 'Goodbye.'); Absent = @('Turn 1 -') },
    @{ Name = 'management-and-undo'; InputLines = @(
        '6', '2', '2', 'Test Seer', '1', '0', '8', '9', '9', '10', 'Edited copy', '0', '0',
        '7', '10', '10', '13', '10', '10', '13', '10', '10', '10', '13', '9', '0');
       Expected = @('Created player 9.', 'Duplicate created with ID 10.', 'Modified player 10.',
        'Deleted player 10.', 'ID 10 | Edited copy | Seer | Village | dead',
        'ID 10 | Test Seer | Seer | Village | alive', 'Unknown player ID.', 'Undoable edits: 0') },
    @{ Name = 'capabilities-and-events'; InputLines = @(
        '11', '3', '5', '12', '3', '5', '1', '11', '2', '5', '12', '1', '5', '2',
        '2', '4', '13', '2', '13', '5', '0');
       Expected = @('This player cannot inspect.', 'This player cannot use potions.',
        'Special action queued.', 'RevealEvent source 2 -> target 5', 'Player 5 died by poison',
        '[5:Werewolf]', 'ID 5 | Elias | Werewolf | Wolves | dead') },
    @{ Name = 'single-turn'; InputLines = @('4', '14', '0'); Expected = @(
        'Turn 1 - Night', 'Next phase: Day | Turns: 1 | Rounds: 0',
        'Turns completed: 1', 'Rounds completed: 0', 'Result: In progress') },
    @{ Name = 'wolf-victory'; InputLines = @('5', '1000', '14', '4', '0'); Expected = @(
        'Result: Wolves win', 'Turns completed: 7', 'Rounds completed: 3', 'Ballots cast: 17',
        'Weighted vote total: 20', 'Eliminations: 6', 'Completed 7 turn(s).',
        'Game already finished: Wolves win.'); Absent = @('Turn 8 -') },
    @{ Name = 'village-demo'; Arguments = '--demo'; Expected = @(
        'Result: Villagers win', 'Turns completed: 5', 'Rounds completed: 2',
        'Ballots cast: 12', 'Weighted vote total: 14', 'Eliminations: 6', 'Goodbye.') },
    @{ Name = 'empty-input'; Expected = @('End of input. Goodbye.'); Absent = @('Turn 1 -') },
    @{ Name = 'incomplete-create'; InputLines = @('6', '2', '2', 'Incomplete player');
       Expected = @('End of input. Goodbye.'); Absent = @('Created player', 'Action failed:') },
    @{ Name = 'unknown-argument'; Arguments = '--invalid'; ExpectedExit = 1;
       ExpectedError = 'Usage: simworld [--demo]'; Absent = @('Loup-Garou SimWorld') },
    @{ Name = 'extra-argument'; Arguments = '--demo extra'; ExpectedExit = 1;
       ExpectedError = 'Usage: simworld [--demo]'; Absent = @('Loup-Garou SimWorld') }
)

$failures = 0
$summary = @("Executable: $resolvedExe")
try {
    foreach ($case in $cases) {
        try {
            Test-ExecutableCase @case
            $line = "PASS $($case.Name)"
        } catch {
            ++$failures
            $line = "FAIL $($case.Name): $($_.Exception.Message)"
        }
        Write-Output $line
        $summary += $line
    }
    $result = "$($cases.Count) executable tests, $failures failures."
    $summary += $result
    $summary | Set-Content -LiteralPath (Join-Path $resolvedReports 'summary.txt') -Encoding UTF8
    Write-Output $result
    Write-Output "Reports: $resolvedReports"
    if ($failures -gt 0) { throw 'Executable verification failed.' }
} finally {
    $env:PATH = $previousPath
}
