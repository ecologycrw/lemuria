# Relatório da fatia 0: Primeiros passos

## Resumo para o dono

O Lemuria abre uma janela com um chão em blocos, visto de cima por uma câmera em 3/4. Você clica num bloco e o personagem anda até lá na hora, enquanto o servidor de teste confere cada passo e corrige o que não vale. Com o comando `py tools/dev.py demo`, o servidor e duas janelas do jogo abrem juntos, e cada jogador vê o outro chegar, andar e sair. A engine também gera a imagem da cena de prova, confere os números dela e mede a fluidez com o comando `py tools/dev.py visual`.

## Critérios de aceitação

Os testes automáticos rodaram nesta sessão, em 2026-11-19: `py tools/dev.py verify` e `py tools/dev.py gpu-test` (os 3 grupos com a etiqueta `gpu` passaram, com a prova visual). Cada linha diz onde fica a prova.

| CA | Prova | Resultado |
|---|---|---|
| CA-01 | teste `[0000-CA-01]` em `Samples/DemoGame/Source/Client/Tests/DemoStyleTest.cpp` (câmera a 50 graus e personagem de 8% a 10% da altura da tela, dos pés à cabeça) e verificação manual da janela na T30, na T33 e na T34 | passou e confirmado |
| CA-02 | testes `[0000-CA-02]` em `Frameworks/Classic/Tests/ThreeQuarterCameraTest.cpp`, inclusive nas bordas e nos cantos | passou |
| CA-03 | testes `[0000-CA-03]` em `PathFinderTest.cpp` e `WorldMovementTest.cpp` (`Frameworks/Classic/Tests`) | passou |
| CA-04 | testes `[0000-CA-04]` em `EntityMotionTest.cpp` e `WorldMovementTest.cpp`, com tempo injetado a 30, 60 e 144 quadros por segundo | passou |
| CA-05 | testes `[0000-CA-05]` em `EntityMotionTest.cpp` e `WorldMovementTest.cpp` | passou |
| CA-06 | testes `[0000-CA-06]` em `EntityMotionTest.cpp`, `WorldMovementTest.cpp` e `Samples/DemoGame/Source/Client/Tests/DemoRulesTest.cpp` (os dois jogadores veem a correção na faixa proibida) | passou |
| CA-07 | testes `[0000-CA-07]` em `WorldMovementTest.cpp` | passou |
| CA-08 | testes `[0000-CA-08]` em `WalkCheckTest.cpp` e `ServerWorldTest.cpp` (`Samples/DemoGame/Source/TestServer/Tests`) | passou |
| CA-09 | teste `[0000-CA-09]` em `Samples/DemoGame/Source/Client/Tests/TwoPlayersTest.cpp`, com conexão de rede real, e verificação manual com `py tools/dev.py demo` na T33 | passou e confirmado |
| CA-10 | testes `[0000-CA-10]` em `FramingTest.cpp`, `MessageCodecTest.cpp` e `DemoAdapterTest.cpp` | passou |
| CA-11 | testes `[0000-CA-11]` em `FramingTest.cpp`, `MessageCodecTest.cpp`, `DemoAdapterTest.cpp`, `DemoGameTest.cpp` e `Frameworks/Classic/Tests/WorldTest.cpp` | passou |
| CA-12 | testes `[0000-CA-12]` em `Frameworks/Classic/Tests/WorldTest.cpp` | passou |
| CA-13 | testes `[0000-CA-13]` em `GameSessionTest.cpp`, `WorldTest.cpp` e `MisbehavingServerTest.cpp` | passou |
| CA-14 | testes `[0000-CA-14]` em `ConnectionsTest.cpp`, `TcpConnectionsTest.cpp`, `GameSessionTest.cpp`, `MisbehavingServerTest.cpp` e `DemoGameTest.cpp`, e verificação manual do título da janela na T30 (sem servidor, com servidor e com o servidor fechado no meio do jogo) | passou e confirmado |
| CA-15 | testes `[0000-CA-15]` em `ProtocolAdapterTest.cpp`, `MisbehavingServerTest.cpp` e `WorldMovementTest.cpp` | passou |
| CA-16 | testes `[0000-CA-16]` em `ConnectionsTest.cpp`, `TcpConnectionsTest.cpp`, `ProtocolAdapterTest.cpp` e `MisbehavingServerTest.cpp` | passou |
| CA-17 | testes `[0000-CA-17]` em `Engine/Source/Runtime/PlatformSDL/Tests/TcpConnectionsTest.cpp` | passou |
| CA-18 | testes `[0000-CA-18]` em `tests/architecture/frame_allocations/FrameAllocationsTest.cpp` e `DrawAllocationsTest.cpp` (este com a etiqueta `gpu`) | passou |
| CA-19 | testes `[0000-CA-19]` em `ProofSceneTest.cpp` e `ProofImageTest.cpp` (com a etiqueta `gpu`) | passou |
| CA-20 | testes em `tools/checks/tests/test_image_measures.py` e verificação manual com `py tools/dev.py gpu-test` nesta sessão: todos os números dentro das faixas | passou e confirmado |
| CA-21 | testes em `tools/checks/tests/test_image_comparison.py` e verificação manual nesta sessão: sem imagem aprovada, o comando avisa e não falha. A tolerância está no [ADR 0021](../../docs/adr/0021-prova-visual-e-tolerancia.md) | passou e confirmado |
| CA-22 | verificação manual no Windows nesta sessão (Vulkan e Direct3D 12 com 0 pontos diferentes) e, na T15 e na T17, no ambiente igual ao da CI do Linux, com o renderizador por software (17 pontos diferentes, dentro da tolerância). A prova definitiva é o check verde do `gpu-test` na CI do Linux, neste PR | confirmado no Windows; falta a CI |
| CA-23 | testes `[0000-CA-23]` em `FrameTimeMeterTest.cpp` e `SmoothnessTest.cpp`, e verificação manual com `py tools/dev.py visual` na T34 | passou e confirmado |

