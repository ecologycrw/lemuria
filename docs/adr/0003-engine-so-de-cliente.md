---
id: 0003
titulo: Engine só de cliente, com servidor de teste mínimo
status: aceita
data: 2026-11-18
substitui:
substituida-por:
---

# ADR 0003: Engine só de cliente, com servidor de teste mínimo

## Contexto

Existem servidores de MMORPG prontos, e cada desenvolvedor pode querer um diferente. O valor do Lemuria está no cliente. Mesmo assim, a engine precisa de algum servidor para ser testada e demonstrada.

## Decisão

- O Lemuria entrega **só o cliente**. Regras de jogo, economia e persistência pertencem ao servidor de cada desenvolvedor.
- O repositório inclui um **servidor de teste mínimo**, em C++, em `tools/test_server/`.
- Ele fala o protocolo próprio da demonstração e implementa só o que as fatias precisam provar.
- Ele **não é um produto:** não tem banco de dados, contas de verdade nem proteção para uso público.

## Consequências

- A demonstração e os testes de ponta a ponta rodam sem depender de nenhum projeto externo.
- Como o servidor de teste é escrito por nós, ele não prova que o encaixe funciona com servidores externos. Essa prova vem do adaptador da fatia 5 ([ADR 0004](0004-adaptador-de-protocolo.md)).
- O escopo do servidor de teste precisa ser vigiado para ele não virar um servidor de verdade.

## Alternativas consideradas

- **Reaproveitar o servidor do Elysium:** amarraria os dois projetos e traria uma segunda linguagem.
- **Só simulações dentro dos testes:** não exercitaria a rede de verdade e não permitiria uma demonstração.
- **Usar um servidor externo na demonstração:** traria para o repositório dependências e conteúdo de terceiros.
