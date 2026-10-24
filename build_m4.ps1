param([switch]$Interactive)
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

New-Item -ItemType Directory -Path build -Force | Out-Null
$previousPath = $env:PATH
try {
    $env:PATH = (Split-Path -Parent $compilerPath) + ';' + $previousPath
    & $compilerPath -std=c++17 -Wall -Wextra -Wpedantic -Werror -I. Player.cpp Teams.cpp Roles.cpp World.cpp Event.cpp Command.cpp tests/m4_tests.cpp -o build/m4_tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'The M4 test build failed.' }
    & $compilerPath -std=c++17 -Wall -Wextra -Wpedantic -Werror -I. Player.cpp Teams.cpp Roles.cpp World.cpp Event.cpp Command.cpp main.cpp -o build/simworld.exe
    if ($LASTEXITCODE -ne 0) { throw 'The M4 demonstration build failed.' }
    & .\build\m4_tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'M4 tests failed.' }
    if ($Interactive) { & .\build\simworld.exe } else { & .\build\simworld.exe --demo }
    if ($LASTEXITCODE -ne 0) { throw 'The M4 demonstration failed.' }
} finally {
    $env:PATH = $previousPath
}
