---
id: 0008
titulo: Ambição gráfica "clássico melhorado" e orçamentos de escala
status: aceita
data: 2026-11-18
substitui:
substituida-por:
---

# ADR 0008: Ambição gráfica "clássico melhorado" e orçamentos de escala

## Contexto

O público do gênero joga em PCs populares, e o estilo visual faz parte da identidade desses jogos. A cena mais difícil é um cerco com centenas de personagens equipados numa GPU integrada.

## Decisão

- **Estilo:** o dos MMORPGs do começo dos anos 2000, com o acabamento de hoje.
- **Entra:** nitidez em qualquer resolução, bordas suavizadas, animação interpolada, luzes dinâmicas simples, sombra suave nos personagens, halo (bloom), distorção, neblina e níveis de qualidade.
- **Fica de fora:** materiais realistas, iluminação global dinâmica e ray tracing.
- **Cada melhoria é uma opção** que o jogo pode ligar ou desligar.
- **Orçamentos:** personagens, partículas e efeitos têm limites, e a qualidade cai antes do FPS (INV-14).
- **Cena de referência:** um cerco medido com 200 e com 300 personagens. O número final sai da medição.
- **Direção técnica:** forward+, skinning na GPU, desenho em lote e menos detalhe no que está longe.

A ambição pode ser ajustada ao longo do desenvolvimento, por um novo ADR.

## Consequências

- A arquitetura de desenho precisa considerar a escala desde a primeira fatia gráfica.
- O piso de hardware é uma proposta até ser medido numa máquina adequada ([TD-0002](../TECH_DEBT.md)).

## Alternativas consideradas

- **Fiel ao clássico:** mais barato, mas com aparência datada nas telas de hoje.
- **Moderno, com materiais realistas:** mudaria o estilo e não rodaria bem em GPUs integradas.
