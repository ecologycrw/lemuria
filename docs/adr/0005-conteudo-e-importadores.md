---
id: 0005
titulo: Formatos próprios, glTF, importadores e cifra plugáveis
status: aceita
data: 2026-11-18
substitui:
substituida-por:
---

# ADR 0005: Formatos próprios, glTF, importadores e cifra plugáveis

## Contexto

A engine precisa carregar modelos, animações, texturas e mapas rapidamente em hardware modesto. O desenvolvedor precisa de um caminho aberto para criar conteúdo e pode já ter arquivos em outros formatos. Ele também pode querer dificultar a extração dos arquivos do jogo.

## Decisão

- **Formatos binários próprios** em tempo de execução, feitos para carregar rápido. Eles são gerados pelo build de conteúdo.
- **glTF 2.0** é a entrada oficial de modelos e animações.
- **Importadores plugáveis:** o desenvolvedor pode acrescentar a leitura de outros formatos no projeto dele. Importadores de formatos de outros jogos não entram neste repositório.
- **Cifra plugável:** o empacotamento aceita uma cifra fornecida pelo desenvolvedor, e cada jogo usa a sua.
- **Sem anti-cheat:** numa engine open source, ele seria só um obstáculo. A proteção real é o servidor decidir tudo.
- Nenhum arquivo é considerado confiável em tempo de execução: tudo é validado antes do uso.

## Consequências

- Quem usa ferramentas comuns, como o Blender, consegue criar conteúdo sem nada especial.
- Existe um passo de build de conteúdo entre o arquivo original e o jogo.
- Os formatos binários precisam de versão e de testes com dados inválidos.
- A cifra dificulta a extração, mas não a impede.

## Alternativas consideradas

- **Carregar glTF diretamente no jogo:** mais simples, mas mais lento e com arquivos maiores.
- **FBX como entrada oficial:** é um formato fechado, que depende de um SDK proprietário.
- **Cifra embutida na engine:** como o código é público, uma cifra única protegeria todos os jogos igualmente mal.
