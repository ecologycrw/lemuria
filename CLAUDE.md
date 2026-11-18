@AGENTS.md

# Claude Code no Lemuria

O `AGENTS.md`, importado acima, contém as regras. Aqui fica só o que é específico do Claude Code.

## Skills

Os comandos do ciclo ficam em `.claude/skills/`: `/situacao`, `/planejar`, `/implementar`, `/verificar`, `/revisar`, `/fechar`, `/retrospectiva`, `/consolidar`, `/propor` e `/consultar-central`. Quando o dono pedir algo que corresponde a uma etapa, use a skill dessa etapa.

## Subagentes

- `revisor`: usado pelo `/revisar`. Trabalha com contexto limpo e só lê.
- `auditor`: usado pelo `/consolidar`.
- Para buscas amplas ou leitura de muitos arquivos, use subagentes de exploração com um modelo mais barato. Guarde o modelo principal para as decisões e o código difícil.

## Hooks e permissões

- **No início da sessão:** um hook mostra a branch, os últimos commits, o STATUS e as propostas abertas.
- **Antes de cada comando de terminal:** o `guard-bash.ps1` bloqueia comandos proibidos e explica o motivo.
- **Antes de cada acesso a arquivos:** o `guard-central.ps1` aplica as regras de acesso à central de conhecimento.
- **Ao terminar um turno com alterações não commitadas:** o `stop-check.ps1` roda a checagem de docs e o build. Se falhar, corrija antes de encerrar.
- **Editar uma trava** pede a confirmação do dono. Explique antes por que a mudança é necessária.

## Sessões

Abra o Lemuria sozinho numa sessão. A central de conhecimento e o Elysium têm skills e regras próprias, e misturá-los aqui causa conflito ([ADR 0013](docs/adr/0013-central-de-conhecimento.md)).

## Commits e PRs

- A identidade da conta de máquina (`lemuria-ci-dev`) já está configurada neste repositório. Não mude o `git config`.
- Termine cada mensagem de commit com a linha `Co-Authored-By` indicada pelo Claude Code.
- A descrição do PR é em português e termina com a linha de atribuição do Claude Code.
