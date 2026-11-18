---
id: 0020
titulo: Fronteira do desenho e troca futura da API gráfica
status: aceita
data: 2026-11-18
substitui:
substituida-por:
---

# ADR 0020: Fronteira do desenho e troca futura da API gráfica

## Contexto

O [ADR 0006](0006-sdl3-plataforma-e-graficos.md) escolheu a SDL GPU para o desenho. Ela não oferece alguns recursos avançados das APIs gráficas, como bindless e mesh shaders. O plano atual da engine não depende deles, mas o dono quer poder usar Vulkan puro, Direct3D ou outra API no futuro, sem que essa troca atinja os jogos.

Na fatia 0, o jogo, o Classic e o Núcleo já não conhecem a SDL GPU: eles só preenchem uma descrição da cena. Dentro da camada de plataforma, porém, o código de desenho fala direto com a SDL GPU. Com um único jeito de desenhar isso custa pouco, mas o custo de uma troca cresceria a cada luz, personagem animado ou efeito escrito da mesma forma.

## Decisão

**A troca é preparada em duas etapas.**

1. **Desde a fatia 0: a fronteira é a descrição da cena.**
   - O jogo, o Classic e o Núcleo só conhecem a descrição da cena e a interface pública do `Renderer`.
   - Nenhum tipo de API gráfica aparece em cabeçalhos públicos.
   - A descrição da cena diz **o que** desenhar, nunca **como** a placa de vídeo faz isso.
2. **Na fatia 2: o desenho é separado em duas partes**, antes de entrar a segunda forma de desenhar.
   - **O que desenhar:** as etapas de desenho, como chão, personagens e luzes. Esta parte não chama a SDL GPU.
   - **Como falar com a placa de vídeo:** uma camada interna e pequena, a única que chama a SDL GPU.
   - A camada interna nasce só com o que as etapas de desenho realmente usarem.
   - Uma checagem automática passa a recusar chamadas à SDL GPU fora dessa camada.

**Trocar de API gráfica continua exigindo um ADR próprio e a aprovação do dono.** A troca reescreve a camada interna, e os shaders recebem os ajustes que a nova API exigir.

## Consequências

- Uma troca de API gráfica não atinge os jogos, o Classic nem o Núcleo.
- A fatia 0 não fica maior.
- A fatia 2 ganha um trabalho obrigatório, registrado na [TD-0009](../TECH_DEBT.md).
- Até a fatia 2, uma troca exigiria reescrever todo o código de desenho, que ainda é pequeno.
- A camada interna não promete esconder todas as diferenças entre as APIs. Um recurso que só exista numa delas pode exigir mudanças nas etapas de desenho.

## Alternativas consideradas

- **Criar agora uma camada completa sobre as APIs gráficas:** atrasaria a fatia 0 e seria desenhada sem saber o que as próximas fatias vão precisar.
- **Não separar nada:** o custo de uma troca cresceria a cada fatia.
- **Trocar agora para Vulkan e Direct3D diretos:** exigiria muito mais código, sem um recurso que justifique isso hoje.
