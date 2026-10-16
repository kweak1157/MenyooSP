$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation = & $locator -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $installation) { throw 'Visual Studio C++ build tools were not found.' }
$developerCommand = Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
$buildDirectory = Join-Path $repoRoot 'Solution\source\_Build\bin\Tests'
New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null

$source = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'Solution\source\Scripting\enums.h')
$body = [regex]::Match($source, '(?s)enum PedHash : DWORD\s*\{(.*?)\};').Groups[1].Value
$names = @([regex]::Matches($body, '(?m)^\s*(\w+)\s*=') | ForEach-Object { 'PedHash::' + $_.Groups[1].Value + ',' })
if ($names.Count -eq 0) { throw 'Stock ped enum was not found.' }
Set-Content -LiteralPath (Join-Path $buildDirectory 'StockPedCatalogue.inl') -Value $names -Encoding utf8

Push-Location -LiteralPath $buildDirectory
try {
    $command = 'call "{0}" -no_logo -arch=x64 -host_arch=x64 >nul && cl /nologo /std:c++20 /EHsc /W3 /WX /I "{1}\Solution\source" /I "{2}" "{3}\PedSelector.cpp" "{1}\Solution\source\Scripting\AddonVehicleModels.cpp" /Fe:PedSelector.exe' -f $developerCommand, $repoRoot, $buildDirectory, $PSScriptRoot
    & $env:ComSpec /d /c $command
    if ($LASTEXITCODE -ne 0) { throw 'Ped selector test compilation failed.' }
    & '.\PedSelector.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Ped selector checks failed.' }
}
finally { Pop-Location }
