# Glossário

Termos usados no projeto, em português simples.

| Termo | Significado |
|---|---|
| **Adaptador de protocolo** | A peça que o desenvolvedor escreve para o Lemuria conversar com o servidor do jogo dele. Traduz os bytes da rede em eventos do jogo, e os pedidos do jogador em bytes |
| **ADR** | *Architecture Decision Record*: um registro curto de uma decisão importante, com o motivo e as alternativas. Fica em `docs/adr/` |
| **Antecipação visual** | O cliente começa a mostrar uma ação antes da resposta do servidor, para parecer rápido. Se o servidor discordar, o cliente corrige |
| **Asset** | Tudo o que não é código: modelos 3D, animações, texturas, ícones, sons |
| **Bloom** | O halo suave em volta das partes da imagem que brilham |
| **Branch** | Uma linha de trabalho paralela no Git. Cada fatia é feita na sua branch, sem mexer na versão oficial |
| **CA** | Critério de aceitação: uma frase verificável que diz quando uma fatia está pronta (por exemplo, `CA-01`) |
| **Central de conhecimento** | Um projeto separado, só no computador do dono, que estuda projetos de referência e escreve lições genéricas para o Lemuria |
| **CI** | Integração contínua: os testes que rodam nas máquinas do GitHub a cada envio de código |
| **Classic** | O framework oficial do Lemuria, com as mecânicas de cliente do estilo clássico. É opcional. Nome provisório |
| **Clássico melhorado** | A ambição gráfica do Lemuria: o estilo dos jogos do começo dos anos 2000, com o acabamento de hoje |
| **clang-tidy** | Uma ferramenta que lê o código C++ e aponta erros e práticas perigosas |
| **CMake** | A ferramenta que descreve como o código C++ é compilado |
| **Commit** | Um ponto de salvamento no histórico do Git |
| **Conta de máquina** | A conta do GitHub usada só pela IA. Ela envia código e abre PRs, mas não aprova nem faz merge |
| **Desenvolvedor** | Quem usa o Lemuria para criar um jogo |
| **Encaixe** | Um ponto da engine que o desenvolvedor pode trocar sem mexer nela: protocolo, importadores, cifra e framework |
| **Fatia** | Uma etapa pequena e completa do desenvolvimento, que entrega algo que dá para ver funcionando |
| **FPS** | Quadros por segundo: quantas vezes por segundo a janela é redesenhada. 60 é fluido; abaixo de 30, dá para ver os saltos |
| **Framework** | A camada com as mecânicas do jogo, entre o Núcleo e o Jogo. Pode ser o Classic ou um criado pelo desenvolvedor |
| **glTF** | Um formato aberto de modelos 3D e animações, exportado por ferramentas como o Blender |
| **GPU** | A placa de vídeo. A **integrada** vem junto do processador e é mais fraca que uma placa separada |
| **Hook** | Um script que roda automaticamente antes ou depois de uma ação (um commit, um comando da IA) e pode bloqueá-la |
| **Importador** | A peça que transforma um formato de arquivo externo no formato próprio da engine |
| **Instancing** | Desenhar a mesma forma muitas vezes, em lugares, tamanhos e cores diferentes, com um pedido só à placa de vídeo. Muito mais rápido que um pedido por objeto |
| **Interpolar** | Calcular as posições intermediárias entre dois momentos, para o movimento ficar suave |
| **Invariante** | Uma regra que nenhuma mudança pode violar (`INV-01`…, em [ARCHITECTURE.md](ARCHITECTURE.md#invariantes)) |
| **Jogador** | Quem joga o jogo que o desenvolvedor criou |
| **Lição** | Um texto genérico da central de conhecimento que ensina uma técnica, sem nada do projeto estudado |
| **LOD** | *Level of detail*: mostrar uma versão mais simples do que está longe da câmera |
| **`main`** | A branch principal: a versão oficial, aprovada pelo dono |
| **Merge (squash)** | Juntar uma branch na `main`. O *squash* transforma todos os commits da branch num commit só |
| **Módulo** | Uma peça da engine com função própria, guardada em uma pasta com o seu código e os seus testes. `Core` e `PlatformSDL` são módulos |
| **Núcleo** | A camada mais baixa da engine (Lemuria Core): gráficos, interface, som, conteúdo e conexões. Não sabe nada de gênero ou de jogo |
| **Orçamento** | O limite do que pode ser mostrado ao mesmo tempo, como personagens ou partículas. Quando acaba, a qualidade cai antes do FPS |
| **PR** | *Pull Request*: o pedido para juntar uma branch na `main`. É onde o dono revisa e aprova |
| **`Private`** | A pasta de um módulo com os detalhes internos, que os outros módulos não podem usar |
| **Protocolo** | As mensagens que cliente e servidor trocam, e o formato delas |
| **`Public`** | A pasta de um módulo com o que os outros módulos podem usar |
| **Push** | Enviar os commits para o GitHub |
| **Quadro** | Uma imagem da janela. A cada quadro o cliente lê o que chegou do servidor, move os personagens e desenha a cena |
| **RAII** | Uma regra de C++: todo recurso tem um dono, e ele é liberado automaticamente quando o dono deixa de existir |
| **Renderizador por software** | Um programa que faz o trabalho da placa de vídeo no processador. Permite testar o desenho em máquinas sem GPU |
| **Sanitizer** | Uma ferramenta que roda junto com os testes e detecta erros de memória |
| **SDL** | A biblioteca que cuida da janela, do mouse e da placa de vídeo. Só a camada de plataforma fala com ela |
| **Servidor de teste** | Um programa mínimo usado só nos testes e na demonstração. Não é um produto |
| **Shader** | Um programa pequeno que roda na placa de vídeo e decide onde e com que cor cada ponto aparece |
| **Skinning** | Deformar a malha de um personagem de acordo com os ossos do esqueleto |
| **Spec** | A especificação de uma fatia: o que ela entrega e como saber que está pronta |
| **Teste de mutação** | Uma ferramenta muda pedacinhos do código de propósito e confere se algum teste falha. A nota é a porcentagem de mudanças que os testes pegaram |
| **Trava** | Qualquer proteção automática: hook, permissão, teste de arquitetura, CI, regra do GitHub |
| **vcpkg** | O programa que baixa e compila as bibliotecas externas de C++ nas versões combinadas |
