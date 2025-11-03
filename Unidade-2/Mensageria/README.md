# 🚀 Guia Rápido de Início

## ⚡ Início Rápido (3 passos)

### 1️⃣ Configurar Credenciais

1. Baixe a chave JSON do Google Cloud Console
2. Salve como `service-account-key.json` nesta pasta
3. Descomente a linha 13 em `index.ts`:
   ```typescript
   keyFilename: './service-account-key.json',
   ```

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

## 🎯 Configuração do arquivo .env
```bash
# Google Cloud Project Configuration
GOOGLE_CLOUD_PROJECT_ID=ultra-tendril-405620
TOPIC_NAME=sistemas-distribuidos
SUBSCRIPTION_NAME_1=mysub-1
SUBSCRIPTION_NAME_2=mysub-2

# Caminho para o arquivo de credenciais do Google Cloud
# Baixe do Console: IAM & Admin > Service Accounts > Create Key (JSON)
GOOGLE_APPLICATION_CREDENTIALS=./service-account-key.json
```


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

