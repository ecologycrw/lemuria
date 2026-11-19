---
id: 0018
titulo: Conexões de rede com SDL3_net e caminho para o GNS
status: aceita
data: 2026-11-18
substitui:
substituida-por:
---

# ADR 0018: Conexões de rede com SDL3_net e caminho para o GNS

## Contexto

O Núcleo precisa abrir conexões e trocar bytes sem que o quadro espere pela rede (INV-04), e só a camada de plataforma pode falar com o sistema operacional (INV-09). A maioria dos servidores clássicos fala TCP. O dono também quer poder oferecer o GameNetworkingSockets (GNS) no futuro.

O GNS não substitui o TCP, porque só conversa com quem também usa GNS. As diferenças entre os dois afetam o contrato: o TCP entrega um fluxo contínuo de bytes, e o GNS entrega mensagens inteiras e permite envio sem garantia de chegada.

O servidor de teste também precisa de TCP e de relógio real. A regra anterior o proibia de usar a camada de plataforma e o sistema operacional ao mesmo tempo, sem deixar caminho.

## Decisão

- **O SDL3_net (licença Zlib) é a implementação de TCP da engine.** Ele fica só em `lemuria_platform_sdl`, e uma checagem automática garante isso.
- **O Núcleo define uma interface própria de conexões.** O Classic, os adaptadores e os jogos só conhecem essa interface.
- **A rede roda em segundo plano** e conversa com o quadro por filas.
- **A interface nasce preparada para mais de um tipo de conexão:**

| Diferença | Como a interface trata |
|---|---|
| Fluxo de bytes ou mensagens inteiras | Entrega blocos de bytes e informa se cada bloco é uma mensagem inteira |
| Garantia de entrega | O envio tem uma opção de garantia, e o padrão é "tudo chega, em ordem" |
| Opção que um tipo de conexão não atende | Cada tipo publica o que oferece, e o pedido devolve um erro claro. A garantia nunca é trocada em silêncio |
| Limites de tamanho de mensagem | Fazem parte do que cada tipo publica |
| Códigos de queda de cada biblioteca | Motivos neutros, com o código original guardado para diagnóstico |

- **O adaptador declara o tipo de conexão** que o servidor dele usa.
- **O GNS não entra agora.** Quando entrar, será um módulo separado e opcional, com ADR próprio e aprovação do dono ([P-0003](../PROPOSALS.md)).
- **O servidor de teste pode usar a camada de plataforma.** SDL, sistema operacional e Classic continuam proibidos para ele.

## Consequências

- Trocar ou somar uma biblioteca de rede mexe só na camada de plataforma ou no módulo novo.
- Jogos que não usarem o GNS não carregam as dependências dele.
- Um teste com uma conexão que imita o GNS protege esse caminho, mas não substitui a integração real.
- O servidor de teste passa a depender da mesma implementação de TCP que o cliente.

## Alternativas consideradas

- **Asio:** é madura, mas muito maior do que o necessário e aumenta o tempo de compilação.
- **Sockets diretos do sistema operacional:** não exigem biblioteca, mas pedem código separado para Windows e Linux.
- **Só o GNS:** impediria a ligação com servidores que falam TCP.
- **Instalar o GNS agora:** traria OpenSSL e Protobuf para uma fatia que não usa nenhum dos dois.
