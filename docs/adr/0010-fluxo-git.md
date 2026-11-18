---
id: 0010
titulo: Fluxo Git com conta de máquina e merge só pelo dono
status: aceita
data: 2026-11-18
substitui:
substituida-por:
---

# ADR 0010: Fluxo Git com conta de máquina e merge só pelo dono

## Contexto

A IA precisa salvar e enviar o trabalho sozinha, mas a versão oficial só pode mudar com a aprovação do dono. O GitHub não deixa ninguém aprovar o próprio PR. Se a IA usar a conta do dono, ela ganha os mesmos poderes dessa conta, inclusive para fazer merge e desligar as proteções.

## Decisão

- **Conta de máquina** só para a IA (`lemuria-ci-dev`), com permissão *Write* no repositório. Ela faz os commits, envia as branches e abre os PRs. O dono aprova e faz o merge com a própria conta, pelo navegador.
- Na máquina de desenvolvimento, o `gh` e o Git ficam logados **só na conta da IA**.
- A identidade dos commits é configurada **só neste repositório**. Os commits trazem a linha `Co-Authored-By` do Claude.
- Os commits são **assinados com uma chave SSH** da conta da IA, **criada antes do primeiro commit**.
- **Uma branch por fatia** (`fatia/NNNN-nome`). Um commit por tarefa, se o build e os testes passarem. A branch é enviada automaticamente.
- O `/fechar` abre o PR. O dono aprova e faz o **squash merge**, que deixa um commit por fatia na `main`.
- A `main` é protegida por um ruleset do GitHub:
  - exige PR com 1 aprovação e revisão do code owner;
  - exige a CI verde em Windows e Linux;
  - bloqueia force push e exclusão;
  - exige commits assinados;
  - não tem exceções.
- Proibido para a IA: force push, push para a `main`, `--no-verify`, `git config`, `gh pr merge`, `gh auth` e `gh api`. Isso vale nas permissões, nos hooks e no GitHub.

O passo a passo está em [docs/setup/GITHUB.md](../setup/GITHUB.md).

## Consequências

- O histórico separa claramente o trabalho da IA (conta de máquina) das decisões do dono (aprovações e merges).
- Mesmo que todas as travas locais falhem, a `main` continua protegida no GitHub.
- O dono passa a usar a conta pessoal só pelo navegador neste PC.

## Alternativas consideradas

- **Commits na conta do dono:** o dono não conseguiria aprovar os PRs, e a IA teria poderes de administrador.
- **Reaproveitar a conta de máquina do Elysium:** misturaria o histórico e as permissões dos dois projetos.
- **Chave de assinatura depois do primeiro commit:** no Elysium, isso bloqueou o primeiro PR.
