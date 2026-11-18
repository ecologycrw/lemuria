---
id: 0006
titulo: SDL3 e SDL GPU, com Windows primeiro
status: aceita
data: 2026-11-18
substitui:
substituida-por:
---

# ADR 0006: SDL3 e SDL GPU, com Windows primeiro

## Contexto

O cliente precisa de janela, entrada, áudio, arquivos e GPU. O Windows vem primeiro, mas as portas de Linux e mobile não podem fechar. O piso de hardware inclui GPUs integradas.

## Decisão

- **SDL3** (licença zlib) cuida da janela, da entrada e do acesso ao sistema.
- **SDL GPU**, a API gráfica do próprio SDL3, cuida do desenho. Ela usa Direct3D 12 ou Vulkan no Windows, Vulkan no Linux e Metal nos sistemas da Apple.
- **Só `lemuria_platform_sdl` inclui o SDL.** Uma checagem automática garante isso (INV-09).
- **Windows primeiro.** Linux compila e roda os testes desde o início. Mobile vem depois.
- O formato e a compilação dos shaders são decididos na fatia 0.

## Consequências

- Portar para outra plataforma mexe principalmente na camada de plataforma.
- O renderizador é nosso: a SDL GPU esconde as diferenças entre as APIs gráficas, mas não traz malhas, materiais nem cenas.
- GPUs sem Direct3D 12 ou Vulkan não são suportadas. As integradas do piso de hardware têm suporte.

## Alternativas consideradas

- **GLFW:** cuida só da janela e da entrada, não tem áudio nem API gráfica, e não roda em Android e iOS. Fecharia a porta do mobile.
- **bgfx:** suporta APIs antigas, como Direct3D 11 e OpenGL, mas é mais uma peça grande para integrar.
- **Direct3D 12 ou Vulkan direto:** muito mais código para escrever e manter.
- **Raylib:** simples, mas baseada em OpenGL, sem as APIs gráficas modernas.
