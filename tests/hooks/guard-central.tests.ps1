# Tests for .claude/hooks/guard-central.ps1. Runs in the CI (Windows) and locally through py tools/dev.py pytests.
# This file is a protected "trava".

$ErrorActionPreference = 'Stop'
$hook = Join-Path $PSScriptRoot '..\..\.claude\hooks\guard-central.ps1'
. $hook

$central = 'C:/Estudos/Central de Teste'
$project = 'C:/Estudos/Lemuria'
$failures = 0
$total = 0

function Assert-Decision {
    param([string] $Expected, [string] $Tool, [hashtable] $ToolInput)
    $script:total++
    $actual = (Get-CentralDecision -ToolName $Tool -ToolInput ([pscustomobject]$ToolInput) -CentralPath $central -WorkingDirectory $project).Decision
    if ($actual -ne $Expected) {
        Write-Output "FAIL: expected '$Expected' but got '$actual' for $Tool $($ToolInput | ConvertTo-Json -Compress)"
        $script:failures++
    }
}

# The lessons can be read.
Assert-Decision 'allow' 'Read' @{ file_path = "$central/licoes/INDEX.md" }
Assert-Decision 'allow' 'Read' @{ file_path = 'C:\Estudos\Central de Teste\licoes\a.md' }
Assert-Decision 'allow' 'Read' @{ file_path = 'c:/estudos/central de teste/LICOES/a.md' }
Assert-Decision 'allow' 'Read' @{ file_path = '/c/Estudos/Central de Teste/licoes/a.md' }
Assert-Decision 'allow' 'Read' @{ file_path = '../Central de Teste/licoes/a.md' }
Assert-Decision 'allow' 'Grep' @{ pattern = 'texture'; path = "$central/licoes" }
Assert-Decision 'allow' 'Glob' @{ pattern = '*.md'; path = "$central/licoes" }
Assert-Decision 'allow' 'Glob' @{ pattern = "$central/licoes/**/*.md" }

# The specs ask the owner.
Assert-Decision 'ask' 'Read' @{ file_path = "$central/docs/specs/a.md" }
Assert-Decision 'ask' 'Grep' @{ pattern = 'x'; path = "$central/docs/specs" }

# Everything else in the knowledge center is blocked.
Assert-Decision 'deny' 'Read' @{ file_path = "$central/Client/src/main.cpp" }
Assert-Decision 'deny' 'Read' @{ file_path = 'C:\Estudos\Central de Teste\Client\src\main.cpp' }
Assert-Decision 'deny' 'Read' @{ file_path = '/c/Estudos/Central de Teste/Server/README.md' }
Assert-Decision 'deny' 'Read' @{ file_path = "$central/knowledge/INDEX.md" }
Assert-Decision 'deny' 'Read' @{ file_path = "$central/docs/MAPA.md" }
Assert-Decision 'deny' 'Read' @{ file_path = "$central/CLAUDE.md" }
Assert-Decision 'deny' 'Read' @{ file_path = "$central/licoes/../Client/a.cpp" }
Assert-Decision 'deny' 'Read' @{ file_path = '../Central de Teste/Client/a.cpp' }
Assert-Decision 'deny' 'Read' @{ file_path = "$central/licoes-falsas/a.md" }
Assert-Decision 'deny' 'Grep' @{ pattern = 'x'; path = $central }
Assert-Decision 'deny' 'Grep' @{ pattern = 'x'; path = "$central/Client" }
Assert-Decision 'deny' 'Glob' @{ pattern = '**/*.cpp'; path = $central }
Assert-Decision 'deny' 'Glob' @{ pattern = "$central/Client/**" }
Assert-Decision 'deny' 'Glob' @{ pattern = '../Client/**'; path = "$central/licoes" }

# Searches that would go through the knowledge center from above are blocked.
Assert-Decision 'deny' 'Grep' @{ pattern = 'x'; path = 'C:/Estudos' }
Assert-Decision 'deny' 'Grep' @{ pattern = 'x'; path = '..' }
Assert-Decision 'deny' 'Glob' @{ pattern = '**/*.cpp'; path = 'C:/Estudos' }
Assert-Decision 'deny' 'Glob' @{ pattern = 'Central de Teste/Client/**'; path = 'C:/Estudos' }

# Nothing is ever written there.
Assert-Decision 'deny' 'Write' @{ file_path = "$central/licoes/nova.md" }
Assert-Decision 'deny' 'Edit' @{ file_path = "$central/licoes/INDEX.md" }
Assert-Decision 'deny' 'Edit' @{ file_path = "$central/docs/specs/a.md" }
Assert-Decision 'deny' 'NotebookEdit' @{ notebook_path = "$central/licoes/a.ipynb" }

