---
name: implementar
description: Implementa a fatia aprovada do Lemuria, uma tarefa por vez, com teste, verificação e commit a cada tarefa. Use depois que o dono aprovou a spec, ou para continuar uma fatia em andamento.
---

# /implementar

## Pré-condições

- A spec da fatia atual está `aprovada` ou `em-andamento`.
- A branch atual é `fatia/NNNN-nome`.

## Passos

1. **Lista de tarefas.** Se `tasks.md` não existe, crie a partir de `templates/tasks.md`. Divida o plano em tarefas pequenas (T01, T02...), cada uma com o critério que atende e como verificar. Mude a spec para `em-andamento`. Commit `docs(NNNN): add task list`.
2. **Uma tarefa por vez,** sempre a próxima pendente:
   1. Marque `[~]` no `tasks.md`.
   2. Escreva o teste primeiro, quando possível. O teste que prova um critério recebe a etiqueta `[NNNN-CA-xx]` no `TEST_CASE`.
   3. Implemente seguindo `docs/CONVENTIONS.md` e o código de referência citado lá. Respeite as invariantes de `docs/ARCHITECTURE.md`.
   4. Rode as verificações da skill `/verificar`.
   5. Se tudo passou: marque `[x]`, faça o commit `tipo(NNNN): Txx descrição` e `git push`.
   6. Se não passar depois de tentativas razoáveis: marque `[!]`, descreva o bloqueio em "Bloqueios" no `tasks.md`, pare e pergunte ao dono.
3. **Ao terminar todas as tarefas,** diga ao dono que a próxima etapa é `/revisar`.

## Durante o trabalho

- **Fora do escopo:** registre com `/propor` e não implemente. A exceção é algo que impede a tarefa atual: nesse caso, pare e pergunte.
- **Simplificação provisória nova:** registre em `docs/TECH_DEBT.md` com a fatia de destino.
- **Biblioteca nova, mudança nos formatos de conteúdo, no contrato do adaptador ou na arquitetura:** só com a aprovação do dono.
- **Dúvida sobre como um projeto de referência funciona:** use `/consultar-central`. Nunca procure o código desses projetos.
- **Sessão longa:** antes que o contexto fique grande demais, atualize "Notas para a próxima sessão" no `tasks.md`, faça o commit e sugira continuar numa sessão nova.

## Nunca

- Enfraquecer ou apagar um teste para "fazer passar".
- Desligar uma checagem, mudar uma trava sem aprovação ou usar `--no-verify`.
- Dizer que algo passou sem ter rodado.
