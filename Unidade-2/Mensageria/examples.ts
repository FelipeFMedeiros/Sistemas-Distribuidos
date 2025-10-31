import { Publisher, Subscriber, PubSubManager } from './index';
import { Message } from '@google-cloud/pubsub';

/**
 * Exemplos práticos de uso do Google Cloud Pub/Sub
 */

// ============================================================================
// EXEMPLO 1: Sistema de Notificações
// ============================================================================

export async function exemploNotificacoes() {
  console.log('\n🔔 EXEMPLO 1: Sistema de Notificações\n');
  
  const publisher = new Publisher('sistemas-distribuidos');
  
  // Enviar notificação de boas-vindas
  await publisher.publishMessage({
    tipo: 'notificacao',
    categoria: 'bem-vindo',
    destinatario: 'felipe@example.com',
    titulo: 'Bem-vindo ao Sistema!',
    mensagem: 'Sua conta foi criada com sucesso.',
    timestamp: new Date().toISOString()
  });

  // Enviar notificação de alerta
  await publisher.publishMessage({
    tipo: 'notificacao',
    categoria: 'alerta',
    destinatario: 'admin@example.com',
    titulo: 'Alerta de Sistema',
    mensagem: 'Uso de CPU acima de 80%',
    severidade: 'alta',
    timestamp: new Date().toISOString()
  });

  console.log('✅ Notificações enviadas!\n');
}

// ============================================================================
// EXEMPLO 2: Processamento de Pedidos E-commerce
// ============================================================================

export async function exemploPedidos() {
  console.log('\n🛒 EXEMPLO 2: Processamento de Pedidos\n');
  
  const publisher = new Publisher('sistemas-distribuidos');
  
  // Pedido realizado
  await publisher.publishMessage({
    tipo: 'pedido',
    status: 'novo',
    pedidoId: 'PED-12345',
    cliente: {
      id: 'C001',
      nome: 'Felipe Medeiros',
      email: 'felipe@example.com'
    },
    itens: [
      { produto: 'Notebook', quantidade: 1, preco: 3500.00 },
      { produto: 'Mouse', quantidade: 2, preco: 50.00 }
    ],
    total: 3600.00,
    timestamp: new Date().toISOString()
  });

  // Pedido confirmado
  await publisher.publishMessage({
    tipo: 'pedido',
    status: 'confirmado',
    pedidoId: 'PED-12345',
    timestamp: new Date().toISOString()
  });

  console.log('✅ Pedidos processados!\n');
}

// ============================================================================
// EXEMPLO 3: Sistema de Logs Distribuído
// ============================================================================

export async function exemploLogs() {
  console.log('\n📝 EXEMPLO 3: Sistema de Logs\n');
  
  const publisher = new Publisher('sistemas-distribuidos');
  
  const logs = [
    { level: 'INFO', servico: 'api', mensagem: 'Servidor iniciado na porta 3000' },
    { level: 'DEBUG', servico: 'database', mensagem: 'Conexão estabelecida com sucesso' },
    { level: 'WARN', servico: 'cache', mensagem: 'Cache miss - buscando do banco' },
    { level: 'ERROR', servico: 'auth', mensagem: 'Tentativa de login falhou' },
    { level: 'INFO', servico: 'api', mensagem: 'Request GET /usuarios - 200 OK' }
  ];

  for (const log of logs) {
    await publisher.publishMessage({
      tipo: 'log',
      ...log,
      hostname: 'server-01',
      timestamp: new Date().toISOString()
    });
  }

  console.log('✅ Logs enviados!\n');
}

// ============================================================================
// EXEMPLO 4: Eventos de Usuário (Analytics)
// ============================================================================

