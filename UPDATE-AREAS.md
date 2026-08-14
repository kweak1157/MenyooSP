# MenyooSP — update map

Read `GLOBAL.md` first. Talk in ASD-STE100.

This repo is the old MAFINS tree (v1.9.0). A live fork is https://github.com/itsjustcurtis/MenyooSP

Use this file to see what is done and where to work next.

---

## Status

| Area | State |
| --- | --- |
| 1. Game version to 1.0.3725 | **Done** |
| 2. Memory / ScriptHookV | **Done** |
| 3. Natives | **Done** |
| 4. Content lists | **Done** |
| 5. Build / CI | **Done** |
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

## 2. Memory / ScriptHookV — DONE

Target: Legacy **1.0.3725**.

### What we did

- MP cars now use a `shop_controller` script scan (same method as SHVDN / the live fork). No more hard global `4540726`.
- `GTAmemory::Init()` now tries 3725-safe patterns first (blip list, GXT, NM, model-request bypass). Old patterns stay as fallback.
- Swim and move speed use the 3521 offsets `{0x8, 0x10A8, 0x1C8}` and `{0x8, 0x10A8, 0xD50}` up to 3725.
- Height still uses `{0x8, 0x88}` up to 3725.
- Unlock runs once in `MenyooMain`, not every tick.

### Files

| File | What to look for |
| --- | --- |
| `Solution/source/Memory/GTAmemory.cpp` | `FindShopController`, `EnableBlockedMpVehiclesInSp`, `GTAmemory::Init`, swim / move |
| `Solution/source/Memory/GTAmemory.h` | `ScriptHeader` / `ScriptTable`, `FindShopController` |
| `Solution/source/Menu/Routine.cpp` | One-time unlock in `MenyooMain`. Unlimited rocket boost uses natives. |

### Still open in this area

- `ScriptHookV.lib` is still the old import lib. The `ScriptHookV.dll` on the PC must be new enough for 3725.
- Unlimited rocket boost now uses natives (`SET_ROCKET_BOOST_FILL` / `SET_ROCKET_BOOST_ACTIVE`). Old memory helpers still stop at 2372 and are unused.

### Do not

- Do not write a hard global for MP cars again.
- Do not add Enhanced (`GTA5_Enhanced.exe`) code unless the user asks.

### Test on 1.0.3725

- Spawn an MP-only vehicle.
- Change player height.
- Change swim speed and move speed.
- Use unlimited rocket boost on an Oppressor.

---

## 3. Natives — DONE

Source: live fork `natives.h` (NativeDB, up to b3095).

### What we did

- Replaced `natives.h` with the live-fork list (6499 natives).
- `WAIT` stays in `natives2.h`.
- `nativeCaller.h`, `types.h`, and `natives2` wrappers did not need a change.

### Files

| File | What to look for |
| --- | --- |
| `Solution/source/Natives/natives.h` | Native list from the live fork |
| `Solution/source/Natives/natives2.h` | Wrappers (`WAIT`, `GET_HASH_KEY`) |
| `Solution/source/Natives/natives2.cpp` | Wrapper impl |
| `Solution/source/Natives/nativeCaller.h` | `invoke` |
| `Solution/source/Natives/types.h` | Types |

### Do not

- Do not restore `WAIT` or `GET_HASH_KEY` inside `natives.h`. They live in `natives2`.

### Next if you touch natives

- NativeDB https://alloc8or.re/gta5/nativedb/ — list stops at b3095. Add later hashes only if a call needs them.

---

## 4. Content lists — DONE

Target: Legacy **1.0.3725**.

### What we did

- Added `VehicleHash` from Cayo Perico through A Safehouse in the Hills (3717 / 3725).
- Added weapons after Drug Wars: Tactical SMG, Battle Rifle, Hacking Device, Snow Launcher, Stun Rod, Newspaper.
- Added Money Fronts and Safehouse ped hashes.
- Added Cayo Perico teleport menu. Island load uses `SET_ISLAND_ENABLED`.
- Added later plate types (eCola through Sprunk).
- Spawn menu still fills extra cars from game memory. Cutscenes still use `CutsceneLabels.txt`.

### Files

| File | What to look for |
| --- | --- |
| `Solution/source/Scripting/enums.h` | `VehicleHash` last value `VEHICLE_XTREME`. `WeaponHash` last value `WEAPON_NEWSPAPER` |
| `Solution/source/Scripting/WeaponIndivs.cpp` | New weapon labels and category rows |
| `Solution/source/Submenus/Teleport/CayoPerico.cpp` | Cayo Perico locations |
| `Solution/source/Submenus/Teleport/Locations.cpp` | Cayo Perico category |
| `Solution/source/Submenus/VehicleModShop.cpp` | Extra plate names |

### Do not

- Do not add Enhanced-only code (`GTA5_Enhanced.exe`) unless the user asks.
- Do not add `WEAPON_STRICKLER`. That weapon is gen9 only.

### Next if you touch lists

- Ped spawn still uses `PedList.xml` at run time.
- Cutscene list still uses `CutsceneLabels.txt` at run time.

---

## 5. Build / CI — DONE

### What we did

- Debug and Release both use toolset `v143` and C++20.
- Windows SDK is `10.0` so MSBuild uses the SDK on the PC.
- Debug now links ScriptHookV and writes to `_Build`.
- The solution file is Visual Studio 2022. Debug maps to Debug.
- `README.md` now says VS 2022 and Legacy 1.0.3725.
- Removed `.appveyor.yml`. GitHub Actions use checkout v4, MSBuild v2, and artifact v4.
- `MENYOO_CURRENT_VER_` is `"1.9.0"`.
- C++20 build now works: `operator==` is const, Cayo Perico includes `GTAentity.h`, and stat min/max use float.

### Files

| File | What to look for |
| --- | --- |
| `Solution/source/Menyoo.vcxproj` | Debug and Release `v143`, `stdcpp20` |
| `Solution/Menyoo.sln` | Visual Studio Version 17 |
| `README.md` | Visual Studio 2022 |
| `.github/workflows/msbuild.yml` | Branch builds |
| `.github/workflows/master_build.yml` | Master / main builds |
| `Solution/source/macros.h` | `MENYOO_CURRENT_VER_` is `"1.9.0"` |

### Do not

- Do not add Premake unless the user asks.
- Do not add Enhanced or FiveM build steps unless the user asks.

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

Suggested next area: **6. Code shape**.
