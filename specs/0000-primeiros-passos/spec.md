---
id: 0000
titulo: Primeiros passos
status: em-andamento
aprovada-em: 2026-11-18
---

# Fatia 0: Primeiros passos

## Resumo para o dono

No fim desta fatia, o Lemuria abre uma janela com um chão em blocos, visto de cima por uma câmera em 3/4. Você clica num bloco e o personagem anda até lá, com o servidor de teste decidindo o que vale. Dois clientes abertos ao mesmo tempo se veem andando no mesmo mapa. A engine também gera uma imagem de uma cena fixa, confere os números dela e mede a fluidez, para que a qualidade visual possa ser verificada desde o começo.

## Escopo

- **Janela e desenho:** janela do jogo, câmera alta em 3/4 e chão em blocos com as cores da paleta do guia visual.
- **Clicar para andar:** o clique vira um bloco do mapa, o caminho desvia dos blocos bloqueados e o movimento é suave em qualquer taxa de quadros.
- **Servidor de teste:** um programa local que guarda o mapa, confere cada caminhada e avisa os clientes do que aconteceu. Ele só aceita conexões do próprio computador.
- **O servidor decide:** o personagem começa a andar na hora, e o servidor pode corrigir. Uma faixa de trilha do mapa é proibida só pelo servidor, para que a correção possa ser vista na tela.
- **Dois clientes no mesmo mapa:** cada um vê o outro aparecer, andar e sumir.
- **Contrato do adaptador de protocolo:** as regras que ligam a engine ao servidor de qualquer jogo, comparadas com o fluxo de um servidor real.
- **Mais de um tipo de conexão:** esta fatia usa TCP, e o contrato já nasce preparado para oferecer também o GameNetworkingSockets (GNS). Cada adaptador declara o tipo de conexão que o servidor dele usa.
- **Prova visual:** um comando que gera a imagem da cena de prova, mede os números e compara com a imagem aprovada por você.
- **Medidor de fluidez:** o tempo médio e o tempo do pior 1% dos quadros.
- **Quadro sem alocar memória:** um teste automático que prova essa regra.
- **Comandos para testar:** um comando abre a demonstração completa e outro gera a prova visual.

## Fora do escopo

| O que fica de fora | Quando entra |
|---|---|
| Personagem de verdade, com modelo e animação (agora é uma forma simples) | fatia 1 |
| Chão pintado, luzes de fogo, manchas de sombra e neblina | fatia 2 |
| Equipamento | fatia 3 |
| Interface do jogo (o estado da conexão aparece no título da janela) | fatia 4 |
| Monstros e combate | fatia 5 |
| Login, escolha de personagem, cifra, reconexão automática e sinal de presença | fatia 6, quando um servidor real exigir. O contrato já deixa espaço para eles |
| Mais de um mapa na demonstração (a lógica de troca de mapa é testada de forma automática) | fatia 6 |
| Instalar e usar o GNS de verdade | numa fatia própria, a ser decidida a partir da proposta P-0003 |
| Janela no Linux (o Linux continua só compilando e rodando os testes) | fatia de plataformas |
| Comprovar o piso de hardware | fatia de escala |

## Critérios de aceitação

### Janela, câmera e movimento

- CA-01: O cliente abre uma janela com chão em blocos, visto por uma câmera a 50 graus, com variação de um grau para mais ou para menos. O personagem ocupa de 8% a 10% da altura da tela, medido dos pés à cabeça.
- CA-02: Um clique na tela identifica o bloco que está sob o cursor, inclusive nas bordas e nos cantos da janela.
- CA-03: O caminho até o bloco clicado desvia dos blocos bloqueados. Um clique em bloco bloqueado ou inalcançável não move o personagem.
- CA-04: Depois do mesmo tempo de caminhada, o personagem está no mesmo ponto com 30, 60 ou 144 quadros por segundo.

### O servidor decide

- CA-05: O personagem começa a andar sem esperar o servidor. Se o servidor não responder nada, o personagem não trava.
- CA-06: Uma correção do servidor substitui a posição, cancela o resto do caminho e o personagem desliza até o ponto certo. Uma resposta antiga, enviada antes da correção, não desfaz a correção.
- CA-07: Quando o servidor aceita um destino mais curto que o pedido, o cliente adota esse destino sem tratar isso como erro.
- CA-08: O servidor de teste recusa caminhos inválidos: passos em blocos bloqueados, passos que não são vizinhos, origem longe demais e velocidade acima do permitido.

### Dois clientes

- CA-09: Com dois clientes ligados ao servidor por uma conexão de rede real, cada um vê o outro aparecer, andar até o destino e sumir quando o outro se desconecta.

### Contrato do adaptador

