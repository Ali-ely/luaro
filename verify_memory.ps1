$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot
New-Item -ItemType Directory -Path build/final -Force | Out-Null

$setupPath = $null
if (!(Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (!(Test-Path -LiteralPath $vswhere)) {
        throw 'Run from a Visual Studio x64 Native Tools prompt or install the C++ build tools.'
    }
    $installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$installation) { throw 'Visual Studio C++ build tools were not found.' }
    $setupPath = Join-Path $installation 'VC\Auxiliary\Build\vcvars64.bat'
}

$commands = @('@echo off')
if ($setupPath) {
    $commands += 'call "' + $setupPath + '" >nul'
    $commands += 'if errorlevel 1 exit /b 1'
}
$coreSources = 'Player.cpp Teams.cpp Roles.cpp World.cpp Event.cpp Command.cpp Capabilities.cpp Console.cpp'
foreach ($milestone in 1..9) {
    $testSource = 'tests\m' + $milestone + '_tests.cpp'
    $sources = $testSource
    if ($milestone -ne 2) { $sources = $coreSources + ' ' + $testSource }
    foreach ($mode in @('asan', 'heap')) {
        $folder = 'build\final\' + $mode + '_m' + $milestone
        New-Item -ItemType Directory -Path $folder -Force | Out-Null
        $binary = 'build\final\m' + $milestone + '_' + $mode + '.exe'
        $options = '/MD /Zi /fsanitize=address'
        if ($mode -eq 'heap') { $options = '/MDd /D_DEBUG' }
        $commands += 'cl /nologo /std:c++17 /EHsc /W4 /WX /I. ' + $options + ' /Fo:' + $folder + '\ /Fd:' + $folder + '\tests.pdb /Fe:' + $binary + ' ' + $sources
        $commands += 'if errorlevel 1 exit /b 1'
        $commands += $binary
        $commands += 'if errorlevel 1 exit /b 1'
    }
}
$batchPath = Join-Path $PSScriptRoot 'build/final/verify_memory.cmd'
[System.IO.File]::WriteAllLines($batchPath, $commands)
& $batchPath 2>&1 | Tee-Object -FilePath build/final/windows-memory.txt
if ($LASTEXITCODE -ne 0) { throw 'AddressSanitizer or debug-heap checks failed.' }
