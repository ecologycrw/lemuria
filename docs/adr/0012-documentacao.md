---
id: 0012
titulo: Documentação como memória do projeto
status: aceita
data: 2026-11-18
substitui:
substituida-por:
---

# ADR 0012: Documentação como memória do projeto

## Contexto

A IA não lembra das sessões anteriores: o que está escrito no repositório é tudo o que ela sabe. Documentos demais diluem a atenção e gastam o limite de uso; documentos desatualizados são piores do que nenhum, porque a IA acredita neles.

## Decisão

- **Idiomas:** código em inglês; specs, planos, relatórios, ADRs e docs do projeto em português.
- **`AGENTS.md` é a fonte única de regras** para qualquer IA. O `CLAUDE.md` importa o `AGENTS.md` e acrescenta só o que é específico do Claude Code.
- **Specs em quatro arquivos por fatia** (`spec`, `plan`, `tasks`, `report`), com o status no front matter e critérios de aceitação numerados (`CA-xx`), cada um com a sua prova.
- **Três tipos de documento:**
  - vivos, atualizados no mesmo PR do código;
  - históricos, nunca editados, apenas substituídos;
  - de ambiente, que só mudam por proposta aprovada.
- **Mermaid** quando ajudar, com diagramas pequenos e validados na CI. O que puder ser gerado a partir do código é gerado.
- **Documentação mínima que cresce a partir do atrito real.** A documentação de um sistema nasce com a fatia que cria esse sistema.
- **Front matter** em specs, ADRs e STATUS, para que as checagens automáticas leiam esses arquivos.

## Consequências

- Toda sessão começa com o mesmo contexto essencial, curto.
- As checagens automáticas mantêm os documentos alinhados: links, status, critérios de aceitação com prova, ADRs em sequência.
- A cada cerca de 5 fatias, uma consolidação remove contradições e o que ficou desatualizado.

## Alternativas consideradas

- **Dezenas de documentos antes do código:** especulação que seria reescrita e diluiria o contexto.
- **Um único README gigante:** difícil de manter e de verificar.
