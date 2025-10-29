// gcc rpc_client.c -o rpc_client
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/*
 * RPC CLIENT (TCP)
 * - Stubs de alto nível:
 *     int rpc_add(const char* ip, int port, int a, int b, int* result_out)
 *     int rpc_echo(const char* ip, int port, const char* msg, char* out, size_t outcap, size_t* outlen)
 * - Cada chamada abre uma conexão, envia request, lê resposta e fecha.
 * - Uso:
 *     ./rpc_client IP PORT add 7 35
 *     ./rpc_client IP PORT echo "mensagem de teste"
 */

#define BUFSZ 4096
enum { OP_ADD = 1, OP_ECHO = 2 };

typedef struct {
    uint32_t op;   // big-endian
    uint32_t len;  // big-endian
} __attribute__((packed)) rpc_hdr_t;

static int connect_tcp(const char *ip, int port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return -1; }
    struct sockaddr_in srv = { 0 };
    srv.sin_family = AF_INET;
    srv.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &srv.sin_addr) != 1) {
        fprintf(stderr, "IP inválido: %s\n", ip); close(s); return -1;
    }
    if (connect(s, (struct sockaddr *) &srv, sizeof srv) < 0) {
        perror("connect"); close(s); return -1;
    }
    return s;
}

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

/* ===========================
 * STUB: rpc_add
 * Encapsula a interface: ADD(a,b) -> int
 * Retorna 0 em sucesso, <0 em erro.
 * =========================== */
int rpc_add(const char *ip, int port, int a, int b, int *result_out) {
    int s = connect_tcp(ip, port);
    if (s < 0) return -1;

    // monta payload
    char payload[8];
    int32_t a_net = (int32_t) htonl((uint32_t) a);
    int32_t b_net = (int32_t) htonl((uint32_t) b);
    memcpy(payload, &a_net, 4);
    memcpy(payload + 4, &b_net, 4);

    rpc_hdr_t h;
    h.op = htonl(OP_ADD);
    h.len = htonl(8);

    if (write_full(s, &h, sizeof h) < 0 || write_full(s, payload, 8) < 0) { perror("send"); close(s); return -1; }

    // lê resposta
    rpc_hdr_t rh;
    if (read_full(s, &rh, sizeof rh) <= 0) { perror("recv header"); close(s); return -1; }
    uint32_t rop = ntohl(rh.op);
    uint32_t rlen = ntohl(rh.len);
    if (rop != OP_ADD || rlen != 4) { fprintf(stderr, "resposta inválida (op=%u len=%u)\n", rop, rlen); close(s); return -1; }

    int32_t ans_net;
    if (read_full(s, &ans_net, 4) <= 0) { perror("recv body"); close(s); return -1; }
    close(s);

    int32_t ans = (int32_t) ntohl((uint32_t) ans_net);
    if (result_out) *result_out = ans;
    return 0;
}

/* ===========================
 * STUB: rpc_echo
