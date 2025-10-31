# 🔐 Configuração de Credenciais do Google Cloud

## Passo a Passo para Configurar a Autenticação

### Opção 1: Service Account Key (Recomendado)

#### 1. Acessar o Google Cloud Console
   - Vá para: https://console.cloud.google.com
   - Selecione o projeto: **ultra-tendril-443317-b1**

#### 2. Criar/Configurar Service Account
   1. No menu lateral, vá para: **IAM & Admin** → **Service Accounts**
   2. Clique em **+ CREATE SERVICE ACCOUNT** (ou use uma existente)
   3. Preencha:
      - **Service account name**: `pubsub-nodejs-app`
      - **Description**: `Service account para aplicação Node.js Pub/Sub`
   4. Clique em **CREATE AND CONTINUE**

#### 3. Adicionar Permissões
   1. Selecione a role: **Pub/Sub Editor** ou **Pub/Sub Admin**
   2. Clique em **CONTINUE**
   3. Clique em **DONE**

#### 4. Criar e Baixar a Chave JSON
   1. Clique na Service Account criada
   2. Vá para a aba **KEYS**
   3. Clique em **ADD KEY** → **Create new key**
   4. Selecione o tipo: **JSON**
   5. Clique em **CREATE**
   6. O arquivo JSON será baixado automaticamente

#### 5. Configurar no Projeto
   1. Renomeie o arquivo baixado para: `service-account-key.json`
   2. Mova o arquivo para a pasta do projeto:
      ```
      c:\Users\Felipe\Documents\Codes\Repositories\SistemasDistribuidos-U1\Unidade-2\mesageria\
      ```
   3. No arquivo `index.ts`, descomente a linha 13:
      ```typescript
      keyFilename: './service-account-key.json',
      ```

⚠️ **IMPORTANTE**: O arquivo `service-account-key.json` já está no `.gitignore` e não será commitado!

---

### Opção 2: Application Default Credentials (ADC)

Se você já tem o Google Cloud SDK instalado:

```powershell
# Fazer login
gcloud auth application-default login

# Configurar o projeto
gcloud config set project ultra-tendril-443317-b1
```

Com esta opção, você NÃO precisa do arquivo `service-account-key.json`.

---

## 🧪 Testar a Configuração

Após configurar as credenciais, teste executando:

```powershell
npm start
```

Você deverá ver:
- ✅ Conexão com o Google Cloud estabelecida
- ✅ Tópico e assinaturas verificados
- ✅ Mensagens sendo publicadas e recebidas

---

## 🐛 Troubleshooting

### Erro: "Could not load the default credentials"

**Solução 1**: Verifique se o arquivo `service-account-key.json` está na pasta correta
**Solução 2**: Execute `gcloud auth application-default login`

### Erro: "Permission denied"

A Service Account precisa das permissões corretas:
- **Pub/Sub Publisher** (para publicar mensagens)
- **Pub/Sub Subscriber** (para receber mensagens)
- **Pub/Sub Editor** (ambos)

### Erro: "Topic not found"

Verifique no Console se o tópico existe:
1. Vá para: **Pub/Sub** → **Topics**
2. Confirme que o tópico `sistemas-distribuidos` existe
3. Verifique o Project ID correto: `ultra-tendril-443317-b1`

---

## 📋 Verificação Manual

Você pode verificar suas configurações manualmente no Console:

### Verificar Tópicos
https://console.cloud.google.com/cloudpubsub/topic/list?project=ultra-tendril-443317-b1

### Verificar Assinaturas
https://console.cloud.google.com/cloudpubsub/subscription/list?project=ultra-tendril-443317-b1

### Verificar Service Accounts
https://console.cloud.google.com/iam-admin/serviceaccounts?project=ultra-tendril-443317-b1

---

## ✅ Checklist de Configuração

- [ ] Service Account criada
- [ ] Permissões configuradas (Pub/Sub Editor ou Admin)
- [ ] Chave JSON baixada e renomeada para `service-account-key.json`
- [ ] Arquivo movido para a pasta do projeto
- [ ] Linha no `index.ts` descomentada (se usando Service Account)
- [ ] Tópico `sistemas-distribuidos` existe no Console
- [ ] Assinaturas `mysub` e `sistemas-distribuidos-sub` existem
- [ ] Teste executado com sucesso: `npm start`

---

## 📚 Links Úteis

- [Autenticação Google Cloud](https://cloud.google.com/docs/authentication/getting-started)
- [Pub/Sub Quickstart](https://cloud.google.com/pubsub/docs/quickstart-client-libraries)
- [IAM Roles para Pub/Sub](https://cloud.google.com/pubsub/docs/access-control)
