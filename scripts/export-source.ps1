# Make a reviewable copy without modifying the live development tree.
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Split-Path $PSScriptRoot -Parent))
$target = Join-Path $root ('analysis/release/source-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
if (Test-Path $target) { throw 'Export target already exists.' }
New-Item -ItemType Directory -Path $target | Out-Null
$files = @('.gitignore','.gitattributes','README.md','CMakeLists.txt')
foreach ($directory in @('docs','scripts','host/src','host/config','launcher/src','launcher/assets','sdk','patches')) {
    $base = Join-Path $root $directory
    $files += Get-ChildItem -LiteralPath $base -Recurse -File -Force |
        Where-Object { $_.FullName -notmatch '[\\/]\.git[\\/]' } |
        ForEach-Object { $_.FullName.Substring($root.Length + 1) }
}
$files += @('host/CMakeLists.txt','host/launcher_icon.rc.in','host/.gitignore','launcher/build.cmd')
$forbidden = '\.(exe|dll|pdb|obj|o|zip|7z|iso|cso|xbe|lmp|bgra|dmp|log|tmp|pyc)$'
foreach ($relative in ($files | Sort-Object -Unique)) {
    if ($relative -match $forbidden -or $relative -match '(^|[\\/])(generated|assets[/\\]default|hdd|logs|CrashDumps|data)([\\/]|$)') {
        throw "Unexpected file on source allowlist: $relative"
    }
    $source = Join-Path $root $relative
    $destination = Join-Path $target $relative
    New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination
}
Get-ChildItem $target -Recurse -File -Force | ForEach-Object {
    [pscustomobject]@{file=$_.FullName.Substring($target.Length+1); bytes=$_.Length; sha256=(Get-FileHash $_.FullName).Hash}
} | ConvertTo-Json -Depth 4 | Set-Content (Join-Path (Split-Path $target -Parent) 'source-manifest.json')
$target | Set-Content (Join-Path $root 'analysis/release/latest-source.txt')
Write-Output $target
