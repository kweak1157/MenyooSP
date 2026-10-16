$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation = & $locator -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $installation) { throw 'Visual Studio C++ build tools were not found.' }
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
    if ($depth -ne 0) { throw "Function body was not complete: $signature" }
    return $source.Substring($start, $end - $start)
}

# Use the real spawn functions with deterministic game-native responses.
$modelSource = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'Solution\source\Scripting\Model.cpp')
$production = "namespace GTAmodel {`n"
foreach ($signature in @('bool Model::Load(DWORD timeOut) const', 'bool Model::IsInCdImage() const', 'void Model::Unload() const')) {
    $production += (Get-FunctionBody $modelSource $signature) + "`n"
}
$production += "}`nnamespace World {`n"
$worldSource = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'Solution\source\Scripting\World.cpp')
foreach ($signature in @('GTAprop CreateProp(GTAmodel::Model model, Vector3 position, bool dynamic, bool placeOnGround)', 'GTAprop CreateProp(GTAmodel::Model model, Vector3 position, const Vector3& rotation, bool dynamic, bool placeOnGround)')) {
    $production += (Get-FunctionBody $worldSource $signature) + "`n"
}
$production += "}`nnamespace sub::Spooner::EntityManagement {`n"
$entitySource = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'Solution\source\Submenus\Spooner\EntityManagement.cpp')
$production += Get-FunctionBody $entitySource 'SpoonerEntity AddProp(const GTAmodel::Model& model, const std::string& name, bool unloadModel)'
$production += "`n}`n"
Set-Content -LiteralPath (Join-Path $buildDirectory 'SpoonerPropSpawnProduction.inl') -Value $production -Encoding utf8

Push-Location -LiteralPath $buildDirectory
try {
    $command = 'call "{0}" -no_logo -arch=x64 -host_arch=x64 >nul && cl /nologo /std:c++20 /EHsc /W3 /WX /I "{1}" /I "{2}\Solution\external" "{3}\SpoonerPropSpawn.cpp" "{2}\Solution\external\pugixml\src\pugixml.cpp" /Fe:SpoonerPropSpawn.exe' -f $developerCommand, $buildDirectory, $repoRoot, $PSScriptRoot
    & $env:ComSpec /d /c $command
    if ($LASTEXITCODE -ne 0) { throw 'Spooner prop test compilation failed.' }
    & '.\SpoonerPropSpawn.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Spooner prop regression checks failed.' }
}
finally { Pop-Location }
