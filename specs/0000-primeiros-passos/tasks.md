# Tarefas da fatia 0

Este arquivo é a memória entre sessões: quem chegar depois lê aqui o que foi feito e continua daqui.

Legenda: `[ ]` pendente · `[~]` em andamento · `[x]` concluída · `[!]` bloqueada

## Etapa 1: Fundações

- [x] T01: Adicionar `sdl3-net` e `shaderc` ao `vcpkg.json`, conferir as licenças instaladas e registrá-las em `docs/THIRD_PARTY.md`. Verificação: `py tools/dev.py verify` passa com as bibliotecas instaladas.
- [x] T02: Permitir que o servidor de teste use a camada de plataforma e fazer a checagem de SDL reconhecer o SDL3_net. Verificação: testes em `tools/checks/tests` provam o que passa a ser aceito e o que continua recusado.
- [x] T03: Liberar `new` e `malloc` só na pasta `tests/architecture/frame_allocations/`. Verificação: testes em `tools/checks/tests` provam que o resto do repositório continua recusado.
- [x] T04: Escrever os ADRs 0017 (contrato do adaptador), 0018 (conexões) e 0019 (shaders). Verificação: `py tools/checks/run.py docs terms` passa.

## Etapa 2: Desenho

- [ ] T05: Matemática do Núcleo: vetores, matrizes, projeção e raio a partir da tela (base de CA-01 e CA-02). Verificação: testes em `Engine/Source/Runtime/Core/Tests`.
- [ ] T06: Relógio do quadro e sorteio injetados no Núcleo (base de CA-04 e CA-19). Verificação: testes em `Engine/Source/Runtime/Core/Tests`.
- [ ] T07: Descrição da cena no Núcleo, só com dados. Verificação: testes em `Engine/Source/Runtime/Core/Tests`.
- [ ] T08: Janela, entrada e relógio real na camada de plataforma. Verificação: teste com a etiqueta `gpu` e abertura manual da janela.
- [ ] T09: Shaders em HLSL compilados no build para SPIR-V e DXIL (CA-22). Verificação: o build gera os dois formatos.
- [ ] T10: Desenho com a SDL GPU, na janela e fora dela, com desenho em lote (CA-22). Verificação: teste com a etiqueta `gpu` desenha numa imagem fora da tela.
- [ ] T11: Câmera 3/4 e escolha do bloco sob o cursor no Classic (CA-01, CA-02). Verificação: testes com as etiquetas `[0000-CA-01]` e `[0000-CA-02]`.
- [ ] T12: Paleta, mapa gerado e cliente que mostra o chão (CA-01). Verificação: a janela mostra o chão, e a IA abre a imagem gerada.

## Etapa 3: Prova visual

- [ ] T13: Cena de prova e imagem em arquivo (CA-19). Verificação: teste com a etiqueta `[0000-CA-19]`, com duas imagens idênticas.
- [ ] T14: Medição dos números da imagem em `tools/checks` (CA-20). Verificação: testes em `tools/checks/tests` e números da cena de prova dentro das faixas.
- [ ] T15: Comparação com a imagem aprovada, tolerância medida e ADR 0020 (CA-21, CA-22). Verificação: testes em `tools/checks/tests` e comparação entre Direct3D 12 e Vulkan.
- [ ] T16: Estatísticas e medidor de fluidez (CA-23). Verificação: teste com a etiqueta `[0000-CA-23]` e medição real na cena de prova.
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

*(ainda não há imagem)*

## Revisão

*(preenchido pelo `/revisar`)*

## Notas para a próxima sessão

- Shaders: o dono trocou o `directx-dxc` pelo `shaderc` em 2026-11-18, por causa dos termos da Microsoft no pacote de Windows daquela ferramenta. O SPIR-V sai do `glslc` (`shaderc`) e o DXIL sai do `dxc` do Windows SDK, que não gera SPIR-V. O Linux compila só SPIR-V. Um shader de exemplo com `[[vk::binding(0, 1)]]` e `register(b0, space1)` compilou nas duas ferramentas.
- Próxima tarefa: T05. A etapa 1 está concluída.
- `glslc` fica em `vcpkg_installed/<triplet do host>/tools/shaderc/`. A regra de compilar shaders deve ficar no `CMakeLists.txt` de `PlatformSDL`, para não mexer em `cmake/`, que é trava.
- A SDL 3.4.20 tem `SDL_SavePNG`, então a imagem da prova não precisa de biblioteca nova. A medição em Python lê o PNG com `zlib`, da biblioteca padrão.
- Mistura inicial de blocos para cumprir as faixas da prova visual (estimativa, confira medindo): 20% sombra profunda, 55% terra na sombra, 17% musgo, 5% trilha ou folhagem e 3% pedra. A mediana estimada do brilho é 0,13.
- A medição de fluidez não pode ficar presa à sincronização vertical, ou todo quadro mediria cerca de 16,7 ms. Use o modo de apresentação sem espera.
- Resposta antiga depois de uma correção: o evento de confirmação traz a origem do movimento, e o Classic só aceita a confirmação se essa origem for o início da caminhada atual.
- Testes com rede real: use porta escolhida pelo sistema, só no próprio computador, e espera por condição com limite.
- O hook `guard-bash` trata como escrita qualquer comando que cite um arquivo de trava e tenha `>`. Não redirecione a saída para arquivo nesses comandos.
- O servidor de teste não pode usar o Classic. O adaptador da demonstração fica em `Samples/DemoGame/Source/Client`, porque `demo_protocol` também não pode usar o Classic.
- `demo_client_starts` e `test_server_starts` hoje esperam que os programas imprimam a versão e terminem. Mantenha esse comportamento com um argumento `--version`, sem enfraquecer os testes.
- A SDL GPU 3.4.20 não tem caminho para o renderizador por software do Direct3D 12. Por isso, a prova visual da CI roda no Linux.
- Os nomes de jogos e servidores de terceiros só são aceitos em `docs/`. Não use esses nomes em `specs/` (a lista fica em `tools/checks/forbidden_terms.txt`).
