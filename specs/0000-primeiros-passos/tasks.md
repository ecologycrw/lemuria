# Tarefas da fatia 0

Este arquivo é a memória entre sessões: quem chegar depois lê aqui o que foi feito e continua daqui.

Legenda: `[ ]` pendente · `[~]` em andamento · `[x]` concluída · `[!]` bloqueada

## Etapa 1: Fundações

- [x] T01: Adicionar `sdl3-net` e `shaderc` ao `vcpkg.json`, conferir as licenças instaladas e registrá-las em `docs/THIRD_PARTY.md`. Verificação: `py tools/dev.py verify` passa com as bibliotecas instaladas.
- [x] T02: Permitir que o servidor de teste use a camada de plataforma e fazer a checagem de SDL reconhecer o SDL3_net. Verificação: testes em `tools/checks/tests` provam o que passa a ser aceito e o que continua recusado.
- [x] T03: Liberar `new` e `malloc` só na pasta `tests/architecture/frame_allocations/`. Verificação: testes em `tools/checks/tests` provam que o resto do repositório continua recusado.
- [x] T04: Escrever os ADRs 0017 (contrato do adaptador), 0018 (conexões) e 0019 (shaders). Verificação: `py tools/checks/run.py docs terms` passa.

## Etapa 2: Desenho

- [x] T05: Matemática do Núcleo: vetores, matrizes, projeção e raio a partir da tela (base de CA-01 e CA-02). Verificação: testes em `Engine/Source/Runtime/Core/Tests`.
- [x] T06: Relógio do quadro e sorteio injetados no Núcleo (base de CA-04 e CA-19). Verificação: testes em `Engine/Source/Runtime/Core/Tests`.
- [x] T07: Descrição da cena no Núcleo, só com dados. Verificação: testes em `Engine/Source/Runtime/Core/Tests`.
- [x] T08: Janela, entrada e relógio real na camada de plataforma. Verificação: teste com a etiqueta `gpu` e abertura manual da janela.
- [x] T09: Shaders em HLSL compilados no build para SPIR-V e DXIL (CA-22). Verificação: o build gera os dois formatos.
- [x] T10: Desenho com a SDL GPU, na janela e fora dela, com desenho em lote (CA-22). Verificação: teste com a etiqueta `gpu` desenha numa imagem fora da tela.
- [x] T11: Câmera 3/4 e escolha do bloco sob o cursor no Classic (CA-02). Verificação: testes com a etiqueta `[0000-CA-02]`. Os valores da câmera são dados do jogo (INV-06), então a etiqueta `[0000-CA-01]` fica nos testes do cliente, na T12.
- [x] T12: Paleta, mapa gerado e cliente que mostra o chão (CA-01). Verificação: a janela mostra o chão, e a IA abre a imagem gerada.

## Etapa 3: Prova visual

- [x] T13: Cena de prova e imagem em arquivo (CA-19). Verificação: teste com a etiqueta `[0000-CA-19]`, com duas imagens idênticas.
- [x] T14: Medição dos números da imagem em `tools/checks` (CA-20). Verificação: testes em `tools/checks/tests` e números da cena de prova dentro das faixas.
- [x] T15: Comparação com a imagem aprovada, tolerância medida e ADR 0021 (CA-21, CA-22). Verificação: testes em `tools/checks/tests` e comparação entre Direct3D 12 e Vulkan.
- [x] T16: Estatísticas e medidor de fluidez (CA-23). Verificação: teste com a etiqueta `[0000-CA-23]` e medição real na cena de prova.
- [x] T17: Comando `py tools/dev.py visual`, com o `gpu-test` rodando também a prova visual. Verificação: o comando mostra a imagem, os números e a fluidez.

## Etapa 4: Contrato e mundo

- [x] T18: Interface de conexões no Núcleo, com tipos de conexão e opções de entrega (CA-14, CA-16). Verificação: testes em `Engine/Source/Runtime/Core/Tests`.
- [x] T19: Conexões TCP em segundo plano com o SDL3_net (CA-14, CA-16, CA-17). Verificação: testes em `Engine/Source/Runtime/PlatformSDL/Tests`.
- [x] T20: Grade e cálculo de caminho no Classic (CA-03). Verificação: testes com a etiqueta `[0000-CA-03]`.
- [x] T21: Eventos, pedidos, capacidades e contrato do adaptador no Classic (CA-15, CA-16). Verificação: adaptador de teste compila e passa nos testes.
- [x] T22: Sessão e entrada no mapa, com fila de retenção, época de mapa e prazos (CA-13, CA-14). Verificação: testes com as etiquetas `[0000-CA-13]` e `[0000-CA-14]`.
- [x] T23: Mundo e entidades, com números temporários (CA-12). Verificação: testes com a etiqueta `[0000-CA-12]`.
- [x] T24: Movimento, antecipação e correção (CA-04, CA-05, CA-06, CA-07). Verificação: testes com as etiquetas correspondentes, com tempo injetado.
- [x] T25: Cenários de servidor "mal-educado" e adaptadores com capacidades diferentes (CA-15, CA-16). Verificação: testes com as etiquetas `[0000-CA-15]` e `[0000-CA-16]`.

