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
    & $compilerPath -std=c++17 -Wall -Wextra -Wpedantic -Werror -I. tests/m2_tests.cpp -o build/m2_tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'The M2 test build failed.' }
    & $compilerPath -std=c++17 -Wall -Wextra -Wpedantic -Werror -I. examples/m2_demo.cpp -o build/m2_demo.exe
    if ($LASTEXITCODE -ne 0) { throw 'The M2 demo build failed.' }
    & .\build\m2_tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'M2 tests failed.' }
    & .\build\m2_demo.exe
    if ($LASTEXITCODE -ne 0) { throw 'The M2 demonstration failed.' }
} finally {
    $env:PATH = $previousPath
}
