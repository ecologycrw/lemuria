---
id: 0007
titulo: RmlUi atrás de uma camada própria e Dear ImGui só em ferramentas
status: aceita
data: 2026-11-18
substitui:
substituida-por:
---

# ADR 0007: RmlUi atrás de uma camada própria e Dear ImGui só em ferramentas

## Contexto

O gênero tem interface pesada: inventário em grade, trade, lojas, chat, party e guild. Cada jogo precisa da própria aparência (skins). As ferramentas de desenvolvimento e depuração têm necessidades bem diferentes.

## Decisão

- **Interface do jogo: RmlUi** (licença MIT). As telas são escritas numa linguagem parecida com HTML e CSS, com data binding, skins e animações.
- **O RmlUi fica atrás de uma camada própria**, para poder ser trocado se um dia falhar num requisito concreto.
- O desenho da interface passa pelo renderizador da engine.
- **Ferramentas e depuração: Dear ImGui** (licença MIT). Nunca aparece para o jogador.
- As duas bibliotecas entram no projeto só na fatia que precisar delas.

## Consequências

- Quem conhece HTML e CSS consegue criar e mudar telas com facilidade.
- A engine mantém a integração do RmlUi com o renderizador.
- O desempenho de telas com muitos elementos, como o inventário, precisa ser medido.

## Alternativas consideradas

- **Interface própria:** daria controle total, mas custaria anos de trabalho em layout, texto, foco, rolagem, digitação e data binding.
- **Dear ImGui na interface do jogo:** não foi feita para interfaces com skins.
- **NoesisGUI:** comercial e de código fechado, o que não combina com uma engine MIT.
- **Ultralight:** pesada e com licença restritiva.
