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

$source = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'Solution\source\Scripting\Game.cpp')
$stringSource = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'Solution\source\Util\StringManip.cpp')
$clipboardSource = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'Solution\source\Util\ExePath.cpp')
$production = (Get-FunctionBody $stringSource 'std::string WideToUtf8(') + "`n"
$production += (Get-FunctionBody $clipboardSource 'std::string GetClipboardTextUtf8()') + "`n"
$production += "namespace Game {`n" + (Get-FunctionBody $source 'std::string InputBox(') + "`n}`n"
Set-Content -LiteralPath (Join-Path $buildDirectory 'InputPasteProduction.inl') -Value $production -Encoding utf8
Push-Location -LiteralPath $buildDirectory
try {
    $command = 'call "{0}" -no_logo -arch=x64 -host_arch=x64 >nul && cl /nologo /std:c++20 /EHsc /W3 /WX /I "{1}" "{2}\InputPaste.cpp" /Fe:InputPaste.exe' -f $developerCommand, $buildDirectory, $PSScriptRoot
    & $env:ComSpec /d /c $command
    if ($LASTEXITCODE -ne 0) { throw 'Input paste test compilation failed.' }
    & '.\InputPaste.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Input paste regression checks failed.' }
}
finally { Pop-Location }