## Prova visual

**A imagem para você aprovar:** `build/visual/proof_scene_direct3d12.png`, gerada por `py tools/dev.py visual` (ou pelo `gpu-test`). A imagem com Vulkan (`build/visual/proof_scene_vulkan.png`) é idêntica, byte a byte.

**O que a imagem mostra:** o chão preenche toda a imagem em tons escuros e quentes: terra na sombra, manchas de sombra profunda (mais fortes na parte de baixo), musgo e folhagem em verde-oliva. A trilha marrom-clara atravessa a imagem na diagonal, do alto até o canto de baixo à direita. As pedras cinzentas aparecem em dois grupos, sobre a folhagem: um no alto à esquerda e outro à direita. No centro, os dois personagens são fáceis de ver e de diferenciar: um claro e um avermelhado, em blocos vizinhos.

**Comparação com o guia visual:** as cores de magia não aparecem no cenário, o chão continua visível nas partes escuras, não há halo nem verde vivo de desenho animado, e nada lembra a tela de outro jogo.

**O que ainda está ruim, e quando muda:**

- as bordas das formas são serrilhadas (ainda sem fatia definida);
- os blocos do chão são bem visíveis (aceito nesta fatia; muda na fatia 2, com o chão pintado);
- os personagens são caixas, sem sombra nos pés (fatia 1).

**Números da imagem, medidos nesta sessão:**

| Medida | Resultado | Faixa exigida |
|---|---|---|
| Brilho típico da imagem (mediana) | 0,137 | 0,11 a 0,19 |
| Parte da imagem em sombra profunda | 13,54% | 12% a 40% |
| Parte da imagem em tons escuros | 83,96% | 78% a 93% |
| Parte da imagem em tons quentes | 96,49% | 75% ou mais |
| Saturação mediana | 0,622 | 0,50 a 0,80 |
| Parte da imagem em pontos de luz | 0,35% | não exigida nesta fatia |

A sombra profunda está perto do limite de baixo (12%). Uma mudança no mapa ou nas cores pode reprovar a imagem.

**Repetição e comparação:** o comando desenha a cena duas vezes com cada API gráfica, e as imagens saem idênticas byte a byte. Vulkan e Direct3D 12 têm 0 pontos diferentes. No ambiente igual ao da CI do Linux, o renderizador por software teve 17 pontos diferentes de 921.600 (0,002%), dentro da tolerância de 0,01%.

**Fluidez (T34, três medições de 600 quadros, depois de 120 de aquecimento):**

| API gráfica | Tempo médio | Tempo do pior 1% | Limite |
|---|---|---|---|
| Direct3D 12 | 0,35 a 0,39 ms | 3,06 a 3,78 ms | 16,6 ms |
| Vulkan | 0,36 a 0,40 ms | 3,46 a 4,71 ms | 16,6 ms |

A medição vale para este computador, na build de desenvolvimento, e não comprova o piso de hardware (TD-0002 e TD-0011).

**O que o revisor viu:** ele abriu as duas imagens, mediu os números de novo e encontrou os mesmos valores, todos dentro das faixas.

## O que você deve testar

Todos os comandos rodam num terminal aberto na pasta do projeto.

**1. Abra a demonstração:**

```bash
py tools/dev.py demo
```

