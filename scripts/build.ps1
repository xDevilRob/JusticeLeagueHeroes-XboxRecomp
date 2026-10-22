param([string]$BuildDirectory = 'build', [int]$Jobs = 6)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if (!(Test-Path (Join-Path $root 'host/assets/default.xbe'))) {
    throw 'Place your matching extracted dump in host/assets before building.'
}
$hash = (Get-FileHash (Join-Path $root 'host/assets/default.xbe') -Algorithm SHA1).Hash
if ($hash -ne 'C94D34F77BFAA0B879BD5D53E3C62FB6B7BDEF98') { throw 'Unsupported default.xbe version.' }
foreach ($tool in @('cmake','ninja','clang++','lld-link')) {
    if (!(Get-Command $tool -ErrorAction SilentlyContinue)) { throw "Missing tool on PATH: $tool. Use an x64 VS Build Tools developer shell with Clang, CMake and Ninja." }
}
$build = if ([IO.Path]::IsPathRooted($BuildDirectory)) { $BuildDirectory } else { Join-Path $root $BuildDirectory }
& cmake -S $root -B $build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
if ($LASTEXITCODE) { throw 'CMake configuration failed.' }
& cmake --build $build --target JusticeLeagueHeroes -j $Jobs
if ($LASTEXITCODE) { throw 'Game build failed.' }
New-Item -ItemType Directory -Path (Join-Path $root 'host/run') -Force | Out-Null
Copy-Item (Join-Path $build 'host/JusticeLeagueHeroes.exe') (Join-Path $root 'host/run/JusticeLeagueHeroes.exe') -Force
& cmd /c (Join-Path $root 'launcher/build.cmd')
if ($LASTEXITCODE) { throw 'Launcher build failed.' }
Write-Output 'Built gamelauncher.exe and host/run/JusticeLeagueHeroes.exe. UI resource preparation remains a separate release step.'
