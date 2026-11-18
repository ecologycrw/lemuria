---
id: 0009
titulo: Repositório público MIT e "especificar, não copiar"
status: aceita
data: 2026-11-18
substitui:
substituida-por:
---

# ADR 0009: Repositório público MIT e "especificar, não copiar"

## Contexto

O Lemuria é open source e aprende com projetos de referência do gênero. Um deles, o MuMain, deriva das fontes de um cliente comercial e não declara licença. O OpenMU tem licença MIT, mas implementa o protocolo de um jogo comercial. Um projeto público não pode herdar esses riscos.

## Decisão

- **Repositório público, com licença MIT.**
- **Nada de outros jogos no repositório:** nomes de conteúdo, assets, textos, formatos de arquivo, protocolos ou código.
- **Especificar, não copiar:** o conhecimento entra só como lições genéricas, escritas com palavras próprias. Nenhum código do MuMain entra aqui, nem adaptado.
- **Menções a nomes** de jogos e servidores de terceiros ficam restritas aos documentos de `docs/`, para explicar o gênero ou um exemplo de uso.
- **Dependências e assets** só com licença compatível com a MIT, registrada.
- Uma checagem automática recusa termos proibidos no código, nos testes e nas ferramentas.
- Segredos nunca entram no repositório.

## Consequências

- O projeto pode ser usado e distribuído sem herdar problemas de licença.
- O desenvolvedor que quiser ligar a engine a um jogo existente assume essa responsabilidade no projeto dele.
- Toda escolha de biblioteca ou asset precisa conferir a licença.

## Alternativas consideradas

- **Repositório privado:** contraria o objetivo de ser uma engine open source.
- **Usar o MuMain como base:** daria um começo rápido, mas sem licença e com código herdado.
- **Incluir um adaptador e importadores do MU:** traria formatos e protocolo de um jogo comercial para um projeto público.
