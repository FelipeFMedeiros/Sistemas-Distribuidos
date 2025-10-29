// gcc multi_client_linux.c -o multi_client_linux -pthread
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

/*
 * Cliente multi-thread (TCP e UDP)
 * - Cria N threads de cliente.
 * - Cada thread envia "MSGBASE-<idx>" e tenta ler a resposta.
 * - Para UDP, configura timeout de recebimento (SO_RCVTIMEO).
 *
 * Uso:
 *   ./multi_client_linux tcp|udp IP PORTA N "MENSAGEM_BASE"
 *
 * Exemplos:
 *   ./multi_client_linux tcp 192.168.56.10 5000 20 "HELLO"
 *   ./multi_client_linux udp 192.168.56.10 6000 50 "PING"
 */

// Estrutura do job para cada thread
typedef struct {
    int proto;           // Protocolo: 0 = TCP, 1 = UDP
    char ip[64];         // IP do servidor
    int port;            // Porta do servidor
    int idx;             // Índice da thread (identificador)
    char msg[512];       // Mensagem base a ser enviada
} job_t;

// Função executada por cada thread TCP
static void *run_tcp(void *p) {
    job_t *j = (job_t *)p;                       // Cast do parâmetro para job_t
    int s = socket(AF_INET, SOCK_STREAM, 0);     // Cria socket TCP
    if (s < 0) { perror("[TCP] socket"); free(j); return NULL; }

    // Configura estrutura do servidor
    struct sockaddr_in srv = {0};
    srv.sin_family = AF_INET;
    srv.sin_port   = htons(j->port);
    if (inet_pton(AF_INET, j->ip, &srv.sin_addr) != 1) {
        fprintf(stderr, "[TCP %d] IP invalido: %s\n", j->idx, j->ip);
        close(s); free(j); return NULL;
    }
    // Tenta conectar ao servidor
    if (connect(s, (struct sockaddr*)&srv, sizeof srv) != 0) {
        perror("[TCP] connect");
        close(s); free(j); return NULL;
    }

    char buf[1024];
    // Formata mensagem com índice da thread
    int n = snprintf(buf, sizeof buf, "%s-%d", j->msg, j->idx);
    // Envia dados para servidor
    if (send(s, buf, n, 0) < 0) { perror("[TCP] send"); close(s); free(j); return NULL; }

    // Recebe resposta do servidor
    int r = recv(s, buf, sizeof buf - 1, 0);
    if (r > 0) {
        buf[r] = '\0';  // Termina string com null
        printf("[TCP %d] %s\n", j->idx, buf);
    } else if (r == 0) {
        printf("[TCP %d] servidor fechou conexao\n", j->idx);
    } else {
        perror("[TCP] recv");
    }
    // TCP: O servidor encerra primeiro (close(ctx->cfd) na função worker), depois o cliente detecta (r == 0 no recv) e também fecha
    close(s);   // Fecha socket
    free(j);    // Libera memória do job
    return NULL;
}

// Função executada por cada thread UDP
static void *run_udp(void *p) {
    job_t *j = (job_t *)p;                       // Cast do parâmetro para job_t
    int s = socket(AF_INET, SOCK_DGRAM, 0);      // Cria socket UDP
    if (s < 0) { perror("[UDP] socket"); free(j); return NULL; }

    // Timeout de 5s para recvfrom (para não travar a thread se não houver resposta)
    struct timeval tv = { .tv_sec = 5, .tv_usec = 0 };
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv) != 0) {
        perror("[UDP] setsockopt(SO_RCVTIMEO)");
        // continua mesmo assim
    }

    // Configura estrutura do servidor
    struct sockaddr_in srv = {0};
    srv.sin_family = AF_INET;
    srv.sin_port   = htons(j->port);
    if (inet_pton(AF_INET, j->ip, &srv.sin_addr) != 1) {
        fprintf(stderr, "[UDP %d] IP invalido: %s\n", j->idx, j->ip);
        close(s); free(j); return NULL;
    }

    char buf[1024];
    // Formata mensagem com índice da thread
    int n = snprintf(buf, sizeof buf, "%s-%d", j->msg, j->idx);

    // Envia dados via UDP (sem conexão)
    if (sendto(s, buf, n, 0, (struct sockaddr *)&srv, sizeof srv) < 0) {
        perror("[UDP] sendto");
        close(s); free(j); return NULL;
    }

    socklen_t sl = sizeof srv;

    // Recebe resposta via UDP
    int r = recvfrom(s, buf, sizeof buf - 1, 0, (struct sockaddr *)&srv, &sl);
    if (r > 0) {
        buf[r] = '\0';
        printf("[UDP %d] %s\n", j->idx, buf);  // Imprime resposta
    } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("[UDP %d] timeout aguardando resposta\n", j->idx);  // Imprime se houve timeout
        } else {
            perror("[UDP] recvfrom");
        }
    }
    // UDP: O cliente encerra (simplesmente para de enviar/receber e fecha o socket)
    close(s);   // Fecha socket
    free(j);    // Libera memória do job
    return NULL;
}

int main(int argc, char **argv) {
    // Verifica se tem argumentos suficientes
    if (argc < 6) {
        fprintf(stderr, "uso: %s tcp|udp IP PORTA N \"MSG\"\n", argv[0]);
        return 1;
    }

    int is_udp = (strcmp(argv[1], "udp") == 0); // Define protocolo
    const char *ip   = argv[2];                 // IP do servidor
    int port         = atoi(argv[3]);           // Porta do servidor
    int N            = atoi(argv[4]);           // Número de threads/clientes
    const char *base = argv[5];                 // Mensagem base

    if (N <= 0) { fprintf(stderr, "N deve ser > 0\n"); return 1; }

    // Aloca array de handles para as threads
    pthread_t *th = (pthread_t *)malloc(sizeof(pthread_t) * N);
    if (!th) { perror("malloc"); return 1; }

    // Cria N threads
    for (int i = 0; i < N; i++) {
        // Aloca e inicializa job para cada thread
        job_t *j = (job_t *)calloc(1, sizeof *j);
        j->proto = is_udp ? 1 : 0;
        strncpy(j->ip, ip, sizeof j->ip - 1);      // Copia IP
        j->port = port;
        j->idx  = i + 1;                           // Índice da thread (começa em 1)
        strncpy(j->msg, base, sizeof j->msg - 1);  // Copia mensagem base

        // Cria thread TCP ou UDP baseado no protocolo
        int rc = pthread_create(&th[i], NULL, is_udp ? run_udp : run_tcp, j);
        if (rc != 0) {
            fprintf(stderr, "pthread_create falhou na thread %d (rc=%d)\n", i+1, rc);
            free(j);
        }
    }

    // Espera todas as threads terminarem
    for (int i = 0; i < N; i++) pthread_join(th[i], NULL);
    free(th);        // Libera array de handles
    return 0;
}