export async function exemploAnalytics() {
  console.log('\n📊 EXEMPLO 4: Analytics de Usuário\n');
  
  const publisher = new Publisher('sistemas-distribuidos');
  
  // Usuário visualizou página
  await publisher.publishMessage({
    tipo: 'evento',
    categoria: 'pageview',
    usuario: 'user_123',
    pagina: '/produtos/notebook',
    sessao: 'sess_abc123',
    timestamp: new Date().toISOString()
  });

  // Usuário clicou em botão
  await publisher.publishMessage({
    tipo: 'evento',
    categoria: 'click',
    usuario: 'user_123',
    elemento: 'btn_comprar',
    pagina: '/produtos/notebook',
    timestamp: new Date().toISOString()
  });

  // Usuário adicionou ao carrinho
  await publisher.publishMessage({
    tipo: 'evento',
    categoria: 'add_to_cart',
    usuario: 'user_123',
    produto: 'notebook-dell-i7',
    valor: 3500.00,
    timestamp: new Date().toISOString()
  });

  console.log('✅ Eventos de analytics enviados!\n');
}

// ============================================================================
// EXEMPLO 5: Subscriber Especializado por Tipo de Mensagem
// ============================================================================

export class NotificationSubscriber {
  private subscriber: Subscriber;

  constructor(subscriptionName: string) {
    this.subscriber = new Subscriber(subscriptionName);
  }

  start(): void {
    this.subscriber.startListening((message: Message) => {
      const data = JSON.parse(message.data.toString());

      // Processa apenas notificações
      if (data.tipo === 'notificacao') {
        this.processarNotificacao(data);
        message.ack();
      } else {
        // Ignora outros tipos
        message.ack();
      }
    });
  }

  private processarNotificacao(notificacao: any): void {
    console.log('\n🔔 Nova Notificação:');
    console.log(`   Para: ${notificacao.destinatario}`);
    console.log(`   Título: ${notificacao.titulo}`);
    console.log(`   Mensagem: ${notificacao.mensagem}`);
    
    // Aqui você poderia:
    // - Enviar email
    // - Enviar SMS
    // - Push notification
    // - Salvar no banco de dados
  }

  stop(): void {
    this.subscriber.stopListening();
  }
}

// ============================================================================
// EXEMPLO 6: Subscriber de Pedidos
// ============================================================================

export class OrderSubscriber {
  private subscriber: Subscriber;

  constructor(subscriptionName: string) {
    this.subscriber = new Subscriber(subscriptionName);
  }

  start(): void {
    this.subscriber.startListening((message: Message) => {
      const data = JSON.parse(message.data.toString());

      if (data.tipo === 'pedido') {
        this.processarPedido(data);
        message.ack();
      } else {
        message.ack();
      }
    });
  }

  private processarPedido(pedido: any): void {
    console.log('\n🛒 Novo Pedido:');
    console.log(`   ID: ${pedido.pedidoId}`);
    console.log(`   Status: ${pedido.status}`);
    
    switch (pedido.status) {
      case 'novo':
        console.log(`   Cliente: ${pedido.cliente.nome}`);
        console.log(`   Total: R$ ${pedido.total.toFixed(2)}`);
        // Processar pagamento
        // Atualizar estoque
        break;
      
      case 'confirmado':
        // Enviar email de confirmação
        // Notificar logística
        break;
      
      case 'enviado':
        // Atualizar tracking
        break;
    }
  }

  stop(): void {
    this.subscriber.stopListening();
  }
}

// ============================================================================
// EXEMPLO 7: Processamento em Lote
// ============================================================================

export async function exemploLote() {
  console.log('\n📦 EXEMPLO 7: Processamento em Lote\n');
  
  const publisher = new Publisher('sistemas-distribuidos');
  
  // Simular processamento de 100 eventos
  const eventos = [];
  for (let i = 1; i <= 100; i++) {
    eventos.push({
      tipo: 'metricas',
      metrica: 'cpu_usage',
      valor: Math.random() * 100,
      servidor: `server-${Math.floor(i / 10) + 1}`,
      timestamp: new Date().toISOString()
    });
  }

  console.log(`Enviando ${eventos.length} eventos em lote...`);
  await publisher.publishBatch(eventos);
  console.log('✅ Lote processado!\n');
}

