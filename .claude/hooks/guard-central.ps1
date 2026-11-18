# Claude Code PreToolUse hook for the tools that read files, write files or run commands.
# Applies the access rules of the knowledge center (docs/adr/0013-central-de-conhecimento.md):
#   licoes/     reading is allowed
#   docs/specs/ reading asks the owner every time
#   the rest    blocked, including from the terminal; writing is always blocked
# The location of the knowledge center comes from .claude/central.local.json, which stays out of Git.
# Tests: tests/hooks/guard-central.tests.ps1. This file is a protected "trava".

function ConvertTo-NormalPath {
    param([string] $Path, [string] $BaseDirectory = '')

    $value = ($Path.Trim().Trim('"', "'")) -replace '\\', '/'
    if ($value -match '^/([a-zA-Z])(/.*)?$') {
        $value = $Matches[1] + ':' + $Matches[2]
    }
    $isRooted = ($value -match '^[a-zA-Z]:(/|$)') -or $value.StartsWith('/')
    if (-not $isRooted -and $BaseDirectory) {
        $value = (ConvertTo-NormalPath -Path $BaseDirectory) + '/' + $value
    }

    $parts = New-Object System.Collections.Generic.List[string]
    foreach ($part in ($value -split '/')) {
        if ($part -eq '' -or $part -eq '.') { continue }
        if ($part -eq '..') {
            if ($parts.Count -gt 1) { $parts.RemoveAt($parts.Count - 1) }
            continue
        }
        $parts.Add($part)
    }
    return ($parts -join '/').ToLowerInvariant()
}

# The part of a search pattern before the first wildcard.
function Get-PathBeforeWildcard {
    param([string] $Pattern)

    $normal = $Pattern -replace '\\', '/'
    $wildcard = $normal.IndexOfAny([char[]]'*?[{')
    if ($wildcard -lt 0) { return $normal }
    $prefix = $normal.Substring(0, $wildcard)
    $slash = $prefix.LastIndexOf('/')
    if ($slash -lt 0) { return '' }
    return $prefix.Substring(0, $slash)
}

function New-Decision {
    param([string] $Decision, [string] $Reason = '')
    return @{ Decision = $Decision; Reason = $Reason }
}

# What the rules say about one path. $Central and $Target are already normalized.
function Get-PathDecision {
    param([string] $Target, [string] $Central, [bool] $IsWrite, [bool] $IsSearch)

    if ($Target -eq $Central -or $Target.StartsWith("$Central/")) {
        $inside = $Target.Substring($Central.Length).TrimStart('/')
        if ($IsWrite) {
            return New-Decision 'deny' 'Nothing is ever written in the knowledge center.'
        }
        if ($inside -eq 'licoes' -or $inside.StartsWith('licoes/')) {
            return New-Decision 'allow' 'The lessons of the knowledge center can be read.'
        }
        if ($inside -eq 'docs/specs' -or $inside.StartsWith('docs/specs/')) {
            return New-Decision 'ask' 'The specs of the knowledge center can only be read with the confirmation of the owner.'
        }
        return New-Decision 'deny' 'Only the lessons of the knowledge center can be read. Use /consultar-central.'
    }
    if ($IsSearch -and $Central.StartsWith("$Target/")) {
        return New-Decision 'deny' 'This search would also go through the knowledge center. Search a narrower folder.'
    }
    return New-Decision 'none'
}

function Get-CommandDecision {
    param([string] $Command, [string] $Central)

    $text = ($Command -replace '\\', '/').ToLowerInvariant()
    $leaf = $Central.Substring($Central.LastIndexOf('/') + 1)
    if ($text.Contains($leaf)) {
        return New-Decision 'deny' 'The knowledge center cannot be reached from the terminal. Use /consultar-central.'
    }

    $recursive = '(\bgrep\b[^;&|]*\s-[a-z]*r|\brg\b|\bfind\b|\btree\b|\bls\b[^;&|]*\s-[a-z]*r|\bget-childitem\b[^;&|]*-recurse|\bgci\b[^;&|]*-recurse|\bdir\b[^;&|]*-recurse)'
    if ($text -notmatch $recursive) {
        return New-Decision 'none'
    }
    $broad = 'A recursive search from this folder would also go through the knowledge center. Search a narrower folder.'
    if ($text -match '(^|[\s"''=])\.\.(/\.\.)*/?($|[\s"''|;&])') {
        return New-Decision 'deny' $broad
    }
    $ancestor = $Central
    while ($ancestor.Contains('/')) {
        $ancestor = $ancestor.Substring(0, $ancestor.LastIndexOf('/'))
        $bashForm = '/' + ($ancestor -replace ':', '')
        foreach ($form in @($ancestor, $bashForm)) {
            if ($text -match ('(^|[\s"''=])' + [regex]::Escape($form) + '/?($|[\s"''|;&])')) {
                return New-Decision 'deny' $broad
            }
        }
    }
    return New-Decision 'none'
}

