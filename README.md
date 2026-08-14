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

## License

Majority of the source code is licensed under the [GNU GPL v3](LICENSE.txt) license.
Source content taken from other projects is tagged with the respective license(s) wherever found.
