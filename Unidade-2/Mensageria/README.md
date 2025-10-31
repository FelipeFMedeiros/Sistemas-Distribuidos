# Sistema de Mensageria com Google Cloud Pub/Sub

Este projeto implementa um sistema de mensageria completo usando Google Cloud Pub/Sub com Node.js e TypeScript.

## 🚀 Configuração do Ambiente

### 1. Instalar Dependências

```powershell
npm install
```

### 2. Configurar Credenciais do Google Cloud

#### Opção A: Usando Service Account (Recomendado para produção)

1. Acesse o [Google Cloud Console](https://console.cloud.google.com)
2. Vá para **IAM & Admin > Service Accounts**
3. Crie uma nova Service Account ou selecione uma existente
4. Adicione a role **Pub/Sub Editor** ou **Pub/Sub Admin**
5. Crie uma chave JSON e baixe o arquivo
6. Salve o arquivo como `service-account-key.json` na raiz do projeto
7. No arquivo `index.ts`, descomente a linha:
   ```typescript
   keyFilename: './service-account-key.json',
   ```

#### Opção B: Usando Application Default Credentials (ADC)

```powershell
gcloud auth application-default login
```

### 3. Verificar Configurações

Seu projeto já está configurado com:
- **Project ID**: `ultra-tendril-443317-b1`
- **Tópico**: `sistemas-distribuidos`
- **Assinaturas**:
  - `mysub`
  - `sistemas-distribuidos-sub`

## 📚 Estrutura do Projeto

```
mesageria/
├── index.ts           # Implementação principal (Publisher, Subscriber, Manager)
├── publisher.ts       # Exemplo de uso do Publisher
├── subscriber.ts      # Exemplo de uso do Subscriber
├── package.json       # Dependências e scripts
├── tsconfig.json      # Configuração do TypeScript
└── README.md          # Esta documentação
```

## 🎯 Como Usar

### Executar Demonstração Completa

```powershell
npm start
```

### Executar apenas o Publisher (Produtor)

```powershell
npm run publisher
```

### Executar apenas o Subscriber (Consumidor)

Em outro terminal:

```powershell
npm run subscriber
```

## 📖 Exemplos de Código

### Publisher - Publicar Mensagens

```typescript
import { Publisher } from './index';

const publisher = new Publisher('sistemas-distribuidos');

// Publicar mensagem simples
await publisher.publishMessage({
  tipo: 'notificacao',
  mensagem: 'Olá, mundo!',
  timestamp: new Date().toISOString()
});

// Publicar lote de mensagens
await publisher.publishBatch([
  { tipo: 'evento', acao: 'login', usuario: 'user1' },
  { tipo: 'evento', acao: 'logout', usuario: 'user2' }
]);
```

### Subscriber - Consumir Mensagens

```typescript
import { Subscriber } from './index';
import { Message } from '@google-cloud/pubsub';

const subscriber = new Subscriber('mysub');

// Handler personalizado
const messageHandler = (message: Message) => {
  const data = JSON.parse(message.data.toString());
  console.log('Mensagem recebida:', data);
  message.ack(); // Confirma o recebimento
};

// Inicia a escuta
subscriber.startListening(messageHandler);
```

### Gerenciador Completo

```typescript
import { PubSubManager } from './index';

const manager = new PubSubManager();

// Verificar recursos
await manager.checkTopic();
await manager.listSubscriptions();

// Obter publisher e publicar
const publisher = manager.getPublisher();
await publisher.publishMessage({ mensagem: 'teste' });

// Adicionar subscriber
const subscriber = manager.addSubscriber('mysub');
subscriber.startListening();
```

## 🔧 Scripts Disponíveis

| Comando | Descrição |
|---------|-----------|
| `npm start` | Executa a demonstração completa |
| `npm run publisher` | Executa apenas o publisher |
| `npm run subscriber` | Executa apenas o subscriber |
| `npm run dev` | Executa com hot-reload (nodemon) |

## 📊 Conceitos do Pub/Sub

### Publisher (Produtor)
- Publica mensagens em um **tópico**
- Não precisa saber quem vai receber as mensagens
- Pode enviar mensagens com atributos customizados

### Subscriber (Consumidor)
- Cria uma **assinatura** para um tópico
- Recebe mensagens do tópico
- Deve confirmar (ACK) ou rejeitar (NACK) cada mensagem

### Modos de Recebimento

1. **Push**: Google Cloud envia mensagens para um endpoint HTTP
2. **Pull**: Aplicação busca mensagens ativamente (usado neste projeto)

### ACK vs NACK

- **ACK (Acknowledge)**: Confirma que a mensagem foi processada com sucesso
- **NACK (Not Acknowledge)**: Indica falha no processamento, mensagem será reenviada

## 🔐 Segurança

⚠️ **IMPORTANTE**: Nunca commite arquivos de credenciais!

O arquivo `.gitignore` já está configurado para ignorar:
- `*.json` (incluindo service account keys)
- `.env` (variáveis de ambiente)
- `node_modules/`

## 📝 Estrutura de Mensagens

### Exemplo de Mensagem

```json
{
  "tipo": "notificacao",
  "mensagem": "Sistema iniciado",
  "timestamp": "2025-10-30T12:00:00.000Z",
  "dados_adicionais": {
    "usuario": "Felipe",
    "acao": "login"
  }
}
```

### Atributos da Mensagem

```typescript
{
  data: Buffer,           // Dados da mensagem (JSON stringified)
  attributes: {           // Metadados
    timestamp: string,
    origin: string,
    // ... outros atributos customizados
  }
}
```

## 🐛 Troubleshooting

### Erro: "Cannot find module '@google-cloud/pubsub'"
```powershell
npm install
```

### Erro de Autenticação
1. Verifique se o arquivo `service-account-key.json` existe
2. Ou configure o ADC: `gcloud auth application-default login`

### Mensagens não estão sendo recebidas
1. Verifique se o tópico existe no Console
2. Verifique se as assinaturas estão configuradas
3. Verifique se há mensagens na fila (Console > Pub/Sub > Assinaturas)

## 📚 Recursos Adicionais

- [Documentação Google Cloud Pub/Sub](https://cloud.google.com/pubsub/docs)
- [Node.js Client Library](https://googleapis.dev/nodejs/pubsub/latest/)
- [Guia de Autenticação](https://cloud.google.com/docs/authentication)

## 👥 Autor

Felipe Medeiros - Sistemas Distribuídos

## 📄 Licença

ISC
