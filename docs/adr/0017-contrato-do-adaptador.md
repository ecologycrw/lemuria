---
id: 0017
titulo: Contrato do adaptador de protocolo
status: aceita
data: 2026-11-18
substitui:
substituida-por:
---

# ADR 0017: Contrato do adaptador de protocolo

## Contexto

O [ADR 0004](0004-adaptador-de-protocolo.md) definiu dois níveis para o adaptador e deixou o contrato detalhado para a fatia 0. O risco é moldar esse contrato no servidor de teste, que responde tudo, em ordem e na hora. Um servidor real pode recusar um pedido em silêncio, encurtar um caminho sem avisar, reaproveitar identificadores e mandar eventos enquanto o mapa ainda carrega.

O contrato foi comparado com o fluxo de um servidor real por meio de uma lição da central de conhecimento, registrada no plano da fatia 0.

## Decisão

**A fronteira do contrato são os bytes e os eventos.** É o único ponto que serve para qualquer servidor.

| Quem | Responde por |
|---|---|
| **Núcleo** | Abrir várias conexões, cada uma com identidade que nunca se repete; entregar blocos de bytes marcados com a conexão de origem; enviar na ordem pedida; nunca interpretar o conteúdo |
| **Adaptador** | Decidir quantas conexões existem e quando trocar de uma para outra; cuidar do formato das mensagens e da cifra; validar tudo; traduzir bytes em eventos e pedidos em bytes; publicar o descritor de capacidades |
| **Classic** | Fases da sessão, eventos, pedidos, mundo, movimento, antecipação e correção |

**Regras que o contrato garante:**

1. **Todo pedido é sem garantia de resposta.** Para o cliente, o silêncio é um aceite provisório, e a falta de resposta nunca trava o personagem.
2. **A correção do servidor vence.** Ela substitui a posição e cancela o caminho. Uma resposta antiga não desfaz uma correção mais nova.
3. **O destino confirmado pode ser diferente do pedido.** Isso não é erro.
4. **Identificadores são opacos e temporários.** Valem de quando uma entidade aparece até quando ela sai, dentro do mesmo mapa. Aparecer com um número em uso substitui a entidade anterior. Um evento sobre número desconhecido é ignorado e contado.
5. **O mundo tem época.** Cada troca de mapa cria uma época local, remove todas as entidades e descarta os eventos da época anterior.
6. **Eventos que chegam durante o carregamento ficam retidos** e são aplicados na ordem. O cliente não espera um sinal de "fim da visão inicial".
7. **Toda espera tem prazo e saída.**
8. **Erros têm dois níveis.** Um erro no formato encerra a conexão. Um erro numa única mensagem só é registrado.
9. **Capacidades opcionais podem faltar.** O framework funciona sem elas e não tem caminho especial para nenhum servidor.

**O descritor de capacidades** informa o tipo de conexão, o modo de envio do movimento, o limite de passos por pedido, se o servidor exige aviso de mapa carregado, o sinal de presença e a política de retomada.

**O adaptador é completo.** Ele recebe os bytes crus. Peças prontas para adaptadores, como enquadrador e kit de conformidade, só nascem quando existir um segundo adaptador real, seguindo a [regra de três](../CONVENTIONS.md#regra-de-três).

## Consequências

- O contrato aceita servidores que funcionam de formas diferentes, sem mudar o framework.
- Os cenários difíceis de um servidor real viram testes automáticos do Classic desde a fatia 0.
- Cada adaptador escreve o próprio enquadramento até as peças prontas existirem.
- O encaixe com um servidor externo continua sem prova até a fatia 6 ([TD-0003](../TECH_DEBT.md)).

## Alternativas consideradas

- **Adaptador fino, com o framework cuidando do enquadramento e da cifra:** deixaria adaptadores curtos, mas não atenderia protocolos com outro formato.
- **Entregar logo as peças prontas:** criaria uma abstração antes de existir um segundo adaptador para mostrar o que de fato se repete.
- **Exigir confirmação de cada pedido:** servidores reais podem nunca responder.