// ============================================================================
// EXEMPLO 8: Chat em Tempo Real
// ============================================================================

export async function exemploChat() {
  console.log('\n💬 EXEMPLO 8: Sistema de Chat\n');
  
  const publisher = new Publisher('sistemas-distribuidos');
  
  // Mensagens de chat
  const mensagens = [
    {
      tipo: 'chat',
      sala: 'sistemas-distribuidos',
      usuario: 'Felipe',
      mensagem: 'Olá, pessoal! 👋',
      timestamp: new Date().toISOString()
    },
    {
      tipo: 'chat',
      sala: 'sistemas-distribuidos',
      usuario: 'Maria',
      mensagem: 'Oi Felipe! Como vai?',
      timestamp: new Date(Date.now() + 1000).toISOString()
    },
    {
      tipo: 'chat',
      sala: 'sistemas-distribuidos',
      usuario: 'Felipe',
      mensagem: 'Tudo bem! Estou testando o Pub/Sub 🚀',
      timestamp: new Date(Date.now() + 2000).toISOString()
    }
  ];

  for (const msg of mensagens) {
    await publisher.publishMessage(msg);
    await new Promise(resolve => setTimeout(resolve, 100));
  }

  console.log('✅ Mensagens de chat enviadas!\n');
}

// ============================================================================
// EXEMPLO 9: Subscriber com Retry Manual
// ============================================================================

export class RobustSubscriber {
  private subscriber: Subscriber;
  private maxRetries = 3;

  constructor(subscriptionName: string) {
    this.subscriber = new Subscriber(subscriptionName);
  }

  start(): void {
    this.subscriber.startListening((message: Message) => {
      this.processarComRetry(message, 0);
    });
  }

  private processarComRetry(message: Message, tentativa: number): void {
    try {
      const data = JSON.parse(message.data.toString());
      
      // Simular processamento que pode falhar
      if (Math.random() > 0.7) {
        throw new Error('Erro simulado');
      }

      console.log(`✅ Mensagem processada na tentativa ${tentativa + 1}`);
      message.ack();
      
    } catch (error) {
      tentativa++;
      
      if (tentativa < this.maxRetries) {
        console.log(`⚠️  Erro na tentativa ${tentativa}. Tentando novamente...`);
        setTimeout(() => this.processarComRetry(message, tentativa), 1000);
      } else {
        console.log(`❌ Falha após ${this.maxRetries} tentativas. NACK.`);
        message.nack();
      }
    }
  }

  stop(): void {
    this.subscriber.stopListening();
  }
}

// ============================================================================
// FUNÇÃO PRINCIPAL - EXECUTAR EXEMPLOS
// ============================================================================

export async function executarExemplos() {
  console.log('🚀 EXEMPLOS PRÁTICOS DO GOOGLE CLOUD PUB/SUB');
  console.log('='.repeat(60));

  try {
    // Descomentar os exemplos que deseja executar:
    
    await exemploNotificacoes();
    await new Promise(r => setTimeout(r, 1000));
    
    await exemploPedidos();
    await new Promise(r => setTimeout(r, 1000));
    
    await exemploLogs();
    await new Promise(r => setTimeout(r, 1000));
    
    await exemploAnalytics();
    await new Promise(r => setTimeout(r, 1000));
    
    await exemploLote();
    await new Promise(r => setTimeout(r, 1000));
    
    await exemploChat();

    console.log('\n' + '='.repeat(60));
    console.log('✅ Todos os exemplos foram executados com sucesso!');
    console.log('\n💡 Dica: Execute o subscriber.ts em outro terminal para ver as mensagens sendo recebidas!');

  } catch (error) {
    console.error('❌ Erro ao executar exemplos:', error);
  }
}

// Executar se for o arquivo principal
if (require.main === module) {
  executarExemplos()
    .then(() => {
      console.log('\n👋 Finalizando...');
      setTimeout(() => process.exit(0), 2000);
    })
    .catch(error => {
      console.error('Erro fatal:', error);
      process.exit(1);
    });
}
