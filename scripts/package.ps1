param([string]$Version='0.1.0-rc1')
$ErrorActionPreference='Stop'
if($Version -notmatch '^[0-9]+\.[0-9]+\.[0-9]+(-[a-zA-Z0-9.-]+)?$'){throw 'Invalid version.'}
$root=Split-Path $PSScriptRoot -Parent
$destination=Join-Path $root ('dist/JusticeLeagueHeroes-'+$Version+'-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
if(Test-Path $destination){throw 'Package already exists.'}
New-Item -ItemType Directory -Path (Join-Path $destination 'host/run') | Out-Null
Copy-Item (Join-Path $root 'gamelauncher.exe') $destination
Copy-Item (Join-Path $root 'host/run/JusticeLeagueHeroes.exe') (Join-Path $destination 'host/run')
Copy-Item (Join-Path $root 'patches') $destination -Recurse
New-Item -ItemType Directory -Path (Join-Path $destination 'notices') | Out-Null
Copy-Item (Join-Path $root 'sdk/LICENSE') (Join-Path $destination 'notices/SDK-LICENSE.txt')
Copy-Item (Join-Path $root 'sdk/THIRD_PARTY.txt') (Join-Path $destination 'notices/THIRD_PARTY.txt')
Copy-Item (Join-Path $root 'docs/PLAYER-README.txt') (Join-Path $destination 'README.txt')
[IO.File]::WriteAllText((Join-Path $destination 'host/run/settings.toml'),"display_mode = `"windowed`"`r`nwidth = 1280`r`nheight = 720`r`nfps_cap = 60`r`nvsync = true`r`nlanguage = 0`r`nbutton_layout = 0`r`n")
Get-ChildItem $destination -Recurse -File | ForEach-Object {
    [pscustomobject]@{file=$_.FullName.Substring($destination.Length+1);sha256=(Get-FileHash $_.FullName).Hash;bytes=$_.Length}
} | ConvertTo-Json | Set-Content (Join-Path $destination 'manifest.json')
Compress-Archive -Path (Join-Path $destination '*') -DestinationPath ($destination+'.zip')
$destination | Set-Content (Join-Path $root 'analysis/release/latest-package.txt')
Write-Output ($destination+'.zip')
