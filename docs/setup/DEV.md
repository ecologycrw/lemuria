# Ambiente de desenvolvimento

## Requisitos

No Windows:

- Windows 10/11 64 bits.
- Visual Studio 2022 ou 2026, com a carga de trabalho **Desenvolvimento para desktop com C++** e o componente **Ferramentas do C++ Clang** (traz o `clang-format` e o `clang-tidy`).
- CMake 3.30 ou mais novo e Ninja.
- vcpkg, com a variável de ambiente `VCPKG_ROOT` apontando para a pasta dele.
- Python 3.12 ou mais novo.
- Git e GitHub CLI (`gh`).

No Linux (usado na CI): Clang, CMake, Ninja, vcpkg e Python 3.

## Primeira vez num clone

Ative os hooks do Git, uma vez por clone:

```bash
git config core.hooksPath .githooks
```

A identidade dos commits da IA e a assinatura estão no [guia do GitHub](GITHUB.md).

A primeira compilação baixa e compila as bibliotecas externas e leva alguns minutos. As seguintes usam o cache do vcpkg.

## Comandos do dia a dia

Tudo passa por um comando só. No Linux, use `python3` no lugar de `py`.

| O quê | Comando |
|---|---|
| **Verificar tudo** | `py tools/dev.py verify` |
| Verificação rápida (a mesma do commit) | `py tools/dev.py quick` |
| Compilar | `py tools/dev.py build` |
| Rodar os testes | `py tools/dev.py test` |
| Rodar os testes com os detectores de erro de memória | `py tools/dev.py sanitize` |
| Conferir a formatação | `py tools/dev.py format` |
| Corrigir a formatação | `py tools/dev.py format --fix` |
| Análise estática | `py tools/dev.py tidy` |
| Checagens do repositório | `py tools/dev.py checks` |
| Testes das checagens e dos hooks | `py tools/dev.py pytests` |
| Abrir a GPU sem janela | `py tools/dev.py gpu-test` |

O `tools/dev.py` encontra o Visual Studio sozinho, então os comandos funcionam em qualquer terminal.

## O que roda automaticamente

| Quando | O quê | Onde |
|---|---|---|
| Antes de cada commit | busca de segredos; com código: `quick`; só com docs: checagens de docs e de termos | `.githooks/pre-commit` |
| Ao escrever a mensagem de commit | formato `tipo(escopo): descrição` | `.githooks/commit-msg` |
| Antes de cada push | bloqueia envio para `main` e `master` | `.githooks/pre-push` |
| A cada PR e a cada push na `main` | tudo de novo, em Windows e Linux, mais a análise estática, os diagramas, os testes de mutação e o resumo de segurança do PR | `.github/workflows/ci.yml` |

## As checagens do repositório

O `tools/checks` confere o que o compilador não confere:

| Checagem | O que recusa |
|---|---|
| `docs` | links quebrados, specs incompletas, critério de aceitação sem prova, seção "Prova visual" ausente, ADRs fora de sequência, item provisório sem destino |
| `structure` | arquivos obrigatórios faltando, módulos fora do padrão, nomes de arquivo errados, biblioteca sem licença registrada |
| `architecture` | dependência proibida entre camadas e uso do SDL ou do sistema operacional fora da camada de plataforma |
| `cpp-rules` | `new` e `delete` soltos, funções perigosas de C, relógio e sorteio do sistema, `reinterpret_cast` |
| `terms` | nomes de outros jogos e dos formatos de arquivo deles fora dos lugares permitidos |

## Testes de mutação

O Mull muda pedacinhos do código de lógica do Núcleo e confere se algum teste falha. Ele roda só na CI, em Linux, e por enquanto **só informa**: não bloqueia o PR ([TD-0004](../TECH_DEBT.md)).

## Central de conhecimento

A central é opcional e existe só no computador do dono ([ADR 0013](../adr/0013-central-de-conhecimento.md)). Para ligá-la, crie o arquivo `.claude/central.local.json`, que fica fora do Git:

```json
{ "path": "C:/caminho/para/a/central" }
```

Sem esse arquivo, a skill `/consultar-central` só avisa que a central não está disponível.

Abra o Lemuria e a central em **sessões separadas**.

## Se um hook bloquear

Leia a mensagem: ela diz o que corrigir. **Nunca** use `--no-verify` para pular uma checagem. Se a regra parecer errada, registre uma proposta (ver [WORKFLOW.md](../WORKFLOW.md#propostas-como-as-necessidades-aparecem)).
