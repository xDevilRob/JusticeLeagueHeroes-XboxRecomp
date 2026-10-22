param([string]$Repository=(Split-Path $PSScriptRoot -Parent))
$ErrorActionPreference='Stop'
$files=@(& git -C $Repository ls-files)
if($LASTEXITCODE -or !$files.Count){throw 'No tracked source files to audit.'}
$bad=@($files | Where-Object {$_ -match '^host/(assets|generated|run)/|(^|/)(hdd|logs|data|build|CrashDumps)/|\.(xbe|iso|cso|lmp|bgra|exe|dll|pdb|zip|dmp)$|(^|/)(dump-path\.txt|preferences\.xml|settings\.toml)$'})
if($bad.Count){throw ('Unexpected tracked files: '+($bad -join ', '))}
$secrets='gh[pousr]_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{30,}|AKIA[0-9A-Z]{16}|-----BEGIN (RSA |OPENSSH |EC )?PRIVATE KEY-----'
foreach($file in $files){
    if($file -match '\.(ico|png|jldp)$'){continue}
    $value=[IO.File]::ReadAllText((Join-Path $Repository $file))
    if($value -match $secrets){throw "Potential credential in $file; contents not printed."}
}
$gitlinks=@(& git -C $Repository ls-files --stage | Where-Object {$_ -match '^160000 '})
if($gitlinks.Count){throw 'Unexpected submodule/gitlink: vendored modified SDK source must be tracked.'}
Write-Output ("PASS: {0} tracked files, no forbidden artifacts, no recognized credential patterns, no gitlinks." -f $files.Count)
