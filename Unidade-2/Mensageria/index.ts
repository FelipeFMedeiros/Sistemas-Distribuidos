import { PubSub, Message, Topic, Subscription } from '@google-cloud/pubsub';
import dotenv from 'dotenv';

// Carrega as variáveis de ambiente do arquivo .env
dotenv.config();

/*
  Função para validar e obter variáveis de ambiente obrigatórias
*/
function getEnvVariable(key: string): string {
    const value = process.env[key];
    if (!value) {
        throw new Error(
            `❌ Variável de ambiente obrigatória não encontrada: ${key}\n` +
                `   Verifique se o arquivo .env está configurado corretamente.`,
        );
    }
    return value;
}

// Configuração do cliente Pub/Sub
const projectId = getEnvVariable('GOOGLE_CLOUD_PROJECT_ID');
const topicName = getEnvVariable('TOPIC_NAME');
const subscriptionName1 = getEnvVariable('SUBSCRIPTION_NAME_1');
const subscriptionName2 = getEnvVariable('SUBSCRIPTION_NAME_2');
const keyFilename = getEnvVariable('GOOGLE_APPLICATION_CREDENTIALS');

// Inicializa o cliente Pub/Sub
const pubSubClient = new PubSub({
    projectId: projectId,
    keyFilename: keyFilename,
});

/*
  Classe para gerenciar operações de Publisher (Produtor)
*/
class Publisher {
    private topic: Topic;

    constructor(topicName: string) {
        this.topic = pubSubClient.topic(topicName);
    }

    /*
     Publica uma mensagem no tópico
    */
    async publishMessage(data: any): Promise<string> {
        try {
            const dataBuffer = Buffer.from(JSON.stringify(data));

            // Adiciona atributos à mensagem
            const messageId = await this.topic.publishMessage({
                data: dataBuffer,
                attributes: {
                    timestamp: new Date().toISOString(),
                    origin: 'node-publisher',
                },
            });

            console.log(`✅ Mensagem publicada com ID: ${messageId}`);
            return messageId;
        } catch (error) {
            console.error('❌ Erro ao publicar mensagem:', error);
            throw error;
        }
    }

    /*
     Publica múltiplas mensagens em lote
    */
    async publishBatch(messages: any[]): Promise<string[]> {
        try {
            const messageIds: string[] = [];

            for (const message of messages) {
                const messageId = await this.publishMessage(message);
                messageIds.push(messageId);
            }

            console.log(`✅ ${messageIds.length} mensagens publicadas com sucesso!`);
            return messageIds;
        } catch (error) {
            console.error('❌ Erro ao publicar lote de mensagens:', error);
            throw error;
        }
    }
}

/*
 Classe para gerenciar operações de Subscriber (Consumidor)
*/
class Subscriber {
    private subscription: Subscription;
    private subscriptionName: string;

    constructor(subscriptionName: string) {
        this.subscriptionName = subscriptionName;
        this.subscription = pubSubClient.subscription(subscriptionName);
    }

    /*
     Inicia a escuta de mensagens (Pull Mode)
    */
    startListening(messageHandler?: (message: Message) => void): void {
        console.log(`🎧 Aguardando mensagens na assinatura: ${this.subscriptionName}...`);

        // Handler padrão ou customizado
        const handleMessage = messageHandler || this.defaultMessageHandler.bind(this);

        // Event listener para mensagens
        this.subscription.on('message', handleMessage);

        // Event listener para erros
        this.subscription.on('error', (error) => {
            console.error(`❌ Erro na assinatura ${this.subscriptionName}:`, error);
        });
    }

    /*
     Handler padrão para processar mensagens
    */
    private defaultMessageHandler(message: Message): void {
        try {
            console.log(`\n📨 Nova mensagem recebida na assinatura: ${this.subscriptionName}`);
            console.log(`   ID da mensagem: ${message.id}`);
            console.log(`   Timestamp de publicação: ${message.publishTime}`);

            // Decodifica os dados
            const data = JSON.parse(message.data.toString());
            console.log(`   Dados:`, data);

            // Atributos da mensagem
            if (message.attributes) {
                console.log(`   Atributos:`, message.attributes);
            }

            // Acknowledge (confirma o recebimento da mensagem)
            message.ack();
            console.log(`   ✅ Mensagem processada e confirmada (ACK)`);
        } catch (error) {
            console.error(`   ❌ Erro ao processar mensagem:`, error);
            // Nack (não confirma - mensagem será reenviada)
            message.nack();
        }
    }

    /*
     Puxa mensagens manualmente (Pull síncrono)
    */
    async pullMessages(maxMessages: number = 10): Promise<void> {
        try {
            console.log(`📥 Buscando até ${maxMessages} mensagens...`);

            // Nota: A API do Pub/Sub não tem método 'pull' direto na Subscription
            // Em vez disso, usamos o modo de escuta ou buscamos do servidor
            console.log('⚠️  Método pull não está disponível. Use startListening() para receber mensagens.');
            console.log('💡 As mensagens são entregues automaticamente via push/pull do servidor.');
        } catch (error) {
            console.error('❌ Erro ao puxar mensagens:', error);
            throw error;
        }
    }

    /*
     Para de escutar mensagens
    */
    stopListening(): void {
        this.subscription.removeAllListeners();
        console.log(`🛑 Subscriber ${this.subscriptionName} parado`);
    }
}

