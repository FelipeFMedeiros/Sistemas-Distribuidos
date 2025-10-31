import { Subscriber } from './index';
import { Message } from '@google-cloud/pubsub';

/**
 * Exemplo de uso do Subscriber (Consumidor de mensagens)
 */
async function runSubscriber() {
  console.log('🚀 Iniciando Subscriber...\n');

  // Escolha qual assinatura usar:
  // - 'mysub'
  // - 'sistemas-distribuidos-sub'
  const subscriptionName = 'mysub';
  
  const subscriber = new Subscriber(subscriptionName);

  console.log(`📡 Conectado à assinatura: ${subscriptionName}`);
  console.log('Aguardando mensagens... (Pressione Ctrl+C para sair)\n');

  // Handler personalizado para processar mensagens
  const messageHandler = (message: Message) => {
    try {
      console.log('\n' + '='.repeat(60));
      console.log('📨 NOVA MENSAGEM RECEBIDA');
      console.log('='.repeat(60));
      
      // Informações da mensagem
      console.log(`📋 ID da Mensagem: ${message.id}`);
      console.log(`⏰ Publicado em: ${message.publishTime}`);
      console.log(`📊 Tamanho: ${message.length} bytes`);
      
      // Decodifica os dados
      const data = JSON.parse(message.data.toString());
      console.log(`📦 Dados da mensagem:`);
      console.log(JSON.stringify(data, null, 2));
      
      // Atributos (se houver)
      if (message.attributes && Object.keys(message.attributes).length > 0) {
        console.log(`🏷️  Atributos:`);
        console.log(JSON.stringify(message.attributes, null, 2));
      }

      // Processa a mensagem baseado no tipo
      if (data.tipo) {
        console.log(`\n🔍 Processando mensagem do tipo: ${data.tipo}`);
        
        switch (data.tipo) {
          case 'notificacao':
            console.log(`   💬 Notificação: ${data.mensagem}`);
            break;
          case 'usuario':
            console.log(`   👤 Ação de usuário: ${data.acao}`);
            break;
          case 'evento':
            console.log(`   📅 Evento: ${data.acao}`);
            break;
          case 'log':
            console.log(`   📝 Log [${data.level}]: ${data.mensagem}`);
            break;
          default:
            console.log(`   ℹ️  Tipo desconhecido`);
        }
      }

      // Confirma o recebimento (ACK)
      message.ack();
      console.log('\n✅ Mensagem processada e confirmada (ACK)');
      console.log('='.repeat(60));

    } catch (error) {
      console.error('\n❌ Erro ao processar mensagem:', error);
      console.log('🔄 Mensagem será reenviada (NACK)');
      // Não confirma a mensagem - ela será reenviada
      message.nack();
    }
  };

  // Inicia a escuta de mensagens
  subscriber.startListening(messageHandler);

  // Tratamento de sinais para encerramento gracioso
  process.on('SIGINT', () => {
    console.log('\n\n🛑 Encerrando subscriber...');
    subscriber.stopListening();
    console.log('👋 Subscriber finalizado!');
    process.exit(0);
  });
}

// Executa o subscriber
runSubscriber().catch((error) => {
  console.error('❌ Erro fatal no subscriber:', error);
  process.exit(1);
});
