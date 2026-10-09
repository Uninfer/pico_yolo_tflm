param(
    [string]$Uf2 = (Join-Path $PSScriptRoot '..\build\pico_yolo_rp2350.uf2')
)

$resolvedUf2 = (Resolve-Path -LiteralPath $Uf2).Path
$drive = Get-Volume | Where-Object FileSystemLabel -eq 'RP2350' |
    Select-Object -First 1 -ExpandProperty DriveLetter

if (-not $drive) {
    throw 'RP2350 BOOTSEL drive not found. Hold BOOTSEL while reconnecting USB.'
}

Copy-Item -LiteralPath $resolvedUf2 -Destination "${drive}:\" -Force
Write-Host "Flashed $resolvedUf2 to ${drive}:"
