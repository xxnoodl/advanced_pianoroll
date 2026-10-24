param([string]$BundlePath = '')
$ErrorActionPreference = 'Stop'
$projectPath = Split-Path -Parent $PSScriptRoot
if (-not $BundlePath) { $BundlePath = Join-Path $projectPath 'build\PianoRoll_artefacts\Release\VST3\Piano Roll.vst3' }
$probePath = Join-Path $projectPath 'build\PianoRollHostProbe_artefacts\Release\PianoRollHostProbe.exe'
$integrationPath = Join-Path $projectPath 'build\Release\pianoroll_integration_tests.exe'
if (-not (Test-Path -LiteralPath $probePath)) { throw 'Build PianoRollHostProbe with PIANOROLL_VERIFY_BINARY=ON first; see verification/README.md.' }
$binaryPath = Join-Path $BundlePath 'Contents\x86_64-win\Piano Roll.vst3'
if (-not (Test-Path -LiteralPath $binaryPath)) { throw 'Expected a complete Windows x64 Piano Roll.vst3 bundle.' }
& $probePath $BundlePath (Join-Path $PSScriptRoot 'binary-host-report.json') | Tee-Object -FilePath (Join-Path $PSScriptRoot 'binary-host-log.txt')
if ($LASTEXITCODE -ne 0) { throw 'Compiled VST3 host verification failed.' }
& python -m unittest discover -s $PSScriptRoot -p 'test_*.py'
if ($LASTEXITCODE -ne 0) { throw 'MIDI comparison helper tests failed.' }
$fixturePath = Join-Path $PSScriptRoot 'fl-fixtures'
& python (Join-Path $PSScriptRoot 'fl_reference.py') generate $fixturePath
if ($LASTEXITCODE -ne 0) { throw 'Fixture generation failed.' }
& $integrationPath $fixturePath
if ($LASTEXITCODE -ne 0) { throw 'Prototype fixture processing failed.' }
$flResults = @()
$settingsPath = Join-Path $fixturePath 'confirmed-FL-settings.json'
$confirmedSettings = @()
if (Test-Path -LiteralPath $settingsPath) { $confirmedSettings = Get-Content -LiteralPath $settingsPath -Raw -Encoding UTF8 | ConvertFrom-Json }
foreach ($case in @('quick-quantize', 'quantize-starts', 'quick-chop', 'glue', 'legato')) {
    $expectedPath = Join-Path $fixturePath "$case.expected.mid"
    & python (Join-Path $PSScriptRoot 'fl_reference.py') compare $expectedPath (Join-Path $fixturePath "$case.prototype.mid") --report (Join-Path $fixturePath "$case.prototype-comparison.json")
    if ($LASTEXITCODE -ne 0) { throw "Prototype output differs from fixture expectation: $case" }
    $references = @($confirmedSettings | Where-Object { $_.case -eq $case })
    if (-not $references.Count -and (Test-Path -LiteralPath (Join-Path $fixturePath "$case.FL.mid"))) {
        $references = @([pscustomobject]@{ case=$case; file="$case.FL.mid"; stepBeats=0.25; snap='1/4 beat (fixture instruction)' })
    }
    if (-not $references.Count) { $flResults += [pscustomobject]@{ Case = $case; Status = 'pending FL export' }; continue }
    foreach ($reference in $references) {
        $flExportPath = Join-Path $fixturePath $reference.file
        if (-not (Test-Path -LiteralPath $flExportPath)) { throw "Confirmed FL reference file is missing: $flExportPath" }
        $prototypePath = Join-Path $fixturePath "$case.prototype.mid"
        if ([double]$reference.stepBeats -ne 0.25) {
            $stepArgument = ([double]$reference.stepBeats).ToString('0.######', [Globalization.CultureInfo]::InvariantCulture)
            & $integrationPath $fixturePath $stepArgument
            if ($LASTEXITCODE -ne 0) { throw 'Processing at the confirmed FL snap setting failed.' }
            $stepTag = ([double]$reference.stepBeats).ToString('F6', [Globalization.CultureInfo]::InvariantCulture)
            $prototypePath = Join-Path $fixturePath "$case.step-$stepTag.prototype.mid"
        }
        & python (Join-Path $PSScriptRoot 'fl_reference.py') compare $prototypePath $flExportPath --report (Join-Path $fixturePath "$($reference.file).comparison.json")
        if ($LASTEXITCODE -ne 0) { throw "FL behavior differs from prototype: $case at $($reference.snap) (see comparison report)" }
        $flResults += [pscustomobject]@{ Case=$case; Snap=$reference.snap; StepBeats=$reference.stepBeats; ReferenceFile=$reference.file; Status='matched' }
    }
}
$manifest = [pscustomobject]@{
    TimestampUtc = [DateTime]::UtcNow.ToString('o')
    TestedBinary = (Resolve-Path -LiteralPath $binaryPath).Path
    SHA256 = (Get-FileHash -LiteralPath $binaryPath -Algorithm SHA256).Hash
    BinaryHostChecksPassed = $true
    PrototypeFixtureComparisonsPassed = 5
    FLComparisons = $flResults
    AbletonSessionTest = 'pending manual host test'
}
$manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'verification-manifest.json') -Encoding UTF8
Write-Output 'Automated checks passed. FL equivalence remains pending for cases without a real FL export.'
