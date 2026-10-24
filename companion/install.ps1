param([string]$Destination = (Join-Path (Split-Path -Parent $PSScriptRoot) 'host-test\live-companion'))
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
$editor = Join-Path $project 'build\PianoRollClipEditor_artefacts\Release\Piano Roll Clip Editor.exe'
if (-not (Test-Path -LiteralPath $editor)) { throw 'Build the Release clip editor first with build.ps1.' }
New-Item -ItemType Directory -Force -Path $Destination | Out-Null
& python (Join-Path $PSScriptRoot 'build_device.py') $Destination
if ($LASTEXITCODE -ne 0) { throw 'Companion device build failed.' }
foreach ($file in @('bridge-max.js','bridge-node.js','bridge-transport.js','clip-protocol.js','README.md')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $file) -Destination $Destination -Force
}
$bin = Join-Path $Destination 'bin'
New-Item -ItemType Directory -Force -Path $bin | Out-Null
Copy-Item -LiteralPath $editor -Destination $bin -Force
Write-Output "Load this file in Live: $(Join-Path $Destination 'Piano Roll Live.amxd')"