- CA-10: Mensagens que chegam partidas byte a byte, ou várias juntas, são lidas corretamente e na ordem.
- CA-11: Dados inválidos ou maliciosos nunca derrubam o programa. Um erro no formato encerra a conexão, e um erro numa única mensagem só é registrado.
- CA-12: Um número de personagem desconhecido é ignorado e contado, sem criar personagem. Um número reutilizado substitui o personagem anterior.
- CA-13: Eventos que chegam enquanto o mapa carrega ficam guardados e são aplicados na ordem. Ao trocar de mapa, todos os personagens são removidos e os eventos do mapa anterior são descartados. Toda espera tem prazo e saída.
- CA-14: Dados de uma conexão já encerrada nunca viram eventos. Se o servidor cair ou não existir, o cliente mostra isso no título da janela e continua respondendo.
- CA-15: Dois adaptadores de teste com capacidades diferentes (um envia os passos, o outro só o destino) funcionam com o mesmo framework, sem nenhum caminho especial para um deles.

- CA-16: Uma conexão de teste que imita o jeito do GNS, com mensagens inteiras e envio sem garantia de chegada, funciona com o mesmo Classic, sem nenhuma mudança nele. Quando um tipo de conexão não atende a uma opção de envio, como o TCP com o envio sem garantia, o pedido devolve um erro claro, e a garantia nunca é trocada em silêncio.

### Garantias do quadro

- CA-17: O quadro nunca espera pela rede, mesmo com o outro lado da conexão parado.
- CA-18: Depois de aquecido, o quadro não aloca memória, com personagens andando e eventos chegando.

### Prova visual e fluidez

- CA-19: O comando da prova visual gera, duas vezes seguidas na mesma máquina, duas imagens idênticas da cena de prova.
- CA-20: A imagem da cena de prova fica dentro das faixas exigidas na seção "Prova visual".
- CA-21: A comparação recusa uma imagem alterada acima da tolerância, aceita a imagem aprovada e avisa, sem falhar, quando ainda não há imagem aprovada. A tolerância é medida e justificada num ADR.
- CA-22: A cena de prova funciona com Direct3D 12 e com Vulkan no Windows, e com o renderizador por software no Linux da CI. As imagens ficam dentro da tolerância entre si.
- CA-23: O medidor de fluidez informa o tempo médio e o tempo do pior 1% de 600 quadros, depois de aquecer. No computador de desenvolvimento, os dois ficam em até 16,6 ms.

## Prova visual

- **Cena de prova:** o mapa de demonstração de 64 por 64 blocos, gerado sempre com o mesmo sorteio, numa imagem de 1280 por 720 pontos. A câmera fica fixa a 50 graus, com dois personagens parados em blocos definidos e o tempo congelado.
- **O que a imagem precisa mostrar:**
  - o chão preenche toda a imagem, em tons escuros e quentes, com terra, musgo, trilha, pedra e sombra da paleta do guia visual;
  - os dois personagens são fáceis de ver e de diferenciar sobre o chão escuro;
  - as cores de magia não aparecem no cenário;
  - nada lembra a tela de outro jogo.
- **Números exigidos:**

| Medida | Faixa exigida nesta fatia |
|---|---|
| Brilho típico da imagem (mediana) | 0,11 a 0,19 |
| Parte da imagem em sombra profunda | 12% a 40% |
| Parte da imagem em tons escuros | 78% a 93% |
| Parte da imagem em tons quentes | 75% ou mais |
| Saturação mediana | 0,50 a 0,80 |
| Parte da imagem em pontos de luz | **não exigida:** as luzes só chegam na fatia 2 |

- **O que ainda não vale:** os blocos ficam visíveis nesta fatia. A regra "blocos repetidos visíveis" do guia visual passa a valer na fatia 2, com o chão pintado.
- **Fluidez:** tempo médio e tempo do pior 1% dos quadros em até 16,6 ms na cena de prova, no computador de desenvolvimento. Esse resultado não comprova o piso de hardware.

## Como o dono testa

Todos os comandos rodam num terminal aberto na pasta do projeto.

**1. Abra a demonstração.** O comando abre o servidor de teste e duas janelas do jogo:

```bash
py tools/dev.py demo
```

**2. Teste o movimento.**

1. Clique em vários pontos do chão numa das janelas. O personagem deve andar até lá de forma suave.
2. Olhe a outra janela. O mesmo personagem deve aparecer andando até o mesmo lugar.
3. Clique num bloco de pedra. O personagem não deve sair do lugar.
4. Clique do outro lado de um grupo de pedras. O personagem deve dar a volta.

**3. Veja o servidor corrigindo.** Clique num ponto além da faixa de trilha proibida, que é indicada no relatório da fatia. O personagem começa a andar, é corrigido e desliza de volta, sem saltar.

**4. Teste a queda.**

1. Feche uma das janelas. O personagem dela deve sumir da outra janela.
2. Feche o servidor de teste. O título da janela restante deve avisar que a conexão caiu, e a janela deve continuar respondendo.

**5. Gere e confira a prova visual:**

```bash
py tools/dev.py visual
```

O comando mostra os números, a fluidez e o caminho da imagem. Abra a imagem e veja se ela agrada. O relatório da fatia traz os comandos para você aprovar essa imagem como referência.

**6. Conte o que achou.** Diga o que funcionou, o que ficou estranho e se a imagem agradou.
