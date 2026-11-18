---
id: 0016
titulo: Direção visual e prova visual
status: aceita
data: 2026-11-18
substitui:
substituida-por:
---

# ADR 0016: Direção visual e prova visual

## Contexto

O [ADR 0008](0008-classico-melhorado.md) escolheu as técnicas gráficas, mas não definiu a aparência desejada. As verificações também não olham a imagem: uma tela toda preta passaria em todas elas. Além disso, a fatia 1 precisa de um personagem de verdade, e a origem da arte só seria decidida na fatia 2.

## Decisão

- **O [guia visual](../VISUAL.md) define a direção de arte da demonstração:** um mundo escuro, aquecido por fogo, em que a luz guia o olhar.
- **As referências do dono ficam fora do repositório.** O guia descreve luz e clima com palavras próprias e guarda só medidas de brilho e cor. Mapas, modelos, nomes e interface são originais (INV-16).
- **Toda fatia que muda a tela tem prova visual:** uma cena fixa gera uma imagem, a IA e o revisor abrem essa imagem, os números são conferidos e só o dono aprova uma imagem como referência.
- **A fluidez é medida desde a fatia 0,** com o tempo médio e o tempo do pior 1% dos quadros.
- **A origem da arte passa a ser decidida na fatia 1,** antes de o primeiro personagem de verdade entrar.

## Consequências

- "Bonito" passa a ter critérios que podem ser conferidos a cada fatia.
- A fatia 0 fica maior, porque precisa entregar a geração da imagem, a medição e o medidor de fluidez.
- Até a fatia 0 existir, a prova visual é só uma regra escrita ([TD-0007](../TECH_DEBT.md)).
- Os números não substituem o olhar do dono: eles só recusam imagens claramente erradas.

## Alternativas consideradas

- **Deixar só o dono avaliar a imagem no fim da fatia:** os erros visuais seriam descobertos tarde e custariam mais para corrigir.
- **Guardar as referências no repositório:** traria imagens de outro jogo para um projeto público.
- **Conferir só números:** uma imagem pode acertar as medidas e continuar feia.
- **Decidir a arte na fatia 2:** a fatia 1 teria de escolher um personagem sem saber de onde a arte viria.
