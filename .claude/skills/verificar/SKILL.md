---
name: verificar
description: Roda todas as verificações do Lemuria (formatação, build, testes, sanitizers, análise estática e checagens) e mostra a saída real, com cada critério de aceitação ligado à sua prova. Use depois de cada tarefa, antes de revisar e antes de fechar uma fatia.
---

# /verificar

## Passos

Rode e mostre a saída real (resumida, mas com os números):

```bash
py tools/dev.py verify
```

No Linux, use `python3` no lugar de `py`.

Se houver uma fatia em andamento, monte também a tabela de critérios:

| CA | Prova | Situação |
|---|---|---|
| CA-01 | teste com a etiqueta `[NNNN-CA-01]` encontrado em `Engine/Source/Runtime/Core/Tests/...` | passou |
| CA-02 | verificação manual prevista no plano | pendente |

Para achar as provas, procure `[NNNN-CA-` nos arquivos `.cpp` das pastas `Tests/` de cada módulo e de `tests/`.

## Resultado

- **Verde** só se o comando passou nesta execução.
- Se algo falhar, mostre o erro, explique em linguagem simples e corrija (dentro do escopo da tarefa).
- Nunca diga que passou sem ter rodado.
