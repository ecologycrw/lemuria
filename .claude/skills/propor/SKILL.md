---
name: propor
description: Registra uma proposta (ideia, problema, regra que falta, bug fora do escopo) na fila docs/PROPOSALS.md do Lemuria, sem implementar. Use quando algo surgir fora da tarefa atual, ou quando o dono pedir algo novo.
---

# /propor

Registra; **não implementa.**

## Passos

1. **Número.** O próximo `P-NNNN` livre, contando as propostas abertas e as processadas.
2. **Entrada.** Na seção "Abertas" de `docs/PROPOSALS.md`, no formato de `templates/proposal.md`:
   - título curto;
   - **Origem:** fatia e tarefa, feedback do dono, medição ou checagem;
   - **Tipo:** fatia, regra, trava, skill, doc, decisão ou bug;
   - **Evidência:** concreta. Se veio do dono, use as palavras do dono;
   - **Prioridade sugerida;**
   - **Status:** `aberta`.
3. **Checagem.** Rode `py tools/checks/run.py docs terms`.
4. **Commit.** Se houver uma fatia em andamento, faça o commit na branch dela: `docs(NNNN): add proposal P-NNNN`. Se não houver, deixe para a próxima branch e avise o dono.
5. **Confirme ao dono** em uma linha: o número da proposta e quando ela será analisada (na próxima `/retrospectiva`).
