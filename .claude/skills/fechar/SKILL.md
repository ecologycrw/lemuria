---
name: fechar
description: Fecha a fatia do Lemuria - escreve o report, atualiza a documentação viva e abre o Pull Request para o dono testar. Use depois do /revisar.
---

# /fechar

## Pré-condições

- Todas as tarefas `[x]`, `/verificar` verde e `/revisar` feito (seção "Revisão" preenchida no `tasks.md`).

## Passos

1. **Report.** Escreva `specs/NNNN-nome/report.md` a partir de `templates/report.md`:
   - a tabela de critérios com a prova de cada um;
   - a prova visual, se a fatia muda o que aparece na tela: as imagens para o dono aprovar, os números medidos e o tempo de quadro;
   - **"O que você deve testar":** passo a passo para o dono, com comandos prontos e o que deve aparecer;
   - desvios do plano, revisão, itens provisórios e propostas geradas;
   - travas alteradas (`git diff --name-only main...HEAD`, comparando com a lista de travas do `AGENTS.md`) e testes removidos, com o motivo.
2. **Documentação viva.**
   - `docs/ARCHITECTURE.md`, se algo do que ele descreve mudou.
   - Documentação de módulo para cada sistema novo, a partir de `templates/module.md`.
   - "Código de referência" em `docs/CONVENTIONS.md`, se surgiu um bom exemplo.
   - `docs/TECH_DEBT.md`: tire os itens resolvidos e inclua os novos.
   - ADR para cada decisão nova, a partir de `templates/adr.md`, com a linha no índice `docs/adr/README.md`.
3. **Status.** Mude a spec para `concluida`. Atualize o `docs/STATUS.md`: a fatia aguarda o teste do dono.
4. **Verifique.** Rode `/verificar`. A checagem de docs confere que todo critério tem prova.
5. **Commit e PR.**
   - Commit `docs(NNNN): close slice N` e `git push`.
   - Escreva a descrição do PR num arquivo temporário, em português, com: resumo, o que testar, critérios e provas, travas alteradas. Termine com a linha de atribuição do Claude Code.
   - Rode `gh pr create --base main --head fatia/NNNN-nome --title "Fatia N: <título>" --body-file <arquivo>`.
6. **Avise o dono:**
   - o link do PR e o passo a passo do que testar;
   - que o check verde no PR é a prova de que tudo passou;
   - que, depois do teste, o dono conta o que achou e a `/retrospectiva` roda no mesmo PR, antes da aprovação.

## Nunca

- Fazer merge. O merge é sempre do dono, pelo navegador.
