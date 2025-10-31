// gcc rpc_server.c -o rpc_server -pthread
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/*
 * RPC SERVER (TCP)
 * - Interface binária simples:
 *     header: uint32_t op, uint32_t len   (ambos big-endian)
 *     payload: depende da op
 * - Operações:
 *     OP_ADD  = 1  -> payload: [int32 a][int32 b]    resp: [int32 soma]
 * - Multithread: uma thread por conexão (cliente)
 * - Simula "processamento lento" com sleep(3)
 */

#define BACKLOG 64
#define BUFSZ   4096

// Enumeração das operações suportadas pelo servidor RPC
enum { OP_ADD = 1 };

// Estrutura do cabeçalho RPC: contém operação e tamanho do payload
typedef struct {
    uint32_t op;   // big-endian: código da operação
    uint32_t len;  // big-endian: tamanho do payload em bytes
} __attribute__((packed)) rpc_hdr_t;

// Contexto de cada cliente: socket e endereço
typedef struct {
    int cfd;                      // file descriptor da conexão do cliente
    struct sockaddr_in caddr;     // endereço IP e porta do cliente
} ctx_t;

// Flag global para controlar o loop principal (sinal SIGINT)
static volatile sig_atomic_t running = 1;

// Handler para SIGINT (Ctrl+C): sinaliza encerramento gracioso
static void on_sigint(int s) { (void) s; running = 0; fprintf(stderr, "[SRV] SIGINT, saindo...\n"); }

// Função auxiliar: garante leitura completa de n bytes do socket
static ssize_t read_full(int fd, void *buf, size_t n) {
    size_t got = 0; char *p = (char *) buf;
    while (got < n) {
        ssize_t r = recv(fd, p + got, n - got, 0);
        if (r == 0) return 0;       // conexão fechada
        if (r < 0) {
            if (errno == EINTR) continue;  // interrupção por sinal, tenta novamente
            return -1;
        }
        got += (size_t) r;
    }
    return (ssize_t) got;
}

// Função auxiliar: garante escrita completa de n bytes no socket
static ssize_t write_full(int fd, const void *buf, size_t n) {
    size_t sent = 0; const char *p = (const char *) buf;
    while (sent < n) {
        ssize_t r = send(fd, p + sent, n - sent, 0);
        if (r <= 0) {
            if (r < 0 && errno == EINTR) continue;
            return -1;
        }
        sent += (size_t) r;
    }
    return (ssize_t) sent;
}

// Implementação da operação ADD: soma dois inteiros
static int32_t svc_add(int32_t a, int32_t b) {
    return a + b;
}

// Função principal: processa uma requisição RPC
static int handle_one_rpc(int cfd) {
    rpc_hdr_t h;
    // 1. Lê o cabeçalho (8 bytes: op + len)
    if (read_full(cfd, &h, sizeof h) <= 0) return -1;

    // 2. Converte de big-endian (rede) para host
    uint32_t op = ntohl(h.op);
    uint32_t len = ntohl(h.len);
    if (len > BUFSZ) {
        fprintf(stderr, "[SRV] payload grande demais (%u)\n", len);
        return -1;
    }

    // 3. Lê o payload (dados da requisição)
    char buf[BUFSZ];
    if (len > 0 && read_full(cfd, buf, len) <= 0) return -1;

    // 4. Simula processamento lento (requisito do trabalho)
    sleep(3);

    // 5. Processa a operação e monta a resposta
    rpc_hdr_t rh = { 0 };
    char out[BUFSZ];
    size_t outlen = 0;

    if (op == OP_ADD) {
        // Operação ADD: espera 2 inteiros (8 bytes)
        if (len != 8) {
            fprintf(stderr, "[SRV] ADD com payload inválido (%u)\n", len);
            return -1;
        }
        // Desserializa os dois inteiros (network byte order -> host)
        int32_t a_net, b_net; memcpy(&a_net, buf, 4); memcpy(&b_net, buf + 4, 4);
        int32_t a = (int32_t) ntohl((uint32_t) a_net);
        int32_t b = (int32_t) ntohl((uint32_t) b_net);

        // Executa a operação
        int32_t ans = svc_add(a, b);
        // Serializa o resultado (host -> network byte order)
        int32_t ans_net = (int32_t) htonl((uint32_t) ans);
        memcpy(out, &ans_net, 4);
        outlen = 4;
    }
    else {
        fprintf(stderr, "[SRV] op desconhecida: %u\n", op);
        return -1;
    }

    // 6. Monta e envia cabeçalho da resposta
    rh.op = htonl(op);
    rh.len = htonl((uint32_t) outlen);

    if (write_full(cfd, &rh, sizeof rh) < 0) return -1;
    // 7. Envia o payload da resposta
    if (outlen && write_full(cfd, out, outlen) < 0) return -1;

    return 0;
}

// Thread worker: atende um cliente
static void *worker(void *p) {
    ctx_t *ctx = (ctx_t *) p;
    // Extrai informações do cliente para log
    char ip[INET_ADDRSTRLEN]; inet_ntop(AF_INET, &ctx->caddr.sin_addr, ip, sizeof ip);
    int cport = ntohs(ctx->caddr.sin_port);
    fprintf(stderr, "[SRV] cliente %s:%d conectado\n", ip, cport);

    // Processa uma requisição RPC
    (void) handle_one_rpc(ctx->cfd);
    // Fecha conexão e libera recursos
    close(ctx->cfd);
    fprintf(stderr, "[SRV] cliente %s:%d desconectado\n", ip, cport);
    free(ctx);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "uso: %s <PORTA>\n", argv[0]);
        return 1;
    }
    int port = atoi(argv[1]);

    // Configura tratamento de SIGINT (Ctrl+C)
    struct sigaction sa = { 0 };
    sa.sa_handler = on_sigint;
    sigaction(SIGINT, &sa, NULL);

    // Cria socket TCP
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) { perror("socket"); return 1; }

    // Permite reusar a porta imediatamente
    int opt = 1; setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

    // Configura endereço do servidor (escuta em todas as interfaces)
    struct sockaddr_in srv = { 0 };
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = INADDR_ANY;  // 0.0.0.0
    srv.sin_port = htons(port);

    // Associa socket ao endereço e porta
    if (bind(sfd, (struct sockaddr *) &srv, sizeof srv) < 0) { perror("bind"); return 1; }
    // Coloca socket em modo de escuta (fila de 64 conexões pendentes)
    if (listen(sfd, BACKLOG) < 0) { perror("listen"); return 1; }

    fprintf(stderr, "[SRV] escutando 0.0.0.0:%d\n", port);

    // Loop principal: aceita conexões
    while (running) {
        struct sockaddr_in cli; socklen_t cl = sizeof cli;
        // Aceita nova conexão (bloqueante)
        int cfd = accept(sfd, (struct sockaddr *) &cli, &cl);
        if (cfd < 0) {
            if (errno == EINTR) break;  // interrompido por sinal
            perror("accept"); continue;
        }
        // Aloca contexto para o cliente
        ctx_t *ctx = (ctx_t *) malloc(sizeof * ctx);
        ctx->cfd = cfd; ctx->caddr = cli;

        // Cria thread detached para atender o cliente
        pthread_t th; pthread_create(&th, NULL, worker, ctx);
        pthread_detach(th);  // thread se auto-limpa ao terminar
    }
    close(sfd);
    fprintf(stderr, "[SRV] encerrado\n");
    return 0;
}
