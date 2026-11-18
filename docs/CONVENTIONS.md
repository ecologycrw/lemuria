# Convenções

> Aqui fica só o que as ferramentas não verificam sozinhas. Formatação, avisos, funções proibidas e dependências entre camadas são garantidos por `.clang-format`, `.clang-tidy`, sanitizers e `tools/checks`.

## Idiomas

| O quê | Idioma |
|---|---|
| Código, identificadores, comentários no código, mensagens de commit | inglês |
| Specs, planos, relatórios, ADRs, STATUS e demais docs do projeto | português |
| Documentação para quem usa a engine | decidido depois (provavelmente inglês) |

## Nomes

| O quê | Padrão | Exemplo |
|---|---|---|
| Pastas do produto e módulos | `PascalCase` | `Engine/Source/Runtime/PlatformSDL` |
| Pastas da infraestrutura | minúsculas | `tools/checks` |
| Alvos do build | `lemuria_<área>` | `lemuria_core` |
| Namespaces | `lemuria::<área>` | `lemuria::core` |
| Arquivos C++ | `PascalCase`, com `.hpp` e `.cpp`, e o nome do tipo principal | `FrameClock.hpp` |
| Includes públicos | `Lemuria/<Área>/<Arquivo>.hpp` | `Lemuria/Core/FrameClock.hpp` |
| Tipos | `PascalCase` | `FrameClock` |
| Funções e variáveis | `snake_case` | `advance_time` |
| Membros privados | `snake_case_` | `elapsed_` |
| Constantes | `kPascalCase` | `kMaxBones` |
| Macros (evitar) | `LEMURIA_MAIUSCULAS` | `LEMURIA_ASSERT` |
| Testes | `<Módulo>/Tests/<Arquivo>Test.cpp` | `Engine/Source/Runtime/Core/Tests/FrameClockTest.cpp` |
| Specs | `specs/NNNN-nome-em-kebab-case/` | `specs/0000-primeiros-passos/` |
| ADRs | `docs/adr/NNNN-nome-em-kebab-case.md` | `docs/adr/0001-cpp20-cmake-vcpkg.md` |

## Commits

Formato: `tipo(escopo): descrição curta em inglês`

- `tipo`: `feat`, `fix`, `docs`, `test`, `refactor`, `perf`, `build`, `ci` ou `chore`.
- `escopo`: o número da fatia (`0000`) ou a área (`prep`, `docs`, `ci`).
- Dentro de uma fatia, a descrição começa pela tarefa: `feat(0000): T03 add tile picking`.
- A linha `Co-Authored-By` do Claude fica no fim da mensagem.

## Padrões da casa

- **A cena é só dados.** O framework preenche a descrição da cena, e o desenho a consome.
- **Ações do jogador são pedidos** ao servidor. O resultado vem de volta como evento.
- **Referência por ID**, e não por ponteiro direto entre objetos de sistemas diferentes.
- **Conteúdo é dado:** definições de modelos, efeitos e afins vêm de dados validados.
- **A interface pensa no destino, a implementação pensa no agora.**
- **Threads só nas fronteiras.** Rede e carga de arquivos rodam em segundo plano e conversam com o quadro por filas.
- **Dependências explícitas:** cada objeto recebe o que precisa no construtor.

## Regras de C++

- **C++20**, sem extensões de compilador e sem módulos do C++20. Um recurso do C++23 só entra por ADR, quando todos os compiladores-alvo o suportarem.
- **Memória com dono claro (RAII).** Use `std::unique_ptr` e containers. `new`, `delete`, `malloc` e `free` soltos são proibidos. `std::shared_ptr` só com um motivo escrito.
- **Ponteiro cru e referência nunca são donos** de nada.
- **Erros por valor:** funções que podem falhar devolvem `tl::expected`. Sem exceções no caminho do quadro. Exceções de bibliotecas externas são tratadas na fronteira com elas.
- **`[[nodiscard]]`** em toda função cujo resultado não pode ser ignorado.
- **Buffers com tamanho:** use `std::span`, `std::string_view` e `std::array`. Funções de C sem limite de tamanho, como `strcpy` e `sprintf`, são proibidas.
- **Conversões explícitas:** nada de conversão de inteiros que possa perder valor sem checar.
- **Dados externos passam por um leitor com limites.** Nunca transforme bytes de rede ou de arquivo em struct com `reinterpret_cast` ou `memcpy` direto.
- **Sem estado global mutável.**
- **Tempo e sorteios** vêm das fontes injetadas. `std::chrono::system_clock`, `steady_clock`, `rand` e `std::random_device` são proibidos fora da camada de plataforma.

## Antipadrões proibidos