## Etapa 5: Protocolo e servidor

- [x] T26: Mensagens da demonstração, enquadramento e leitura com limites (CA-10, CA-11). Verificação: testes com dados inválidos em `Samples/DemoGame/Source/Protocol/Tests`.
- [x] T27: Regras do servidor de teste (CA-08). Verificação: testes com a etiqueta `[0000-CA-08]`.
- [x] T28: Programa do servidor de teste, aceitando conexões só do próprio computador. Verificação: teste com conexão real.

## Etapa 6: Jogo completo

- [x] T29: Adaptador da demonstração (CA-10, CA-11). Verificação: testes em `Samples/DemoGame/Source/Client/Tests`.
- [x] T30: Cliente completo, com estado da conexão no título da janela (CA-01, CA-14). Verificação: abertura manual da demonstração.
- [x] T31: Teste de ponta a ponta com dois clientes (CA-09). Verificação: teste com a etiqueta `[0000-CA-09]`, com conexão real.
- [ ] T32: Teste do quadro sem alocação de memória (CA-18). Verificação: teste com a etiqueta `[0000-CA-18]` em `tests/architecture/frame_allocations`.
- [ ] T33: Comando `py tools/dev.py demo` e documentação viva (arquitetura, módulos, glossário, convenções, itens provisórios). Verificação: `py tools/dev.py verify` passa.
- [ ] T34: Prova visual final da fatia. Verificação: a IA abre a imagem, confere os números e a fluidez e anota o resultado abaixo.

## Bloqueios

*(nenhum)*

## Prova visual

**T12, cena de prova em 1280 por 720, com Vulkan e Direct3D 12.**

- **O que a imagem mostra:** o chão preenche toda a imagem em tons escuros e quentes, com manchas de terra, musgo, folhagem e sombra. A faixa de trilha atravessa o mapa, e as pedras aparecem em grupos, sobre a folhagem. Os dois personagens se destacam do chão e são fáceis de diferenciar.
- **O que ficou ruim:** as bordas das formas são serrilhadas e os blocos do chão são bem visíveis. O serrilhado ainda não tem fatia definida. Os blocos visíveis são aceitos nesta fatia e mudam na fatia 2.
- **Primeira medição reprovada:** a sombra profunda ficou em 6,7%, abaixo da faixa. A parte de sombra do mapa subiu de 20% para 30%.
- **Números atuais (medição provisória, a ferramenta definitiva vem na T14):** brilho mediano 0,137; sombra profunda 13,5%; tons escuros 84,0%; pontos de luz 0,35% (não exigido); tons quentes 96,5%; saturação mediana 0,622. Todos dentro das faixas exigidas.
- **Vulkan e Direct3D 12:** na primeira geração, os dois arquivos saíram idênticos byte a byte. A comparação definitiva fica para a T15.

**T13, a mesma cena gerada duas vezes em cada API gráfica.**

- **O que a imagem mostra:** a tarefa não mudou a aparência. O chão continua preenchendo toda a imagem em tons escuros e quentes, com a trilha na diagonal, as pedras em grupos e os dois personagens fáceis de diferenciar no centro.
- **O que ficou ruim:** continuam o serrilhado nas bordas e os blocos visíveis, já anotados na T12.
- **Repetição:** as quatro imagens (duas com Vulkan e duas com Direct3D 12) saíram idênticas byte a byte neste computador.
- **O teste detecta diferenças:** com um defeito colocado de propósito, que mudava a cena a cada geração, o teste `[0000-CA-19]` falhou. O defeito foi retirado.
- **Números:** não foram medidos de novo, porque a cena não mudou. A ferramenta definitiva vem na T14.

**T14, primeira medição com a ferramenta definitiva, com Vulkan e Direct3D 12.**

| Medida | Resultado | Faixa exigida |
|---|---|---|
| Brilho típico da imagem (mediana) | 0,137 | 0,11 a 0,19 |
| Parte da imagem em sombra profunda | 13,54% | 12% a 40% |
| Parte da imagem em tons escuros | 83,96% | 78% a 93% |
| Parte da imagem em tons quentes | 96,49% | 75% ou mais |
| Saturação mediana | 0,622 | 0,50 a 0,80 |
| Parte da imagem em pontos de luz | 0,35% | não exigida nesta fatia |

