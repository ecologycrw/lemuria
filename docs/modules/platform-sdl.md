# Módulo: Plataforma (PlatformSDL)

> Documento vivo. Nasce na fatia que cria o sistema e é atualizado no mesmo PR que muda o sistema.

## Responsabilidade

A única parte da engine que fala com o SDL e com o sistema operacional (INV-09): janela, entrada, relógio real, desenho com a SDL GPU, imagem em arquivo e conexões TCP. Ela **não** decide o que desenhar nem o que fazer com os bytes da rede: recebe a cena pronta do [Núcleo](core.md) e entrega bytes crus ao adaptador.

## Onde fica

| Alvo do build | Camada | Papel |
|---|---|---|
| `lemuria_platform_sdl` | Núcleo | biblioteca estática em `Engine/Source/Runtime/PlatformSDL`, namespace `lemuria::platform`. Depende do `lemuria_core`; o SDL3 e o SDL3_net ficam escondidos dentro dela |
| shaders em `Engine/Shaders/` | Núcleo | compilados durante a configuração do build e embutidos na biblioteca ([ADR 0019](../adr/0019-shaders-em-hlsl.md)) |

## Conceitos

| Cabeçalho | O que oferece |
|---|---|
| `Window.hpp` | `Window::open` e `poll_input`, que nunca espera. `InputFrame` guarda os cliques do quadro (até 8), o pedido de fechar e o tamanho da janela, sem pedir memória |
| `Renderer.hpp` | `Renderer::for_window` (desenha na janela) e `Renderer::offscreen` (desenha numa imagem, para a prova visual e os testes). `draw(cena)` só lê a cena. Escolhe Vulkan ou Direct3D 12 por `graphics_api`, ou deixa o sistema escolher |
| `Image.hpp` | `Image` em RGBA e `save_png` |
| `MonotonicClock.hpp` | `monotonic_nanoseconds()`, o relógio real. Só o laço principal do programa lê esse relógio |
| `PlatformInfo.hpp` | versão do SDL e teste da placa de vídeo sem janela |
| `TcpConnections.hpp` | o tipo de conexão TCP do [Núcleo](core.md). Os sockets vivem numa linha de execução própria, criada só na primeira conexão, que conversa com o quadro por lotes |
| `TcpListener.hpp` | um ponto de escuta só para o próprio computador, usado pelo servidor de teste e pelos testes |

**Os shaders:** `Shape.vert.hlsl` e `Shape.frag.hlsl` desenham blocos e caixas em lote (uma instância por forma). O `glslc` gera SPIR-V em todos os sistemas, e o `dxc` do Windows SDK gera DXIL no Windows. A configuração falha com uma mensagem clara se uma das ferramentas faltar.

**Limites do TCP:**

| Limite | Valor |
|---|---|
| Bytes por envio (`kMaxSendBytes`) | 64 KiB |
| Bytes esperando para sair, por conexão (`kMaxQueuedSendBytes`) | 1 MiB; acima disso, o envio é recusado |
| Bytes recebidos esperando o `poll` (`kMaxUndeliveredBytes`) | 1 MiB; acima disso, a leitura para, e o servidor é freado |
| Prazo para conectar e para fechar (`TcpTimeouts`) | 10 s e 2 s |

## Como usar

O laço do programa, como em `Samples/DemoGame/Source/Client/Private/Main.cpp`:

1. `Window::open`, depois `Renderer::for_window` (a janela precisa viver mais que o renderizador).
2. A cada quadro: `window.poll_input(input)`; se `input.quit_requested`, termina; `FrameClock::advance(monotonic_nanoseconds())`; atualiza o jogo; `renderer.draw(cena)`.
3. Para a rede, o programa cria um `TcpConnections` e o entrega ao adaptador.

Para gerar uma imagem: `Renderer::offscreen`, `draw`, `read_image` e `save_png`.

## Invariantes relevantes

- **INV-03:** o desenho recebe a cena só para leitura.
- **INV-04:** a rede roda em segundo plano, e o quadro só troca lotes com ela.
- **INV-09:** o SDL fica privado; nenhum cabeçalho público mostra um tipo do SDL ou de uma API gráfica ([ADR 0020](../adr/0020-fronteira-do-desenho.md)).
- **INV-13:** a troca de lotes reaproveita a memória, e os lotes de envio reservam 64 pedidos e 64 KB desde o começo.

## Testes

Ficam em `Engine/Source/Runtime/PlatformSDL/Tests`.

| Alvo | O que cobre |
|---|---|
| `platform_sdl_tests` | imagem em PNG, cliques do quadro, relógio que nunca volta, versão do SDL, `TcpListener` e `TcpConnections` com conexões reais no próprio computador: ordem dos bytes, prazos, queda, fechamento e limites (CA-14, CA-16, CA-17). As portas de teste vêm de `TcpTestSupport.hpp` |
| `platform_gpu_tests` (etiqueta `gpu`) | placa de vídeo sem janela, desenho numa imagem (fundo, blocos, profundidade e repetição) e janela. Roda com `py tools/dev.py gpu-test` |

## Itens provisórios

- [TD-0006](../TECH_DEBT.md): no Linux, o SDL é compilado sem janelas de verdade.
- [TD-0009](../TECH_DEBT.md): o desenho fala direto com a SDL GPU.
- [TD-0014](../TECH_DEBT.md): o teste de alocação não vê a memória do SDL e do driver.
- [TD-0016](../TECH_DEBT.md): um único passe de desenho, sem luzes nem sombras.
- [TD-0022](../TECH_DEBT.md): com Vulkan e a janela minimizada, o laço provavelmente não pausa.
- [TD-0023](../TECH_DEBT.md): o clique e o tamanho da janela usam unidades diferentes.