- Singletons e "managers" globais.
- Herança profunda.
- Uma interface para cada classe "por precaução", e camadas sem motivo.
- Abstração antes da necessidade.
- Lógica de jogo dentro das funções de desenho.
- Otimização sem medição.

## Régua de qualidade

O que um revisor sênior recusaria no Lemuria, mesmo com o build e os testes verdes:

- **Função longa:** uma função que faz várias etapas em sequência. Cada etapa vira uma função com nome.
- **Classe com mais de uma responsabilidade:** se a descrição da classe precisa de "e também", ela se divide.
- **Código repetido:** na terceira cópia, vira uma função só (ver a [regra de três](#regra-de-três)).
- **Tratamento de erro que esconde o problema:** um erro ignorado, ou um `catch` que engole o que não sabe tratar.
- **Nome vago:** `Manager`, `Helper`, `Data`, `Info`, `Util`, `Handler`, `temp`. O nome diz o que a coisa é ou faz.
- **Teste que verifica a implementação e não o comportamento:** o teste confere o que o sistema entrega, não como ele faz por dentro. Para formatos binários, os bytes são fixados no teste.

Uma parte é verificada por máquina: o clang-tidy limita a complexidade das funções. Um código que passa do limite é melhorado; o limite só é afrouxado com um motivo real aprovado pelo dono. O resto é conferido pelo revisor, na lista obrigatória dele.

## Regra de três

Uma abstração só nasce quando o mesmo problema aparece pela terceira vez. Na primeira vez, resolve-se direto. Na segunda, nota-se a repetição. Na terceira, abstrai-se, já sabendo o que de fato se repete. A regra vale também para criar alvos novos. Os [encaixes](ARCHITECTURE.md#encaixes) são a exceção, porque são requisitos do produto.

## Código de referência

A IA imita o código que já existe. Por isso, os melhores exemplos de cada padrão são listados aqui conforme surgem.

| Para... | Siga |
|---|---|
| declarar uma biblioteca, um programa ou testes | `cmake/LemuriaModule.cmake`, com o exemplo em `Engine/Source/Runtime/Core/CMakeLists.txt` |
| ler dados externos com validação e devolver erros por valor | `Engine/Source/Runtime/Core/Private/Version.cpp` |
| testar uma leitura com dados inválidos | `Engine/Source/Runtime/Core/Tests/VersionTest.cpp` |
| esconder uma biblioteca externa atrás da camada de plataforma | `Engine/Source/Runtime/PlatformSDL/Private/PlatformInfo.cpp` |

## Testes

- O framework é o Catch2. O nome do teste descreve o comportamento: `"tile picker returns the tile under the cursor"`.
- Um teste que prova um critério de aceitação recebe a etiqueta `[NNNN-CA-xx]`.
- Tempo e sorteios nos testes vêm das fontes injetadas, nunca do sistema.
- Todo leitor de dados externos tem testes com dados inválidos.
- Um teste nunca é apagado ou enfraquecido para "fazer passar".

## Documentação

- Curta. Aponta para outro documento em vez de repetir o que está nele.
- Diagramas Mermaid quando ajudarem:

| Tipo | Para quê |
|---|---|
| `flowchart LR` | fluxos e processos |
| `flowchart TB` | camadas e hierarquias |
| `sequenceDiagram` | quem fala com quem, e em que ordem |
| `stateDiagram-v2` | estados e transições |

- No máximo cerca de 15 elementos por diagrama, uma ideia por diagrama.
- O diagrama complementa o texto: toda regra importante também fica escrita.
- O que puder ser gerado a partir do código é gerado, não desenhado à mão.

## Scripts

- Ferramentas internas em Python 3, só com a biblioteca padrão.
- Hooks do Git em `sh` (rodam no Windows, pelo Git, e no Linux).
- Hooks do Claude Code em PowerShell 5.1, salvos em UTF-8 com BOM.

## Conteúdo de terceiros

- **Nada de outros jogos:** nomes de conteúdo, assets, textos, formatos de arquivo, protocolos ou código. Mapas, itens, monstros e textos são originais.
- **Especificar, não copiar:** o Lemuria aprende com projetos de referência só por lições genéricas, escritas com palavras próprias ([ADR 0009](adr/0009-repositorio-publico-e-terceiros.md)).
- **Menções a nomes** de jogos e servidores de terceiros só aparecem nos documentos de `docs/`, para explicar o gênero ou um exemplo de uso. Nunca em `Engine/`, `Frameworks/`, `Samples/`, `tests/` ou `tools/`, exceto na lista de termos proibidos da checagem.
- Assets externos só com licença compatível (CC0, CC-BY com atribuição, MIT), registrada junto do asset.
- Código de outros projetos só com licença compatível com a MIT e com atribuição.
- Bibliotecas externas ficam registradas, com a licença, em [THIRD_PARTY.md](THIRD_PARTY.md).