# The terminal cannot reach it.
Assert-Decision 'deny' 'Bash' @{ command = 'cat "C:/Estudos/Central de Teste/licoes/a.md"' }
Assert-Decision 'deny' 'Bash' @{ command = 'cat "/c/Estudos/Central de Teste/Client/a.cpp"' }
Assert-Decision 'deny' 'Bash' @{ command = 'cd ../"Central de Teste" && ls' }
Assert-Decision 'deny' 'PowerShell' @{ command = 'Get-Content "C:\Estudos\Central de Teste\Client\a.cpp"' }
Assert-Decision 'deny' 'Bash' @{ command = 'grep -rn secret ..' }
Assert-Decision 'deny' 'Bash' @{ command = 'find ../.. -name "*.cpp"' }
Assert-Decision 'deny' 'Bash' @{ command = 'grep -r secret "C:/Estudos"' }
Assert-Decision 'deny' 'Bash' @{ command = 'rg secret /c/Estudos/' }
Assert-Decision 'deny' 'PowerShell' @{ command = 'Get-ChildItem -Recurse ..' }

# Normal work is not affected.
Assert-Decision 'none' 'Read' @{ file_path = "$project/docs/VISION.md" }
Assert-Decision 'none' 'Read' @{ file_path = 'docs/VISION.md' }
Assert-Decision 'none' 'Write' @{ file_path = "$project/src/a.cpp" }
Assert-Decision 'none' 'Grep' @{ pattern = 'x'; path = "$project/src" }
Assert-Decision 'none' 'Grep' @{ pattern = 'x' }
Assert-Decision 'none' 'Glob' @{ pattern = 'src/**/*.cpp' }
Assert-Decision 'none' 'Bash' @{ command = 'py tools/dev.py verify' }
Assert-Decision 'none' 'Bash' @{ command = 'grep -rn "CA-01" tests/' }
Assert-Decision 'none' 'Bash' @{ command = 'ls ..' }
Assert-Decision 'none' 'Bash' @{ command = 'cd .. && pwd' }
Assert-Decision 'none' 'Bash' @{ command = 'grep -r x "C:/Estudos/Lemuria/src"' }

# The real script: what it prints and how it exits.
function Invoke-Hook {
    param([string] $ProjectDirectory, [string] $Json)
    $start = New-Object System.Diagnostics.ProcessStartInfo
    $start.FileName = 'powershell'
    $start.Arguments = "-NoProfile -ExecutionPolicy Bypass -File `"$hook`""
    $start.UseShellExecute = $false
    $start.RedirectStandardInput = $true
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    $start.EnvironmentVariables['CLAUDE_PROJECT_DIR'] = $ProjectDirectory
    $process = [System.Diagnostics.Process]::Start($start)
    $bytes = (New-Object System.Text.UTF8Encoding($false)).GetBytes($Json)
    $process.StandardInput.BaseStream.Write($bytes, 0, $bytes.Length)
    $process.StandardInput.Close()
    $output = $process.StandardOutput.ReadToEnd()
    $errors = $process.StandardError.ReadToEnd()
    $process.WaitForExit()
    return @{ Code = $process.ExitCode; Output = $output; Errors = $errors }
}

function Assert-Hook {
    param([string] $Name, $Result, [int] $Code, [string] $Text)
    $script:total++
    if ($Result.Code -ne $Code -or -not ($Result.Output + $Result.Errors).Contains($Text)) {
        Write-Output "FAIL: $Name -> exit $($Result.Code), output '$($Result.Output)', errors '$($Result.Errors)'"
        $script:failures++
    }
}

$temporary = Join-Path ([System.IO.Path]::GetTempPath()) ([System.IO.Path]::GetRandomFileName())
New-Item -ItemType Directory -Path (Join-Path $temporary '.claude') | Out-Null
$configuration = Join-Path $temporary '.claude/central.local.json'
try {
    $read = '{"tool_name":"Read","cwd":"C:/Estudos/Lemuria","tool_input":{"file_path":"C:/Estudos/Central de Teste/%s"}}'

    Assert-Hook 'no configuration' (Invoke-Hook $temporary ($read -replace '%s', 'Client/a.cpp')) 0 ''

    Set-Content -LiteralPath $configuration -Value '{"path":"C:/Estudos/Central de Teste"}' -Encoding UTF8
    Assert-Hook 'code is blocked' (Invoke-Hook $temporary ($read -replace '%s', 'Client/a.cpp')) 2 'Only the lessons'
    Assert-Hook 'lessons are allowed' (Invoke-Hook $temporary ($read -replace '%s', 'licoes/a.md')) 0 '"permissionDecision":"allow"'
    Assert-Hook 'specs ask the owner' (Invoke-Hook $temporary ($read -replace '%s', 'docs/specs/a.md')) 0 '"permissionDecision":"ask"'
    Assert-Hook 'unreadable request' (Invoke-Hook $temporary 'not json') 2 'blocked for safety'

    Set-Content -LiteralPath $configuration -Value 'not json' -Encoding UTF8
    Assert-Hook 'invalid configuration' (Invoke-Hook $temporary ($read -replace '%s', 'licoes/a.md')) 2 'central.local.json is invalid'
}
finally {
    Remove-Item -LiteralPath $temporary -Recurse -Force
}

Write-Output "guard-central tests: $($total - $failures) of $total passed."
if ($failures -gt 0) { exit 1 }
exit 0
