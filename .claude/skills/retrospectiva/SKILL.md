---
name: retrospectiva
description: Processa o que a fatia do Lemuria ensinou - aplica a escada de erros, organiza as propostas e recomenda a próxima fatia. Roda no mesmo PR da fatia, depois que o dono testou e antes de aprovar o merge.
---

# /retrospectiva

## Pré-condições

- O PR da fatia está aberto e o dono já testou.
- A branch atual é a da fatia.

## Passos

1. **Feedback do dono.** Se houve problemas ou pedidos no teste, registre cada um com `/propor`, usando as palavras do dono. Um bug que impede a fatia de cumprir um critério de aceitação não é proposta: corrija na própria fatia (`/implementar`, `/verificar`) e atualize o report.
2. **Escada de erros.** Leia "Erros e lições" deste report e dos reports das fatias anteriores. Um erro que apareceu pela segunda vez vira proposta de regra; se puder ser verificado por máquina, a proposta é de teste, hook ou análise automática.
3. **Organize as propostas abertas.** Para cada uma, recomende: fatia nova, ADR, regra, trava ou descartar, com o motivo.
4. **Próxima fatia.** Proponha de 1 a 3 opções, com uma recomendação. Pese:
   1. a ordem do ROADMAP;
   2. as dependências;
   3. o risco, porque o mais arriscado vem primeiro;
   4. o que o dono pediu.
5. **Pergunte ao dono:** qual a próxima fatia e o que fazer com cada proposta. Apresente em português simples.
6. **Depois das respostas:**
   - `docs/ROADMAP.md`: mova a próxima fatia, ajuste os horizontes (o Claude Code vai pedir a confirmação do dono);
   - `docs/PROPOSALS.md`: mova as propostas decididas para "Processadas", com o destino;
   - `docs/STATUS.md`: `spec-atual` vazio, fase `entre-fatias`, próxima ação `/planejar`;
   - mudanças em travas que o dono aprovou: aplique (cada edição pede a confirmação do dono).
7. **Commit e push.** Commit `docs(NNNN): retrospective of slice N` e `git push`. Rode `/verificar`.
8. **Avise o dono** que o PR está pronto para a aprovação e o squash merge pelo navegador.
9. **A cada cerca de 5 fatias,** sugira um `/consolidar` depois do merge.
