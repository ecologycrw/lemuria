---
id: 0019
titulo: Shaders em HLSL, compilados durante o build
status: aceita
data: 2026-11-18
substitui:
substituida-por:
---

# ADR 0019: Shaders em HLSL, compilados durante o build

## Contexto

O [ADR 0006](0006-sdl3-plataforma-e-graficos.md) deixou para a fatia 0 a decisão sobre o formato e a compilação dos shaders. A SDL GPU precisa de SPIR-V para o Vulkan e de DXIL para o Direct3D 12. O piso de hardware aceita as duas APIs gráficas, então a engine precisa dos dois formatos.

A primeira opção foi o pacote `directx-dxc`, que gera os dois formatos com uma só ferramenta. O pacote de Windows traz termos próprios da Microsoft, que limitam o uso ao Windows e preveem coleta de dados de uso. Esses termos valeriam para qualquer pessoa que compilasse este projeto open source.

## Decisão

- **Os shaders são escritos uma única vez, em HLSL,** e ficam em `Engine/Shaders/`.
- **SPIR-V:** gerado pelo `glslc`, do pacote `shaderc` (Apache-2.0), no Windows e no Linux.
- **DXIL:** gerado pelo `dxc` do Windows SDK, que já é exigido para compilar o projeto no Windows. O Linux não gera DXIL, porque não tem Direct3D 12.
- **A compilação acontece durante o build,** e o resultado é embutido na engine. Nenhum shader compilado entra no repositório.
- **Se o `dxc` não for encontrado no Windows, o build para** com uma mensagem clara.

## Consequências

- Quem compila o projeto não precisa aceitar termos além dos que o Windows SDK já exige.
- Nenhuma dessas ferramentas entra no jogo distribuído.
- Duas ferramentas compilam o mesmo código, então os resultados podem divergir. A prova visual compara as imagens das duas APIs gráficas.
- A versão do `dxc` depende do Windows SDK instalado e não é fixada pelo repositório.

## Alternativas consideradas

- **`directx-dxc`:** usaria uma só ferramenta, mas traria os termos descritos no contexto.
- **Só Vulkan:** dispensaria o DXIL, mas mudaria o [ADR 0006](0006-sdl3-plataforma-e-graficos.md) e poderia excluir computadores com drivers antigos.
- **SDL_shadercross:** ainda é uma versão preliminar e também dependeria do mesmo compilador para gerar DXIL.
- **Guardar shaders compilados no repositório:** seriam arquivos binários impossíveis de revisar.
