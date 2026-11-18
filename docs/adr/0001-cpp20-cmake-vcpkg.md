---
id: 0001
titulo: C++20, CMake e vcpkg, com MSVC e Clang
status: aceita
data: 2026-11-18
substitui:
substituida-por:
---

# ADR 0001: C++20, CMake e vcpkg, com MSVC e Clang

## Contexto

O Lemuria é um cliente gráfico que precisa rodar bem em hardware modesto, com controle fino de memória, e chegar um dia a Linux e mobile. O código é escrito principalmente por IA, e o C++ não protege contra erros de memória como outras linguagens.

## Decisão

- **Linguagem:** C++20, sem extensões de compilador.
- **Sem módulos do C++20:** as ferramentas de build e de análise ainda tropeçam neles.
- **C++23 recurso a recurso:** cada recurso entra por ADR, quando todos os compiladores-alvo o suportarem.
- **Compiladores:** MSVC no Windows e Clang no Linux. O Clang também é o caminho para o mobile.
- **Build:** CMake com presets e Ninja.
- **Bibliotecas externas:** vcpkg em modo manifesto, com as versões fixadas no repositório.
- **Erros por valor:** `tl::expected` até o `std::expected` estar disponível em todos os compiladores-alvo. Sem exceções no caminho do quadro.

## Consequências

- O mesmo código é compilado por dois compiladores desde o início, o que revela código não portável cedo.
- As travas precisam compensar a falta de proteção da linguagem: avisos como erro, análise estática e sanitizers ([ADR 0011](0011-ferramentas-de-verificacao.md)).
- A primeira compilação das bibliotecas é demorada. As seguintes usam cache.

## Alternativas consideradas

- **C++23 como base:** traz recursos úteis, mas o suporte ainda é desigual, principalmente nos compiladores de mobile.
- **C# (.NET):** é a linguagem do Elysium, que este projeto substitui. A decisão do dono é usar C++.
- **Conan no lugar do vcpkg:** funciona bem, mas o vcpkg integra de forma mais simples com o Visual Studio e com o CMake.
- **Baixar as bibliotecas direto pelo CMake (FetchContent):** deixaria cada build mais lento e sem cache compartilhado.
