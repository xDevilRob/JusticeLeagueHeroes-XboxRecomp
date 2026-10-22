Justice League Heroes - Windows recompilation - release candidate

1. Extract the entire ZIP into a writable folder.
2. Open gamelauncher.exe.
3. Select Settings -> Choose dump folder. Select your extracted Xbox game
   containing default.xbe and res. This version does not select ISO files.
4. The launcher validates the game version and reconstructs PC interface,
   controls and Portuguese translation resources from your original data.
5. Select resolution and screen mode, click Apply, then Play.

Supported default.xbe SHA-1: c94d34f77bfaa0b879bd5d53e3c62fb6b7bdef98
Required: Windows x64, DirectX 12-capable graphics and .NET Framework 4.x.
Keep all package folders together. Do not move or modify your original dump.

After the initial dump selection you can open
host/run/JusticeLeagueHeroes.exe directly. Both launch methods share
host/run/settings.toml and host/run/hdd (saves). If your dump moves,
select its new location in the launcher.

Use Saves in the launcher to back up or import progress. Close the game
before changing the dump, restoring saves or applying launcher settings.
Keep backups separately before replacing a build.

In-game Options contains languages and button layouts. Display options
are available both in-game and in launcher Settings.

Mods currently only lists a folder. Automatic updates are not implemented;
the Updates page opens the repository's Releases page.

Original game data and generated UI files are not included in this ZIP.
The included deltas require matching original resource hashes.
Dependency license notices are in notices/.
