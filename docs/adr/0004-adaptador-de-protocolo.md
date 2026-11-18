---
id: 0004
titulo: Adaptador de protocolo plugável
status: aceita
data: 2026-11-18
substitui:
substituida-por:
---

# ADR 0004: Adaptador de protocolo plugável

## Contexto

O Lemuria precisa funcionar com servidores que ele não controla, como o OpenMU. Cada servidor tem o próprio formato de mensagem, a própria criptografia e a própria sequência de conexões. Ao mesmo tempo, o repositório não pode conter o protocolo de outros jogos ([ADR 0009](0009-repositorio-publico-e-terceiros.md)).

## Decisão

O contrato tem dois níveis:

1. **Núcleo: conexões e bytes.** Abre e fecha conexões em segundo plano, entrega os bytes recebidos e envia os bytes pedidos. Nunca interpreta o conteúdo.
2. **Framework: eventos e pedidos.** O Classic define o vocabulário do gênero. O **adaptador**, escrito pelo desenvolvedor, traduz entre esse vocabulário e os bytes.

O contrato precisa aceitar:

- mais de uma conexão em sequência, controladas pelo adaptador;
- formato de mensagem e criptografia próprios de cada servidor;
- antecipação visual, com correção vinda do servidor;
- eventos que chegam antes de o mundo estar pronto;
- dados inválidos ou maliciosos, recusados pelo adaptador antes de virarem evento.

**O adaptador pertence ao projeto do desenvolvedor.** Este repositório contém apenas o adaptador da demonstração, com um protocolo próprio.

**Como o encaixe é provado:**

- toda fatia que alterar o contrato o compara com o fluxo real de um servidor externo, usando as lições da central de conhecimento;
- a fatia 5 constrói um adaptador de prova para o OpenMU, num repositório privado e separado.

## Consequências

- A engine não fica presa a um servidor nem a um protocolo.
- Mudar o contrato do adaptador exige a aprovação do dono.
- Até a fatia 5, o encaixe com um servidor externo não está provado ([TD-0003](../TECH_DEBT.md)).

## Alternativas consideradas

- **Um protocolo único da engine:** obrigaria cada servidor a se adaptar ao Lemuria, o que inviabiliza servidores existentes.
- **Adaptador do OpenMU dentro do repositório:** colocaria o protocolo de outro jogo num projeto público.
- **Adaptador em linguagem de script:** dependeria de uma decisão que ainda não foi tomada ([ADR 0002](0002-tres-camadas.md)).
