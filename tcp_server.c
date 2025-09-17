// gcc tcp_server.c -o tcp_server -pthread
#define _GNU_SOURCE 
#include <arpa/inet.h> // Funções de conversão de endereços
#include <errno.h> 
#include <netinet/in.h> // Definições de estruturas de endereços
#include <pthread.h> // Biblioteca para threads POSIX
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> // Funções de sockets
#include <time.h>
#include <unistd.h>

#define BACKLOG 64  // Máximo de conexões pendentes na fila
#define BUFSZ   1024  // Tamanho do buffer para mensagens

/*
 * Servidor TCP multi-thread
 * - Escuta em uma porta TCP especificada.
 * - Para cada conexão aceita, cria uma thread para processar a requisição do cliente.
 * - Cada thread recebe uma mensagem, simula processamento demorado (sleep) e responde ao cliente com eco e ID da thread.
 * - Permite múltiplos clientes simultâneos, evidenciando concorrência.
 * - Encerramento via Ctrl+C
 *
 * Uso:
 *   ./tcp_server <PORTA>
 *
 * Exemplo:
 *   ./tcp_server 6000
 */

// Estrutura para passar dados para cada thread (contexto da conexão)
typedef struct { int cfd; struct sockaddr_in caddr; } ctx_t;
static volatile sig_atomic_t running = 1;  // Controla se o servidor continua rodando (tipo seguro para sinais)

// Função que cada thread executa para atender um cliente
static void *worker(void *p) {
    ctx_t *ctx = (ctx_t *) p;
    char ip[INET_ADDRSTRLEN];
    // Converte o endereço IP do cliente para string legível
    inet_ntop(AF_INET, &ctx->caddr.sin_addr, ip, sizeof ip);
    int cport = ntohs(ctx->caddr.sin_port);  // Converte porta para host byte order
    fprintf(stderr, "[TCP] conexão %s:%d\n", ip, cport);

    char buf[BUFSZ];
    // Recebe dados do cliente
    ssize_t n = recv(ctx->cfd, buf, BUFSZ - 1, 0);

    if (n <= 0) {  // Se não recebeu dados ou erro
        close(ctx->cfd);
        free(ctx);
        return NULL;
    }
    buf[n] = '\0';  // Termina a string
    fprintf(stderr, "[TCP] recebido de %s:%d: %s\n", ip, cport, buf);
    sleep(1);  
    fprintf(stderr, "[TCP] processando %s:%d...\n", ip, cport);
    sleep(5);  // Simula processamento demorado

    // Prepara resposta de eco com ID da thread
    char out[BUFSZ]; snprintf(out, sizeof out, "OK TCP thr=%lu eco: %s",
        (unsigned long) pthread_self(), buf);
    send(ctx->cfd, out, strlen(out), 0);  // Envia resposta
    close(ctx->cfd); free(ctx);  // Limpa recursos
    fprintf(stderr, "[TCP] fim %s:%d\n", ip, cport);
    return NULL;
}

// Handler para sinal SIGINT (Ctrl+C) - para encerrar servidor graciosamente
static void on_sig(int s) { 
    (void) s; 
    running = 0; 
    fprintf(stderr, "[TCP] sinal SIGINT recebido, encerrando...\n");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "uso: %s <porta>\n", argv[0]);
        return 1;
    }
    int port = atoi(argv[1]);
    
    // Configura handler para Ctrl+C usando sigaction (mais robusto em multi-thread)
    struct sigaction sa;
    sa.sa_handler = on_sig;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    // Cria socket TCP
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) {
        perror("socket");
        return 1;
    }
    // Permite reutilizar a porta imediatamente após encerrar
    int opt = 1; setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

    // Configura endereço do servidor
    struct sockaddr_in srv = { 0 };
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = INADDR_ANY;  // Aceita conexões de qualquer IP
    srv.sin_port = htons(port);  // Converte porta para network byte order

    // Associa socket ao endereço
    if (bind(sfd, (struct sockaddr *) &srv, sizeof srv) < 0) {
        perror("bind");
        return 1;
    }

    // Coloca socket em modo de escuta
    if (listen(sfd, BACKLOG) < 0) {
        perror("listen");
        return 1;
    }
    fprintf(stderr, "[TCP] escutando 0.0.0.0:%d\n", port);

    // Loop principal: aceita conexões enquanto running = 1
    while (running) {
        struct sockaddr_in c; socklen_t cl = sizeof c;
        // Aceita nova conexão (bloqueia até chegada de cliente)
        int cfd = accept(sfd, (struct sockaddr *) &c, &cl);

        if (cfd < 0) {
            if (errno == EINTR) break;  // Interrompido por sinal
            perror("accept");
            continue;
        }

        // Cria contexto para a nova conexão
        ctx_t *ctx = malloc(sizeof * ctx);
        ctx->cfd = cfd; ctx->caddr = c;
        
        // Cria thread para atender este cliente
        pthread_t th;
        pthread_create(&th, NULL, worker, ctx);
        pthread_detach(th);  // Thread se limpa automaticamente ao terminar
    }

    close(sfd); fprintf(stderr, "[TCP] encerrado\n");
    return 0;
}
