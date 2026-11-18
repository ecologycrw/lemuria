---
name: situacao
description: Mostra onde o projeto Lemuria está e qual é a próxima ação, em português simples para o dono do produto. Use quando o dono perguntar "onde estamos", "o que falta", "qual o próximo passo" ou ao retomar o trabalho.
---

# /situacao

Responde "onde estamos?" sem mudar nenhum arquivo. (O nome não é `/status` porque esse já é um comando interno do Claude Code.)

## Passos

1. Leia `docs/STATUS.md`, as seções "Agora" e "Próximas" de `docs/ROADMAP.md` e conte as propostas abertas em `docs/PROPOSALS.md`.
2. Se o STATUS tiver `spec-atual`, leia `specs/NNNN-*/spec.md` e `tasks.md` (quantas tarefas feitas, em andamento, bloqueadas).
3. Rode `git status --short` e `git log --oneline -5`. Se a branch atual tiver PR aberto, rode `gh pr view --json number,state,url,statusCheckRollup`.
4. Responda ao dono em português simples, usando os termos de `docs/GLOSSARY.md`:
   - **Onde estamos:** uma ou duas frases.
   - **O que foi feito desde o último marco.**
   - **O que está esperando por você:** aprovações, testes, decisões.
   - **Próxima ação recomendada:** qual comando (`/planejar`, `/implementar`, `/fechar`...) e por quê.
5. Se algo estiver inconsistente (STATUS diferente do que o Git mostra, tarefas marcadas mas sem commit), diga isso claramente.
