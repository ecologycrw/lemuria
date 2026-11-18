# Decisões de arquitetura (ADRs)

Cada ADR registra uma decisão importante, o motivo e as alternativas. Um ADR aceito não é editado: se a decisão mudar, um ADR novo o substitui e diz isso no front matter (`substitui` / `substituida-por`). O modelo fica em `templates/adr.md`.

| ADR | Decisão | Status |
|---|---|---|
| [0001](0001-cpp20-cmake-vcpkg.md) | C++20, CMake e vcpkg, com MSVC e Clang | aceita |
| [0002](0002-tres-camadas.md) | Três camadas com framework substituível, sem script por enquanto | aceita |
| [0003](0003-engine-so-de-cliente.md) | Engine só de cliente, com servidor de teste mínimo | aceita |
| [0004](0004-adaptador-de-protocolo.md) | Adaptador de protocolo plugável | aceita |
| [0005](0005-conteudo-e-importadores.md) | Formatos próprios, glTF, importadores e cifra plugáveis | aceita |
| [0006](0006-sdl3-plataforma-e-graficos.md) | SDL3 e SDL GPU, com Windows primeiro | aceita |
| [0007](0007-interface.md) | RmlUi atrás de uma camada própria e Dear ImGui só em ferramentas | aceita |
| [0008](0008-classico-melhorado.md) | Ambição gráfica "clássico melhorado" e orçamentos de escala | aceita |
| [0009](0009-repositorio-publico-e-terceiros.md) | Repositório público MIT e "especificar, não copiar" | aceita |
| [0010](0010-fluxo-git.md) | Fluxo Git com conta de máquina e merge só pelo dono | aceita |
| [0011](0011-ferramentas-de-verificacao.md) | Ferramentas de verificação | aceita |
| [0012](0012-documentacao.md) | Documentação como memória do projeto | aceita |
| [0013](0013-central-de-conhecimento.md) | Central de conhecimento em sessão separada | aceita |
| [0014](0014-organizacao-do-repositorio.md) | Organização do repositório em Engine, Frameworks e Samples | aceita |
| [0015](0015-arquivos-cpp-em-pascalcase.md) | Arquivos C++ e pastas de include em PascalCase | aceita |
