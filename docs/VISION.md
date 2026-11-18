# Visão

> Documento vivo. Mudanças exigem aprovação do dono do produto.

## O que é

O Lemuria é uma engine open source (MIT), em C++20, para desenvolvedores criarem o **cliente de MMORPGs no estilo clássico**. A referência de gênero são os MMORPGs de ação do início dos anos 2000, como o Mu Online. Esses jogos têm em comum:

- mapas em grade e câmera fixa em 3/4;
- clicar para andar e combate por alvo;
- progressão por itens com nível e opções;
- personagens com equipamento visível e efeitos chamativos;
- interface pesada, com inventário em grade.

O Lemuria não tem afiliação com nenhum desses jogos ou empresas. Nenhum asset, arquivo, formato, protocolo ou código deles faz parte do repositório (ver [Conteúdo de terceiros](CONVENTIONS.md#conteúdo-de-terceiros)).

"Lemuria" é um nome provisório.

## Para quem

Para **desenvolvedores** que querem criar o cliente de um MMORPG desse estilo sem construir gráficos, interface, conteúdo e rede do zero. O **jogador** é quem joga o que esse desenvolvedor criar.

O desenvolvedor pode:

- usar o **Framework Classic**, que já traz as mecânicas de cliente do gênero;
- adaptar só as partes do Classic que quiser;
- **criar o próprio framework** diretamente sobre o Núcleo.

## Só o cliente

O Lemuria não inclui um servidor de jogo. Cada desenvolvedor escolhe o servidor e escreve um **adaptador de protocolo** para ligar a engine a ele. Um servidor open source como o OpenMU é um exemplo do que precisa ser possível ligar, mas esse adaptador pertence ao projeto do desenvolvedor, e não a este repositório ([ADR 0004](adr/0004-adaptador-de-protocolo.md)).

## Como a engine é construída

A engine é construída junto com um **jogo de demonstração** (Lemuria Demo, nome provisório) e um **servidor de teste mínimo**. Eles servem para:

- provar que a engine funciona;
- mostrar o que ela faz;
- servir de modelo inicial para quem for criar um jogo.

A demonstração é uma amostra para testes, não um jogo comercial. O servidor de teste existe só para os testes e para a demonstração, e não é um produto.

## Pilares

1. **O cliente mostra o que o servidor decidiu.** Ele pode antecipar o visual para parecer rápido, mas aceita a correção.
2. **Encaixes em vez de suposições.** Protocolo, formatos de conteúdo, cifra de arquivos e framework podem ser trocados pelo desenvolvedor.
3. **A engine define os mecanismos; o jogo define os números e a arte.** Conteúdo é dado.
4. **Feito para hardware modesto.** O público joga muito em PCs populares e GPUs integradas.
5. **Verificável.** Tudo o que uma máquina consegue checar é checado por máquina.

## Ambição gráfica: clássico melhorado

O estilo continua o dos MMORPGs do começo dos anos 2000, com o acabamento de hoje ([ADR 0008](adr/0008-classico-melhorado.md)). A ambição pode ser ajustada ao longo do desenvolvimento.

| Continua clássico | Melhora |
|---|---|
| 3D com câmera 3/4 e mapas em grade | Nitidez em qualquer resolução, com interface que se ajusta à tela |
| Modelos simples e texturas pintadas à mão | Bordas suavizadas |
| Brilho de item por nível, asas e efeitos chamativos | Animação suave em qualquer taxa de quadros |
| Rodar em GPU integrada | Luzes dinâmicas simples e sombra suave nos personagens |
| | Halo nas partes que brilham, distorção de calor e água, neblina natural |
| | Menos detalhe no que está longe e níveis de qualidade automáticos |

Cada melhoria é uma opção: quem quiser o visual antigo desliga, e quem quiser algo mais polido liga.

## Pilares do cliente

- Personagens modulares (elmo, armadura, calça, luvas, botas, armas, asas) em grande quantidade na tela.
- Brilho de item por nível e opções, definido por dados.
- Partículas e efeitos de skill com orçamento e redução de qualidade à distância.
- Interface pesada (inventário em grade, trade, lojas, chat, party, guild), com data binding e skins.
- Câmera em 3/4 por padrão; cada jogo pode liberar zoom e rotação.
- Windows primeiro, sem fechar as portas de Linux e mobile.

## Piso de hardware

> **Proposta, ainda não comprovada.** O computador de desenvolvimento atual é mais forte do que o piso, e os números medidos nele não provam estas metas ([TD-0002](TECH_DEBT.md)).

| | Mínimo | Recomendado |
|---|---|---|
| Sistema | Windows 10 64 bits | Windows 10/11 64 bits |
| CPU | 4 threads (Core i3 de 6ª a 8ª geração, Ryzen 3 2200G) | 6 núcleos |
| RAM | 8 GB | 16 GB |
| GPU | Integrada: Intel UHD 620/630, Radeon Vega 8 | GTX 1050 Ti / 1650 ou equivalente |
| Disco | HD comum | SSD |
| Resolução | 1366×768 | 1920×1080 |
| Meta | 60 FPS caçando; 30 FPS estáveis no cerco, em qualidade baixa | 60 FPS no cerco, em qualidade média |

A GPU precisa ter Direct3D 12 ou Vulkan.

A **cena de referência** é um cerco com personagens equipados, com asas e skills ao mesmo tempo. Ela será medida com **200 e com 300 personagens**, e o número final sai dessa medição.

## Não-objetivos

O que o Lemuria **não** faz, de propósito:

- **Não é um servidor.** As regras do jogo, a economia e a persistência pertencem ao servidor de cada desenvolvedor.
- **Não é uma engine genérica.** Servir para jogos de corrida, plataforma ou tiro não é objetivo.
- **Sem anti-cheat.** Numa engine open source, ele seria só um obstáculo. A proteção real é o servidor decidir tudo. A engine oferece apenas um encaixe para cada jogo cifrar os próprios arquivos.
- **Sem motor de física.** A colisão é a grade de tiles.
- **Sem console.**
- **Sem mundo aberto com streaming contínuo.** Mapas são separados e carregados no teleporte.
- **Sem ray tracing, iluminação global dinâmica ou geometria virtualizada.**
- **Sem suporte embutido** a clientes, arquivos ou protocolos de outros jogos. Quem precisar disso escreve o próprio adaptador e os próprios importadores.

## Ainda não decidido

- **Linguagem de script.** O jogo começa em C++ e dados. Uma linguagem de script pode entrar no futuro, por ADR, se surgir a necessidade ([ADR 0002](adr/0002-tres-camadas.md)).
