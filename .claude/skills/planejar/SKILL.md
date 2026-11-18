---
name: planejar
description: Escolhe a próxima fatia do Lemuria e escreve a spec e o plano para o dono aprovar. Use quando não houver fatia em andamento e o dono quiser começar a próxima etapa.
---

# /planejar

Transforma o próximo item do roadmap numa spec e num plano aprovados. **Não implementa nada.**

## Pré-condições

- Nenhuma fatia em andamento: o `spec-atual` do `docs/STATUS.md` está vazio. Se não estiver, pare e explique ao dono.
- A árvore de trabalho está limpa (`git status --short` vazio).

## Passos

1. **Contexto.** Leia `docs/ROADMAP.md`, `docs/STATUS.md`, as propostas abertas em `docs/PROPOSALS.md`, `docs/TECH_DEBT.md`, o report da última fatia e as seções de `docs/ARCHITECTURE.md` que a fatia toca.
2. **Escolha da fatia.** É a primeira de "Próximas" no ROADMAP, a menos que a última retrospectiva tenha decidido outra. Se ela depender de algo que ainda não existe, proponha antes uma fatia de fundação e pergunte ao dono.
3. **Conhecimento.** Se a fatia precisar entender como um cliente ou servidor de referência resolve um problema, use `/consultar-central`. Toda fatia que alterar o contrato do adaptador de protocolo precisa compará-lo com o fluxo real de um servidor externo ([ADR 0004](../../../docs/adr/0004-adaptador-de-protocolo.md)).
4. **Branch.** Rode `git switch main`, `git pull --ff-only` e `git switch -c fatia/NNNN-nome`. A fatia N usa o número N com quatro dígitos; o nome é curto, em kebab-case.
5. **Spec.** Crie `specs/NNNN-nome/spec.md` a partir de `templates/spec.md`, com `status: rascunho`. Os critérios de aceitação são verificáveis: cada um é provado por um teste automático ou por uma verificação manual descrita. Escreva "Como o dono testa" com passos que ele consiga seguir sozinho. Se a fatia mudar o que aparece na tela, leia `docs/VISUAL.md` e preencha a seção "Prova visual"; se não mudar, escreva "não se aplica".
6. **Plano.** Crie `plan.md` a partir de `templates/plan.md`. Preencha com cuidado a "Visão de destino" e as "Simplificações provisórias" (ver "interface pensa no destino, implementação no agora" em `docs/CONVENTIONS.md`). Liste bibliotecas novas, ADRs necessários, mudanças nos formatos de conteúdo ou no contrato do adaptador e as lições da central usadas.
7. **Checagem.** Rode `py tools/checks/run.py docs terms`.
8. **Apresente ao dono**, em português simples:
   - o que a fatia entrega e como testar;
   - os critérios de aceitação em linguagem simples;
   - o que fica de fora;
   - o que precisa da decisão do dono (bibliotecas novas, ADRs, formatos de conteúdo, contrato do adaptador);
   - que, ao aprovar, a fatia passa para "Agora" no ROADMAP.

   Pergunte se o dono aprova.
9. **Só depois de um "sim" explícito:**
   - mude o status da spec para `aprovada` e preencha `aprovada-em`;
   - no `docs/ROADMAP.md`, mova a fatia de "Próximas" para "Agora", com o link da spec, e a anterior para "Concluídas", se ainda não estiver lá. A aprovação da spec vale para essa mudança; qualquer outra mudança no ROADMAP continua precisando de aprovação própria;
   - atualize o STATUS (`spec-atual: NNNN`, `branch`, "Agora" e "Próxima ação");
   - commit `docs(NNNN): plan slice N` e `git push -u origin fatia/NNNN-nome`.

Se o dono pedir mudanças, ajuste a spec e o plano e volte ao passo 8.

## Nunca

- Começar a implementar.
- Mudar o ROADMAP ou a VISION sem a aprovação do dono (mover a fatia aprovada para "Agora" já está coberto pela aprovação da spec).
