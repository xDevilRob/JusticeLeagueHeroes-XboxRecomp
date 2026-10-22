# SDK provenance

Upstream: https://github.com/TGP482/XboxXBERecomp

Base commit: `40e9c35451305f09ca9d86f677e4b9c2c37ff26b`.

The `sdk/` directory is a vendored source copy, not a submodule. Its MIT license and original copyright notice remain in `sdk/LICENSE`. Dependency notices remain in `sdk/THIRD_PARTY.txt`.

Local runtime modifications at the first-release preparation point:

- app/app.h
- app/game_files.cpp
- app/main.cpp
- core/input.cpp
- core/input.h
- gpu/d3d12.cpp
- gpu/ext.h
- kernel/files.cpp

Game-specific host hooks are under `host/src/common`. Generated game C++ and the original executable are excluded from version control. Build them from the matching user-provided dump.
