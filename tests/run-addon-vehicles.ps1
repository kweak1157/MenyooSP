$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation = & $locator -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $installation) { throw 'Visual Studio C++ build tools were not found.' }

$developerCommand = Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
$buildDirectory = Join-Path $repoRoot 'Solution\source\_Build\bin\Tests'
New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null

# Compile the production filter with deterministic model availability responses.
$source = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'Solution\source\Scripting\ModelNames.cpp')
$signature = 'static void PopulateAddonVehicleModelsArray()'
$start = $source.IndexOf($signature)
if ($start -lt 0) { throw 'Add-on vehicle filter was not found.' }
$openingBrace = $source.IndexOf('{', $start + $signature.Length)
$depth = 1
$end = $openingBrace + 1
while ($depth -gt 0 -and $end -lt $source.Length) {
    if ($source[$end] -eq '{') { ++$depth }
    elseif ($source[$end] -eq '}') { --$depth }
    ++$end
}
if ($depth -ne 0) { throw 'Add-on vehicle filter body is incomplete.' }
Set-Content -LiteralPath (Join-Path $buildDirectory 'AddonVehicleProduction.inl') -Value $source.Substring($start, $end - $start) -Encoding utf8

# Read every official model from the enum, independently of the filter catalogue.
$enumSource = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'Solution\source\Scripting\enums.h')
$enumBody = [regex]::Match($enumSource, '(?s)enum VehicleHash : DWORD\s*\{(.*?)\};').Groups[1].Value
$modelNames = @([regex]::Matches($enumBody, '\bVEHICLE_[A-Z0-9_]+(?=\s*=)') | ForEach-Object { $_.Value + ',' })
if ($modelNames.Count -eq 0) { throw 'Official vehicle enum is empty or was not found.' }
Set-Content -LiteralPath (Join-Path $buildDirectory 'StockVehicleCatalogue.inl') -Value $modelNames -Encoding utf8

Push-Location -LiteralPath $buildDirectory
try {
    $command = 'call "{0}" -no_logo -arch=x64 -host_arch=x64 >nul && cl /nologo /std:c++20 /EHsc /W3 /WX /I "{1}\Solution\source" /I "{2}" "{3}\AddonVehicles.cpp" "{1}\Solution\source\Scripting\AddonVehicleModels.cpp" /Fe:AddonVehicles.exe' -f $developerCommand, $repoRoot, $buildDirectory, $PSScriptRoot
    & $env:ComSpec /d /c $command
    if ($LASTEXITCODE -ne 0) { throw 'Add-on vehicle test compilation failed.' }
    & '.\AddonVehicles.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Add-on vehicle checks failed.' }
}
finally {
    Pop-Location
}
