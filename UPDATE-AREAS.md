# MenyooSP — update map

Read `GLOBAL.md` first. Talk in ASD-STE100.

This repo is the old MAFINS tree (v1.8.1). A live fork is https://github.com/itsjustcurtis/MenyooSP

Use this file to see what is done and where to work next.

---

## Status

| Area | State |
| --- | --- |
| 1. Game version to 1.0.3725 | **Done** |
| 2. Memory / ScriptHookV | Open |
| 3. Natives | Open |
| 4. Content lists | Open |
| 5. Build / CI | Open |
| 6. Code shape | Open |

---

## 1. Game version — DONE

Target: Legacy **1.0.3725**.

### What we did

- Added all ScriptHookV versions from 2802 to 3725. The enum order must match ScriptHookV. Do not skip versions.
- Height checks now accept 3725.
- MP-vehicle unlock still uses the 2802 global as default.

### Files

| File | What to look for |
| --- | --- |
| `Solution/source/main.h` | `enum eGameVersion` — last Legacy value is `VER_1_0_3725_0` |
| `Solution/source/Memory/GTAmemory.cpp` | `EnableBlockedMpVehiclesInSp`, `GetPlayerHeight`, `SetPlayerHeight` |
| `Solution/source/Memory/GTAmemory.h` | `GetGameVersion()` |

### Do not

- Do not put `VER_1_0_3725_0` right after 2802. That breaks `getGameVersion()`.
- Do not add 3751 / 3788 / 3889 unless the user asks.

### Next if you touch versions

- Confirm ScriptHookV.dll on the PC is new enough for 3725.
- Official list: SHVDN `source/scripting_v3/GTA/GameVersion.cs`

---

## 2. Memory / ScriptHookV — OPEN (high)

Patterns and offsets may be wrong on 3725.

### Files

| File | What to fix |
| --- | --- |
| `Solution/source/Memory/GTAmemory.cpp` | `GTAmemory::Init()` — patterns. `EnableBlockedMpVehiclesInSp` — global `4540726` may be wrong. Swim / move / boost still stop at old builds (2245 / 2372). |
| `Solution/source/Memory/GTAmemory.h` | Version helpers |
| `Solution/external/ScriptHookV/SDK/lib/ScriptHookV.lib` | Old SDK lib |

### Find it

Search: `FindPattern`, `VER_1_0_2245`, `VER_1_0_2372`, `4540726`

---

## 3. Natives — OPEN (high)

`natives.h` is old. New hashes may be missing. Old names may be wrong.

### Files

| File | What to fix |
| --- | --- |
| `Solution/source/Natives/natives.h` | Native list (very large) |
| `Solution/source/Natives/natives2.h` | Wrappers (`WAIT`, etc.) |
| `Solution/source/Natives/natives2.cpp` | Wrapper impl |
| `Solution/source/Natives/nativeCaller.h` | `invoke` |
| `Solution/source/Natives/types.h` | Types |

### Find it

Search: `invoke<`, NativeDB https://alloc8or.re/gta5/nativedb/

---

## 4. Content lists — OPEN (high)

Add vehicles, weapons, peds, teleports, interiors, cutscenes after Cayo / Tuners / later DLC.

### Files

| File | What to fix |
| --- | --- |
| `Solution/source/Scripting/ModelNames.h` | Vehicle hash arrays |
| `Solution/source/Scripting/ModelNames.cpp` | `PopulateVehicleModelsArray` |
| `Solution/source/Scripting/WeaponIndivs.h` / `.cpp` | Weapons |
| `Solution/source/Scripting/enums.h` / `.cpp` | Game enums |
| `Solution/source/Submenus/VehicleSpawner.cpp` | Spawn menu |
| `Solution/source/Submenus/VehicleModShop.cpp` | Mods |
| `Solution/source/Submenus/WeaponOptions.cpp` | Weapon menu |
| `Solution/source/Submenus/PedModelChanger.cpp` | Ped models |
| `Solution/source/Submenus/Teleport/` | Locations / interiors |
| `Solution/source/Submenus/CutscenePlayer.cpp` | Cutscenes |

### Find it

Search: `g_vehHashes`, `PopulateVehicleModelsArray`, `TeleLocation`

---

## 5. Build / CI — OPEN (medium)

### Files

| File | What to fix |
| --- | --- |
| `Solution/source/Menyoo.vcxproj` | Debug toolset is `v141`. Release is `v143`. C++20 only on Release. |
| `README.md` | Still says Visual Studio 2017. Repo is archived. |
| `.appveyor.yml` | Old CI. No `.github/workflows` here. |
| `Solution/source/macros.h` | `MENYOO_CURRENT_VER_` is `"1.8.1"` |

---

## 6. Code shape — OPEN (low)

Do this after the game works on 3725.

### Files

| File | Problem |
| --- | --- |
| `Solution/source/Menu/Routine.cpp` | Very large loop file (~3300 lines) |
| `Solution/source/Menu/submenu_switch.cpp` | Giant `switch (currentsub)` |
| `Solution/source/Menu/submenu_enum.h` | Submenu IDs |
| `Solution/source/Menu/Menu.cpp` | Menu draw / input |

---

## How to start a new session

1. Read `GLOBAL.md` and this file.
2. Ask the user which open area to do next.
3. Do one area. Then mark it **Done** in the table above.
4. Do not commit unless the user asks.

Suggested next area: **2. Memory / ScriptHookV** (test spawn, height, MP cars on 1.0.3725).
