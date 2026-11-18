---
id: 0013
titulo: Central de conhecimento em sessão separada
status: aceita
data: 2026-11-18
substitui:
substituida-por:
---

# ADR 0013: Central de conhecimento em sessão separada

## Contexto

O dono mantém, só no próprio computador, uma central de conhecimento que estuda o OpenMU e o MuMain. Ela tem skills, agentes, regras e hooks próprios, e guarda o que descobre em lições genéricas. O Lemuria precisa desse conhecimento, mas não pode ter contato com o código estudado ([ADR 0009](0009-repositorio-publico-e-terceiros.md)).

Abrir os dois projetos na mesma sessão misturaria comandos com o mesmo nome e regras que se contradizem, e deixaria o código sem licença ao alcance de quem escreve o Lemuria.

## Decisão

- **Sessões separadas.** Cada projeto é aberto na própria sessão, com as próprias skills e regras.
- **A ligação é só por arquivos de leitura.** A skill `/consultar-central` procura a resposta nas lições.
- Se a lição não existir, a skill escreve a pergunta pronta para o dono levar à central. A central investiga e escreve a lição.
- A fatia registra quais lições usou.

**Regras de acesso, aplicadas por um hook (`guard-central`):**

| Parte da central | Acesso |
|---|---|
| `licoes/` | leitura liberada |
| `docs/specs/` | leitura só com a confirmação do dono, a cada acesso |
| Código estudado e todo o resto | bloqueado, inclusive por comando de terminal |
| Escrita em qualquer parte | sempre bloqueada |

- O caminho da central fica em `.claude/central.local.json`, fora do Git. O repositório público não contém caminhos pessoais.
- O repositório não depende da central. Sem ela, como na CI, a skill só avisa que ela não está disponível.

## Consequências

- O conhecimento chega ao Lemuria sem carregar código nem conteúdo de terceiros.
- Levar uma pergunta à central é um passo manual do dono.
- A central precisa produzir lições antes das fatias que dependem delas.

## Alternativas consideradas

- **Abrir a central como pasta adicional na sessão do Lemuria:** causaria os conflitos descritos no contexto.
- **Copiar as lições para este repositório:** criaria duas versões do mesmo texto.
- **Confiar só em regras escritas:** a IA poderia ler o código por engano; o hook impede isso.
