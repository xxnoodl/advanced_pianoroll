param(
    [switch]$CoreOnly,
    [string]$JucePath = "",
    [ValidateSet("Debug", "Release")][string]$Configuration = "Release"
)
$ErrorActionPreference = "Stop"
$projectPath = $PSScriptRoot
$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmakeCommand) {
    $cmakePath = $cmakeCommand.Source
} else {
    $vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswherePath)) { throw "CMake and Visual Studio C++ Build Tools are required." }
    $cmakeCandidates = @(& $vswherePath -latest -products '*' -find '**\cmake.exe')
    if (-not $cmakeCandidates.Count) { throw "Install the C++ CMake tools component in Visual Studio Build Tools." }
    $cmakePath = $cmakeCandidates[0]
}
$buildPath = Join-Path $projectPath $(if ($CoreOnly) { 'build-core' } else { 'build' })
$configureArgs = @('-S', $projectPath, '-B', $buildPath, '-A', 'x64')
if ($CoreOnly) { $configureArgs += '-DPIANOROLL_CORE_ONLY=ON' }
else {
    $configureArgs += '-DPIANOROLL_CORE_ONLY=OFF'
    if (-not $JucePath) {
        $localJucePath = Join-Path $projectPath 'vendor\JUCE'
        if (Test-Path -LiteralPath (Join-Path $localJucePath 'CMakeLists.txt')) { $JucePath = $localJucePath }
    }
    if ($JucePath) { $configureArgs += "-DJUCE_SOURCE_DIR=$JucePath" }
}
& $cmakePath @configureArgs
if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed. If GitHub is unreachable, clone JUCE 8.0.12 outside this session into vendor\JUCE and rerun this script." }
& $cmakePath --build $buildPath --config $Configuration --parallel 4
if ($LASTEXITCODE -ne 0) { throw "Compilation failed." }
$ctestPath = Join-Path (Split-Path -Parent $cmakePath) 'ctest.exe'
& $ctestPath --test-dir $buildPath -C $Configuration --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "Tests failed." }
if (-not $CoreOnly) {
    Write-Output "Clip editor: $buildPath\PianoRollClipEditor_artefacts\$Configuration\Piano Roll Clip Editor.exe"
    Write-Output "Standalone: $buildPath\PianoRoll_artefacts\$Configuration\Standalone\Piano Roll.exe"
    Write-Output "VST3 bundle: $buildPath\PianoRoll_artefacts\$Configuration\VST3\Piano Roll.vst3"
}