- **Resultado:** as duas APIs gráficas deram os mesmos números, todos dentro das faixas, e iguais aos da medição provisória da T12.
- **Ponto de atenção:** a sombra profunda está perto do limite inferior de 12%. Uma mudança no mapa ou nas cores pode reprovar a imagem.
- **Os testes detectam erros:** 25 de 26 defeitos colocados de propósito numa cópia do código foram reprovados. O defeito restante não muda nenhum resultado, porque só troca a escolha entre dois valores iguais.

**T15, comparação entre os renderizadores.** O renderizador por software foi medido num ambiente igual ao da CI (Ubuntu 24.04, Clang 18 e Mesa 25.2.8), criado com o Docker, com a autorização do dono.

| Comparação | Pontos diferentes (acima de 2 unidades) | Resultado |
|---|---|---|
| Vulkan e Direct3D 12, na mesma placa de vídeo | 0 de 921.600 | aceita |
| Placa de vídeo e renderizador por software | 17 de 921.600 (0,002%) | aceita |
| Menor mudança real medida (personagens 5% mais baixos) | 832 de 921.600 (0,090%) | recusada |

- **O que a imagem do Linux mostra:** a mesma cena do Windows, sem diferença visível. O chão, a trilha, as pedras e os dois personagens estão nos mesmos lugares. Os números da imagem são os mesmos da T14.
- **Repetição no Linux:** as duas imagens geradas saíram idênticas byte a byte, e os testes `[0000-CA-19]` passaram com o renderizador por software.
- **Tolerância:** até 2 unidades de cor por canal e até 0,01% de pontos diferentes ([ADR 0021](../../docs/adr/0021-prova-visual-e-tolerancia.md)). As 10 mudanças reais medidas foram recusadas.
- **Os testes detectam erros:** os 18 defeitos colocados de propósito numa cópia do código foram reprovados.

**T16, fluidez da cena de prova na janela, sem esperar pelo monitor (600 quadros depois de 120 de aquecimento, três medições por API gráfica).**

| API gráfica | Tempo médio | Tempo do pior 1% | Limite |
|---|---|---|---|
| Vulkan | 0,37 a 0,43 ms | 2,81 a 2,88 ms | 16,6 ms |
| Direct3D 12 | 0,37 a 0,41 ms | 2,56 a 2,75 ms | 16,6 ms |

- **A imagem não mudou:** o arquivo gerado é idêntico ao da T14, byte a byte.
- **O medidor mede de verdade:** esperando pelo monitor de 60 Hz, a média deu 16,67 ms, o valor esperado.
- **Janela minimizada:** com Vulkan, os quadros deixam de ser desenhados, e a medição agora é recusada com uma mensagem clara. Com Direct3D 12, os quadros continuam sendo desenhados, e a medição vale.
- **Os testes detectam erros:** os 14 defeitos colocados de propósito no código foram reprovados.
- **Limite da medição:** o resultado vale para este computador, na build de desenvolvimento (TD-0011), e não comprova o piso de hardware (TD-0002).

**T17, comando `py tools/dev.py visual`.**

- **O que o comando faz:** desenha a imagem duas vezes com cada API gráfica, confere se saíram iguais, mostra o caminho da imagem e os números, compara as APIs entre si e com a imagem aprovada, e mede a fluidez.
- **O que a imagem mostra:** abri `build/visual/proof_scene_direct3d12.png`. É a mesma cena da T14, com o chão escuro e quente, a trilha, as pedras em grupos e os dois personagens fáceis de diferenciar.
- **Windows:** todos os números ficaram dentro das faixas, Vulkan e Direct3D 12 tiveram 0 pontos diferentes, e a fluidez ficou em 0,41 ms de média e até 2,89 ms no pior 1%.
- **Ambiente igual ao da CI do Linux:** `gpu-test` e `visual` passaram com o renderizador por software, limitados a 6 núcleos. A imagem do Linux teve 17 pontos diferentes da imagem do Windows, dentro da tolerância.
- **Ainda sem imagem aprovada:** o comando avisa e não falha. O dono aprova a imagem no fim da fatia.
- **Os testes detectam erros:** os 23 defeitos colocados de propósito numa cópia do `tools/dev.py` foram reprovados.

**T30, cliente completo ligado ao servidor de teste.**

