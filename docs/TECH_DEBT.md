# Itens provisórios

Tudo o que foi simplificado de propósito, ou que ainda precisa de uma decisão ou de uma prova. Todo item tem um destino, e a checagem automática recusa um item sem destino. Quando o item é resolvido, ele sai desta tabela e a resolução fica registrada no report da fatia.

| ID | Item provisório | Por que é provisório | Destino | Origem |
|---|---|---|---|---|
| TD-0001 | A demonstração usa formas geradas por código, sem arte de verdade | as primeiras fatias não precisam de arte; a origem dos modelos (pacotes gratuitos com licença livre ou artista contratado) é decidida antes de o primeiro personagem de verdade entrar | fatia 1 | preparação |
| TD-0002 | O piso de hardware é uma proposta, ainda não comprovada | o computador de desenvolvimento não tem GPU integrada e é mais forte que o recomendado; a confirmação exige uma máquina do piso | fatia de escala | preparação |
| TD-0003 | O encaixe do adaptador de protocolo não foi provado com um servidor externo | o servidor de teste é escrito por nós e sempre se encaixa; só um adaptador de verdade revela o que a documentação não mostra | fatia 6 | preparação |
| TD-0004 | Os testes de mutação só informam e não bloqueiam o PR | a ferramenta precisa se mostrar estável na CI antes de virar obrigatória; se não ficar, o plano B é um piso de cobertura de testes | fatia 1 | preparação, parte 2 |
| TD-0006 | No Linux, o SDL é compilado sem suporte a janelas de verdade (X11 e Wayland) | hoje o Linux só compila e roda os testes; o suporte entra quando o cliente for levado para o Linux | fatia de plataformas | preparação, parte 2 |
| TD-0008 | O teste que conta as alocações de memória do quadro (INV-13) ainda não existe | o quadro só nasce com a primeira janela da engine | fatia 0 | auditoria do ambiente |
| TD-0009 | O código de desenho fala direto com a SDL GPU, sem separar o que desenhar de como falar com a placa de vídeo | com uma única forma de desenhar, a separação ainda não teria o que organizar; ela é obrigatória antes da segunda forma de desenhar ([ADR 0020](adr/0020-fronteira-do-desenho.md)) | fatia 2 | fatia 0 |
| TD-0010 | A medição da imagem mostra a parte em pontos de luz, mas ainda não exige a faixa dela | as luzes só chegam com o clima; até lá, os únicos pontos claros da imagem são os personagens | fatia 2 | fatia 0 |
| TD-0011 | A fluidez é medida na build de desenvolvimento, sem otimização | ainda não existe uma configuração de release; o jogo distribuído tende a ser mais rápido do que esta medição | fatia de escala | fatia 0 |
| TD-0012 | Cada busca de caminho limpa a memória do mapa inteiro, e uma busca longa num mapa de 1.024 por 1.024 blocos pode levar um quadro inteiro na build de desenvolvimento (16,1 ms em média e 20,2 ms no pior caso) | o mapa da demonstração tem 64 por 64 blocos, e a busca leva 0,68 ms; otimizar antes de existir um mapa grande seria trabalho sem medição real | fatia de escala | fatia 0 |
| TD-0013 | O servidor de teste não limita os bytes que esperam para sair para um cliente que envia pedidos e não lê as respostas | ele só aceita conexões do próprio computador e serve só aos testes e à demonstração; provar o limite exigiria encher o buffer do sistema operacional, o que deixaria o teste instável | fatia de escala | fatia 0 |
