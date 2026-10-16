$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation = & $locator -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $installation) { throw 'Visual Studio C++ build tools were not found.' }

$developerCommand = Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
$buildDirectory = Join-Path $repoRoot 'Solution\source\_Build\bin\Tests'
New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null
$menuSource = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'Solution\source\Menu\Menu.cpp')

# Compile the actual startup values and menu functions, with GTA calls replaced
# by the deterministic inputs in MenuFirstOpen.cpp. Do not copy their logic.
$globals = [regex]::Match($menuSource, '(?ms)^UINT16 Menu::currentsub =.*?(?=^Scaleform Menu::scaleform_menuGlare;)').Value
if (-not $globals) { throw 'Menu startup declarations were not found.' }
$bindings = [regex]::Matches($menuSource, '(?m)^(?:bool g_menuNotOpenedYet|std::pair<UINT16, UINT16> menubindsGamepad|UINT16 menubinds) = .*;').Value -join "`n"
$production = $bindings + "`n" + $globals + "`nconst int initialPendingSubmenu = Menu::SetSub_delayed;`n"
foreach ($name in @('isBinds', 'while_closed', 'while_opened', 'Up', 'Down', 'Bottom', 'Top', 'SetSub_previous', 'SetSub_new', 'SetSub_closed', 'sub_handler')) {
    $signature = [regex]::Match($menuSource, "(?m)^(?:bool|void) Menu::$name\([^`r`n]*\)")
    if (-not $signature.Success) { throw "Menu::$name was not found." }
    $openingBrace = $menuSource.IndexOf('{', $signature.Index + $signature.Length)
    $depth = 1
    $end = $openingBrace + 1
    while ($depth -gt 0 -and $end -lt $menuSource.Length) {
        if ($menuSource[$end] -eq '{') { ++$depth }
        elseif ($menuSource[$end] -eq '}') { --$depth }
        ++$end
    }
    if ($depth -ne 0) { throw "Menu::$name body was not complete." }
    $production += $menuSource.Substring($signature.Index, $end - $signature.Index) + "`n"
}
$spoonerSource = Get-Content -Raw -LiteralPath (Join-Path $repoRoot 'Solution\source\Submenus\Spooner\SpoonerMode.cpp')
$production += "namespace sub::Spooner::SpoonerMode {`n"
$production += ([regex]::Matches($spoonerSource, '(?m)^\s*(?:BYTE bindsKeyboard|std::pair<UINT16, UINT16> bindsGamepad|bool bEnabled) = .*;').Value -join "`n") + "`n"
foreach ($name in @('OpenMainMenu', 'IsHotkeyPressed', 'TurnOn', 'Toggle')) {
    $signature = [regex]::Match($spoonerSource, "(?m)^\s*(?:bool|void) $name\([^`r`n]*\)")
    if (-not $signature.Success) { throw "SpoonerMode::$name was not found." }
    $openingBrace = $spoonerSource.IndexOf('{', $signature.Index + $signature.Length)
    $depth = 1
    $end = $openingBrace + 1
    while ($depth -gt 0 -and $end -lt $spoonerSource.Length) {
        if ($spoonerSource[$end] -eq '{') { ++$depth }
        elseif ($spoonerSource[$end] -eq '}') { --$depth }
        ++$end
    }
    if ($depth -ne 0) { throw "SpoonerMode::$name body was not complete." }
    $production += $spoonerSource.Substring($signature.Index, $end - $signature.Index) + "`n"
}
$production += "}`n"
Set-Content -LiteralPath (Join-Path $buildDirectory 'MenuFirstOpenProduction.inl') -Value $production -Encoding utf8

Push-Location -LiteralPath $buildDirectory
try {
    $command = 'call "{0}" -no_logo -arch=x64 -host_arch=x64 >nul && cl /nologo /std:c++20 /EHsc /W3 /WX /I "{1}\Solution\source" /I "{2}" "{3}\MenuFirstOpen.cpp" "{1}\Solution\source\Util\keyboard.cpp" user32.lib /Fe:MenuFirstOpen.exe' -f $developerCommand, $repoRoot, $buildDirectory, $PSScriptRoot
    & $env:ComSpec /d /c $command
    if ($LASTEXITCODE -ne 0) { throw 'Menu test compilation failed.' }
    & '.\MenuFirstOpen.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Menu first-open regression checks failed.' }
}
finally {
    Pop-Location
}
