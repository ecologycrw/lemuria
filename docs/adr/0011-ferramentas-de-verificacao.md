---
id: 0011
titulo: Ferramentas de verificação
status: aceita
data: 2026-11-18
substitui:
substituida-por:
---

# ADR 0011: Ferramentas de verificação

## Contexto

O dono do produto não revisa código linha por linha, e o C++ permite erros de memória que outras linguagens impedem. As regras precisam ser garantidas por máquina, não depender de a IA lembrar delas. Princípio: **toda regra que uma máquina consegue verificar vira teste, análise, hook ou CI, não parágrafo.**

## Decisão

| Necessidade | Ferramenta |
|---|---|
| Comando único de verificação | `tools/dev.py`, em Python 3, só com a biblioteca padrão |
| Testes | Catch2 |
| Avisos do compilador | tratados como erro, no MSVC e no Clang |
| Erros de memória | AddressSanitizer no MSVC; AddressSanitizer e UndefinedBehaviorSanitizer no Clang |
| Práticas perigosas e complexidade | clang-tidy |
| Formatação | clang-format e `.editorconfig` |
| Dependências entre camadas, SDL isolado e termos proibidos | `tools/checks`, em Python 3, com testes próprios |
| Docs, specs e estrutura do repositório | `tools/checks` |
| Framework substituível | um framework de exemplo compilado só com o Núcleo |
| Antes de cada commit e push | hooks do Git em `sh` |
| Antes de cada comando da IA, no início e no fim da sessão | hooks do Claude Code em PowerShell 5.1 |
| Verificação independente | GitHub Actions em Windows e Linux |
| Desenho sem placa de vídeo | renderizadores de software na CI |
| Diagramas | `@mermaid-js/mermaid-cli` na CI |
| Força dos testes | testes de mutação com o Mull, no Linux |

**Toda trava precisa provar que funciona:** um exemplo com a violação precisa ser recusado.

**Testes de mutação:**

- rodam no Linux, só nas partes de lógica;
- o desenho na tela fica de fora, porque o teste de mutação não funciona bem nessa parte;
- começam só informando e passam a bloquear o PR quando estiverem estáveis, com a aprovação do dono;
- se o Mull não funcionar de forma estável, o plano B é um piso de cobertura de testes, que é uma garantia menor.

## Consequências

- Uma regra quebrada aparece como build vermelho, teste falhando ou commit bloqueado, com a explicação.
- Há três linguagens de script além do C++ (Python, `sh` e PowerShell), todas restritas às ferramentas.
- As travas em Python rodam mesmo quando o build do C++ está quebrado.
- O pre-commit roda build e testes, o que deixa cada commit mais lento. Isso é reavaliado se ficar incômodo.

## Alternativas consideradas

- **Regras só em texto:** a IA pode esquecer ou ignorar.
- **Ferramentas internas em C++:** dependeriam de um build já feito, e uma trava que não roda deixaria a ação passar.
- **GoogleTest no lugar do Catch2:** funciona bem, mas as etiquetas do Catch2 ligam cada teste ao critério de aceitação de forma direta.
