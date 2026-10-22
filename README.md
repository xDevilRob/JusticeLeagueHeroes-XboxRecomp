# Justice League Heroes — Xbox recompilation for Windows

Unofficial native Windows recompilation. The local build has been played through by the project tester. The first release candidate is being validated.

This project uses XboxXBERecomp by TGP482, with local runtime and game-specific changes. Preserve the notices in `sdk/LICENSE` and `sdk/THIRD_PARTY.txt`.

## Current features

- Native Direct3D 12 rendering and XAudio2 audio.
- 720p, 1080p, 1440p and 2160p; windowed and fullscreen options.
- Original language choices plus a local Brazilian Portuguese translation.
- Original Xbox, modern Xbox, PlayStation, and keyboard/mouse button layouts.
- Launcher: dump selection, display settings, save backup and restore.
- Direct game launch after configuring the dump through the launcher.

Mods currently have a folder listing only. Automatic updating is not implemented. DLAA, frame generation, 120 FPS and PSP characters are not included.

## Your game files

Provide your own extracted Xbox dump. The currently supported `default.xbe` SHA-1 is `c94d34f77bfaa0b879bd5d53e3c62fb6b7bdef98`. Original game files, generated game code, saves and downloaded toolchains are excluded from the source publication list.

## Local development

The source build uses Visual Studio 2022 Build Tools, Windows SDK, Clang/LLD, CMake and Ninja. Run `powershell -File scripts/build.ps1` from an x64 developer shell. See [build instructions](docs/BUILD.md).

`launcher/build.cmd` builds `gamelauncher.exe` using the .NET Framework compiler present on Windows. The launcher expects `host/run/JusticeLeagueHeroes.exe`, and generated UI resources in `host/ui`.

Release packages contain the executables and binary deltas in `patches/ui`. When you select your dump, the launcher checks the original resources and reconstructs the interface and translation locally. No full LMP resources or generated UI files are included in the package. Keep the generated resources for direct game launch after setup.

Mods and automatic updates remain unfinished. See [release review](docs/RELEASE-REVIEW.md).
