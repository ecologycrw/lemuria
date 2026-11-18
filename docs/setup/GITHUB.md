# Configuração do GitHub

Passo a passo para colocar o Lemuria no GitHub com as proteções do projeto. Os passos marcados **(dono)** são feitos pelo dono do produto; os marcados **(IA)**, pela IA.

- Repositório: `ecologycrw/lemuria` (público).
- Conta do dono: `ecologycrw`, usada **só pelo navegador**.
- Conta da IA: `lemuria-ci-dev` (nome sugerido), usada pelo `gh` e pelo Git neste PC.

O guia tem duas partes. A **Parte A** vem antes do primeiro commit. A **Parte B** vem no fim da preparação, quando o repositório já estiver pronto para ser publicado.

Se você escolher outro nome para a conta da IA, troque `lemuria-ci-dev` nos comandos.

---

# Parte A: antes do primeiro commit

> **Faça a Parte A inteira antes do primeiro commit.** A regra da `main` exige commits assinados, e um commit feito antes da chave não pode ser assinado depois sem reescrever o histórico, o que é proibido. No Elysium isso aconteceu e bloqueou o primeiro PR.

## 1. Criar a conta da IA (dono, navegador)

1. Abra uma **janela anônima** e entre em github.com/signup.
2. Crie a conta `lemuria-ci-dev` com um e-mail que você controle. Ele precisa ser diferente do e-mail da sua conta pessoal.
3. Confirme o e-mail.
4. Ative a verificação em duas etapas em **Settings → Password and authentication**.
5. Deixe essa janela anônima aberta: você vai usá-la no passo 2.

A IA usa uma conta separada para não ter os seus poderes. Ela envia código e abre PRs, mas não consegue aprovar nem fazer merge.

## 2. Trocar o login deste PC para a conta da IA (dono, terminal)

Saia da conta antiga da IA:

```bash
gh auth logout --hostname github.com --user elysium-ci-dev
```

Entre na conta nova:

```bash
gh auth login --hostname github.com --git-protocol https --web --scopes admin:ssh_signing_key
```

O comando mostra um código. Abra o link **na janela anônima logada como `lemuria-ci-dev`**, cole o código e autorize. Depois:

```bash
gh auth setup-git
```

```bash
gh auth status
```

O `gh auth status` deve mostrar **só** `lemuria-ci-dev`.

Por último, abra o **Gerenciador de Credenciais do Windows → Credenciais do Windows** e remova as entradas `git:https://github.com` que existirem.

## 3. Criar a chave de assinatura dos commits (dono, terminal)

Crie a chave. Quando ela pedir uma senha (*passphrase*), aperte **Enter** duas vezes, sem digitar nada:

```bash
ssh-keygen -t ed25519 -C "lemuria-ci-dev signing" -f "$HOME/.lemuria_signing_key"
```

Cadastre a chave pública na conta da IA:

```bash
gh ssh-key add "$HOME/.lemuria_signing_key.pub" --type signing --title "Lemuria signing"
```

A chave privada (`.lemuria_signing_key`, sem `.pub`) nunca sai do seu PC e nunca entra no repositório.

## 4. Configurar o Git deste repositório (dono, terminal)

Rode **dentro da pasta do projeto**, um comando por vez. Estas configurações valem só para o Lemuria.

```bash
git config user.name "lemuria-ci-dev"
```

```bash
git config user.email "$(gh api user --jq '.id')+$(gh api user --jq '.login')@users.noreply.github.com"
```

```bash
git config gpg.format ssh
```

```bash
git config user.signingkey "$HOME/.lemuria_signing_key.pub"
```

```bash
git config commit.gpgsign true
```

```bash
git config core.hooksPath .githooks
```

## 5. Avisar a IA (dono)

Diga à IA que a Parte A terminou. Ela confere a configuração, faz o primeiro commit assinado na `main` e cria a branch `preparacao/ambiente`.

---

# Parte B: publicar o repositório

Só comece a Parte B quando a IA avisar que a preparação está pronta para ser publicada.

## 6. Criar o repositório (dono, navegador)

1. Em github.com, logado na sua conta pessoal: **+ → New repository**.
2. Nome `lemuria`, **Public**. **Não** marque README, `.gitignore` nem licença: o repositório precisa nascer vazio.
3. **Create repository**.

## 7. Convidar a conta da IA (dono e conta da IA, navegador)

1. No repositório: **Settings → Collaborators → Add people** → `lemuria-ci-dev` → papel **Write**.
2. Na janela anônima, logada como `lemuria-ci-dev`, aceite o convite (chega por e-mail ou em github.com/notifications).

## 8. Configurações do repositório (dono, navegador)

Em **Settings**:

| Onde | O que fazer |
|---|---|
| General → Pull Requests | Deixar marcado **só** "Allow squash merging". Marcar "Automatically delete head branches" |
| Actions → General → Workflow permissions | **Read repository contents and packages permissions** |
| Actions → General → Fork pull request workflows | **Require approval for all external contributors** |
| Advanced Security (ou Code security) | Ativar **Secret Protection** e **Push protection** |
| Advanced Security → Private vulnerability reporting | **Enable** |
| Moderation options → Interaction limits | **Limit to repository collaborators** (o máximo de tempo disponível) |

## 9. Enviar a main pela primeira vez (dono, terminal)

A IA adiciona o endereço do repositório (`git remote add origin ...`). Depois, você envia a `main`, uma única vez:

```bash
git push -u origin main
```

Depois disso, ninguém envia nada direto para a `main`: tudo entra por PR.

## 10. Proteger a main (dono, navegador)

**Settings → Rules → Rulesets → New ruleset → Import a ruleset** e escolha o arquivo [`docs/setup/ruleset-main.json`](ruleset-main.json) deste projeto. Confira e clique em **Create**.

A regra exige na `main`:

- PR com 1 aprovação, do code owner, de alguém que não fez o último envio;
- aprovações antigas descartadas a cada novo envio;
- CI verde em Windows e Linux, análise estática aprovada e diagramas válidos;
- só squash merge, histórico linear e commits assinados;
- nada de force push nem de apagar a branch;
- nenhuma exceção, nem para o dono.

## 11. PR #1 (IA)

A IA envia a branch `preparacao/ambiente`, abre o PR #1 e confere:

- a CI rodando nos dois sistemas;
- o PR bloqueado até a sua aprovação.

## 12. Revisar e fazer o merge (dono, navegador)

No PR: aba **Files changed → Review changes → Approve**. Depois, **Squash and merge**.

## Renovação

O login do `gh` da conta da IA não expira sozinho. Se algum dia ele sair, repita o passo 2, sem o comando de `logout`.
