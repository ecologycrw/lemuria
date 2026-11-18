---
id: 0002
titulo: Três camadas com framework substituível, sem script por enquanto
status: aceita
data: 2026-11-18
substitui:
substituida-por:
---

# ADR 0002: Três camadas com framework substituível, sem script por enquanto

## Contexto

O desenvolvedor precisa usar a engine como base para o jogo dele. Alguns vão querer as mecânicas do gênero prontas. Outros vão querer criar o próprio framework.

## Decisão

- **Núcleo (Lemuria Core):** plataforma, gráficos, interface, som, conteúdo e conexões. Não sabe nada de gênero nem de jogo.
- **Framework Classic:** as mecânicas de cliente do gênero, configuráveis por dados. É oficial, mas **opcional**.
- **Jogo:** dados, arte, textos e adaptador de protocolo.
- As dependências só descem. O Núcleo não depende do Classic.
- **O Classic não tem porta dos fundos:** ele usa só o que o Núcleo oferece publicamente. Se precisar de algo novo, esse algo vira público para todos.
- Um **framework de exemplo**, compilado só com o Núcleo, prova na CI que a troca continua possível.
- **Linguagem do jogo:** C++ e dados. Uma linguagem de script fica para o futuro, por ADR, se surgir a necessidade.

## Consequências

- O desenvolvedor pode usar o Classic inteiro, adaptar partes dele ou ignorá-lo.
- A interface pública do Núcleo precisa ser boa o bastante para sustentar um framework completo.
- Sem script, mudar a lógica do jogo exige compilar C++.

## Alternativas consideradas

- **Engine monolítica com as mecânicas embutidas:** mais simples no começo, mas impediria um framework próprio.
- **Classic obrigatório:** obrigaria todo jogo a seguir as mesmas mecânicas.
- **Lua desde o início:** adicionaria uma biblioteca e uma camada de integração antes de existir a necessidade.
