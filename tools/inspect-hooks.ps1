param(
    [Parameter(Mandatory)][string]$Dll,
    [Parameter(Mandatory)][string]$Baseline,
    [Parameter(Mandatory)][string]$Version,
    [Parameter(Mandatory)][string]$Source,
    [Parameter(Mandatory)][string]$BaselineSource,
    [string]$Python = 'python'
)
$ErrorActionPreference = 'Stop'
& $Python (Join-Path $PSScriptRoot 'inspect_hooks.py') --dll $Dll --baseline $Baseline --version $Version --source $Source --baseline-source $BaselineSource
if ($LASTEXITCODE) { throw 'Inspection failed; no completed profile entry was emitted.' }
