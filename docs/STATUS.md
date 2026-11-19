---
fase: fatia
spec-atual: 0000
branch: fatia/0000-primeiros-passos
atualizado-em: 2026-11-19
---

# Status

## Agora

**Fatia 0 (Primeiros passos) em construção.** As etapas 1 (Fundações), 2 (Desenho), 3 (Prova visual) e 4 (Contrato e mundo) estão concluídas: a janela mostra o chão em blocos com Vulkan e Direct3D 12, `py tools/dev.py visual` gera a imagem de prova, e o framework passa nos cenários de servidor "mal-educado" com dois adaptadores que funcionam de jeitos diferentes. O andamento fica nas [tarefas da fatia](../specs/0000-primeiros-passos/tasks.md).

## Próxima ação

Rodar `/implementar` na branch `fatia/0000-primeiros-passos` para continuar da tarefa T26, as mensagens da demonstração, na etapa 5 (Protocolo e servidor).

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
