$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation = & $locator -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$developerCommand = Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
$buildDirectory = Join-Path $repoRoot 'Solution\source\_Build\bin\Tests'
New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null
$source = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'Solution\source\Submenus\PedSelector.cpp')
$source = [regex]::Replace($source, '(?m)^#include[^\r\n]*', '')
Set-Content -LiteralPath (Join-Path $buildDirectory 'PedSelectorPerformanceProduction.inl') -Value $source -Encoding utf8
Push-Location -LiteralPath $buildDirectory
try {
    $command = 'call "{0}" -no_logo -arch=x64 -host_arch=x64 >nul && cl /nologo /std:c++20 /EHsc /W3 /WX /I "{1}\Solution\source" /I "{2}" "{3}\PedSelectorPerformance.cpp" /Fe:PedSelectorPerformance.exe' -f $developerCommand, $repoRoot, $buildDirectory, $PSScriptRoot
    & $env:ComSpec /d /c $command
    if ($LASTEXITCODE -ne 0) { throw 'Ped selector performance test compilation failed.' }
    & '.\PedSelectorPerformance.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Ped selector performance checks failed.' }
}
finally { Pop-Location }
