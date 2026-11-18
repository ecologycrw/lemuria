# Guia visual

> Documento vivo. Mudanças exigem a aprovação do dono do produto. O motivo de cada regra está no [ADR 0016](adr/0016-direcao-visual-e-prova-visual.md).

Este guia define o que "bonito" significa na demonstração do Lemuria e como a IA prova que chegou lá. Cores, luzes e números daqui são **dados da demonstração**, nunca valores fixos no Núcleo ou no Classic (INV-06). Cada jogo feito com a engine pode ter o próprio estilo.

## A ideia central

**Um mundo escuro, aquecido por fogo, em que a luz é rara e por isso guia o olhar.**

1. **A sombra é a base.** A maior parte da tela fica em tons escuros, mas sem perder o desenho do chão.
2. **A luz é um acontecimento.** Fogueiras, tochas e magias criam ilhas de luz que puxam o olho.
3. **Quente contra escuro.** O âmbar do fogo contrasta com sombras marrons e esverdeadas.
4. **Tudo parece pintado à mão.** Grama, terra e pedra têm textura rica, sem brilho de material realista.
5. **O personagem nunca se perde.** Personagens e efeitos são o que há de mais legível na tela.

## De onde vem este guia

O dono escolheu duas capturas de tela de um jogo do gênero como referência de **luz e clima**. As imagens ficam só com o dono e não entram no repositório (INV-16).

| Usamos das referências | Nunca usamos |
|---|---|
| Como a luz e a sombra se distribuem | Mapas, disposição dos cenários e lugares |
| O equilíbrio entre escuro e claro | Modelos, texturas, ícones e efeitos |
| A sensação de cada ambiente | Nomes, textos e arte ou layout da interface |

**Regra de originalidade:** semelhança de gênero é bem-vinda, cópia não. Uma imagem que possa ser confundida com a tela de outro jogo está reprovada.

## Dois climas

| | Campo | Cidade ao anoitecer |
|---|---|---|
| Sensação | Natureza densa, um pouco ameaçadora | Abrigo, movimento e calor |
| Chão | Grama musgo, trilhas de terra, pedras e samambaias | Pedra gasta, canteiros e madeira |
| Luz principal | Clarões suaves entre manchas de sombra | Braseiros e tochas em pontos marcados |
| Luz de destaque | Fogueiras isoladas | Símbolos que brilham no piso das áreas importantes |
| Cor dominante | Verde-oliva escuro e marrom | Âmbar e marrom |

## Paleta

| Papel | Cor | Onde aparece |
|---|---|---|
| Sombra profunda | `#15120A` | Debaixo de árvores, cantos e frestas |
| Terra na sombra | `#2B2110` | Base dos chãos escuros |
| Musgo | `#3A3A16` | Grama em meia-luz |
| Folhagem iluminada | `#5C5A24` | Grama e plantas nos clarões |
| Trilha | `#6E5630` | Caminhos de terra e madeira clara |
| Pedra | `#4A4A44` | Rochas e construções, como descanso neutro para o olho |
| Água profunda | `#0A1C33` | Lagos e rios, o único azul do cenário |
| Fogo | `#E8862A` | Tochas, braseiros e fogueiras |
| Centro do fogo | `#FFC766` | Só no ponto mais forte de cada chama |
| Magia | `#3FE0B0`, `#4AA8FF`, `#A05CFF`, `#FFD34D` | **Reservadas** para efeitos, itens e informações importantes |

As cores de magia são as mais vivas da tela. Se o cenário usar essas cores, os efeitos perdem a força.

## Luz

- **Luz geral baixa:** nada fica totalmente preto, e o desenho do chão continua visível na sombra.
- **Manchas de sombra grandes e suaves:** elas cobrem boa parte do chão e quebram a repetição dos blocos. Pintadas no mapa, custam quase nada e funcionam em GPU integrada.
- **Luzes de fogo:** alcançam poucos blocos, perdem força aos poucos e tremulam de leve. O tremor usa o tempo injetado (INV-05).
- **Halo só no que brilha de verdade:** fogo, magia e itens. O cenário não ganha halo.
- **Cantos da tela um pouco mais escuros,** para levar o olhar ao centro.
- **Neblina leve,** só para dar profundidade ao que está longe.

## Câmera e composição

- Câmera alta em 3/4, olhando para baixo em um ângulo próximo de 50 graus.
- O personagem ocupa por volta de 8% a 10% da altura da tela.
- Trilhas, muros e fileiras de luz conduzem o olhar pelo mapa.
- Objetos aparecem em grupos, como pedras com plantas ao redor. Evite objetos soltos e espalhados por igual.
- Nenhum padrão de blocos pode ser visível de longe.

## Personagens e efeitos

