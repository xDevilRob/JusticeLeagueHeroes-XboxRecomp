# First release review

Status: [v0.1.0-rc1](https://github.com/xDevilRob/JusticeLeagueHeroes-XboxRecomp/releases/tag/v0.1.0-rc1) published as a pre-release on 2026-10-22.

## Source inclusion

Include launcher C# source, supplied icon, game-specific C++ hooks, host CMake and configuration, and the modified SDK source with its license and third-party notices. Do not initialize the SDK as an unmodified upstream submodule: eight runtime files currently have local modifications.

The SDK currently has its own `.git` directory. An export must copy its sources without that directory so the parent repository contains the actual modified source instead of an accidental gitlink.

## Keep local

- `Justice League Heroes/`, `host/assets/`: original extracted game data.
- `host/generated/`: C++ generated from the original executable.
- `host/run/`, other host test folders, `launcher/data/`: settings, saves, logs and backups.
- `tools/`: downloaded CMake, LLVM, Ninja and archives.
- `analysis/`, `diagnostics/`, `experiments/`, `dist/`: development evidence and local artifacts.
- `localization/*.source.json` and current localization catalogs: contain extracted game text; exclude pending a dedicated translation packaging review.
- `dump-path.txt`: a machine-specific absolute path; never ship it.

## Distribution findings

The current local ZIP contains eight full LMP resources and `control-layouts.bin`. The LMPs preserve original menus, fonts or text; the controls binary also contains bitmap data sampled/copied from the game. They must not be treated as wholly new project assets merely because they were modified.

Implemented copy/literal compressed deltas in `patches/ui`, with SHA-256 verification for each original input and final output. The launcher reconstructs all eight resources on dump selection or Play, writes only into host/ui and preserves the dump. Isolated first-run tests reconstructed every file byte-for-byte; invalid original hashes are rejected. Patches are derived modifications, not an independent license to original game assets.

Include SDK LICENSE and THIRD_PARTY.txt in the release package. Check actual linked dependencies against these notices. The supplied JLH icon is user-provided; its provenance is not independently established.

## Checks remaining

- [x] Default-deny root `.gitignore` created.
- [x] Original data, generated game code, local user data and tools identified.
- [x] Modified SDK identified; avoid losing local changes on export.
- [x] Export source using an explicit allowlist; final scan before upload.
- [x] Add a portable build entry point and toolchain instructions.
- [x] Reconstruct modified UI/translation resources from the user's dump on first run.
- [ ] Verify launcher display selectors interactively (DrawToBitmap does not capture native selection text reliably).
- [x] Include dependency notices and a distribution hash manifest.
- [x] Compile from clean exported source; verify first-run reconstruction and direct startup with no launcher environment. Shared paths are identical by construction; manual UI/play testing remains recommended for this release candidate.
- [x] Configure the user's GitHub repository and authentication.
- [x] Upload 150 reviewed source files and publish v0.1.0-rc1 with ZIP and SHA256SUMS.txt.

Keep local runtime data in place; exclusions do not require deleting or moving original files.
