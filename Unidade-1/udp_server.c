// gcc udp_server.c -o udp_server -pthread
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

#define BUFSZ 2048 // Tamanho do buffer para mensagens

/*
 * Servidor UDP multi-thread
 * - Escuta em uma porta UDP especificada.
 * - Para cada mensagem recebida, cria uma thread para processar a requisição.
 * - Cada thread simula processamento demorado (sleep) e responde ao cliente com eco e ID da thread.
 * - Permite múltiplos clientes simultâneos, evidenciando concorrência.
 * - Encerramento via Ctrl+C
 *
 * Uso:
 *   ./udp_server <PORTA>
 *
 * Exemplo:
 *   ./udp_server 6000
 */

// Estrutura que armazena dados de uma tarefa para processamento em thread
typedef struct {
    int sfd;                    // Socket file descriptor
    struct sockaddr_in cli;     // Endereço do cliente
    socklen_t clisz;           // Tamanho da estrutura do cliente
    char *data;                // Dados recebidos
    size_t len;                // Tamanho dos dados
} task_t;

static volatile int running = 1;  // Variável de controle do loop principal

// Função executada por cada thread para processar requisições
static void *worker(void *p) {
    task_t *t = (task_t *) p;
    char ip[INET_ADDRSTRLEN];
    
    // Converte IP do cliente para string legível
    inet_ntop(AF_INET, &t->cli.sin_addr, ip, sizeof ip);
    int cport = ntohs(t->cli.sin_port);  // Extrai a porta do cliente com ntohs()

    fprintf(stderr, "[UDP] de %s:%d: %.*s\n", ip, cport, (int) t->len, t->data);
    fprintf(stderr, "[UDP] processando %s:%d...\n", ip, cport);
    
    sleep(5);  // Simula processamento demorado

    // Prepara resposta incluindo ID da thread
    char out[BUFSZ];
    int n = snprintf(out, sizeof out, "OK UDP thr=%lu eco: %.*s",
        (unsigned long) pthread_self(), (int) t->len, t->data);

    // Envia resposta de volta para o cliente
    sendto(t->sfd, out, n, 0, (struct sockaddr *) &t->cli, t->clisz); 
    
    // UDP não fecha conexão, é stateless

    // Libera memória alocada
    free(t->data);
    free(t);
    return NULL;
}

// Handler para sinal SIGINT (Ctrl+C)
static void on_sig(int s) {
    (void)s;
    running = 0;
    fprintf(stderr, "[UDP] sinal SIGINT recebido, encerrando...\n");
}

int main(int argc, char **argv) {
    if (argc != 2) { 
        fprintf(stderr, "uso: %s <porta>\n", argv[0]); 
        return 1; 
    }

    int port = atoi(argv[1]);

    struct sigaction sa;
    sa.sa_handler = on_sig;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction");
        return 1;
    }
    
    // Cria socket UDP
    int sfd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (sfd < 0) { 
        perror("socket"); 
        return 1; 
    }

    // Permite reutilizar endereço
    int opt = 1; 
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
    
    // Configura estrutura do servidor
    struct sockaddr_in srv = { 0 }; 
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = INADDR_ANY;  // Escuta em todas as interfaces
    srv.sin_port = htons(port);        // Converte porta para network byte order

    // Associa socket ao endereço/porta
    if (bind(sfd, (struct sockaddr *) &srv, sizeof srv) < 0) { 
        perror("bind"); 
        return 1; 
    }

    fprintf(stderr, "[UDP] escutando 0.0.0.0:%d\n", port);

    // Loop principal do servidor
    while (running) {
        char buf[BUFSZ]; 
        struct sockaddr_in cli; 
        socklen_t cl = sizeof cli;

        // Recebe dados de qualquer cliente
        ssize_t n = recvfrom(sfd, buf, sizeof buf, 0, (struct sockaddr *) &cli, &cl); // recvfrom() é a função que recebe dados UDP

        if (n < 0) { 
            if (errno == EINTR) break; 
            perror("recvfrom"); continue; 
        }

        // Cria estrutura de tarefa para a thread
        task_t *t = malloc(sizeof * t); 
        t->sfd = sfd; t->cli = cli; 
        t->clisz = cl;
        t->data = malloc(n);  // Aloca memória para os dados
        
        memcpy(t->data, buf, n);  // Copia dados recebidos
        t->len = (size_t) n;
        
        // Cria nova thread para processar a requisição
        pthread_t th; 
        pthread_create(&th, NULL, worker, t); 
        pthread_detach(th);  // Thread será limpa automaticamente
    }

    close(sfd); 
    fprintf(stderr, "[UDP] encerrado\n"); 
    return 0;
}
