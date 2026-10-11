# Generates the three short sound effects the game looks for:
#   click.wav   - a soft blip for every button press
#   correct.wav - a small rising arpeggio for a right answer
#   wrong.wav   - a small falling tone for a wrong answer
#
# They are written as 16 bit mono 44.1 kHz WAV files, the plainest format
# SFML reads without any extra library. Running this script again simply
# overwrites them, so the files can be edited or replaced at any time.
#
#   powershell -ExecutionPolicy Bypass -File tools\make_sounds.ps1

param(
    [string]$OutFolder = (Join-Path $PSScriptRoot "..\assets\sounds")
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $OutFolder)) {
    New-Item -ItemType Directory -Force -Path $OutFolder | Out-Null
}

$SampleRate = 44100

# --- a very small sine wave generator ---------------------------------------
# One note is a frequency, a length in milliseconds, a volume and a little
# fade-in/fade-out so the note never clicks at its edges.
function New-Note([double]$freq, [double]$ms, [double]$volume, [double]$fadeMs = 8) {
    $count = [int]($SampleRate * $ms / 1000.0)
    $fadeSamples = [int]($SampleRate * $fadeMs / 1000.0)
    $samples = New-Object 'System.Collections.Generic.List[double]'

    for ($i = 0; $i -lt $count; $i++) {
        # A quiet second harmonic makes the tone sound less like a test beep.
        $value = [Math]::Sin(2 * [Math]::PI * $freq * $i / $SampleRate)
        $value += 0.25 * [Math]::Sin(4 * [Math]::PI * $freq * $i / $SampleRate)

        $envelope = 1.0
        if ($fadeSamples -gt 0) {
            if ($i -lt $fadeSamples) {
                $envelope = $i / $fadeSamples
            } elseif ($i -ge ($count - $fadeSamples)) {
                $envelope = ($count - $i) / $fadeSamples
            }
        }

        $samples.Add($value * $volume * $envelope)
    }
    # The comma stops PowerShell from spreading the list out over the
    # pipeline. Without it the caller would receive loose numbers, not a list.
    return ,$samples
}

# --- joins notes and adds a touch of silence between them -------------------
function Join-Notes($parts, [double]$gapMs) {
    $result = New-Object 'System.Collections.Generic.List[double]'
    $first = $true
    foreach ($part in $parts) {
        if (-not $first) {
            $gap = [int]($SampleRate * $gapMs / 1000.0)
            for ($i = 0; $i -lt $gap; $i++) { $result.Add(0.0) }
        }
        $result.AddRange($part)
        $first = $false
    }
    return ,$result
}

# --- writes the samples into a 16 bit mono WAV file -------------------------
function Write-Wav([string]$path, $samples) {
    $count = $samples.Count
    $dataSize = $count * 2

    $ms = New-Object System.IO.MemoryStream
    $bw = New-Object System.IO.BinaryWriter($ms)

    $ascii = [System.Text.Encoding]::ASCII

    $bw.Write($ascii.GetBytes("RIFF"))
    $bw.Write([int](36 + $dataSize))
    $bw.Write($ascii.GetBytes("WAVE"))

    $bw.Write($ascii.GetBytes("fmt "))
    $bw.Write([int]16)                 # size of this block
    $bw.Write([int16]1)                # 1 = PCM
    $bw.Write([int16]1)                # 1 channel = mono
    $bw.Write([int]$SampleRate)
    $bw.Write([int]($SampleRate * 2))  # bytes per second
    $bw.Write([int16]2)                # block align
    $bw.Write([int16]16)               # bits per sample

    $bw.Write($ascii.GetBytes("data"))
    $bw.Write([int]$dataSize)

    foreach ($s in $samples) {
        $clamped = [Math]::Max(-1.0, [Math]::Min(1.0, $s))
        $bw.Write([int16]($clamped * 32000))
    }

    $bw.Flush()
    [System.IO.File]::WriteAllBytes($path, $ms.ToArray())
    $bw.Dispose()
    $ms.Dispose()
}

# --- the three effects ------------------------------------------------------
# A soft, low blip. Used for every button, so it stays quiet and short.
$click = Join-Notes @((New-Note 880 45 0.18), (New-Note 1320 35 0.10)) 0

# C5 - E5 - G5: a small rising arpeggio, the usual "that is right" sound.
$correct = Join-Notes @((New-Note 523.25 110 0.26),
                        (New-Note 659.25 110 0.26),
                        (New-Note 783.99 200 0.24)) 8

# A falling pair with a little shake, the usual "not this time" sound.
$wrong = Join-Notes @((New-Note 320 130 0.26), (New-Note 210 220 0.24)) 10

Write-Wav (Join-Path $OutFolder "click.wav")   $click
Write-Wav (Join-Path $OutFolder "correct.wav") $correct
Write-Wav (Join-Path $OutFolder "wrong.wav")   $wrong

Write-Output ""
Write-Output "Sound files written to $OutFolder"
Get-ChildItem -Path $OutFolder -Filter *.wav | ForEach-Object {
    Write-Output ("  {0,-14} {1,7} bytes" -f $_.Name, $_.Length)
}
