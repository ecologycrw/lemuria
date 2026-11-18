# Itens provisórios

Tudo o que foi simplificado de propósito, ou que ainda precisa de uma decisão ou de uma prova. Todo item tem um destino, e a checagem automática recusa um item sem destino. Quando o item é resolvido, ele sai desta tabela e a resolução fica registrada no report da fatia.

| ID | Item provisório | Por que é provisório | Destino | Origem |
|---|---|---|---|---|
| TD-0001 | A demonstração usa formas geradas por código, sem arte de verdade | as primeiras fatias não precisam de arte; a origem dos modelos (pacotes gratuitos com licença livre ou artista contratado) é decidida antes de o primeiro personagem de verdade entrar | fatia 1 | preparação |
| TD-0002 | O piso de hardware é uma proposta, ainda não comprovada | o computador de desenvolvimento não tem GPU integrada e é mais forte que o recomendado; a confirmação exige uma máquina do piso | fatia de escala | preparação |
| TD-0003 | O encaixe do adaptador de protocolo não foi provado com um servidor externo | o servidor de teste é escrito por nós e sempre se encaixa; só um adaptador de verdade revela o que a documentação não mostra | fatia 6 | preparação |
| TD-0004 | Os testes de mutação só informam e não bloqueiam o PR | a ferramenta precisa se mostrar estável na CI antes de virar obrigatória; se não ficar, o plano B é um piso de cobertura de testes | fatia 1 | preparação, parte 2 |
| TD-0005 | O teste da GPU por software roda só no Linux | o Windows da CI não tem placa de vídeo, e o renderizador por software dele ainda precisa ser avaliado com a SDL GPU | fatia 0 | preparação, parte 2 |
| TD-0006 | No Linux, o SDL é compilado sem suporte a janelas de verdade (X11 e Wayland) | hoje o Linux só compila e roda os testes; o suporte entra quando o cliente for levado para o Linux | fatia de plataformas | preparação, parte 2 |
| TD-0007 | A prova visual e o medidor de fluidez ainda são regras escritas, sem um comando que os execute | os dois dependem de a engine já desenhar alguma coisa | fatia 0 | pacote visual |
