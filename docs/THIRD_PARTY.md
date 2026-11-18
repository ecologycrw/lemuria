# Bibliotecas de terceiros

Toda biblioteca do `vcpkg.json` fica registrada aqui com a licença dela ([ADR 0009](adr/0009-repositorio-publico-e-terceiros.md)). A checagem `structure` recusa uma biblioteca sem registro. Uma biblioteca nova precisa da aprovação do dono.

| Biblioteca | Licença | Para quê | Entra no jogo distribuído |
|---|---|---|---|
| `sdl3` | zlib; um cabeçalho do OpenGL ES usa Apache-2.0 | janela, entrada e placa de vídeo | sim |
| `tl-expected` | CC0-1.0 | devolver erros por valor | sim |
| `catch2` | BSL-1.0 | testes automáticos | não |

As ferramentas usadas só no desenvolvimento, como compiladores e analisadores, não entram no jogo e não ficam nesta lista.