/*
 Classe principal para gerenciar o sistema de mensageria
*/
class PubSubManager {
    private publisher: Publisher;
    private subscribers: Map<string, Subscriber>;

    constructor() {
        this.publisher = new Publisher(topicName);
        this.subscribers = new Map();
    }

    /*
     Adiciona um subscriber
    */
    addSubscriber(name: string): Subscriber {
        const subscriber = new Subscriber(name);
        this.subscribers.set(name, subscriber);
        return subscriber;
    }

    /*
     Obtém o publisher
    */
    getPublisher(): Publisher {
        return this.publisher;
    }

    /*
     Obtém um subscriber específico
    */
    getSubscriber(name: string): Subscriber | undefined {
        return this.subscribers.get(name);
    }

    /*
     Verifica se o tópico existe
    */
    async checkTopic(): Promise<boolean> {
        try {
            const [exists] = await pubSubClient.topic(topicName).exists();
            console.log(`Tópico '${topicName}': ${exists ? '✅ Existe' : '❌ Não existe'}`);
            return exists;
        } catch (error) {
            console.error('Erro ao verificar tópico:', error);
            return false;
        }
    }

    /*
     Verifica se uma assinatura existe
    */
    async checkSubscription(subscriptionName: string): Promise<boolean> {
        try {
            const [exists] = await pubSubClient.subscription(subscriptionName).exists();
            console.log(`Assinatura '${subscriptionName}': ${exists ? '✅ Existe' : '❌ Não existe'}`);
            return exists;
        } catch (error) {
            console.error('Erro ao verificar assinatura:', error);
            return false;
        }
    }

    /*
     Lista todos os tópicos do projeto
    */
    async listTopics(): Promise<void> {
        try {
            const [topics] = await pubSubClient.getTopics();
            console.log('\n📋 Tópicos disponíveis:');
            topics.forEach((topic) => console.log(`   - ${topic.name}`));
        } catch (error) {
            console.error('Erro ao listar tópicos:', error);
        }
    }

    /*
     Lista todas as assinaturas do tópico
    */
    async listSubscriptions(): Promise<void> {
        try {
            const [subscriptions] = await pubSubClient.topic(topicName).getSubscriptions();
            console.log(`\n📋 Assinaturas do tópico '${topicName}':`);
            subscriptions.forEach((sub) => console.log(`   - ${sub.name}`));
        } catch (error) {
            console.error('Erro ao listar assinaturas:', error);
        }
    }
}

/*
 Função de demonstração
*/
async function demo() {
    console.log('🚀 Iniciando demonstração do Google Cloud Pub/Sub\n');
    console.log('='.repeat(60));

    const manager = new PubSubManager();

    // Verifica o ambiente
    console.log('\n📊 Verificando configuração...');
    await manager.checkTopic();
    await manager.checkSubscription(subscriptionName1);
    await manager.checkSubscription(subscriptionName2);

    // Lista recursos
    await manager.listTopics();
    await manager.listSubscriptions();

    console.log('\n' + '='.repeat(60));
    console.log('\n📤 PUBLICANDO MENSAGENS\n');

    // Publica mensagens de exemplo
    const publisher = manager.getPublisher();

    await publisher.publishMessage({
        tipo: 'evento',
        mensagem: 'Sistema iniciado',
        timestamp: new Date().toISOString(),
    });

    await publisher.publishMessage({
        tipo: 'dados',
        usuario: 'Felipe',
        acao: 'login',
        timestamp: new Date().toISOString(),
    });

    // Publica lote de mensagens
    await publisher.publishBatch([
        { tipo: 'teste', numero: 1 },
        { tipo: 'teste', numero: 2 },
        { tipo: 'teste', numero: 3 },
    ]);

    console.log('\n' + '='.repeat(60));
    console.log('\n📥 CONSUMINDO MENSAGENS\n');

    // Configura subscribers
    const subscriber1 = manager.addSubscriber(subscriptionName1);
    const subscriber2 = manager.addSubscriber(subscriptionName2);

    // Subscriber 1 - Modo de escuta contínua
    subscriber1.startListening((message) => {
        console.log(`\n[SUBSCRIBER 1] Mensagem recebida:`);
        console.log(`   ID: ${message.id}`);
        console.log(`   Dados:`, JSON.parse(message.data.toString()));
        message.ack();
    });

    // Subscriber 2 - Modo de escuta contínua com handler padrão
    subscriber2.startListening();

    // Aguarda 10 segundos para processar mensagens
    console.log('\n⏳ Aguardando mensagens por 10 segundos...\n');

    await new Promise((resolve) => setTimeout(resolve, 10000));

    // Para os subscribers
    subscriber1.stopListening();
    subscriber2.stopListening();

    console.log('\n' + '='.repeat(60));
    console.log('\n✅ Demonstração concluída!');
    console.log('\nPara usar novamente:');
    console.log('  - Publisher: const publisher = new Publisher("sistemas-distribuidos")');
    console.log('  - Subscriber: const subscriber = new Subscriber("mysub")');
}

// Exporta as classes e funções
export { PubSubManager, Publisher, Subscriber, demo };

// Executa a demonstração se for o arquivo principal
if (require.main === module) {
    demo()
        .catch(console.error)
        .finally(() => {
            console.log('\n👋 Encerrando aplicação...');
            setTimeout(() => process.exit(0), 1000);
        });
}