- **A janela do jogo:** com o servidor e dois clientes abertos, capturei a janela do segundo cliente. O título dizia "Lemuria Demo - on the map", e a janela mostrava o chão escuro e quente, a trilha, as pedras, o próprio personagem (claro) e o outro jogador (avermelhado) no bloco ao lado. O servidor mostrou 1, depois 2 jogadores no mapa, e depois a saída dos dois.
- **O título em cada situação (CA-14):** sem servidor, "the test server could not be reached"; com servidor, "on the map"; com o servidor fechado no meio do jogo, "the connection to the test server was lost". Nos três casos, a janela continuou respondendo.
- **A cena de prova não mudou:** `py tools/dev.py visual` gerou a mesma imagem, com 0 pontos diferentes da anterior, e todos os números dentro das faixas. A fluidez ficou em 0,32 ms de média e 1,78 ms no pior 1% com Direct3D 12, e 0,34 ms e 1,87 ms com Vulkan.

## Revisão

*(preenchido pelo `/revisar`)*

## Notas para a próxima sessão

- Shaders: o dono trocou o `directx-dxc` pelo `shaderc` em 2026-11-18, por causa dos termos da Microsoft no pacote de Windows daquela ferramenta. O SPIR-V sai do `glslc` (`shaderc`) e o DXIL sai do `dxc` do Windows SDK, que não gera SPIR-V. O Linux compila só SPIR-V. Um shader de exemplo com `[[vk::binding(0, 1)]]` e `register(b0, space1)` compilou nas duas ferramentas.
- Próxima tarefa: T32. As etapas 1 a 5 e as tarefas T29, T30 e T31 estão concluídas, com commits enviados.
- A T33 muda o `tools/dev.py`, que é uma trava: a edição pede a confirmação do dono. O plano aprovado já prevê o comando `demo` (abre o servidor de teste e duas janelas do jogo). Explique ao dono o motivo antes de editar.
- As tarefas T28, T29 e T30 não passaram por uma rodada própria de defeitos colocados de propósito, como as anteriores. Vale fazer isso antes do `/revisar`, ou registrar o motivo de não fazer. A T31 cobriu uma parte: o teste de ponta a ponta reprovou os 7 defeitos colocados no servidor (não avisar a chegada a quem já estava, não mostrar ao recém-chegado quem já estava, não avisar a saída), no adaptador (ignorar a caminhada dos outros) e no jogo (sair sem encerrar, não enviar a caminhada, não mover ninguém).
- O código da fatia ainda não rodou na CI do Linux, que só roda em PR e na `main`. O Clang de lá é mais rigoroso que o MSVC (veja a nota sobre funções sem uso abaixo).
- Para compilar, use `py tools/dev.py build`: o `cmake --build` direto não encontra o ambiente do Visual Studio. O `py tools/dev.py format` só confere; para formatar, rode o `clang-format.exe -i` do Visual Studio (`VC/Tools/Llvm/x64/bin`) nos arquivos alterados.
- As regras do servidor ficam na biblioteca `test_server_lib` (`Samples/DemoGame/Source/TestServer`), sem nenhuma conexão: `ServerWorld` recebe as mensagens de cada cliente (`receive`), a saída de um cliente (`remove`) e o tempo, e responde por um `ServerOutbox` (`send` e `disconnect`). O `ServerHost` (T28) escolhe os `ClientId`, lê os quadros com o `FrameReader` e envia o que o `ServerOutbox` receber.
- O fluxo do servidor: `Hello` com a versão certa coloca o jogador no bloco livre mais perto do centro e responde `MapEntered`; versão errada, servidor cheio ou nenhum bloco livre desconectam. Depois de `MapLoaded`, o jogador vê quem já está no mundo e é visto por eles. Uma caminhada aceita volta como `WalkConfirmed` para o jogador e `EntityWalked` para os outros; um pedido sem passos chega aos outros como `PositionCorrected`. Uma caminhada recusada vira `PositionCorrected` para todos, no bloco em que o servidor acredita que o jogador está, e o jogador para ali. A saída de um jogador vira `EntityLeft` para os outros.
- As regras de caminhada (CA-08) ficam em `check_walk`: uma caminhada que começa na rota atual vale a partir do momento em que o jogador chega ali; fora da rota, ela precisa começar a até `origin_tolerance_steps` passos do bloco atual e paga essa distância na velocidade de andar. Se o jogador só pudesse estar na origem mais de `early_tolerance_seconds` depois de agora, o pedido é rápido demais. Cada passo precisa tocar o anterior, cair num bloco permitido pelo servidor e, na diagonal, ter livres os dois blocos ao lado.
- O servidor com conexões de verdade é o `ServerHost` (`TestServer/ServerHost.hpp`), também na `test_server_lib`: recebe um `TcpListener` já aberto e, a cada `serve(now)`, aceita conexões, lê as mensagens e encerra o que precisa terminar. Os valores da demonstração ficam em `demo_server_config()` (mapa `DemoMap::kId`, velocidade `kWalkTilesPerSecond`, 1 passo e 0,5 s de tolerância, 8 jogadores) e `demo_host_limits()` (16 conexões). O teste de ponta a ponta da T31 pode rodar o `ServerHost` no mesmo processo, como faz o `ServerHostTest.cpp`.
- O programa `test_server` escuta em `127.0.0.1`, na porta `demo_protocol::kDefaultServerPort` (24000, em `Protocol.hpp`), ou na porta de `--port`. Ele avisa "Listening on 127.0.0.1:<porta>" quando está pronto e roda até a janela ser fechada. Uma porta ocupada encerra o programa com erro.
- Uma conexão TCP só aceita `send` depois de `on_connected`. O cliente de teste espera a conexão abrir antes de enviar.
- Testes com rede real fora da camada de plataforma usam a mesma faixa de portas (24100 a 24499) e esperam por condição com limite de 20 s. Na demonstração, isso fica na biblioteca `demo_test_support` (`Samples/DemoGame/Source/TestSupport`, `DemoTestSupport/RealNetwork.hpp`): `kFirstTestPort`, `kTestPortCount` e `wait_until(condição, rodada)`. A cópia da camada de plataforma (`TcpTestSupport.hpp`) continua separada, porque a Engine não pode depender de `Samples`.
- O teste de ponta a ponta (CA-09) fica em `Samples/DemoGame/Source/Client/Tests/TwoPlayersTest.cpp`: um `ServerHost` e duas `DemoGame` com `TcpConnections` de verdade, no mesmo processo. O tempo do jogo avança 1/60 s por rodada, mas nunca passa à frente do relógio real, para os prazos da sessão (15 s para conectar, por exemplo) valerem de verdade mesmo com rodadas muito rápidas. Cada jogador anda 3 blocos em linha reta, que é o único caminho mais curto, por isso o servidor aceita exatamente os passos do cliente. O teste leva cerca de 0,9 s por cenário.
- O adaptador da demonstração é o `DemoAdapter` (`DemoClient/DemoAdapter.hpp`, na `demo_client_lib`). Ele recebe um `core::Connections` (TCP de verdade no programa, uma conexão roteirizada nos testes) e o endereço do servidor, e recusa conexões que não sejam TCP. Ao conectar, envia `Hello` e entrega `SessionPhaseReached{EnteringMap}`. As capacidades ficam em `kDemoCapabilities`. Um `EntityLeft` por desconexão vira `EntityLeaveReason::Removed`.
- O jogo de um jogador, sem janela, é a `DemoGame` (`DemoClient/DemoGame.hpp`): `create(adapter)`, `start()`, `click(tile)`, `frame(FrameTime)`, `describe(scene)`, `focus()` e `status()`. O programa com janela só lê a entrada, converte o clique em bloco com a câmera e desenha. O teste de ponta a ponta da T31 pode usar duas `DemoGame` com `TcpConnections` e `DemoAdapter` de verdade, e um `ServerHost` no mesmo processo.
- O `demo_client` aceita `--port`, com a porta padrão 24000. O título da janela muda só quando o estado da conexão muda.
- A conexão roteirizada dos testes do cliente fica em `Samples/DemoGame/Source/Client/Tests/ScriptedConnections.hpp`.
- O cliente das tarefas T29 e T30 precisa combinar com o servidor: velocidade `kWalkTilesPerSecond`, `DiagonalRule::WhenBothSidesAreFree`, `max_steps_per_request = kMaxWalkSteps` (16) e `needs_map_loaded_notice = true`. Velocidades diferentes fariam o servidor corrigir caminhadas certas.
- A T27 passou por 43 defeitos colocados de propósito numa cópia do código. Na primeira rodada, quatro não compilaram e foram reescritos, e um sobreviveu, revelando que o teste de "mapa errado" não conferia se o jogador tinha entrado no mundo. O teste foi reforçado, e agora os 43 são reprovados.
- O hook `guard-bash` também bloqueia um comando que escreve arquivos (como um `write_text` do Python) e cita `tools/dev.py`. Edite num passo e rode o `dev.py` em outro.
- As mensagens da demonstração ficam em `Samples/DemoGame/Source/Protocol`: `Messages.hpp` (os dados), `Framing.hpp` (`FrameReader` e `EncodedFrame`) e `MessageCodec.hpp` (`encode`, `read_client_message` e `read_server_message`). Cada quadro tem dois bytes com o tamanho do corpo, o menor primeiro, e o corpo começa pelo tipo da mensagem. O corpo tem no máximo 128 bytes.
- Dois níveis de erro (ADR 0017): um `FrameError` quebra o formato, e a conexão precisa ser encerrada; um `MessageError` vale só para aquela mensagem, que é contada e deixada de lado, e a leitura continua.
- O fluxo pensado para a T27 a T29: o cliente manda `Hello` com `protocol_version()` logo ao conectar; o servidor responde com `MapEntered`; o cliente manda `MapLoaded` depois de carregar o mapa (o adaptador declara `needs_map_loaded_notice`); o pedido de caminhada leva a origem e até 16 passos (`kMaxWalkSteps`), então o adaptador declara `max_steps_per_request = 16`. Um pedido sem passos quer dizer "parar na origem", como o `World` envia ao clicar no passo que está sendo andado.
- Números de personagem nunca são 0, e os blocos vão até 1.023. `tile_from` converte coordenadas `int` e recusa as que não cabem numa mensagem; o adaptador e o servidor devem usá-lo.
- O `FrameReader` só avança porque recusa o quadro sem corpo: um defeito que aceitava esse quadro fez a leitura entrar em laço infinito. Mantenha essa recusa.
- A T26 passou por 29 defeitos colocados de propósito numa cópia do código. Na primeira rodada, um sobreviveu e revelou a falta de um teste para tamanhos em que o segundo byte importa. O teste foi criado, e agora os 29 são reprovados.
- O Clang da CI do Linux trata função sem uso como erro, e o MSVC não avisa. Confira os auxiliares dos testes antes do commit.
- Os cenários de servidor "mal-educado" ficam em `Frameworks/Classic/Tests/MisbehavingServerTest.cpp`. O `TestGame` de lá liga o adaptador, a sessão e o mundo como o cliente da T30 deve fazer, e serve de modelo para ele: a cada quadro, `session.update`, depois `world.update` e o envio do pedido de caminhada. Em `on_map_entered`, chama `world.enter_map`; quando o mapa carregar, `world.set_grid` e depois `session.finish_map_load`.
- Cada cenário roda com dois servidores: um recebe os passos por um fluxo de bytes, que chega byte a byte, e pede o aviso de mapa carregado; o outro recebe só o destino, por mensagens inteiras, com as caminhadas enviadas sem garantia de chegada. O `TestGame` nunca pergunta qual é o servidor: só monta o `MovementRules` a partir das capacidades.
- O protocolo de teste ganhou três mensagens: "trocando de mapa" (7), "troca de mapa recusada" (8) e "blocos mudaram" (9).
- A T25 não precisou mudar o código do framework: todos os cenários passaram na primeira rodada. Para provar que os testes detectam erros, 11 defeitos foram colocados de propósito no `GameSession` e no `World`, um de cada vez, e os testes da T25 reprovaram todos.
- O cenário de recusar a reconexão fica para a fatia 6, como diz o plano.
- O movimento fica em `World` e `EntityMotion`. A cada quadro, o cliente da T30 chama `world.update`, depois `world.take_walk_request` e envia o pedido com `send_request` do adaptador. O clique chama `world.walk_own_to`.
- `MovementRules` precisa repetir o que o adaptador declara em `AdapterCapabilities`: `walk_requests` e `max_steps_per_request`.
- Uma confirmação só vale para a caminhada atual: a origem precisa ser a mesma e, quando o pedido leva passos, o destino confirmado precisa ser um desses passos ou a própria origem. O resto é ignorado e contado em `stale_confirmation_count`.
- Caminhadas longas vão em partes. Sem resposta do servidor, o personagem continua andando até o destino.
- O servidor de teste da T27 precisa aceitar pedidos em partes, em que a origem de cada parte é o fim da anterior.
- A T24 passou por 96 defeitos colocados de propósito numa cópia do código, e os testes reprovaram todos. Na primeira rodada, seis sobreviveram: três revelaram faltas de teste, que foram criados, e três mostraram linhas sem efeito, que foram removidas.
- O mundo fica em `lemuria::classic::World`. Em `on_map_entered`, o jogo chama `enter_map`. Quando o mapa carregar, chama `set_grid` e só depois `finish_map_load`, porque o mundo recusa eventos enquanto não tem a grade.
- A T24 precisa tratar `OwnWalkConfirmed`, que o mundo ainda recusa e conta. `EntityWalked` hoje só guarda o destino, e o personagem ainda não anda.
- O jogador nunca é removido do mundo enquanto há um mapa, e sempre tem lugar reservado. Quando o mundo enche, novos personagens são recusados e contados.
- A T23 passou por 59 defeitos colocados de propósito numa cópia do código. Um deles sobreviveu na primeira rodada e revelou uma falta de teste para áreas que começam antes do mapa. O teste foi criado, e agora os 59 são reprovados.
- A sessão fica em `lemuria::classic::GameSession`. O jogo implementa `SessionListener`: em `on_map_entered`, remove tudo do mundo, carrega o mapa e chama `finish_map_load` com o bilhete recebido. Um bilhete de um carregamento antigo não muda nada.
- A T22 prova a parte da sessão de CA-13 e CA-14. A remoção dos personagens ao trocar de mapa precisa de um teste com o mundo, na T23. O aviso de queda no título da janela vem na T30.
- Eventos que chegam antes do primeiro mapa são descartados e contados. Se a fila de retenção encher, a sessão termina com `TooManyHeldEvents`, porque o mundo deixaria de ser confiável.
- Só as esperas pelo servidor e pelo carregamento do mapa têm prazo. A escolha de personagem depende do jogador, e o silêncio do servidor dentro do mundo é normal.
- A T22 passou por 79 defeitos colocados de propósito numa cópia do código, e os testes reprovaram todos.
- O contrato do adaptador fica em `Lemuria/Classic/ProtocolAdapter.hpp`, com `GameEvent.hpp`, `GameRequest.hpp` e `AdapterCapabilities.hpp`. A sessão da T22 deve chamar `check_capabilities` antes de usar um adaptador.
- Eventos e pedidos são valores simples, sem memória própria, então a fila de retenção da T22 pode guardá-los sem alocar. O limite de passos por evento ou pedido é `WalkSteps::kCapacity` (32).
- O adaptador e a conexão de teste ficam em `Frameworks/Classic/Tests/AdapterTestSupport.hpp`, para as tarefas T22 a T25 reutilizarem.
- A T21 prova só a parte do contrato de CA-15 e CA-16. A prova completa, com a sessão, o mundo e o movimento do Classic, vem na T25.
- A T21 passou por 23 defeitos colocados de propósito numa cópia do código, e os testes reprovaram todos.
- A grade fica em `lemuria::classic::TileGrid` e o cálculo de caminho em `lemuria::classic::PathFinder`. Chame `prepare_for` quando o mapa carregar, para a busca não alocar memória durante o quadro. A T32 precisa provar isso com o contador de alocações.
- Cada jogo escolhe a regra de diagonal em `PathRules`. O servidor de teste não pode usar o Classic, então a T27 precisa conferir os passos com a mesma regra escolhida pelo cliente da demonstração.
- O personagem pode sair de um bloco bloqueado, porque o servidor pode colocá-lo em qualquer lugar. Só o destino precisa estar livre.
- A T20 passou por 67 defeitos colocados de propósito numa cópia do código, e os testes reprovaram todos.
- Tempo de uma busca na build de desenvolvimento, com limite de 4.096 blocos: 0,68 ms em média no mapa de 64 por 64, 3,6 ms no de 256 por 256 e 16,1 ms, com pior caso de 20,2 ms, no de 1.024 por 1.024 (TD-0012).
- As conexões TCP ficam em `lemuria::platform::TcpConnections` (`Lemuria/Platform/TcpConnections.hpp`). Os sockets vivem numa thread própria, que só conversa com o quadro pela `TcpMailbox`. Limites: 64 KiB por envio, 1 MiB na fila de envio de cada conexão e 1 MiB de bytes recebidos esperando o `poll`. Neste computador, um envio levou 1,4 ms em média até chegar ao outro lado, e 3,2 ms no pior caso.
- O servidor de teste (T28) deve usar `lemuria::platform::TcpListener` (`Lemuria/Platform/TcpListener.hpp`), que só aceita conexões do próprio computador. Ele escuta em `127.0.0.1`, então o cliente precisa conectar nesse endereço, e não em `localhost`, que pode virar IPv6.
- A T19 passou por 18 defeitos colocados de propósito no código, e os testes reprovaram todos. A restrição do `TcpListener` ao próprio computador não tem teste automático, porque exigiria uma segunda interface de rede.
- Cada tipo de conexão deriva de `lemuria::core::Connections` (`Lemuria/Core/Connections.hpp`) e implementa `traits`, `start_connecting`, `queue_bytes`, `stop_connection` e `collect_events`. A classe base já garante as regras comuns: números que não se repetem, limite de 8 conexões abertas, recusa de opções de envio não oferecidas e nenhum dado depois do fim da conexão. O `collect_events` roda na thread do quadro e não pode esperar pela rede.
- A TD-0007 foi resolvida pelo comando `py tools/dev.py visual` e saiu de `docs/TECH_DEBT.md`. O report da fatia precisa registrar essa resolução.
- A fluidez é medida com `demo_client --measure-frame-times [--graphics-api nome]`. O comando abre uma janela visível por alguns segundos e termina com código 1 se passar do limite de 16,6 ms. A CI do Linux não tem janela de verdade, então essa medição fica só no computador de desenvolvimento.
- Não encerre o turno com um build em segundo plano: o hook `stop-check` compila ao mesmo tempo e os dois builds entram em conflito.
- A comparação roda com `py tools/checks/run.py compare-image --image arquivo.png`. Sem `--reference`, ela usa `tests/visual/approved/proof_scene.png`, e só o dono coloca a imagem nessa pasta.
- A TD-0005 foi resolvida pelo ADR 0021 e saiu de `docs/TECH_DEBT.md`. O report da fatia precisa registrar essa resolução.
- A CA-22 passou num ambiente igual ao da CI do Linux. A prova definitiva é o check verde da CI, quando o PR da fatia for aberto.
- O volume `lemuria-t15-cache` do Docker guarda as bibliotecas compiladas no ambiente igual ao da CI (cerca de 6 GB). Peça ao dono para removê-lo no fim da fatia. Ao usar o Docker, limite o contêiner a 6 dos 12 núcleos, a pedido do dono, e feche o Docker Desktop ao terminar.
- Para repetir a prova nesse ambiente: use a imagem `ubuntu:24.04` com `--cpus=6` e o volume montado em `/cache`; instale os pacotes de `.github/actions/setup-build/action.yml`, além de `build-essential`, `git`, `curl`, `zip`, `unzip`, `tar`, `pkg-config`, `python3`, `autoconf`, `automake`, `libtool` e `ca-certificates`; defina `VCPKG_ROOT=/cache/vcpkg`, `VCPKG_DOWNLOADS=/cache/downloads`, `VCPKG_DEFAULT_BINARY_CACHE=/cache/archives`, `VCPKG_MAX_CONCURRENCY=6`, `CMAKE_BUILD_PARALLEL_LEVEL=6` e `SDL_VIDEODRIVER=offscreen`; coloque no `PATH` a pasta do CMake indicada por `vcpkg fetch cmake`, porque o do Ubuntu 24.04 é antigo; rode `python3 tools/dev.py gpu-test` numa cópia do código dentro do contêiner.
- A medição roda com `py tools/checks/run.py measure-image --image arquivo.png`. O leitor de PNG fica em `tools/checks/lemuria_checks/png_reader.py`.
- O dono escolheu separar o desenho por etapas ([ADR 0020](../../docs/adr/0020-fronteira-do-desenho.md)). Nesta fatia, nenhum tipo de API gráfica pode aparecer em cabeçalhos públicos. A separação interna fica para a fatia 2 (TD-0009).
- O cliente já aceita `--proof-image arquivo`, `--graphics-api nome` e `--frames quantidade`. A cena de prova fica em `DemoClient/ProofScene.hpp`, e `DemoClient/ProofImage.hpp` desenha essa cena numa imagem em memória.
- `glslc` fica em `vcpkg_installed/<triplet do host>/tools/shaderc/`. A regra de compilar shaders deve ficar no `CMakeLists.txt` de `PlatformSDL`, para não mexer em `cmake/`, que é trava.
- A SDL 3.4.20 tem `SDL_SavePNG`, então a imagem da prova não precisa de biblioteca nova. A medição em Python lê o PNG com `zlib`, da biblioteca padrão.
- Mistura de blocos do mapa: 30% sombra profunda, 45% terra na sombra, 17% musgo, 5% folhagem e 3% pedra, além da faixa de trilha.
- A medição de fluidez não pode ficar presa à sincronização vertical, ou todo quadro mediria cerca de 16,7 ms. Use o modo de apresentação sem espera.
- Resposta antiga depois de uma correção: o evento de confirmação traz a origem do movimento, e o Classic só aceita a confirmação se essa origem for o início da caminhada atual.
- Testes com rede real: o SDL3_net não informa a porta escolhida pelo sistema, então cada teste procura uma porta livre na faixa de `TcpTestSupport.hpp`, só no próprio computador, e espera por condição com limite.
- O hook `guard-bash` trata como escrita qualquer comando que cite um arquivo de trava e tenha `>`. Não redirecione a saída para arquivo nesses comandos.
- O servidor de teste não pode usar o Classic. O adaptador da demonstração fica em `Samples/DemoGame/Source/Client`, porque `demo_protocol` também não pode usar o Classic.
- `demo_client_starts` e `test_server_starts` hoje esperam que os programas imprimam a versão e terminem. Mantenha esse comportamento com um argumento `--version`, sem enfraquecer os testes.
- A SDL GPU 3.4.20 não tem caminho para o renderizador por software do Direct3D 12. Por isso, a prova visual da CI roda no Linux.
- Os nomes de jogos e servidores de terceiros só são aceitos em `docs/`. Não use esses nomes em `specs/` (a lista fica em `tools/checks/forbidden_terms.txt`).
