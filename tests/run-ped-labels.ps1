$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation = & $locator -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$developerCommand = Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
$buildDirectory = Join-Path $repoRoot 'Solution\source\_Build\bin\Tests'
New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null
function Get-FunctionBody([string] $source, [string] $signature) {
    $start = $source.IndexOf($signature)
    if ($start -lt 0) { throw "Function was not found: $signature" }
    $openingBrace = $source.IndexOf('{', $start + $signature.Length)
    $depth = 1
    $end = $openingBrace + 1
    while ($depth -gt 0 -and $end -lt $source.Length) {
        if ($source[$end] -eq '{') { ++$depth }
        elseif ($source[$end] -eq '}') { --$depth }
        ++$end
    }
    return $source.Substring($start, $end - $start)
}
$labels = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'Solution/source/Scripting/ModelNames.cpp')
$production = ''
if ($labels.Contains('void RegisterAddonPedModelName(')) {
    $production = "#define HAS_ADDON_PED_NAMES 1`n"
    $production += [regex]::Match($labels, '(?m)^static std::unordered_map<DWORD, std::pair<std::string, std::string>> addonPedModelNames;').Value + "`n"
    $production += (Get-FunctionBody $labels 'void RegisterAddonPedModelName(') + "`n"
} else { $production = "#define HAS_ADDON_PED_NAMES 0`n" }
$production += (Get-FunctionBody $labels 'std::string get_ped_model_label(') + "`nnamespace sub {`n"
$changer = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'Solution/source/Submenus/PedModelChanger.cpp')
$production += (Get-FunctionBody $changer 'void ChangeModel_(GTAmodel::Model model)') + "`n}`nnamespace sub::Spooner::EntityManagement {`n"
$entities = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'Solution/source/Submenus/Spooner/EntityManagement.cpp')
if ($entities.Contains('void RestorePedName(')) { $production += (Get-FunctionBody $entities 'void RestorePedName(') + "`n" }
$production += "}`nnamespace sub::Spooner::SpoonerMode {`n"
$mode = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'Solution/source/Submenus/Spooner/SpoonerMode.cpp')
$production += (Get-FunctionBody $mode 'bool GetEntityPtr(') + "`n" + (Get-FunctionBody $mode 'SpoonerEntity GetEntityPtrValue(') + "`n}`n"
Set-Content -LiteralPath (Join-Path $buildDirectory 'PedLabelsProduction.inl') -Value $production -Encoding utf8
Push-Location -LiteralPath $buildDirectory
try {
    $command = 'call "{0}" -no_logo -arch=x64 -host_arch=x64 >nul && cl /nologo /std:c++20 /EHsc /W3 /WX /I "{1}" "{2}\PedLabels.cpp" /Fe:PedLabels.exe' -f $developerCommand, $buildDirectory, $PSScriptRoot
    & $env:ComSpec /d /c $command
    if ($LASTEXITCODE -ne 0) { throw 'Ped label test compilation failed.' }
    & '.\PedLabels.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Ped label checks failed.' }
}
finally { Pop-Location }
