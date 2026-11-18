# Propostas

Fila única de tudo o que surgiu fora da tarefa atual: ideias, problemas, regras que estão faltando e lições aprendidas. A fila é processada na `/retrospectiva`, e cada entrada vira fatia, decisão, regra ou trava, ou é descartada. Ver [WORKFLOW.md](WORKFLOW.md#propostas-como-as-necessidades-aparecem).

Formato de cada entrada (o modelo completo fica em `templates/proposal.md`):

```markdown
### P-0001: título curto
- Origem: fatia e tarefa, feedback do dono, medição ou checagem
- Tipo: fatia | regra | trava | skill | doc | decisão | bug
- Evidência: o que mostra que isso é necessário
- Prioridade sugerida: alta | média | baixa
- Status: aberta
```

## Abertas

### P-0001: incluir o tipo "agente" nas propostas
- Origem: feedback do dono, na auditoria do ambiente
- Tipo: trava
- Evidência: o dono perguntou se o ambiente está "preparado para criar ou sugerir por conta própria criação de rules, hooks, agents, skils e docs" e respondeu "Sim, registrar". A lista de tipos em `docs/PROPOSALS.md`, `templates/proposal.md` e na skill `/propor` não tem uma categoria para sugerir um agente novo
- Prioridade sugerida: baixa
- Status: aberta

### P-0002: resolver o conflito entre `/propor` e `/planejar`
- Origem: registro da proposta P-0001, entre duas fatias
- Tipo: trava
- Evidência: sem fatia em andamento, o passo 4 do `/propor` manda deixar a proposta "para a próxima branch", mas as pré-condições do `/planejar` exigem `git status --short` vazio. A proposta não salva bloqueia o planejamento da fatia seguinte. Na P-0001, foi preciso abrir um PR só para a proposta. O dono respondeu "Sim, registrar"
- Prioridade sugerida: média
- Status: aberta

### P-0003: oferecer o GNS como segunda opção de conexão
- Origem: feedback do dono, no planejamento da fatia 0
- Tipo: fatia
- Evidência: o dono disse "A ideia é essa, dar opção de servidores que falam TCP como você disse, mas também opção para usar o GNS". O GameNetworkingSockets (GNS) só conversa com quem também o usa, então ele entra ao lado do TCP com SDL3_net, num módulo separado e opcional. A versão 1.6.1 disponível no vcpkg usa a licença BSD-3-Clause e depende de OpenSSL e Protobuf. A fatia 0 deixa o contrato preparado, com uma conexão de teste que imita o GNS, mas não instala a biblioteca
- Prioridade sugerida: média
- Status: aberta

### P-0004: definir quando o histórico do STATUS é atualizado
- Origem: feedback do dono, durante a fatia 0
- Tipo: regra
- Evidência: o dono perguntou "normalmente quando que a seção ## Histórico do STATUS.md recebe algo ?". Nenhuma skill nem documento do fluxo define isso, e a conclusão das etapas 1 e 2 da fatia 0 não foi registrada nessa seção. Sugestão: atualizar ao aprovar a spec, ao fechar a fatia e ao fazer o merge
- Prioridade sugerida: baixa
- Status: aberta

## Processadas

*(nenhuma)*
