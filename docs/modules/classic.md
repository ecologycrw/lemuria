# Módulo: Classic

> Documento vivo. Nasce na fatia que cria o sistema e é atualizado no mesmo PR que muda o sistema.

## Responsabilidade

O framework oficial: as mecânicas de cliente de um MMORPG clássico em grade, configuradas por dados. Ele guarda a sessão, o mundo e o movimento, calcula caminhos, posiciona a câmera 3/4 e define o contrato do adaptador de protocolo. Ele **não** conhece o formato das mensagens de nenhum servidor (isso é do adaptador), não fala com o SDL e não tem números ou nomes de um jogo específico (INV-06).

## Onde fica

| Alvo do build | Camada | Papel |
|---|---|---|
| `lemuria_classic` | Classic | biblioteca estática em `Frameworks/Classic`, namespace `lemuria::classic`. Depende só do `lemuria_core` |

## Conceitos

**Contrato do adaptador** ([ADR 0017](../adr/0017-contrato-do-adaptador.md)):

| Cabeçalho | O que oferece |
|---|---|
| `ProtocolAdapter.hpp` | o que cada jogo escreve: `capabilities`, `begin_session`, `poll`, `send_request` e `end_session`. Tudo roda no quadro, e nada espera pela rede |
| `AdapterCapabilities.hpp` | o que o servidor do jogo faz: tipo de conexão, se a caminhada leva os passos ou só o destino, máximo de passos por pedido, aviso de mapa carregado, sinal de presença e o que fazer quando a conexão cai. `check_capabilities` recusa combinações sem sentido |
| `GameEvent.hpp` | os eventos que o adaptador entrega: fases da sessão, fim da sessão (sempre o último), entrada no mapa, personagem que aparece, anda ou sai, confirmação, correção e blocos que mudaram |
| `GameRequest.hpp` | os pedidos do jogo ao servidor: caminhar e avisar que o mapa carregou. Silêncio do servidor vale como um "sim" provisório |
| `GameEventQueue.hpp`, `WalkSteps.hpp`, `TileCoord.hpp`, `WorldIds.hpp` | fila de eventos com capacidade fixa, lista de até 32 passos, coordenada de bloco e números de personagem e de mapa |

Eventos e pedidos são valores simples, de tamanho fixo, que podem ser guardados sem pedir memória.

**Sessão, mundo e movimento:**

| Cabeçalho | O que oferece |
|---|---|
| `SessionPhase.hpp` e `GameSession.hpp` | `GameSession` conduz as fases (conectar, entrar no mapa, estar no mundo, trocar de mapa) com prazos em `SessionLimits`. Os eventos que chegam enquanto o mapa carrega ficam guardados e são soltos aos poucos, na ordem. Um bilhete de carregamento antigo não muda nada |
| `World.hpp` | `World` guarda o jogador e os outros personagens, com lugar reservado desde `create`. O jogador começa a andar na hora (`walk_own_to`), o pedido sai por `take_walk_request`, uma correção do servidor vence e uma resposta antiga é ignorada e contada |
| `EntityMotion.hpp` | o movimento de um personagem, igual em qualquer taxa de quadros. Uma correção desliza até o ponto certo, sem salto |
| `TileGrid.hpp` e `PathFinder.hpp` | a grade (até 1.024 blocos por lado) e o cálculo de caminho, com a regra de diagonal escolhida pelo jogo. O destino precisa estar livre; a origem pode estar bloqueada |
| `ThreeQuarterCamera.hpp` | a câmera 3/4 que acompanha um ponto e diz qual bloco está sob o cursor |

## Como usar

O jogo liga as peças assim, como em `Samples/DemoGame/Source/Client/Private/DemoGame.cpp`:

1. `World::create(máximo de outros personagens, regras de movimento)` e `GameSession(adaptador, prazos, ouvinte)`, depois `session.start()`.
2. Em `on_map_entered`: `world.enter_map`; quando o mapa carregar, `world.set_grid` (que já prepara o cálculo de caminho) e só depois `session.finish_map_load(bilhete)`.
3. Em `on_world_event`: `world.apply(evento)`.
4. A cada quadro: `session.update`, `world.update`, `world.take_walk_request()` e `adapter.send_request`.
5. O clique: `camera.tile_under(ponto da tela)` e `world.walk_own_to(bloco)`.

O modelo de um jogo completo, sem janela, é o `TestGame` de `Frameworks/Classic/Tests/MisbehavingServerTest.cpp`.

## Invariantes relevantes

- **INV-01:** o jogador anda antes da resposta, mas a correção do servidor sempre vence.
- **INV-02 e INV-06:** depende só do Núcleo, e os números do jogo chegam por dados (`MovementRules`, `CameraSettings`, `SessionLimits`).
- **INV-04:** nada espera pela rede.
- **INV-12:** eventos sobre personagens desconhecidos, confirmações antigas e mapas que não contêm o jogador são recusados e contados.
- **INV-13 e INV-14:** fila, mundo e cálculo de caminho reservam a memória no começo.

## Testes

Ficam em `Frameworks/Classic/Tests` (alvo `classic_tests`). As peças de teste compartilhadas são `AdapterTestSupport.hpp` (adaptador e conexão de teste) e `WorldTestSupport.hpp`.

| Arquivo | O que cobre |
|---|---|
| `ThreeQuarterCameraTest.cpp` | bloco sob o cursor, inclusive nas bordas (CA-02) |
| `PathFinderTest.cpp` e `TileGridTest.cpp` | caminho que desvia, diagonais, limites da busca e da grade (CA-03) |
| `EntityMotionTest.cpp` e `WorldMovementTest.cpp` | movimento em qualquer taxa de quadros, antecipação, correção e destino encurtado (CA-03 a CA-07, CA-15) |
| `WorldTest.cpp` | personagens, grade e troca de mapa (CA-11, CA-12, CA-13) |
| `GameSessionTest.cpp` | fases, prazos, eventos guardados e motivos de fim (CA-13, CA-14) |
| `ProtocolAdapterTest.cpp` e `AdapterCapabilitiesTest.cpp` | adaptadores com capacidades e tipos de conexão diferentes (CA-15, CA-16) |
| `MisbehavingServerTest.cpp` | servidores que se comportam mal, com dois adaptadores diferentes (CA-13 a CA-16) |
| `GameEventQueueTest.cpp` e `WalkStepsTest.cpp` | fila e lista de passos com capacidade fixa |

## Itens provisórios

- [TD-0003](../TECH_DEBT.md): o contrato ainda não foi provado com um servidor externo.
- [TD-0012](../TECH_DEBT.md): a busca de caminho limpa a memória do mapa inteiro.
- [TD-0018](../TECH_DEBT.md) e [TD-0019](../TECH_DEBT.md): reconexão e sinal de presença ainda não existem.
- [TD-0021](../TECH_DEBT.md): as regras de movimento repetem o que o adaptador declara.
