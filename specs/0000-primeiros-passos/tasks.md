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
- [ ] T17: Comando `py tools/dev.py visual`, com o `gpu-test` rodando também a prova visual. Verificação: o comando mostra a imagem, os números e a fluidez.

## Etapa 4: Contrato e mundo

- [ ] T18: Interface de conexões no Núcleo, com tipos de conexão e opções de entrega (CA-14, CA-16). Verificação: testes em `Engine/Source/Runtime/Core/Tests`.
- [ ] T19: Conexões TCP em segundo plano com o SDL3_net (CA-14, CA-16, CA-17). Verificação: testes em `Engine/Source/Runtime/PlatformSDL/Tests`.
- [ ] T20: Grade e cálculo de caminho no Classic (CA-03). Verificação: testes com a etiqueta `[0000-CA-03]`.
- [ ] T21: Eventos, pedidos, capacidades e contrato do adaptador no Classic (CA-15, CA-16). Verificação: adaptador de teste compila e passa nos testes.
- [ ] T22: Sessão e entrada no mapa, com fila de retenção, época de mapa e prazos (CA-13, CA-14). Verificação: testes com as etiquetas `[0000-CA-13]` e `[0000-CA-14]`.
- [ ] T23: Mundo e entidades, com números temporários (CA-12). Verificação: testes com a etiqueta `[0000-CA-12]`.
- [ ] T24: Movimento, antecipação e correção (CA-04, CA-05, CA-06, CA-07). Verificação: testes com as etiquetas correspondentes, com tempo injetado.
- [ ] T25: Cenários de servidor "mal-educado" e adaptadores com capacidades diferentes (CA-15, CA-16). Verificação: testes com as etiquetas `[0000-CA-15]` e `[0000-CA-16]`.

## Etapa 5: Protocolo e servidor

- [ ] T26: Mensagens da demonstração, enquadramento e leitura com limites (CA-10, CA-11). Verificação: testes com dados inválidos em `Samples/DemoGame/Source/Protocol/Tests`.
- [ ] T27: Regras do servidor de teste (CA-08). Verificação: testes com a etiqueta `[0000-CA-08]`.
- [ ] T28: Programa do servidor de teste, aceitando conexões só do próprio computador. Verificação: teste com conexão real.

## Etapa 6: Jogo completo

- [ ] T29: Adaptador da demonstração (CA-10, CA-11). Verificação: testes em `Samples/DemoGame/Source/Client/Tests`.
- [ ] T30: Cliente completo, com estado da conexão no título da janela (CA-01, CA-14). Verificação: abertura manual da demonstração.
- [ ] T31: Teste de ponta a ponta com dois clientes (CA-09). Verificação: teste com a etiqueta `[0000-CA-09]`, com conexão real.
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

## Revisão

*(preenchido pelo `/revisar`)*

## Notas para a próxima sessão

- Shaders: o dono trocou o `directx-dxc` pelo `shaderc` em 2026-11-18, por causa dos termos da Microsoft no pacote de Windows daquela ferramenta. O SPIR-V sai do `glslc` (`shaderc`) e o DXIL sai do `dxc` do Windows SDK, que não gera SPIR-V. O Linux compila só SPIR-V. Um shader de exemplo com `[[vk::binding(0, 1)]]` e `register(b0, space1)` compilou nas duas ferramentas.
- Próxima tarefa: T17. As etapas 1 e 2 e as tarefas T13 a T16 estão concluídas.
- A fluidez é medida com `demo_client --measure-frame-times [--graphics-api nome]`. O comando abre uma janela visível por alguns segundos e termina com código 1 se passar do limite de 16,6 ms. A CI do Linux não tem janela de verdade, então essa medição fica só no computador de desenvolvimento.
- Não encerre o turno com um build em segundo plano: o hook `stop-check` compila ao mesmo tempo e os dois builds entram em conflito.
- A comparação roda com `py tools/checks/run.py compare-image --image arquivo.png`. Sem `--reference`, ela usa `tests/visual/approved/proof_scene.png`, e só o dono coloca a imagem nessa pasta.
- A TD-0005 foi resolvida pelo ADR 0021 e saiu de `docs/TECH_DEBT.md`. O report da fatia precisa registrar essa resolução.
- A CA-22 ainda depende da T17, que faz o `gpu-test` rodar a prova visual na CI do Linux.
- O volume `lemuria-t15-cache` do Docker guarda as bibliotecas compiladas no ambiente igual ao da CI (cerca de 6 GB). Peça ao dono para removê-lo no fim da fatia. Ao usar o Docker, limite o contêiner a 6 dos 12 núcleos, a pedido do dono.
- A medição roda com `py tools/checks/run.py measure-image --image arquivo.png`. O leitor de PNG fica em `tools/checks/lemuria_checks/png_reader.py` e pode ser usado pela comparação da T15.
- O dono escolheu separar o desenho por etapas ([ADR 0020](../../docs/adr/0020-fronteira-do-desenho.md)). Nesta fatia, nenhum tipo de API gráfica pode aparecer em cabeçalhos públicos. A separação interna fica para a fatia 2 (TD-0009).
- O cliente já aceita `--proof-image arquivo`, `--graphics-api nome` e `--frames quantidade`. A cena de prova fica em `DemoClient/ProofScene.hpp`, e `DemoClient/ProofImage.hpp` desenha essa cena numa imagem em memória.
- `glslc` fica em `vcpkg_installed/<triplet do host>/tools/shaderc/`. A regra de compilar shaders deve ficar no `CMakeLists.txt` de `PlatformSDL`, para não mexer em `cmake/`, que é trava.
- A SDL 3.4.20 tem `SDL_SavePNG`, então a imagem da prova não precisa de biblioteca nova. A medição em Python lê o PNG com `zlib`, da biblioteca padrão.
- Mistura de blocos do mapa: 30% sombra profunda, 45% terra na sombra, 17% musgo, 5% folhagem e 3% pedra, além da faixa de trilha.
- A medição de fluidez não pode ficar presa à sincronização vertical, ou todo quadro mediria cerca de 16,7 ms. Use o modo de apresentação sem espera.
- Resposta antiga depois de uma correção: o evento de confirmação traz a origem do movimento, e o Classic só aceita a confirmação se essa origem for o início da caminhada atual.
- Testes com rede real: use porta escolhida pelo sistema, só no próprio computador, e espera por condição com limite.
- O hook `guard-bash` trata como escrita qualquer comando que cite um arquivo de trava e tenha `>`. Não redirecione a saída para arquivo nesses comandos.
- O servidor de teste não pode usar o Classic. O adaptador da demonstração fica em `Samples/DemoGame/Source/Client`, porque `demo_protocol` também não pode usar o Classic.
- `demo_client_starts` e `test_server_starts` hoje esperam que os programas imprimam a versão e terminem. Mantenha esse comportamento com um argumento `--version`, sem enfraquecer os testes.
- A SDL GPU 3.4.20 não tem caminho para o renderizador por software do Direct3D 12. Por isso, a prova visual da CI roda no Linux.
- Os nomes de jogos e servidores de terceiros só são aceitos em `docs/`. Não use esses nomes em `specs/` (a lista fica em `tools/checks/forbidden_terms.txt`).
