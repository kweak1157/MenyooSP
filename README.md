# MenyooSP

Trainer for Grand Theft Auto V (single-player). This tree is the old MAFINS code, updated for Legacy **1.0.3725**.

A live fork with more work is https://github.com/itsjustcurtis/MenyooSP

## Requirements

- [ScriptHookV by Alexander Blade](http://www.dev-c.com/gtav/scripthookv/) new enough for game version 1.0.3725
- GTA V Legacy 1.0.3725

## Build

Visual Studio 2022 required (toolset **v143**, C++20).

Open `Solution/Menyoo.sln` and build **Release | x64**. Output is `Solution/source/_Build/bin/Release/Menyoo.asi`.

## Install

Copy the `Menyoo.asi` file along with the `menyooStuff` folder to the Grand Theft Auto V game directory.

**IMPORTANT:** Make sure that you have asiloader and ScriptHookV installed and that they are up to date.

## Addon vehicles

Press **F9** to start Spooner mode and open the **Object Spooner** menu, with **Spawn Entity Into World** selected. This also works before the first **F8** menu open. Press **Back** to return to the Menyoo main menu, or **F8** to close the menu and use the Spooner camera.

In Spooner mode (**F9**), open **Spawn Entity Into World > Addon Vehicles**, directly below **Vehicle**. It uses the same custom pack filter and installed model names. Highlighting a vehicle uses Spooner's world preview. Selecting it uses Spooner's normal vehicle placement and spawn settings, and adds it to the entity database for editing and saving.

**Saved Vehicles** is directly below **Addon Vehicles** in **Spawn Entity Into World**. It shows only saved vehicle names, including vehicles saved in subfolders. Select a name to immediately spawn that vehicle at the Spooner camera target, or near the player if the camera is off. Saved custom parts, paint, and attachments are loaded. The vehicle and its attachments are added to the Spooner database for editing and saving. File controls remain available in **Vehicle Spawner > Saved Vehicles**.

Open **Vehicle Options > Addon Vehicles** to spawn installed add-on vehicles. The same list is available in **Vehicle Spawner > Addon Vehicles**. It reads vehicle asset names from custom packs in the game's `mods/update/x64/dlcpacks` folder, including archives inside each `dlc.rpf`, then lists only models that the game recognises as available vehicles. The menu shows installed model names, such as `380sx` and `chiron17`, so reused GTA display names cannot be confused with stock vehicles. Folder names do not need to match vehicle model names. Packs with matching folders in the original `update/x64/dlcpacks`, patch packs, and standard vehicle model hashes are excluded.

The add-on list is populated when Menyoo starts. Add-on packs must be installed and enabled in GTA V before they can appear; this menu does not install them. The archive reader supports unencrypted RPF7 tables (NONE and OpenIV OPEN) and uncompressed embedded archives. Encrypted or compressed archive tables are skipped. Replacement mods that use a standard model hash do not appear.

Run `tests/run-addon-vehicles.ps1` to check add-on filtering and stock catalogue coverage. When adding official vehicle hashes to `VehicleHash` in `Solution/source/Scripting/enums.h`, also add their constants to `Solution/source/Scripting/StockVehicleModels.h`.

## Addon Ped Selector

Open **Player Options > Addon Ped Selector**, directly below **Model Changer**. It uses the standard Menyoo menu layout, active theme, fonts, position, and controls. It scans installed ped assets in both `mods/update/x64/dlcpacks` and `update/x64/dlcpacks`, including archives inside each pack. Known stock models from `PedHash` are excluded. Select a ped in the list to change your player model with the existing Menyoo model changer.

In Spooner mode (**F9**), open **Spawn Entity Into World > Addon Peds**, directly below **Ped**. This opens the same add-on list with cached discovery and fixed portrait previews. Highlighting a ped also uses Spooner's normal world preview. Selecting it spawns a new ped at the Spooner camera target (or near the player when the camera is inactive), applies Spooner's spawn settings, and adds it to the entity database for editing and saving. The player's model stays unchanged.

Both menus show only ped names and a fixed preview. Browse and select peds with the normal Menyoo keyboard, controller, or mouse controls. Discovery starts automatically in the background when Menyoo starts. A pack must be installed and enabled in GTA V before its models can appear. An empty list shows a short loading or empty-list message.

A small portrait appears at a fixed position beside the top of the standard menu. The portrait stays in place as you move through the ped list, and follows the menu position setting. When a ped has no custom PNG, Menyoo requests a GTA headshot after you pause on that ped for 0.2 seconds. It streams only that model, creates one hidden temporary ped, and removes it after capture or timeout. Up to eight portraits are cached while the selector is open; headshot resources are released when you leave. Some custom models may not support headshots.

Optional custom portraits go in `menyooStuff/PedPreviews/<model-name>.png`, for example `menyooStuff/PedPreviews/superman.png`. Use the lower-case model name. A custom PNG takes priority over the automatic headshot. Restart Menyoo after adding or replacing an image. Model names appear by default; captions from `PedList.xml` are used when present.

Available ped names are saved in `menyooStuff/PedSelectorCache.txt`. Later game sessions validate those names first, then scan the packs in the background for changes. The first scan still needs disk access. Native model validation runs in small batches, and the menu draws only visible rows. Selecting a ped requests its model without blocking the menu; the change occurs when GTA finishes streaming. A highlighted model is also streamed after the preview delay to reduce the wait when selected. Leaving the selector cancels a pending change. Large ped models can still take time to stream from disk.

Discovered add-on names are also used in Spooner's entity database and right-click selection menu, including when the player changes to an add-on model while already in the database. Existing blank or automatic hash labels recover the discovered name when displayed or selected; custom entity names are preserved. Models whose name is not yet known show their hash as a fallback.

The pack reader supports unencrypted RPF7 directory tables (NONE and OpenIV OPEN), uncompressed embedded archives, and loose `.ydd` and `.yft` assets. Encrypted tables and compressed embedded archives are skipped. Replacement peds with known stock hashes are excluded. Keep `Scripting/StockPedModels.h` in sync when adding stock hashes to `PedHash` in `enums.h`.

Run `tests/run-ped-selector.ps1` to check pack discovery and stock catalogue coverage. Run `tests/run-ped-selector-performance.ps1` to check bounded menu work, deferred switching, timeout, portrait cleanup, and cached discovery using deterministic game responses. Portrait appearance, actual streaming time, and player model changes also need a GTA V Story Mode check.

Run `tests/run-ped-labels.ps1` to check add-on model names through the real player model changer and Spooner selection flow, including recovery of old labels and preservation of custom entity names.

## Paste text

Open a search field or text input, then press **Ctrl+V** or **Shift+Insert**. The copied text replaces the field contents. You can edit it before you press **Enter**. Press **Escape** to cancel. For copied text with multiple lines, only the first line is used. Text is limited to the field's maximum length.

Run `tests/run-input-paste.ps1` to check paste input and clipboard handling.

## Vehicle routes in task sequences

On the driver's ped, open **Task Sequence > Add New > Follow Route (If In Vehicle)**. Use **Add Player Position** to record each route point in order, or **Add Camera Target Position** while using the Spooner camera. Points can be removed just like in the walking **Follow Route** task. Set the speed in KMPH, driving style, and enough task duration to complete the route, then start the sequence. The ped must be in the driver's seat when this task starts; an **Enter Vehicle** or **Warp Into Vehicle** task can precede it.

Road vehicles track the line between your recorded points and drive through intermediate steering targets. Speed changes are gradual; small tracking deviations and gentle bends do not trigger speed cuts. The driver plans braking across nearby checkpoints for sharp bends and the final stop. The speed setting is the maximum; larger corrections and braking can reduce it. Points advance only within 1.5 metres. The driver stops for traffic instead of swerving, changing lanes or taking shortcuts; the driving-style setting controls whether it also stops at lights. At the final point, the vehicle stops with the parking brake applied and the task sequence ends, keeping the driver there until you manually restart or stop it. Any tasks after this route will not run. Place points along a clear, driveable path and add points at bends. Routes and their settings are saved with Spooner files and copied with task sequences.

Run `tests/run-vehicle-route.ps1` to check route ordering, vehicle checks, saved settings, and task copying.

## Vehicle blowup in task sequences

On a vehicle, open **Task Sequence > Add New > Vehicle Blowup**. The vehicle explodes with sound when the sequence reaches this task. The task turns off invincibility and explosion protection on that vehicle. Use a **Nothing** task before it to add a delay. The task is saved with Spooner files and copied with task sequences.

## License

Majority of the source code is licensed under the [GNU GPL v3](LICENSE.txt) license.
Source content taken from other projects is tagged with the respective license(s) wherever found.
