param([switch]$Interactive, [switch]$MemoryChecks)
$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot

$compilerCommand = Get-Command g++ -ErrorAction SilentlyContinue
if ($compilerCommand) {
    $compilerPath = $compilerCommand.Source
} elseif (Test-Path -LiteralPath 'C:\Qt\Tools\mingw1310_64\bin\g++.exe') {
    $compilerPath = 'C:\Qt\Tools\mingw1310_64\bin\g++.exe'
} else {
    throw 'g++ was not found. Add your C++ compiler bin folder to PATH.'
}

$coreSources = @('Player.cpp', 'Teams.cpp', 'Roles.cpp', 'World.cpp', 'Event.cpp',
                 'Command.cpp', 'Capabilities.cpp', 'Console.cpp')
New-Item -ItemType Directory -Path build/final -Force | Out-Null
$previousPath = $env:PATH
try {
    $env:PATH = (Split-Path -Parent $compilerPath) + ';' + $previousPath
    & $compilerPath -std=c++17 -Wall -Wextra -Wpedantic -Werror -I. @coreSources main.cpp -o build/simworld.exe
    if ($LASTEXITCODE -ne 0) { throw 'Application build failed.' }
    'Final GCC verification' | Set-Content -Encoding Unicode build/final/gcc-tests.txt
    foreach ($milestone in 1..9) {
        $source = 'tests/m' + $milestone + '_tests.cpp'
        $binary = 'build/final/m' + $milestone + '_tests.exe'
        $sources = @($source)
        if ($milestone -ne 2) { $sources = $coreSources + $sources }
        & $compilerPath -std=c++17 -Wall -Wextra -Wpedantic -Werror -I. @sources -o $binary
        if ($LASTEXITCODE -ne 0) { throw "M$milestone test build failed." }
        $results = & ('.\' + $binary)
        $testExit = $LASTEXITCODE
        $results | Tee-Object -FilePath build/final/gcc-tests.txt -Append
        if ($testExit -ne 0) { throw "M$milestone tests failed." }
    }

    $demo = & .\build\simworld.exe --demo
    if ($LASTEXITCODE -ne 0) { throw 'Application demonstration failed.' }
    $demo | Set-Content build/final/demo.txt
    $demoText = $demo -join [Environment]::NewLine
    foreach ($expected in @('Result: Villagers win', 'Turns completed: 5', 'Rounds completed: 2',
                            'Ballots cast: 12', 'Weighted vote total: 14', 'Eliminations: 6')) {
        if (!$demoText.Contains($expected)) { throw "Unexpected demo report: $expected" }
    }
    $playthrough = @('5', '1000', '14', '4', '0') | & .\build\simworld.exe
    if ($LASTEXITCODE -ne 0) { throw 'Console playthrough failed.' }
    $playthrough | Set-Content build/final/playthrough.txt
    $playthroughText = $playthrough -join [Environment]::NewLine
    foreach ($expected in @('Result: Wolves win', 'Turns completed: 7', 'Rounds completed: 3',
                            'Ballots cast: 17', 'Weighted vote total: 20', 'Eliminations: 6',
                            'Game already finished: Wolves win.')) {
        if (!$playthroughText.Contains($expected)) { throw "Unexpected console report: $expected" }
    }
    Write-Output 'Both application playthroughs matched their expected reports.'
    if ($MemoryChecks) {
        & (Join-Path $PSScriptRoot 'verify_memory.ps1')
        if (!$?) { throw 'Windows memory verification failed.' }
    }

    # Only root production sources: no executables, test mains, or build folders.
    $sourceFiles = Get-ChildItem -LiteralPath $PSScriptRoot -File |
        Where-Object { $_.Extension -in @('.h', '.cpp') } |
        Select-Object -ExpandProperty FullName
    Compress-Archive -LiteralPath $sourceFiles -DestinationPath build/Team_900244034_900244346.zip -Force
    Write-Output 'Source archive: build/Team_900244034_900244346.zip'
    if ($Interactive) {
        & .\build\simworld.exe
        if ($LASTEXITCODE -ne 0) { throw 'Application failed.' }
    }
} finally {
    $env:PATH = $previousPath
}
