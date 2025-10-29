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
 *     OP_ECHO = 2  -> payload: [bytes msg]           resp: [bytes eco]
 * - Multithread: uma thread por conexão (cliente)
 * - Simula "processamento lento" com sleep(3)
 */

#define BACKLOG 64
#define BUFSZ   4096

enum { OP_ADD = 1, OP_ECHO = 2 };

typedef struct {
    uint32_t op;   // big-endian
    uint32_t len;  // big-endian
} __attribute__((packed)) rpc_hdr_t;

typedef struct {
    int cfd;
    struct sockaddr_in caddr;
} ctx_t;

static volatile sig_atomic_t running = 1;

static void on_sigint(int s) { (void) s; running = 0; fprintf(stderr, "[SRV] SIGINT, saindo...\n"); }

// util: leitura/escrita total
static ssize_t read_full(int fd, void *buf, size_t n) {
    size_t got = 0; char *p = (char *) buf;
    while (got < n) {
        ssize_t r = recv(fd, p + got, n - got, 0);
        if (r == 0) return 0;
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        got += (size_t) r;
    }
    return (ssize_t) got;
}
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

// Implementações de negócio (lado servidor)
static int32_t svc_add(int32_t a, int32_t b) {
    // Aqui seria sua lógica real; mantemos simples.
    return a + b;
}
static size_t svc_echo(const char *in, size_t inlen, char *out, size_t outcap) {
    // Eco direto (poderia upper-case, etc). Garante limite do buffer.
    size_t n = inlen < outcap ? inlen : outcap;
    memcpy(out, in, n);
    return n;
}

// Dispatch (stub do servidor): decodifica, chama implementação e envia resposta
static int handle_one_rpc(int cfd) {
    rpc_hdr_t h;
    if (read_full(cfd, &h, sizeof h) <= 0) return -1;

    uint32_t op = ntohl(h.op);
    uint32_t len = ntohl(h.len);
    if (len > BUFSZ) {
        fprintf(stderr, "[SRV] payload grande demais (%u)\n", len);
        return -1;
    }

    char buf[BUFSZ];
    if (len > 0 && read_full(cfd, buf, len) <= 0) return -1;

    // Simula processamento lento (requisito do trabalho)
    sleep(3);

    // Monta resposta
    rpc_hdr_t rh = { 0 };
    char out[BUFSZ];
    size_t outlen = 0;

    if (op == OP_ADD) {
        if (len != 8) {
            fprintf(stderr, "[SRV] ADD com payload inválido (%u)\n", len);
            return -1;
        }
        int32_t a_net, b_net; memcpy(&a_net, buf, 4); memcpy(&b_net, buf + 4, 4);
        int32_t a = (int32_t) ntohl((uint32_t) a_net);
        int32_t b = (int32_t) ntohl((uint32_t) b_net);

        int32_t ans = svc_add(a, b);
        int32_t ans_net = (int32_t) htonl((uint32_t) ans);
        memcpy(out, &ans_net, 4);
        outlen = 4;
    }
    else if (op == OP_ECHO) {
        outlen = svc_echo(buf, len, out, sizeof out);
    }
    else {
        fprintf(stderr, "[SRV] op desconhecida: %u\n", op);
        return -1;
    }

    rh.op = htonl(op);
    rh.len = htonl((uint32_t) outlen);

    if (write_full(cfd, &rh, sizeof rh) < 0) return -1;
    if (outlen && write_full(cfd, out, outlen) < 0) return -1;

    return 0;
}

static void *worker(void *p) {
    ctx_t *ctx = (ctx_t *) p;
    char ip[INET_ADDRSTRLEN]; inet_ntop(AF_INET, &ctx->caddr.sin_addr, ip, sizeof ip);
    int cport = ntohs(ctx->caddr.sin_port);
    fprintf(stderr, "[SRV] cliente %s:%d conectado\n", ip, cport);

    // Atende 1 requisição por conexão (simples). Poderia ser um loop.
    (void) handle_one_rpc(ctx->cfd);
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

    struct sigaction sa = { 0 };
    sa.sa_handler = on_sigint;
    sigaction(SIGINT, &sa, NULL);

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) { perror("socket"); return 1; }

    int opt = 1; setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

    struct sockaddr_in srv = { 0 };
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = INADDR_ANY;
    srv.sin_port = htons(port);

    if (bind(sfd, (struct sockaddr *) &srv, sizeof srv) < 0) { perror("bind"); return 1; }
    if (listen(sfd, BACKLOG) < 0) { perror("listen"); return 1; }

    fprintf(stderr, "[SRV] escutando 0.0.0.0:%d\n", port);

    while (running) {
        struct sockaddr_in cli; socklen_t cl = sizeof cli;
        int cfd = accept(sfd, (struct sockaddr *) &cli, &cl);
        if (cfd < 0) {
            if (errno == EINTR) break;
            perror("accept"); continue;
        }
        ctx_t *ctx = (ctx_t *) malloc(sizeof * ctx);
        ctx->cfd = cfd; ctx->caddr = cli;

        pthread_t th; pthread_create(&th, NULL, worker, ctx);
        pthread_detach(th);
    }
    close(sfd);
    fprintf(stderr, "[SRV] encerrado\n");
    return 0;
}
