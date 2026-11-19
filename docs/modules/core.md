# Módulo: Núcleo (Core)

> Documento vivo. Nasce na fatia que cria o sistema e é atualizado no mesmo PR que muda o sistema.

## Responsabilidade

A base da engine: matemática, tempo e sorteio injetados, descrição da cena, medição da fluidez e a interface de conexões por bytes. Ele **não** fala com o SDL nem com o sistema operacional (isso é da [plataforma](platform-sdl.md)), não conhece o formato das mensagens de nenhum servidor e não sabe nada de um jogo específico.

## Onde fica

| Alvo do build | Camada | Papel |
|---|---|---|
| `lemuria_core` | Núcleo | biblioteca estática em `Engine/Source/Runtime/Core`, namespace `lemuria::core`. Depende só do `tl::expected` |

## Conceitos

| Cabeçalho | O que oferece |
|---|---|
| `Result.hpp` | `Result<V>` e `make_error`: toda operação que pode falhar devolve o erro como valor |
| `Version.hpp` | `engine_version()` e `parse_version`, que valida o texto recebido (INV-12) |
| `FrameClock.hpp` | `FrameClock::advance(agora)` transforma leituras do relógio em `FrameTime`. O primeiro quadro tem duração zero, um relógio que volta conta como zero e uma pausa longa é limitada a 0,25 s |
| `SeededRandom.hpp` | sorteio PCG32 com semente: a mesma sequência em todo compilador. Não serve para segredos |
| `Math.hpp` | `Vec2`, `Vec3`, `Vec4`, `Mat4`, `look_at`, `perspective`, projeção na tela e raio a partir da tela. Mão direita, Y para cima, chão no plano XZ |
| `SceneDescription.hpp` | a cena como dados: cor de fundo, blocos do chão e caixas, cada lista com capacidade fixa. Quando a lista enche, o item é recusado e contado |
| `FrameTimeMeter.hpp` | tempo médio e tempo do pior 1% dos quadros, depois do aquecimento. Reserva a memória no começo, então `record` não pede memória |
| `Connections.hpp` | a interface de conexões: abrir, enviar bytes, fechar e recolher eventos. Cada tipo de conexão publica o que oferece (`ConnectionTraits`): se entrega mensagens inteiras, se aceita envio sem garantia e o tamanho máximo de envio |

**As promessas das conexões**, garantidas pela classe base `Connections` para todo tipo de conexão:

- um número de conexão nunca se repete, e no máximo 8 conexões ficam abertas (`kMaxOpenConnections`);
- nenhuma chamada espera pela rede (INV-04), e tudo roda na linha de execução do quadro;
- uma conexão só aceita envio depois de `on_connected`, e `on_closed` é sempre o último evento;
- nenhum dado de uma conexão encerrada chega ao jogo, nem o que já estava a caminho;
- uma opção de envio que o tipo de conexão não atende devolve erro, e a garantia nunca é trocada em silêncio.

## Como usar

- **Um tipo de conexão novo** deriva de `Connections` e implementa `traits`, `start_connecting`, `queue_bytes`, `stop_connection` e `collect_events`. A base já validou o endereço, os bytes e as opções. O exemplo é `TcpConnections`, na [plataforma](platform-sdl.md).
- **O laço do programa** lê o relógio real da plataforma e passa a leitura para `FrameClock::advance`. Os testes passam leituras inventadas.
- **A cena** é preenchida pelo jogo a cada quadro e entregue ao desenho, que só a lê. O exemplo é `Samples/DemoGame/Source/Client/Private/GroundScene.cpp`.

## Invariantes relevantes

- **INV-03:** a cena é só dados, e o desenho a recebe sem poder alterá-la.
- **INV-04:** as conexões nunca esperam pela rede.
- **INV-05:** o tempo e o sorteio vêm de fora; o Núcleo nunca lê o relógio do sistema.
- **INV-07:** as conexões entregam bytes e nunca leem o conteúdo deles.
- **INV-13 e INV-14:** listas da cena e medidor de fluidez com capacidade fixa, reservada no começo.

## Testes

Ficam em `Engine/Source/Runtime/Core/Tests` (alvo `core_tests`).

| Arquivo | O que cobre |
|---|---|
| `ConnectionsTest.cpp` | as promessas da classe base, com um tipo de conexão de teste (CA-14, CA-16) |
| `FrameClockTest.cpp` | primeiro quadro, duração, limite de pausa e relógio que volta |
| `FrameTimeMeterTest.cpp` | média e pior 1% de 600 quadros, com aquecimento (CA-23) |
| `MathTest.cpp` | inversa, câmera, projeção e raio de ida e volta |
| `SceneDescriptionTest.cpp` | cores, ordem, capacidade e itens recusados |
| `SeededRandomTest.cpp` | sequência fixa, sementes diferentes e limites |
| `ResultTest.cpp` e `VersionTest.cpp` | resultado com erro e leitura de versão com texto inválido |

## Itens provisórios

- [TD-0011](../TECH_DEBT.md): a fluidez é medida na build de desenvolvimento.