- A silhueta é reconhecível mesmo sobre o chão escuro.
- Todo personagem tem uma sombra suave nos pés, para não parecer flutuar.
- Efeitos são curtos, vivos e fáceis de ler. Vários ao mesmo tempo não podem esconder os personagens.
- Quanto maior o nível de um item, mais forte o brilho.

## Interface

- Fundo escuro com molduras discretas, para não competir com o mundo.
- Cor forte só no que é informação: vida, energia, raridade e alertas.
- Texto sempre legível em qualquer resolução.
- Arte e layout são próprios e serão definidos na fatia 3.

## O que reprova uma imagem

- Tela vazia, preta, de uma cor só ou sem o objeto que a spec pede.
- Imagem lavada ou acinzentada, sem contraste entre luz e sombra.
- Sombra tão escura que o chão desaparece.
- Sombras de borda dura no cenário.
- Halo exagerado, que apaga os detalhes.
- Verde vivo de desenho animado.
- Blocos repetidos visíveis.
- Personagem que se confunde com o fundo.
- Semelhança com a tela de outro jogo.

## Números da imagem

Estas faixas transformam a aparência em algo que uma máquina consegue conferir. Elas vêm da medição das duas referências e valem para a área do mundo, sem a interface.

| Medida | Campo | Cidade | Faixa aceita |
|---|---|---|---|
| Brilho típico da imagem (mediana) | 0,14 | 0,16 | 0,11 a 0,19 |
| Parte da imagem em sombra profunda (brilho abaixo de 0,10) | 35% | 16% | 12% a 40% |
| Parte da imagem em tons escuros (brilho abaixo de 0,25) | 87% | 87% | 78% a 93% |
| Parte da imagem em pontos de luz (brilho acima de 0,60) | 0,6% | 0,8% | 0,2% a 2% |
| Parte da imagem em tons quentes | 90% | 97% | 75% ou mais |
| Saturação mediana | 0,72 | 0,68 | 0,50 a 0,80 |

- **Brilho:** `0,2126 × R + 0,7152 × G + 0,0722 × B`, com os valores finais da imagem entre 0 e 1.
- **Tom quente:** o vermelho é pelo menos 1,25 vez maior que o azul.
- **Saturação:** `(maior canal − menor canal) / maior canal`.

As faixas são um piso de qualidade, não a definição de beleza. Uma imagem pode passar nos números e ainda ser reprovada por quem olha. As faixas completas valem quando a cena já tem luz e materiais. Antes disso, a fatia diz na spec quais medidas ela já precisa cumprir.

## Prova visual

Vale para toda fatia que muda o que aparece na tela.

1. **Cena de prova.** A spec define uma cena fixa: mesma câmera, mesmos objetos, mesmo tempo e mesmos sorteios (INV-05). A mesma cena gera sempre a mesma imagem.
2. **A IA olha.** A cada tarefa visual, a IA gera a imagem, abre o arquivo e compara com este guia. Ela anota no `tasks.md` o que viu, inclusive o que ficou ruim. Rodar o comando sem abrir a imagem não conta.
3. **Os números conferem.** A medição da imagem precisa ficar dentro das faixas que a spec exigir.
4. **O revisor olha sozinho.** Ele abre as imagens sem confiar na descrição de quem fez o trabalho.
5. **O dono aprova.** O report mostra as imagens, e só o dono transforma uma imagem em referência aprovada. Depois disso, uma mudança que altere essa imagem sem aprovação é recusada.

## Fluidez

- **Meta:** 60 FPS, ou seja, cada quadro em até 16,6 ms.
- **O que medir:** o tempo médio e o tempo do pior 1% dos quadros na cena de prova. A média sozinha esconde os engasgos.
- **Sem atrapalhar o quadro:** o medidor não aloca memória durante o quadro (INV-13) e não espera por disco (INV-04).
- **Limite da medição:** o computador de desenvolvimento mostra a tendência, mas não comprova o piso de hardware ([TD-0002](TECH_DEBT.md)).

## O que a fatia 0 precisa entregar

- Um comando que desenha a cena de prova e salva a imagem em um arquivo.
- A medição dos números da imagem.
- A comparação com a imagem aprovada, com uma tolerância justificada.
- O medidor de fluidez.
- A definição de onde a prova roda na CI, já que o Windows da CI não tem placa de vídeo ([TD-0005](TECH_DEBT.md)).

## Quando cada parte chega

| Etapa do [roteiro](ROADMAP.md) | O que já precisa parecer certo |
|---|---|
| Fatia 0 | Câmera, paleta, tons do chão, prova visual e medidor de fluidez |
| Fatia 1 | Personagem legível sobre o chão escuro, com sombra nos pés |
| Fatia 2 | Equipamento e brilho de item por nível |
| Fatia 3 | Interface própria, seguindo este guia |
| Fatia 4 | Combate com efeitos e números legíveis |
| Efeitos | Luzes de fogo, sombras suaves, halo e neblina: o clima completo das referências |
