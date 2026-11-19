# Bibliotecas de terceiros

Toda biblioteca do `vcpkg.json` fica registrada aqui com a licença dela ([ADR 0009](adr/0009-repositorio-publico-e-terceiros.md)). A checagem `structure` recusa uma biblioteca sem registro. Uma biblioteca nova precisa da aprovação do dono.

| Biblioteca | Licença | Para quê | Entra no jogo distribuído |
|---|---|---|---|
| `sdl3` | zlib; um cabeçalho do OpenGL ES usa Apache-2.0 | janela, entrada e placa de vídeo | sim |
| `sdl3-net` | Zlib | conexões de rede | sim |
| `shaderc` | Apache-2.0 | compilar shaders para SPIR-V durante o build | não |
| `tl-expected` | CC0-1.0 | devolver erros por valor | sim |
| `catch2` | BSL-1.0 | testes automáticos | não |

O `shaderc` instala junto o glslang, o SPIRV-Tools e o SPIRV-Headers, todos com licenças abertas. O glslang inclui um trecho sob GPL 3 com a exceção do Bison. Essas ferramentas rodam só durante o build e não entram no jogo.

As ferramentas usadas só no desenvolvimento, como compiladores e analisadores, não entram no jogo e não ficam nesta lista.
