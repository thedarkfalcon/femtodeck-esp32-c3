param(
  [string]$Fqbn = "esp32:esp32:esp32c3",
  [string]$Sketch = "femtodeck-c3"
)

$ErrorActionPreference = "Stop"

$versionPath = Join-Path $PSScriptRoot "femtodeck-c3\Version.h"
$content = Get-Content -Raw -LiteralPath $versionPath

$major = [regex]::Match($content, "VERSION_MAJOR = (\d+)").Groups[1].Value
$minor = [regex]::Match($content, "VERSION_MINOR = (\d+)").Groups[1].Value
$buildMatch = [regex]::Match($content, "BUILD_NUMBER = (\d+)")

if (-not $major -or -not $minor -or -not $buildMatch.Success) {
  throw "Could not read version fields from $versionPath"
}

$nextBuild = [int]$buildMatch.Groups[1].Value + 1
$buildText = "v$major.$minor b$nextBuild"

$content = $content -replace "(BUILD_NUMBER = )\d+(;)", "`${1}$nextBuild`$2"
$content = $content -replace '(BUILD_TEXT = ")[^"]+(";)', "`${1}$buildText`$2"

Set-Content -LiteralPath $versionPath -Value $content -NoNewline

Write-Host "Building $buildText"
arduino-cli compile --fqbn $Fqbn $Sketch
