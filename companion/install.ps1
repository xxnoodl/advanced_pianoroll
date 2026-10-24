param([string]$Destination = '')
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
$packagedEditor = Join-Path $PSScriptRoot 'bin\Piano Roll Clip Editor.exe'
if (-not $Destination) {
    if (Test-Path -LiteralPath $packagedEditor) { $Destination = $PSScriptRoot }
    else { $Destination = Join-Path $project 'host-test\live-companion' }
}
$editor = Join-Path $project 'build\PianoRollClipEditor_artefacts\Release\Piano Roll Clip Editor.exe'
if (-not (Test-Path -LiteralPath $editor) -and (Test-Path -LiteralPath $packagedEditor)) { $editor = $packagedEditor }
if (-not (Test-Path -LiteralPath $editor)) { throw 'Build the Release clip editor first with build.ps1.' }
New-Item -ItemType Directory -Force -Path $Destination | Out-Null
& python (Join-Path $PSScriptRoot 'build_device.py') $Destination --resolve-paths
if ($LASTEXITCODE -ne 0) { throw 'Companion device build failed.' }
foreach ($file in @('bridge-max.js','bridge-node.js','bridge-transport.js','clip-protocol.js','README.md','build_device.py','install.ps1')) {
    $sourceFile = Join-Path $PSScriptRoot $file
    if ([IO.Path]::GetFullPath($sourceFile) -ne [IO.Path]::GetFullPath((Join-Path $Destination $file))) {
        Copy-Item -LiteralPath $sourceFile -Destination $Destination -Force
    }
}
$bin = Join-Path $Destination 'bin'
New-Item -ItemType Directory -Force -Path $bin | Out-Null
if ([IO.Path]::GetFullPath($editor) -ne [IO.Path]::GetFullPath((Join-Path $bin 'Piano Roll Clip Editor.exe'))) { Copy-Item -LiteralPath $editor -Destination $bin -Force }
Write-Output "Load this file in Live: $(Join-Path $Destination 'Piano Roll Live.amxd')"
