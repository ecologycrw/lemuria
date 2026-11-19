---
id: 0021
titulo: Onde a prova visual roda e qual é a tolerância da comparação
status: aceita
data: 2026-11-18
substitui:
substituida-por:
---

# ADR 0021: Onde a prova visual roda e qual é a tolerância da comparação

## Contexto

A prova visual compara a imagem da cena de prova com a imagem aprovada pelo dono. A imagem aprovada nasce no computador de desenvolvimento, mas a CI desenha sem placa de vídeo, com um renderizador por software. Os dois caminhos não geram imagens exatamente iguais, então a comparação precisa de uma tolerância: larga o bastante para aceitar essa variação e estreita o bastante para recusar uma mudança de verdade.

O Windows da CI não tem placa de vídeo, e a SDL GPU 3.4.20 não oferece um caminho para o renderizador por software do Direct3D 12 ([TD-0005](../TECH_DEBT.md)).

### O que foi medido

A cena de prova tem 1280 por 720 pontos, ou 921.600 no total. O renderizador por software foi medido num ambiente igual ao da CI: Ubuntu 24.04, Clang 18 e Mesa 25.2.8.

| Comparação | Pontos com alguma diferença | Pontos com diferença acima de 2 | Maior diferença (de 255) |
|---|---|---|---|
| A mesma API gráfica, duas vezes | 0 | 0 | 0 |
| Vulkan e Direct3D 12, na mesma placa de vídeo | 0 | 0 | 0 |
| Placa de vídeo e renderizador por software | 88.671 (9,62%) | 17 (0,002%) | 118 |

Entre a placa de vídeo e o renderizador por software, 88.654 pontos diferem em apenas 1 de 255, por arredondamento. Os outros 17 ficam na borda entre duas formas e assumem a cor do ponto vizinho.

Para saber o tamanho de uma mudança real, pequenas alterações foram feitas na cena, uma de cada vez:

| Mudança na cena | Pontos com diferença acima de 2 |
|---|---|
| Personagens 5% mais baixos | 0,090% |
| Cor de um personagem 12 unidades mais escura | 0,557% |
| Lateral das caixas 5% mais escura | 0,846% |
| Um personagem movido um bloco | 1,110% |
| Cor da pedra 6 unidades mais clara | 2,952% |
| Câmera inclinada meio grau | 3,570% |
| Câmera girada meio grau | 6,142% |
| Cor da trilha 4 unidades mais clara | 8,776% |
| Cor da sombra profunda 3 unidades mais clara | 13,543% |
| Cor do musgo 3 unidades mais clara | 19,059% |

## Decisão

- **Um ponto só conta como diferente quando algum canal de cor muda mais de 2 unidades, de 255.** O arredondamento medido é de 1 unidade, e a segunda unidade é a margem para outras placas de vídeo.
- **A imagem é recusada quando mais de 0,01% dos pontos são diferentes,** o que equivale a 92 pontos na cena de prova. Esse limite fica cerca de 5 vezes acima da diferença medida entre os renderizadores (17 pontos) e 9 vezes abaixo da menor mudança real medida (832 pontos).
- **A imagem aprovada fica em `tests/visual/approved/proof_scene.png`.** Sem ela, a comparação avisa e não falha, porque só o dono aprova uma imagem.
- **Na CI, a prova visual roda no Linux,** com o renderizador por software. **No Windows, ela roda no computador de desenvolvimento,** com Vulkan e Direct3D 12. Isso resolve a TD-0005.
- **A tolerância é medida de novo** quando a cena de prova mudar de natureza, como na fatia 2, com o chão pintado e as luzes, ou quando uma imagem correta for recusada em outra placa de vídeo.

## Consequências

- A CI pode conferir, sem placa de vídeo, a imagem aprovada no computador de desenvolvimento.
- Uma mudança de cor de até 2 unidades passa despercebida pela comparação. Os números da imagem e o olhar do dono continuam valendo.
- O Direct3D 12 não é conferido na CI. Ele só é testado no computador de desenvolvimento.
- A medição usou uma placa de vídeo e um renderizador por software. Outras placas podem exigir uma nova medição.

## Alternativas consideradas

- **Exigir imagens idênticas:** 9,62% dos pontos mudam só por arredondamento, e a CI recusaria toda imagem correta.
- **Limitar só a diferença de cor em cada ponto:** os 17 pontos de borda mudam até 118 unidades, e a CI recusaria uma imagem correta.
- **Usar uma nota geral de semelhança entre as imagens:** uma mudança pequena e concentrada, como um personagem mais baixo, quase não mexeria na nota.
- **Guardar uma imagem aprovada para cada renderizador:** o dono teria de aprovar várias imagens da mesma cena.
- **Rodar a prova no Windows da CI:** a SDL GPU não oferece um renderizador por software para o Direct3D 12.
