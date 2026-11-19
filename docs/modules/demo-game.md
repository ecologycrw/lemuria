# Módulo: Jogo de demonstração (DemoGame)

> Documento vivo. Nasce na fatia que cria o sistema e é atualizado no mesmo PR que muda o sistema.

## Responsabilidade

O jogo de exemplo que prova a engine: o protocolo próprio da demonstração, o cliente com janela, o servidor de teste e as peças dos testes com rede real. Ele é o modelo de como um desenvolvedor liga o [Classic](classic.md) ao servidor do jogo dele. O servidor de teste **não** é um produto: existe só para os testes e para a demonstração ([ADR 0003](../adr/0003-engine-so-de-cliente.md)).

## Onde fica

| Alvo do build | Camada | Papel |
|---|---|---|
| `demo_protocol` | Jogo | mensagens, enquadramento e mapa da demonstração (`Samples/DemoGame/Source/Protocol`). Depende só do `lemuria_core` |
| `demo_client_lib` e `demo_client` | Jogo | o cliente: a biblioteca, que os testes usam sem janela, e o programa com janela (`Samples/DemoGame/Source/Client`) |
| `test_server_lib` e `test_server` | Jogo | o servidor de teste: regras e conexões numa biblioteca, e o programa (`Samples/DemoGame/Source/TestServer`). Usa a plataforma, mas nunca o Classic |
| `demo_test_support` | Jogo | portas e espera com limite para os testes com rede real (`Samples/DemoGame/Source/TestSupport`) |

## Conceitos

**Protocolo** (`DemoProtocol/`):

- `Framing.hpp`: cada quadro tem 2 bytes com o tamanho do corpo (o menor primeiro) e um corpo de até 128 bytes, que começa pelo tipo da mensagem. O `FrameReader` aceita quadros partidos ou juntos e guarda no máximo um quadro incompleto.
- Dois níveis de erro ([ADR 0017](../adr/0017-contrato-do-adaptador.md)): um `FrameError` quebra o formato e encerra a conexão; um `MessageError` descarta só aquela mensagem.
- `Messages.hpp` e `MessageCodec.hpp`: do cliente, `Hello`, `WalkRequest` (até 16 passos; sem passos quer dizer "parar") e `MapLoaded`; do servidor, `MapEntered`, `EntityAppeared`, `EntityWalked`, `WalkConfirmed`, `PositionCorrected` e `EntityLeft`. Todo campo é conferido (INV-12).
- `DemoMap.hpp`: o mapa de 64 por 64 blocos, gerado por código com uma semente fixa. A faixa de trilha (blocos 22 a 42 em X, 27 e 28 em Z) parece livre para o cliente, mas o servidor a proíbe, para a correção aparecer na tela.
- `DemoRules.hpp` e `Protocol.hpp`: velocidade de 4 blocos por segundo, que cliente e servidor precisam usar, e a porta padrão 24000.

**Cliente** (`DemoClient/`):

- `DemoAdapter.hpp`: o adaptador da demonstração. Usa TCP, envia os passos (até 16 por pedido) e avisa quando o mapa carregou (`kDemoCapabilities`). Não pede memória depois de criado.
- `DemoGame.hpp`: o jogo de um jogador, sem janela: sessão, mundo, clique, quadro, cena, câmera e o texto do estado da conexão para o título da janela.
- `DemoFrame.hpp`: os passos do quadro entre ler a janela e desenhar. O programa e o teste de alocação do quadro chamam essa classe do mesmo jeito.
- `DemoStyle.hpp`, `GroundScene.hpp`, `ProofScene.hpp`, `ProofImage.hpp` e `Smoothness.hpp`: cores e câmera do [guia visual](../VISUAL.md), chão e personagens na cena, a cena de prova fixa, a imagem da prova e o limite de fluidez.
- O programa `demo_client` aceita `--port`, `--graphics-api`, `--proof-image`, `--frames`, `--measure-frame-times` e `--version`, e recusa o resto.

**Servidor de teste** (`TestServer/`):

- `ServerWorld.hpp`: as regras, sem nenhuma conexão. Recebe as mensagens, a saída de um cliente e o tempo, e responde por um `ServerOutbox`.
- `WalkCheck.hpp` e `ServerMotion.hpp`: a conferência de cada caminhada (origem longe demais, rápido demais, passo que não é vizinho, bloco proibido, diagonal que corta canto) e onde cada jogador está em cada momento.
- `ServerHost.hpp`: o servidor com conexões de verdade. A cada `serve(agora)`, aceita conexões, lê as mensagens e encerra o que precisa terminar. Valores da demonstração: 8 jogadores, 16 conexões, tolerância de 1 passo e de 0,5 s.
- O programa `test_server` escuta só em `127.0.0.1` e avisa "Listening on 127.0.0.1:<porta>" quando está pronto.

## Como usar

- **Abrir a demonstração:** `py tools/dev.py demo` abre o servidor e duas janelas do jogo.
- **Gerar a prova visual:** `py tools/dev.py visual`.
- **Escrever um adaptador:** siga `Samples/DemoGame/Source/Client/Private/DemoAdapter.cpp` ([código de referência](../CONVENTIONS.md#código-de-referência)).
- **Testar com rede real:** siga `Samples/DemoGame/Source/Client/Tests/RealMatch.hpp` e `TwoPlayersTest.cpp`. Os testes procuram uma porta livre entre 24100 e 24499 e esperam por condição, com limite de 20 s.

## Invariantes relevantes

- **INV-01:** o servidor de teste decide cada caminhada, e o cliente aceita a correção.
- **INV-02:** o protocolo e o servidor de teste nunca usam o Classic.
- **INV-05:** as regras do servidor recebem o tempo de quem chama; o relógio real fica só nos programas e na espera dos testes.
- **INV-12:** todo quadro e toda mensagem são conferidos antes do uso.
- **INV-13:** o adaptador, o enquadramento e o quadro do cliente não pedem memória depois de aquecidos.

## Testes

| Alvo | O que cobre |
|---|---|
| `demo_protocol_tests` | mapa, enquadramento e mensagens, com dados partidos, juntos e inválidos (CA-10, CA-11) |
| `demo_client_tests` | adaptador (CA-10, CA-11), título e câmera do jogo (CA-11, CA-14), regras combinadas entre cliente e servidor e correção na faixa proibida (CA-06), estilo e tamanho do personagem (CA-01), cena de prova (CA-19), fluidez (CA-23) e dois jogadores com rede real (CA-09) |
| `demo_client_gpu_tests` (etiqueta `gpu`) | a imagem da prova sai igual duas vezes (CA-19) |
| `test_server_tests` | conferência das caminhadas e regras do servidor (CA-08), movimento no servidor e conexões reais |
| testes dos programas | `demo_client_starts`, `test_server_starts` e a recusa de opções inválidas do `test_server` |

## Itens provisórios

- [TD-0001](../TECH_DEBT.md): os personagens são formas geradas por código.
- [TD-0010](../TECH_DEBT.md): a faixa de pontos de luz da imagem ainda não é exigida.
- [TD-0013](../TECH_DEBT.md): o servidor de teste não limita os bytes que esperam para sair.
- [TD-0015](../TECH_DEBT.md): o mapa é gerado por código.
- [TD-0017](../TECH_DEBT.md): não há login nem escolha de personagem.
- [TD-0020](../TECH_DEBT.md): o adaptador escreve o próprio enquadramento.