function Get-CentralDecision {
    param(
        [string] $ToolName,
        $ToolInput,
        [string] $CentralPath,
        [string] $WorkingDirectory = ''
    )

    $central = ConvertTo-NormalPath -Path $CentralPath

    if ($ToolName -eq 'Bash' -or $ToolName -eq 'PowerShell') {
        return Get-CommandDecision -Command ([string]$ToolInput.command) -Central $central
    }

    $isWrite = @('Edit', 'Write', 'MultiEdit', 'NotebookEdit') -contains $ToolName
    $isSearch = @('Grep', 'Glob') -contains $ToolName
    $targets = New-Object System.Collections.Generic.List[string]

    if ($isSearch) {
        $root = [string]$ToolInput.path
        if ([string]::IsNullOrWhiteSpace($root)) { $root = $WorkingDirectory }
        if ($ToolName -eq 'Glob' -and $ToolInput.pattern) {
            $beforeWildcard = Get-PathBeforeWildcard -Pattern ([string]$ToolInput.pattern)
            $targets.Add((ConvertTo-NormalPath -Path $beforeWildcard -BaseDirectory $root))
        }
        else {
            $targets.Add((ConvertTo-NormalPath -Path $root -BaseDirectory $WorkingDirectory))
        }
    }
    else {
        foreach ($name in @('file_path', 'notebook_path')) {
            $value = [string]$ToolInput.$name
            if (-not [string]::IsNullOrWhiteSpace($value)) {
                $targets.Add((ConvertTo-NormalPath -Path $value -BaseDirectory $WorkingDirectory))
            }
        }
    }

    $result = New-Decision 'none'
    foreach ($target in $targets) {
        $decision = Get-PathDecision -Target $target -Central $central -IsWrite $isWrite -IsSearch $isSearch
        if ($decision.Decision -eq 'deny') { return $decision }
        if ($decision.Decision -ne 'none') { $result = $decision }
    }
    return $result
}

if ($MyInvocation.InvocationName -ne '.') {
    [Console]::InputEncoding = New-Object System.Text.UTF8Encoding($false)
    [Console]::OutputEncoding = New-Object System.Text.UTF8Encoding($false)

    function Stop-WithBlock([string] $Reason) {
        [Console]::Error.WriteLine("Blocked by .claude/hooks/guard-central.ps1: $Reason")
        exit 2
    }

    try {
        $payload = [Console]::In.ReadToEnd().TrimStart([char]0xFEFF) | ConvertFrom-Json
    }
    catch {
        Stop-WithBlock 'the request could not be read, so it was blocked for safety.'
    }

    $projectDirectory = if ($env:CLAUDE_PROJECT_DIR) { $env:CLAUDE_PROJECT_DIR } else { [string]$payload.cwd }
    $configuration = Join-Path $projectDirectory '.claude/central.local.json'
    if (-not (Test-Path -LiteralPath $configuration)) {
        exit 0
    }

    try {
        $centralPath = [string](Get-Content -LiteralPath $configuration -Raw -Encoding UTF8 | ConvertFrom-Json).path
    }
    catch {
        $centralPath = ''
    }
    if ([string]::IsNullOrWhiteSpace($centralPath)) {
        Stop-WithBlock '.claude/central.local.json is invalid: it needs a "path" field. Ask the owner to fix it.'
    }

    $decision = Get-CentralDecision -ToolName ([string]$payload.tool_name) -ToolInput $payload.tool_input `
        -CentralPath $centralPath -WorkingDirectory ([string]$payload.cwd)

    switch ($decision.Decision) {
        'deny' { Stop-WithBlock $decision.Reason }
        { $_ -eq 'allow' -or $_ -eq 'ask' } {
            $answer = @{ hookSpecificOutput = @{
                    hookEventName            = 'PreToolUse'
                    permissionDecision       = $decision.Decision
                    permissionDecisionReason = $decision.Reason
                } }
            [Console]::Out.WriteLine(($answer | ConvertTo-Json -Compress -Depth 4))
        }
    }
    exit 0
}