Deve abrir uma janela do servidor de teste, com a mensagem "Listening on 127.0.0.1:24000", e duas janelas do jogo. As duas janelas do jogo abrem no mesmo lugar: arraste uma delas para o lado. O título de cada uma deve dizer "Lemuria Demo - on the map".

**2. Teste o movimento:**

1. Clique em vários pontos do chão numa das janelas. O personagem claro deve andar até lá, de forma suave.
2. Olhe a outra janela. O mesmo personagem aparece avermelhado e anda até o mesmo lugar.
3. Clique numa pedra (os blocos cinzentos). O personagem não deve sair do lugar.
4. Clique do outro lado de um grupo de pedras. O personagem deve dar a volta.

**3. Veja o servidor corrigindo.** A faixa proibida é a trilha marrom-clara que atravessa a tela na diagonal, acima e à direita do ponto onde os personagens começam. Para o cliente, ela parece livre, mas o servidor não deixa ninguém pisar nela. Clique num ponto do outro lado da trilha. O personagem começa a andar, é corrigido quando chega na trilha e desliza de volta, sem saltar. A outra janela mostra a mesma coisa.

**4. Teste a queda:**

1. Feche uma das janelas do jogo. O personagem dela deve sumir da outra janela.
2. Feche a janela do servidor de teste. O título da janela que sobrou deve mudar para "the connection to the test server was lost", e a janela deve continuar respondendo (dá para arrastar e fechar).

**5. Gere e confira a prova visual:**

```bash
py tools/dev.py visual
```

O comando mostra o caminho da imagem, os números, a comparação entre Vulkan e Direct3D 12 e a fluidez. No fim, ele avisa "ainda não há imagem aprovada". Isso é esperado. Abra a imagem `build/visual/proof_scene_direct3d12.png` e veja se ela agrada.

**6. Aprove a imagem, se ela agradar.** Só você pode colocar a imagem na pasta das imagens aprovadas. No PowerShell, rode:

```powershell
New-Item -ItemType Directory -Force tests/visual/approved | Out-Null
Copy-Item build/visual/proof_scene_direct3d12.png tests/visual/approved/proof_scene.png
```

Depois, rode `py tools/dev.py visual` de novo. O aviso deve sumir, e a comparação com a imagem aprovada deve dizer "a imagem está dentro da tolerância". A IA inclui a imagem no commit durante a `/retrospectiva`.

**7. Conte o que achou.** Diga o que funcionou, o que ficou estranho e se a imagem agradou.

**Depois do teste (opcional):** o volume `lemuria-t15-cache` do Docker guarda cerca de 6 GB usados para repetir a prova do Linux. Se quiser liberar o espaço, abra o Docker Desktop e rode `docker volume rm lemuria-t15-cache`.

## Desvios do plano

- **Ferramenta de shaders:** o plano previa o `directx-dxc`. Em 2026-11-18, o dono aprovou a troca pelo `shaderc` (licença Apache-2.0), porque o pacote de Windows daquela ferramenta traz termos da Microsoft que limitam o uso. O SPIR-V sai do `glslc`, e o DXIL sai do `dxc` que já vem no Windows SDK.
- **ADR novo durante a fatia:** o dono decidiu separar o desenho por etapas ([ADR 0020](../../docs/adr/0020-fronteira-do-desenho.md)). Nesta fatia, nenhum tipo de API gráfica aparece nos cabeçalhos públicos, e a separação interna fica para a fatia 2 (TD-0009).
- **Teste de alocação:** o plano colocava esse teste na etapa 3. Ele ficou na etapa 6 (T32), porque só faz sentido com o quadro completo, com rede e jogo.
- **Mistura do mapa:** a primeira medição reprovou a sombra profunda (6,7%). A parte de sombra do mapa subiu de 20% para 30%, e a imagem passou.
- **CA-01:** a spec não dizia como medir a altura do personagem. A linha dos pés até a cabeça fica entre 8% e 10% da altura da tela, mas a silhueta inteira da caixa ocupa cerca de 15%. Em 2026-11-19, o dono decidiu: "Dos pés à cabeça". A spec foi ajustada, e a imagem não mudou.
- **Alvos novos além do plano:** `demo_test_support` (as peças que os testes com rede real da demonstração compartilham) e a classe `DemoFrame` (os passos do quadro do programa com janela, criada na revisão para o teste de alocação medir o mesmo quadro do programa).
- **Programas com `--version`:** os testes `demo_client_starts` e `test_server_starts` esperavam que os programas imprimissem a versão e terminassem. Os programas ganharam a opção `--version`, sem enfraquecer os testes.
- **Documentação dos módulos:** a T33 atualizou a arquitetura, mas não criou as páginas dos módulos. Elas foram escritas no fechamento, em `docs/modules/`: [Núcleo](../../docs/modules/core.md), [Plataforma](../../docs/modules/platform-sdl.md), [Classic](../../docs/modules/classic.md) e [jogo de demonstração](../../docs/modules/demo-game.md). O fluxo de trabalho não dizia onde essas páginas ficam, e passou a dizer.
- **O que ficou para a CI do Linux:** a T34 não repetiu a prova no ambiente igual ao da CI, porque o Docker não estava ligado. O trecho do contador de alocações que só existe no Linux (`std::aligned_alloc`) também ainda não foi compilado. A CI deste PR confere os dois.

