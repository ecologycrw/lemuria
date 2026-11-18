---
id: 0015
titulo: Arquivos C++ e pastas de include em PascalCase
status: aceita
data: 2026-11-18
substitui:
substituida-por:
---

# ADR 0015: Arquivos C++ e pastas de include em PascalCase

## Contexto

O [ADR 0014](0014-organizacao-do-repositorio.md) colocou as pastas do produto em `PascalCase`, mas manteve os arquivos C++ em `snake_case`. O resultado misturava dois estilos no mesmo caminho, como `Core/Private/version.cpp`. A engine ainda é pequena, então este é o momento mais barato para uniformizar os nomes.

## Decisão

- **Arquivos C++ usam `PascalCase`.** O arquivo leva o nome do tipo principal que guarda: `FrameClock` fica em `FrameClock.hpp` e `FrameClock.cpp`.
- **Arquivos de teste terminam em `Test`:** `FrameClockTest.cpp`.
- **As pastas de include também usam `PascalCase`:** `Lemuria/Core/FrameClock.hpp` e `DemoProtocol/Protocol.hpp`.
- **O restante continua igual:** namespaces (`lemuria::core`), alvos do build (`lemuria_core`), funções e variáveis (`advance_time`) e tipos (`FrameClock`).
- Este ADR atualiza o ponto do ADR 0014 que mantinha os includes em `snake_case`. O restante daquele ADR continua valendo.

## Consequências

- O caminho inteiro de um arquivo segue um único estilo, e o nome do arquivo indica o tipo que ele guarda.
- O Windows trata `Version.hpp` e `version.hpp` como o mesmo arquivo, mas o Linux não. Um include escrito com a caixa errada compila no Windows e falha na CI do Linux, que bloqueia o merge.
- As regras de camada comparam os includes sem diferenciar maiúsculas de minúsculas, para que um include com a caixa errada não escape delas no Windows.

## Alternativas consideradas

- **Manter `snake_case`:** segue o estilo da biblioteca padrão do C++, mas continuaria misturando estilos dentro dos caminhos.
- **Trocar só os arquivos e manter `lemuria/core/` nos includes:** deixaria dois estilos dentro de cada include.
- **Trocar também funções e variáveis para `PascalCase`:** mudaria todo o código sem necessidade.
