param([ValidateSet('Debug', 'Release')][string]$Configuration = 'Release')
$ErrorActionPreference = 'Stop'
$projectPath = $PSScriptRoot
$artifactPath = Join-Path $projectPath "build\PianoRoll_artefacts\$Configuration"
$standalonePath = Join-Path $artifactPath 'Standalone\Piano Roll.exe'
$vstBundlePath = Join-Path $artifactPath 'VST3\Piano Roll.vst3'
if (-not (Test-Path -LiteralPath $standalonePath) -or -not (Test-Path -LiteralPath $vstBundlePath)) { throw 'Build both targets before packaging.' }
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$releaseVersion = (Select-String -LiteralPath (Join-Path $projectPath 'CMakeLists.txt') -Pattern 'project\(PianoRoll VERSION ([\d.]+)').Matches[0].Groups[1].Value
$packagePath = Join-Path $projectPath "dist\PianoRoll-$releaseVersion-win64-$stamp"
New-Item -ItemType Directory -Path $packagePath | Out-Null
Copy-Item -LiteralPath $standalonePath -Destination $packagePath
Copy-Item -LiteralPath $vstBundlePath -Destination $packagePath -Recurse
$sourcePath = Join-Path $packagePath 'SourceProject'
New-Item -ItemType Directory -Path $sourcePath | Out-Null
foreach ($name in @('Source', 'tests', 'companion', 'CMakeLists.txt', 'build.ps1', 'package.ps1', 'README.md', 'INVESTIGATION.md', 'VIDEO_REFERENCE.md', 'VALIDATION.md', 'THIRD_PARTY.md', 'rea-evidence.json')) {
    Copy-Item -LiteralPath (Join-Path $projectPath $name) -Destination $sourcePath -Recurse
}
$verificationSource = Join-Path $projectPath 'verification'
if (Test-Path -LiteralPath $verificationSource) {
    $verificationDestination = Join-Path $sourcePath 'verification'
    New-Item -ItemType Directory -Path $verificationDestination | Out-Null
    foreach ($name in @('README.md', 'RESULTS.md', 'run-checks.ps1', 'fl_reference.py', 'test_fl_reference.py', 'binary-host-report.json', 'verification-manifest.json')) {
        $verificationFile = Join-Path $verificationSource $name
        if (Test-Path -LiteralPath $verificationFile) { Copy-Item -LiteralPath $verificationFile -Destination $verificationDestination }
    }
    $fixtureSource = Join-Path $verificationSource 'fl-fixtures'
    if (Test-Path -LiteralPath $fixtureSource) {
        $fixtureDestination = Join-Path $verificationDestination 'fl-fixtures'
        New-Item -ItemType Directory -Path $fixtureDestination | Out-Null
        Get-ChildItem -LiteralPath $fixtureSource -File | Where-Object { $_.Extension -in @('.mid', '.json') -or $_.Name.EndsWith('.before-0.3.1') } | Copy-Item -Destination $fixtureDestination
    }
}
Copy-Item -LiteralPath (Join-Path $projectPath 'README.md') -Destination $packagePath
Copy-Item -LiteralPath (Join-Path $projectPath 'host-test\SYNTH-ROUTING.md') -Destination $packagePath
$juceLicensePath = Join-Path $projectPath 'vendor\JUCE\LICENSE.md'
if (Test-Path -LiteralPath $juceLicensePath) { Copy-Item -LiteralPath $juceLicensePath -Destination (Join-Path $packagePath 'JUCE-LICENSE.md') }
foreach ($previewName in @('pianoroll-preview.png', 'pianoroll-selection-preview.png', 'pianoroll-lfo-preview.png', 'pianoroll-250pct-ui-100.png', 'pianoroll-250pct-ui-125.png', 'pianoroll-250pct-ui-150.png', 'pianoroll-live-bridge-preview.png')) {
    $previewPath = Join-Path $projectPath "build\$previewName"
    if (Test-Path -LiteralPath $previewPath) { Copy-Item -LiteralPath $previewPath -Destination $packagePath }
}
$companionInstall = Join-Path $projectPath "host-test\live-companion-$releaseVersion"
if (-not (Test-Path -LiteralPath $companionInstall)) { $companionInstall = Join-Path $projectPath 'host-test\live-companion' }
if (Test-Path -LiteralPath $companionInstall) { Copy-Item -LiteralPath $companionInstall -Destination (Join-Path $packagePath 'LiveCompanion') -Recurse }
Compress-Archive -LiteralPath $packagePath -DestinationPath "$packagePath.zip"
Write-Output "$packagePath.zip"
