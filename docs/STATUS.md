---
fase: fatia
spec-atual: 0000
branch: fatia/0000-primeiros-passos
atualizado-em: 2026-11-19
---

# Status

## Agora

**Fatia 0 (Primeiros passos) fechada, esperando o teste do dono.** As 34 tarefas e a revisão estão concluídas, e o PR da branch `fatia/0000-primeiros-passos` está aberto. O comando `py tools/dev.py demo` abre o servidor de teste e duas janelas do jogo: cada jogador vê o outro no mapa, e o título da janela mostra o estado da conexão. A prova visual gera a mesma imagem com Vulkan e Direct3D 12, com todos os números dentro das faixas. O [relatório da fatia](../specs/0000-primeiros-passos/report.md) traz as provas e o passo a passo do teste.

## Próxima ação

O dono testa seguindo a seção "O que você deve testar" do [relatório](../specs/0000-primeiros-passos/report.md#o-que-você-deve-testar), aprova a imagem da cena de prova se ela agradar e confere se o check da CI ficou verde no PR. Depois, o dono conta o que achou, e a `/retrospectiva` roda no mesmo PR, antes do merge.

## Bloqueios

Nenhum.

## Histórico

| Data | O que aconteceu |
|---|---|
| 2026-11-18 | Plano da preparação aprovado pelo dono |
| 2026-11-18 | Etapa 0 concluída: conta de máquina `lemuria-ci-dev`, chave de assinatura e primeiro commit assinado na `main` |
| 2026-11-18 | Parte 1 aprovada pelo dono: regras dos agentes, visão, arquitetura, fluxo, convenções, roadmap, modelos e 13 ADRs |
| 2026-11-18 | Parte 2 aprovada pelo dono: esqueleto em C++, comando único, checagens, hooks do Git e CI; cada trava foi quebrada de propósito para provar que funciona |
| 2026-11-18 | Parte 3 aprovada pelo dono: 10 skills, revisor, auditor, hooks do Claude Code, permissões e acesso controlado à central de conhecimento |
| 2026-11-18 | Parte 4: repositório publicado, `main` protegida e PR #1 aberto |
| 2026-11-18 | Repositório reorganizado em `Engine`, `Frameworks` e `Samples` ([ADR 0014](adr/0014-organizacao-do-repositorio.md)) |
| 2026-11-18 | Arquivos C++ e pastas de include passam a usar `PascalCase` ([ADR 0015](adr/0015-arquivos-cpp-em-pascalcase.md)) |
| 2026-11-18 | Guia visual e regras de prova visual definidos ([ADR 0016](adr/0016-direcao-visual-e-prova-visual.md)) |
| 2026-11-18 | Auditoria do ambiente: guia visual e imagens aprovadas protegidos, prova visual exigida pelas checagens e licenças das bibliotecas registradas |
| 2026-11-18 | Spec e plano da fatia 0 aprovados pelo dono, com TCP e a preparação para oferecer o GNS como opção de conexão |
| 2026-11-19 | Fatia 0 fechada: 34 tarefas, revisão independente, relatório escrito e PR aberto para o teste do dono |
