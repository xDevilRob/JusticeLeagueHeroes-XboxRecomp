# Windows source build

Requires Windows x64, Visual Studio 2022 Build Tools (C++ workload and Windows SDK), LLVM Clang/LLD, CMake >=3.25 and Ninja. Use an x64 developer command prompt so Windows headers and import libraries are available. Tool installation is not automated.

Place the matching extracted Xbox dump in `host/assets/` (`default.xbe` and `res/`). Neither the dump nor generated game code is versioned. From the repository root:

```powershell
powershell -File scripts/build.ps1
```

CMake downloads its pinned source dependencies on the first build: Zydis, toml++, Dear ImGui and SDL. The SDK recompiles the user's XBE locally into `host/generated/` and builds the game. The launcher uses the Windows .NET Framework compiler.

This builds `gamelauncher.exe` and `host/run/JusticeLeagueHeroes.exe`. The checked-in `patches/ui` deltas reconstruct the generated UI/translation overlay when the user selects the matching dump in the launcher. No Python installation is required to apply them. `scripts/make-ui-patches.py` is a maintainer tool to regenerate deltas after editing a local overlay, not a player setup requirement.

Local developers may still use `build-game.cmd` with the original downloaded toolchain; it is intentionally excluded from the public source export because it depends on local diagnostics paths.
