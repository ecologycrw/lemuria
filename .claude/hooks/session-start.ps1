# Claude Code SessionStart hook: prints where the project is, so every session starts with the same context.
# Whatever this script prints is added to the session context. This file is a protected "trava".

[Console]::OutputEncoding = New-Object System.Text.UTF8Encoding($false)
if ($env:CLAUDE_PROJECT_DIR) { Set-Location -LiteralPath $env:CLAUDE_PROJECT_DIR }

function Get-Section([string[]] $Lines, [string] $StartPattern, [string] $EndPattern) {
    $start = -1
    for ($i = 0; $i -lt $Lines.Count; $i++) {
        if ($start -lt 0 -and $Lines[$i] -match $StartPattern) { $start = $i; continue }
        if ($start -ge 0 -and $Lines[$i] -match $EndPattern) { return $Lines[$start..($i - 1)] }
    }
    if ($start -ge 0) { return $Lines[$start..($Lines.Count - 1)] }
    return @()
}

$branch = git rev-parse --abbrev-ref HEAD 2>$null
$log = git log --oneline -5 2>$null

$status = @()
if (Test-Path docs/STATUS.md) {
    $status = Get-Section (Get-Content docs/STATUS.md -Encoding UTF8) '^## Agora' '^## Bloqueios'
}

$openProposals = 0
if (Test-Path docs/PROPOSALS.md) {
    $open = Get-Section (Get-Content docs/PROPOSALS.md -Encoding UTF8) '^## Abertas' '^## Processadas'
    $openProposals = @($open | Where-Object { $_ -match '^### P-\d{4}' }).Count
}

Write-Output 'Lemuria - contexto do inicio da sessao (hook session-start)'
Write-Output "Branch atual: $branch"
Write-Output 'Ultimos commits:'
$log | ForEach-Object { Write-Output "  $_" }
Write-Output ''
Write-Output 'Trecho de docs/STATUS.md:'
$status | ForEach-Object { Write-Output "  $_" }
Write-Output ''
Write-Output "Propostas abertas em docs/PROPOSALS.md: $openProposals"
Write-Output 'Regras: AGENTS.md (carregado pelo CLAUDE.md). Para orientar o dono, use /situacao.'
exit 0
