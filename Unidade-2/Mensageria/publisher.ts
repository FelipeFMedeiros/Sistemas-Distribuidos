import { Publisher } from './index';

/*
 Exemplo de uso do Publisher (Produtor de mensagens)
*/
async function runPublisher() {
    console.log('🚀 Iniciando Publisher...\n');

    const publisher = new Publisher('sistemas-distribuidos');

    try {
        // Exemplo 1: Publicar mensagem simples
        console.log('📤 Exemplo 1: Mensagem simples');
        await publisher.publishMessage({
            tipo: 'notificacao',
            titulo: 'Bem-vindo!',
            mensagem: 'Sistema de mensageria inicializado',
            timestamp: new Date().toISOString(),
        });

        // Aguarda um pouco
        await new Promise((resolve) => setTimeout(resolve, 1000));

        // Exemplo 2: Publicar dados de usuário
        console.log('\n📤 Exemplo 2: Dados de usuário');
        await publisher.publishMessage({
            tipo: 'usuario',
            acao: 'cadastro',
            dados: {
                nome: 'Felipe',
                email: 'felipe@example.com',
                curso: 'Sistemas Distribuídos',
            },
            timestamp: new Date().toISOString(),
        });

        await new Promise((resolve) => setTimeout(resolve, 1000));

        // Exemplo 3: Publicar múltiplas mensagens
        console.log('\n📤 Exemplo 3: Lote de mensagens');
        const mensagens = [
            { tipo: 'evento', acao: 'login', usuario: 'user1' },
            { tipo: 'evento', acao: 'logout', usuario: 'user2' },
            { tipo: 'evento', acao: 'update', usuario: 'user3' },
            { tipo: 'log', level: 'info', mensagem: 'Sistema operando normalmente' },
            { tipo: 'log', level: 'warning', mensagem: 'Uso de memória elevado' },
        ];

        await publisher.publishBatch(mensagens);

        console.log('\n✅ Todas as mensagens foram publicadas com sucesso!');
    } catch (error) {
        console.error('❌ Erro ao publicar mensagens:', error);
    }
}

// Executa o publisher
runPublisher()
    .catch(console.error)
    .finally(() => {
        console.log('\n👋 Publisher finalizado!');
        process.exit(0);
    });
