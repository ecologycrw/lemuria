# Roadmap

> Muda só com aprovação do dono do produto. A retrospectiva decide a próxima fatia; ela passa de "Próximas" para "Agora" quando o dono aprova a spec dela, no `/planejar`.

## Concluídas

*(nenhuma ainda)*

## Agora

**Preparação.** Documentos, esqueleto do código com as travas automáticas, ambiente de IA e GitHub. Ver o [STATUS.md](STATUS.md).

## Próximas

| Fatia | Nome | O que entrega, do ponto de vista de quem testa |
|---|---|---|
| 0 | Primeiros passos | Uma janela com chão em blocos, câmera 3/4 e clicar para andar, ligada ao servidor de teste; dois clientes se veem no mesmo mapa |
| 1 | Personagem vivo | Um personagem de verdade, importado do glTF, com animação suave em qualquer taxa de quadros |
| 2 | Equipamento | Peças trocáveis (elmo, armadura, armas), asas e brilho de item por nível. Aqui se decide de onde vem a arte |
| 3 | Interface | Janelas com RmlUi, inventário em grade e arrastar itens |
| 4 | Caçar | Monstros, combate, números de dano, drops no chão e pegar o item |
| 5 | Prova do OpenMU | Um adaptador de prova, num repositório privado separado: login, entrar no mapa, andar, ver outros jogadores, atacar e pegar um item. Corrige no Lemuria o que a prova revelar |

## Depois

- **Efeitos:** partículas na GPU, luzes dinâmicas, halo, distorção e sombras suaves.
- **Escala:** o cenário de cerco com 200 e 300 personagens, qualidade reduzida para o que está longe e a confirmação do piso de hardware.
- **Social:** chat, party e guild na interface.
- **Som:** música e efeitos sonoros.
- **Ferramentas:** editor de mapas, editores de dados e editor de efeitos.
- **Distribuição:** empacotamento do jogo, cifra plugável, patcher e patches que baixam só a diferença.
- **Plataformas:** Linux e mobile.

Quando uma fatia depende de algo que ainda não existe, uma fatia de fundação técnica entra antes dela.