## Revisão

O revisor independente trabalhou com contexto limpo em 2026-11-19. Ele achou 0 críticos, 3 importantes e 8 menores. Todos foram tratados:

| # | Gravidade | Achado | O que foi feito |
|---|---|---|---|
| 1 | importante | O servidor de teste aceitava como origem um bloco que o jogador já tinha deixado para trás na rota | corrigido, com testes `[0000-CA-08]` novos |
| 2 | importante | A checagem de camadas não reconhecia os alvos e os cabeçalhos novos da demonstração | corrigido, com a aprovação do dono, porque `tools/checks/` é trava |
| 3 | importante | Testes fracos nas tarefas T28, T29 e T30 | testes novos no servidor, no adaptador, no cliente e de ponta a ponta (`DemoRulesTest.cpp`); o que ficou sem teste está explicado nas [tarefas da fatia](tasks.md#revisão) |
| 4 | menor | A posição de entrada no mapa não era conferida contra a grade (INV-12) | corrigido, com testes `[0000-CA-11]` |
| 5 | menor | O cliente escondia um erro de configuração ao iniciar a sessão | corrigido: o programa mostra o erro e termina |
| 6 | menor | Um mapa desconhecido aparecia no título como "disconnected" | corrigido, com o título conferido no teste |
| 7 | menor | Toda configuração regravava os shaders embutidos e recompilava o desenho | corrigido: o arquivo só é regravado quando muda |
| 8 | menor | Código de teste repetido | em parte: `RealMatch.hpp` passou a ser compartilhado; a cópia em `MeasuredMatch.hpp` continua, porque a biblioteca de testes do servidor não pode depender do Classic |
| 9 | menor | As regras de movimento repetem o que o adaptador declara, sem conferência | registrado como TD-0021 |
| 10 | menor | Com Vulkan e a janela minimizada, o laço do cliente provavelmente gira sem pausa | registrado como TD-0022 |
| 11 | menor | O clique vem em coordenadas da janela, e o tamanho, em pixels | registrado como TD-0023 |

## Travas alteradas

| Trava | Mudança | Aprovação do dono |
|---|---|---|
| `vcpkg.json` | `sdl3-net` e `shaderc` | no plano, em 2026-11-18, e a troca do `directx-dxc` pelo `shaderc` no mesmo dia |
| `tools/checks/lemuria_checks/architecture_check.py` e `tools/checks/tests/test_architecture_check.py` | o servidor de teste pode usar a camada de plataforma; a checagem de SDL reconhece o SDL3_net (T02); a checagem de camadas reconhece todos os nomes da demonstração (achado 2 da revisão) | no plano (T02) e em 2026-11-19 ("Sim, as duas") |
| `tools/checks/lemuria_checks/cpp_rules_check.py` e `tools/checks/tests/test_cpp_rules_check.py` | `new` e `malloc` liberados só na pasta do contador de alocações (T03) | no plano |
| `tools/checks/lemuria_checks/image_measures.py`, `image_comparison.py`, `png_reader.py` e `cli.py`, com os testes deles | medição dos números da imagem e comparação com a imagem aprovada (T14 e T15) | no plano |
| `tools/dev.py`, com `tools/checks/tests/test_dev_visual.py` e `test_dev_demo.py` | comandos `visual` e `demo`; o `gpu-test` também roda a prova visual (T17 e T33) | no plano |
| `tests/architecture/CMakeLists.txt` e a pasta nova `tests/architecture/frame_allocations/` | teste que conta a memória pedida pelo quadro aquecido (T32); na revisão, ele passou a usar a `DemoFrame` | no plano (T32) e em 2026-11-19 ("Sim, as duas") |
| `docs/ROADMAP.md` | a fatia 0 passou de "Próximas" para "Agora", e a preparação foi para "Concluídas" | aprovação da spec, em 2026-11-18, como manda o próprio roadmap |

As mudanças que afrouxam um ponto (servidor de teste usando a plataforma, `new` e `malloc` numa única pasta) vieram com testes provando que o resto da regra continua recusando o que deve.

## Testes removidos

Nenhum. O resumo de segurança do PR conta 595 testes adicionados e 0 removidos.

## Itens provisórios criados

| ID | Item | Destino |
|---|---|---|
| TD-0009 | O código de desenho fala direto com a SDL GPU | fatia 2 |
| TD-0010 | A faixa de pontos de luz ainda não é exigida | fatia 2 |
| TD-0011 | A fluidez é medida na build de desenvolvimento | fatia de escala |
| TD-0012 | A busca de caminho limpa a memória do mapa inteiro, e uma busca longa num mapa enorme pode levar um quadro | fatia de escala |
| TD-0013 | O servidor de teste não limita os bytes que esperam para sair para um cliente que não lê | fatia de escala |
| TD-0014 | O teste de alocação não vê a memória do SDL, do driver e da leitura da janela | fatia de escala |
| TD-0015 | O mapa da demonstração é gerado por código | fatia 2 |
| TD-0016 | O desenho usa um único passe, sem luzes nem sombras | fatia 2 |
| TD-0017 | A demonstração não tem login nem escolha de personagem | fatia 6 |
| TD-0018 | Uma queda de conexão encerra a sessão, sem reconexão automática | fatia 6 |
| TD-0019 | O sinal de presença é declarado, mas não é enviado | fatia 6 |
| TD-0020 | O adaptador da demonstração escreve o próprio enquadramento | fatia 6 |
| TD-0021 | As regras de movimento repetem o que o adaptador declara | fatia 6 |
| TD-0022 | Com Vulkan e a janela minimizada, o laço do cliente provavelmente não pausa | fatia 1 |
| TD-0023 | O clique e o tamanho da janela usam unidades diferentes | fatia de plataformas |

**Itens resolvidos nesta fatia, que saíram de `docs/TECH_DEBT.md`:**

- **TD-0005** (onde a prova visual roda e qual é a tolerância): resolvido pelo [ADR 0021](../../docs/adr/0021-prova-visual-e-tolerancia.md).
- **TD-0007** (falta um comando para a prova visual): resolvido pelo comando `py tools/dev.py visual`.
- **TD-0008** (falta provar que o quadro aquecido não pede memória): resolvido pelo teste em `tests/architecture/frame_allocations` (T32).

## Erros e lições

Erros da IA nesta fatia, como primeira ocorrência da escada. Os que se repetiram estão marcados, para a `/retrospectiva` decidir se viram proposta.

- **Pedidos de memória no quadro aquecido.** As conexões TCP (T19) pediam memória durante o quadro em três pontos. Só o teste da T32 achou. Lição: um código que roda a cada quadro precisa do teste de alocação desde a tarefa que o cria.
- **Checagem de camadas desatualizada.** A T02 ajustou a checagem só para o servidor de teste. Os alvos e as pastas criados depois (`demo_client_lib`, `test_server_lib`, `demo_test_support` e `TestServer/`) passaram despercebidos até a revisão. Lição: um alvo novo precisa ser reconhecido pelas checagens. Isso pode ser conferido por máquina.
- **Regra do servidor com uma brecha.** O servidor aceitava voltar a caminhada inteira de uma vez, a partir de um bloco já deixado para trás (T27).
- **Dado da rede usado sem conferir.** A posição de entrada no mapa não era conferida contra a grade (INV-12).
- **Testes fracos (repetiu).** As rodadas de defeitos colocados de propósito revelaram testes faltando na T23, na T24, na T26 e na T27, e esses testes foram criados. Nas tarefas T28 a T30, essa rodada não foi feita, e a revisão encontrou testes fracos ali.
- **Spec ambígua.** A CA-01 não dizia como medir a altura do personagem, e a dúvida só apareceu na prova final (T34).
- **Comandos bloqueados pelo hook (repetiu).** O `guard-bash` bloqueou mais de uma vez comandos que citavam uma trava e escreviam arquivo. A trava funcionou. As formas permitidas ficaram anotadas nas [tarefas da fatia](tasks.md#notas-para-a-próxima-sessão).

## Propostas geradas

- [P-0003](../../docs/PROPOSALS.md): oferecer o GNS como segunda opção de conexão (planejamento).
- [P-0004](../../docs/PROPOSALS.md): definir quando o histórico do STATUS é atualizado (durante a fatia).
- [P-0005](../../docs/PROPOSALS.md): mostrar como texto as falhas que hoje abrem uma janela do Windows nos testes (revisão).
