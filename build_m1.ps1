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
    & $compilerPath -std=c++17 -Wall -Wextra -Wpedantic -Werror -I. Player.cpp Teams.cpp Roles.cpp World.cpp examples/m1_demo.cpp -o build/m1_demo.exe
    if ($LASTEXITCODE -ne 0) { throw 'The demo build failed.' }
    & $compilerPath -std=c++17 -Wall -Wextra -Wpedantic -Werror -I. Player.cpp Teams.cpp Roles.cpp World.cpp tests/m1_tests.cpp -o build/m1_tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'The test build failed.' }
    & .\build\m1_tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'M1 tests failed.' }
    & .\build\m1_demo.exe
    if ($LASTEXITCODE -ne 0) { throw 'The demonstration failed.' }
} finally {
    $env:PATH = $previousPath
}
