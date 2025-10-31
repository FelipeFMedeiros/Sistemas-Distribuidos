# 🚀 Guia Rápido de Início

## ⚡ Início Rápido (3 passos)

### 1️⃣ Configurar Credenciais

**Escolha UMA das opções:**

#### Opção A - Service Account (Recomendado)
1. Baixe a chave JSON do Google Cloud Console
2. Salve como `service-account-key.json` nesta pasta
3. Descomente a linha 13 em `index.ts`:
   ```typescript
   keyFilename: './service-account-key.json',
   ```

#### Opção B - Google Cloud SDK
```powershell
gcloud auth application-default login
```

📖 **Detalhes completos**: Veja `CREDENTIALS_SETUP.md`

---

### 2️⃣ Testar Tudo

```powershell
npm start
```

Você verá:
- ✅ Verificação do tópico e assinaturas
- ✅ Mensagens sendo publicadas
- ✅ Mensagens sendo recebidas
- ✅ Processamento completo

---

### 3️⃣ Usar no Seu Código

#### Terminal 1 - Publisher (Produtor)
```powershell
npm run publisher
```

#### Terminal 2 - Subscriber (Consumidor)
```powershell
npm run subscriber
```

---

## 📝 Exemplos de Código

### Publicar uma Mensagem

```typescript
import { Publisher } from './index';

const publisher = new Publisher('sistemas-distribuidos');

await publisher.publishMessage({
  tipo: 'notificacao',
  mensagem: 'Olá!',
  timestamp: new Date().toISOString()
});
```

### Receber Mensagens

```typescript
import { Subscriber } from './index';

const subscriber = new Subscriber('mysub');

subscriber.startListening((message) => {
  const data = JSON.parse(message.data.toString());
  console.log('Recebido:', data);
  message.ack(); // Confirma
});
```

---

## 🎯 Seu Setup Atual

✅ **Project ID**: `ultra-tendril-443317-b1`  
✅ **Tópico**: `sistemas-distribuidos`  
✅ **Assinaturas**:
   - `mysub`
   - `sistemas-distribuidos-sub`

---

## 🆘 Problemas?

### Erro de Autenticação
```powershell
# Opção 1: Verifique o arquivo JSON
dir service-account-key.json

# Opção 2: Use o gcloud
gcloud auth application-default login
```

### Ver Logs Detalhados
```powershell
# Modo desenvolvimento com reload automático
npm run dev
```

### Testar Conexão Manualmente
```powershell
# Listar tópicos do projeto
gcloud pubsub topics list --project=ultra-tendril-443317-b1

# Listar assinaturas
gcloud pubsub subscriptions list --project=ultra-tendril-443317-b1
```

---

## 📚 Próximos Passos

1. ✅ Configure as credenciais
2. ✅ Execute `npm start` para testar
3. ✅ Explore `publisher.ts` e `subscriber.ts`
4. ✅ Adapte para seu caso de uso
5. ✅ Leia `README.md` para mais detalhes

---

## 🎓 Recursos de Aprendizado

- **Google Cloud Console**: https://console.cloud.google.com
- **Pub/Sub Dashboard**: https://console.cloud.google.com/cloudpubsub
- **Documentação**: https://cloud.google.com/pubsub/docs

---

**Pronto para começar?** Execute `npm start` 🚀
