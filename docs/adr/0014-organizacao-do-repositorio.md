---
id: 0014
titulo: Organização do repositório em Engine, Frameworks e Samples
status: aceita
data: 2026-11-18
substitui:
substituida-por:
---

# ADR 0014: Organização do repositório em Engine, Frameworks e Samples

## Contexto

O esqueleto inicial separava o código em `src/`, `samples/` e `tests/`. Essa divisão funciona, mas não mostra as três camadas da arquitetura ([ADR 0002](0002-tres-camadas.md)) e afasta cada módulo dos próprios testes. A engine ainda é só um esqueleto, então este é o momento mais barato para definir a organização definitiva.

## Decisão

Adotamos um modelo híbrido, inspirado na organização de engines conhecidas:

- **As camadas ficam na raiz:** `Engine/` é o Núcleo, `Frameworks/` guarda o Classic e outros frameworks, e `Samples/` guarda os exemplos de jogo.
- **Inicial maiúscula é produto; minúscula é infraestrutura do repositório:** `cmake/`, `docs/`, `templates/`, `tests/` e `tools/` continuam como estão.
- **Todo módulo tem o mesmo formato:** `Public/` (o que outros módulos podem usar), `Private/` (detalhes internos), `Tests/` (os testes do módulo) e `CMakeLists.txt`.
- **Os módulos da engine ficam em `Engine/Source/Runtime/`.** Os programas próprios da engine, quando existirem, ficam em `Engine/Source/Programs/`.
- **Um jogo tem `Source/`, `Content/` e `Config/`.** `Samples/DemoGame` é o modelo para os jogos dos desenvolvedores.
- **A pasta `tests/` da raiz guarda só o que vale para o repositório inteiro:** `architecture/` e `hooks/`.
- **Uma pasta só nasce quando tiver uso.** `Engine/Shaders`, `Engine/Content`, `Samples/DemoGame/Content` e `Samples/DemoGame/Config` serão criadas pelas fatias que precisarem delas.
- **Os nomes usados no código não mudam:** alvos do build, includes e namespaces continuam em `snake_case`, como `lemuria_core` e `lemuria/core/version.hpp`.
- **O servidor de teste vai para `Samples/DemoGame/Source/TestServer`.** Ele fala o protocolo da demonstração, então não pode ficar em `Engine/` sem fazer o Núcleo depender de um jogo (INV-02). Este ponto atualiza o local indicado no [ADR 0003](0003-engine-so-de-cliente.md); o restante daquele ADR continua valendo.

## Consequências

- As camadas e o caráter opcional do Classic ficam visíveis logo na primeira pasta.
- Cada módulo pode ser movido ou substituído junto com os próprios testes.
- As checagens automáticas passam a conhecer o formato dos módulos e recusam a volta das pastas antigas.
- Os caminhos ficam mais longos, o que exige atenção ao limite de caminhos do Windows.
- `Samples/` e `samples/` são a mesma pasta no Windows, mas não no Linux. A CI no Linux detecta nomes escritos com a caixa errada.

## Alternativas consideradas

- **Manter `src/`, `samples/` e `tests/`:** é simples, mas não mostra as camadas e separa os módulos dos seus testes.
- **Copiar por inteiro o modelo de uma engine grande:** traria pastas e cerimônias pensadas para milhares de módulos.
- **Transformar cada módulo em um pacote independente:** exigiria arquivos de descrição e um gerenciador de pacotes sem necessidade agora.
- **Pastas por assunto direto na raiz:** é fácil de navegar, mas as camadas deixam de aparecer na estrutura.
