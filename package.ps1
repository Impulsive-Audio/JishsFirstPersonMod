param([string]$ModVersion = "2.1.1-1", [string]$ReSkateVersion = "2.1.1")

$ErrorActionPreference = 'Stop'

$releaseName = if ($ModVersion.StartsWith($ReSkateVersion)) { "JishsFirstPersonMod-$ModVersion" } else { "JishsFirstPersonMod-$ModVersion-reskate-$ReSkateVersion" }

$releasePath = Join-Path $PSScriptRoot ($releaseName + '.zip')

function Get-RelativePath([string]$Base, [string]$Full) {

    $Base = $Base.TrimEnd('\','/')

    return $Full.Substring($Base.Length).TrimStart('\','/').Replace('\','/')

}

function Add-FileToArchive($Archive, [string]$Full, [string]$Entry) {

    [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($Archive, $Full, $Entry, [IO.Compression.CompressionLevel]::Optimal) | Out-Null

}

function Add-TreeToArchive($Archive, [string]$Root, [string]$Prefix) {

    foreach ($file in (Get-ChildItem -LiteralPath $Root -Recurse -File)) {

        Add-FileToArchive $Archive $file.FullName ($Prefix + '/' + (Get-RelativePath $Root $file.FullName))

    }

}

Push-Location $PSScriptRoot

try {

    if (!(Test-Path release/dinput8.dll) -or

        !(Test-Path release/scripts/Custom/JishsFirstPersonMod.dll) -or

        !(Test-Path release/scripts/Custom/jishs_first_person_mod.lua) -or

        !(Test-Path release/scripts/Custom/FirstPerson/presets/bundled/recommended.ini)) { throw 'Run build.ps1 before packaging.' }

    $legacyDll = Join-Path $PSScriptRoot 'release/JishsFirstPersonMod.dll'

    if (Test-Path -LiteralPath $legacyDll) { Remove-Item -LiteralPath $legacyDll }

    $licensePaths = @('vendor/LICENSE','vendor/External/detours/LICENSE.txt','vendor/External/rapidjson/LICENSE.txt','vendor/External/imgui/LICENSE.txt')

    $licenseText = ($licensePaths | ForEach-Object { "--- $_ ---`n" + [IO.File]::ReadAllText((Join-Path $PSScriptRoot $_)) }) -join "`n`n"

    [IO.File]::WriteAllText((Join-Path $PSScriptRoot 'release/LICENSE.txt'),$licenseText,[Text.UTF8Encoding]::new($false))

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem

    # Build the install archive ourselves so entries always use forward slashes.
    # Compress-Archive on Windows PowerShell writes backslashes, which some
    # launcher/zip readers treat as literal filename characters.
    $stream = [IO.File]::Open($releasePath, [IO.FileMode]::Create)

    $archive = [IO.Compression.ZipArchive]::new($stream, [IO.Compression.ZipArchiveMode]::Create)

    try {

        Add-FileToArchive $archive (Join-Path $PSScriptRoot 'release/dinput8.dll') 'dinput8.dll'

        Add-FileToArchive $archive (Join-Path $PSScriptRoot 'release/LICENSE.txt') 'LICENSE.txt'

        Add-TreeToArchive $archive (Join-Path $PSScriptRoot 'release/Mods') 'Mods'

        Add-TreeToArchive $archive (Join-Path $PSScriptRoot 'release/scripts') 'scripts'

    } finally { $archive.Dispose(); $stream.Dispose() }

    $sourcePath = Join-Path $PSScriptRoot ($releaseName + '-source.zip')

    $stream = [IO.File]::Open($sourcePath, [IO.FileMode]::Create)

    $archive = [IO.Compression.ZipArchive]::new($stream, [IO.Compression.ZipArchiveMode]::Create)

    try {

        foreach ($file in (Get-ChildItem -LiteralPath $PSScriptRoot -Recurse -File)) {

            $relative = Get-RelativePath $PSScriptRoot $file.FullName

            if ($relative -match '^(build|release|\.hook-tools|\.git|\.vs)/|(^|/)__pycache__/|^(build-output|build|configure|tests)\.log$|\.zip$|\.pyc$') { continue }

            Add-FileToArchive $archive $file.FullName $relative

        }

    } finally { $archive.Dispose(); $stream.Dispose() }

    Get-FileHash -LiteralPath $releasePath,$sourcePath

} finally { Pop-Location }
