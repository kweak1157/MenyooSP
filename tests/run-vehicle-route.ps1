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

# Compile the real task declaration, persistence and execution with a recorded game API.
$taskRoot = Join-Path $repoRoot 'Solution\source\Submenus\Spooner'
$header = Get-Content -Raw -LiteralPath (Join-Path $taskRoot 'STSTasks.h')
$start = $header.IndexOf('class DriveFollowRoute final')
$end = $header.IndexOf('class FollowEntity final', $start)
Set-Content -LiteralPath (Join-Path $buildDirectory 'VehicleRouteDeclaration.inl') -Value $header.Substring($start, $end - $start) -Encoding utf8
$baseSource = Get-Content -Raw -LiteralPath (Join-Path $taskRoot 'STSTask.cpp')
$production = "namespace sub::Spooner {`n"
foreach ($signature in @('void STSTask::Assign(STSTask* right)', 'void STSTask::GetXmlNode(pugi::xml_node& nodeTask) const', 'void STSTask::ImportXmlNode(pugi::xml_node& nodeTask)', 'void STSTask::Run(void* ve)', 'void STSTask::Tick(void* ve)', 'void STSTask::End(void* ve)')) {
    $production += (Get-FunctionBody $baseSource $signature) + "`n"
}
$production += "namespace STSTasks {`n"
$taskSource = Get-Content -Raw -LiteralPath (Join-Path $taskRoot 'STSTasks.cpp')
foreach ($signature in @('void DriveFollowRoute::GetXmlNodeTaskSpecific(pugi::xml_node& nodeTask) const', 'void DriveFollowRoute::ImportXmlNodeTaskSpecific(pugi::xml_node& nodeTask)', 'void DriveFollowRoute::ImportTaskDataSpecific(STSTask* otherTsk)', 'DriveFollowRoute::DriveFollowRoute()', 'void DriveFollowRoute::RunP(GTAped& ep)', 'void DriveFollowRoute::GetDrivingCommand(const Vector3& position, Vector3& target, float& speed) const', 'void DriveFollowRoute::TickP(GTAped& ep)', 'void DriveFollowRoute::EndP(GTAped& ep)')) {
    $production += (Get-FunctionBody $taskSource $signature) + "`n"
}
$production += "}`n"
$sequenceSource = Get-Content -Raw -LiteralPath (Join-Path $taskRoot 'SpoonerTaskSequence.cpp')
foreach ($signature in @('SpoonerTaskSequence::SpoonerTaskSequence()', 'void SpoonerTaskSequence::AddTask(std::unique_ptr<STSTask> tskPtr)', 'void SpoonerTaskSequence::Start()', 'void SpoonerTaskSequence::Reset(bool deleteTasks)', 'void SpoonerTaskSequence::Tick(void* ev)', 'bool SpoonerTaskSequence::IsActive() const')) {
    $production += (Get-FunctionBody $sequenceSource $signature) + "`n"
}
$production += "}`n"
Set-Content -LiteralPath (Join-Path $buildDirectory 'VehicleRouteProduction.inl') -Value $production -Encoding utf8

Push-Location -LiteralPath $buildDirectory
try {
    $command = 'call "{0}" -no_logo -arch=x64 -host_arch=x64 >nul && cl /nologo /std:c++20 /EHsc /W3 /WX /I "{1}" /I "{2}\Solution\external" "{3}\VehicleRoute.cpp" "{2}\Solution\external\pugixml\src\pugixml.cpp" /Fe:VehicleRoute.exe' -f $developerCommand, $buildDirectory, $repoRoot, $PSScriptRoot
    & $env:ComSpec /d /c $command
    if ($LASTEXITCODE -ne 0) { throw 'Vehicle route test compilation failed.' }
    & '.\VehicleRoute.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Vehicle route checks failed.' }
}
finally { Pop-Location }
