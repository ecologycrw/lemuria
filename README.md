# Lemuria

> Nome provisório. *Lemuria is an open source C++ client engine for building classic-style MMORPGs. Documentation is currently in Portuguese.*

O Lemuria é uma engine open source, escrita em C++20, para desenvolvedores criarem o **cliente** de MMORPGs no estilo clássico: mapas em grade, câmera em 3/4, clicar para andar, combate por alvo, itens com nível e opções, personagens com equipamento trocável e interface pesada.

O Lemuria não inclui um servidor de jogo. Cada desenvolvedor liga a engine ao servidor que escolher, escrevendo um adaptador de protocolo.

A engine tem três camadas:

- **Núcleo (Lemuria Core):** janela, gráficos, interface, som, entrada, conteúdo e conexões de rede.
- **Framework Classic:** as mecânicas de cliente do gênero, prontas e configuráveis por dados. É opcional: o desenvolvedor pode criar o próprio framework sobre o Núcleo.
- **Jogo:** os dados, a arte e o adaptador de protocolo de cada jogo feito com a engine.

## Estado atual

**Em preparação.** Ainda não há nada para rodar. Veja [docs/STATUS.md](docs/STATUS.md) e [docs/ROADMAP.md](docs/ROADMAP.md).

## Documentação

- [Visão](docs/VISION.md): o que o produto é e o que ele não é
- [Guia visual](docs/VISUAL.md): a aparência desejada e como ela é conferida
- [Arquitetura](docs/ARCHITECTURE.md): como as peças se encaixam
- [Fluxo de trabalho](docs/WORKFLOW.md): como o projeto é desenvolvido
- [Decisões (ADRs)](docs/adr/README.md): por que cada decisão foi tomada
- [Glossário](docs/GLOSSARY.md): os termos usados no projeto
- [Bibliotecas de terceiros](docs/THIRD_PARTY.md): as dependências e as licenças delas

## Desenvolvimento assistido por IA

Quase todo o código deste projeto é escrito por IA, com revisão e aprovação humana. Os commits da IA são feitos por uma conta de máquina e identificados como coautoria do Claude. Veja [docs/WORKFLOW.md](docs/WORKFLOW.md).

## Contribuições

Ainda não estão abertas. Isso vai mudar quando a base estiver estável.

## Segurança

Para relatar uma vulnerabilidade, veja [SECURITY.md](SECURITY.md).

## Licença

[MIT](LICENSE).

## Aviso

O Lemuria é um projeto independente, sem afiliação com nenhuma empresa, jogo ou marca. Nenhum asset, arquivo, formato, protocolo ou código de outros jogos faz parte deste repositório. Marcas citadas na documentação pertencem aos seus donos.
