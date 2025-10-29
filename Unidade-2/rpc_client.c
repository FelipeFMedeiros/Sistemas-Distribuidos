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
 *     int rpc_echo(const char* ip, int port, const char* msg,
 *                  char* out, size_t outcap, size_t* outlen)
 * - Cada chamada abre uma conexão, envia request, lê resposta e fecha.
 * - Uso:
 *     ./rpc_client IP PORT add 7 35
 *     ./rpc_client IP PORT echo "mensagem de teste"
 */

#define BUFSZ 4096
enum { OP_ADD = 1, OP_ECHO = 2 };

typedef struct {
  uint32_t op;   // big-endian
  uint32_t len;  // big-endian (bytes do payload)
} __attribute__((packed)) rpc_hdr_t;

static int connect_tcp(const char* ip, int port){
  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0){ perror("socket"); return -1; }
  struct sockaddr_in srv;
  memset(&srv, 0, sizeof srv);
  srv.sin_family = AF_INET;
  srv.sin_port = htons(port);
  if (inet_pton(AF_INET, ip, &srv.sin_addr) != 1){
    fprintf(stderr, "IP inválido: %s\n", ip); close(s); return -1;
  }
  if (connect(s, (struct sockaddr*)&srv, sizeof srv) < 0){
    perror("connect"); close(s); return -1;
  }
  return s;
}

static ssize_t read_full(int fd, void *buf, size_t n){
  size_t got = 0; char *p = (char*)buf;
  while (got < n){
    ssize_t r = recv(fd, p + got, n - got, 0);
    if (r == 0) return 0;             // peer fechou
    if (r < 0){
      if (errno == EINTR) continue;
      return -1;
    }
    got += (size_t)r;
  }
  return (ssize_t)got;
}
static ssize_t write_full(int fd, const void *buf, size_t n){
  size_t sent = 0; const char *p = (const char*)buf;
  while (sent < n){
    ssize_t r = send(fd, p + sent, n - sent, 0);
    if (r <= 0){
      if (r < 0 && errno == EINTR) continue;
      return -1;
    }
    sent += (size_t)r;
  }
  return (ssize_t)sent;
}

/* ===========================
 * STUB: rpc_add
 * Encapsula a interface: ADD(a,b) -> int
 * Retorna 0 em sucesso, <0 em erro.
 * =========================== */
int rpc_add(const char* ip, int port, int a, int b, int *result_out){
  int s = connect_tcp(ip, port);
  if (s < 0) return -1;

  // monta payload
  char payload[8];
  int32_t a_net = (int32_t)htonl((uint32_t)a);
  int32_t b_net = (int32_t)htonl((uint32_t)b);
  memcpy(payload,     &a_net, 4);
  memcpy(payload + 4, &b_net, 4);

  rpc_hdr_t h;
  h.op  = htonl(OP_ADD);
  h.len = htonl(8);

  if (write_full(s, &h, sizeof h) < 0 || write_full(s, payload, 8) < 0){
    perror("send"); close(s); return -1;
  }

  // lê resposta
  rpc_hdr_t rh;
  if (read_full(s, &rh, sizeof rh) <= 0){
    perror("recv header"); close(s); return -1;
  }
  uint32_t rop  = ntohl(rh.op);
  uint32_t rlen = ntohl(rh.len);
  if (rop != OP_ADD || rlen != 4){
    fprintf(stderr, "resposta inválida (op=%u len=%u)\n", rop, rlen);
    close(s); return -1;
  }

  int32_t ans_net;
  if (read_full(s, &ans_net, 4) <= 0){
    perror("recv body"); close(s); return -1;
  }
  close(s);

  int32_t ans = (int32_t)ntohl((uint32_t)ans_net);
  if (result_out) *result_out = ans;
  return 0;
}

/* ===========================
 * STUB: rpc_echo
 * Encapsula a interface: ECHO(msg) -> string
 * - Envia a string sem o '\0'.
 * - Recebe eco (len bytes). Copia para 'out' com truncamento seguro
 *   e NUL-termina se houver espaço.
 * - outlen (se não-NULL) recebe o tamanho real retornado pelo servidor.
 * Retorna 0 em sucesso, <0 em erro.
 * =========================== */
int rpc_echo(const char* ip, int port, const char* msg,
             char* out, size_t outcap, size_t* outlen){
  if (!msg){ errno = EINVAL; return -1; }
  size_t mlen = strlen(msg);
  if (mlen > UINT32_MAX){ errno = EINVAL; return -1; }

  int s = connect_tcp(ip, port);
  if (s < 0) return -1;

  rpc_hdr_t h;
  h.op  = htonl(OP_ECHO);
  h.len = htonl((uint32_t)mlen);

  if (write_full(s, &h, sizeof h) < 0){
    perror("send header"); close(s); return -1;
  }
  if (mlen > 0 && write_full(s, msg, mlen) < 0){
    perror("send payload"); close(s); return -1;
  }

  // lê header da resposta
  rpc_hdr_t rh;
  if (read_full(s, &rh, sizeof rh) <= 0){
    perror("recv header"); close(s); return -1;
  }
  uint32_t rop  = ntohl(rh.op);
  uint32_t rlen = ntohl(rh.len);
  if (rop != OP_ECHO){
    fprintf(stderr, "resposta inválida para ECHO (op=%u)\n", rop);
    close(s); return -1;
  }
  if (rlen > BUFSZ){ // limite simples de sanidade
    fprintf(stderr, "resposta muito grande (%u bytes)\n", rlen);
    close(s); return -1;
  }

  // lê corpo
  char buf[BUFSZ];
  if (rlen > 0){
    if (read_full(s, buf, rlen) <= 0){
      perror("recv body"); close(s); return -1;
    }
  }
  close(s);

  // copia para 'out' com truncamento e NUL-terminação
  if (out && outcap > 0){
    size_t ncopy = rlen < (outcap - 1) ? rlen : (outcap - 1);
    if (outcap == 1) ncopy = 0; // só cabe '\0'
    if (ncopy > 0) memcpy(out, buf, ncopy);
    out[ncopy] = '\0';
  }
  if (outlen) *outlen = rlen;

  return 0;
}

/* ===========================
 * MAIN de utilitário
 * =========================== */
static void usage(const char* prog){
  fprintf(stderr,
    "Uso:\n"
    "  %s IP PORT add A B\n"
    "  %s IP PORT echo MENSAGEM...\n"
    "\nExemplos:\n"
    "  %s 192.168.56.102 5000 add 7 35\n"
    "  %s 192.168.56.102 5000 echo \"mensagem de teste\"\n",
    prog, prog, prog, prog);
}

int main(int argc, char** argv){
  if (argc < 5){
    usage(argv[0]);
    return 1;
  }
  const char* ip = argv[1];
  int port = atoi(argv[2]);
  const char* cmd = argv[3];

  if (strcmp(cmd, "add") == 0){
    if (argc != 6){
      fprintf(stderr, "add requer 2 inteiros: A B\n");
      usage(argv[0]);
      return 1;
    }
    int a = atoi(argv[4]);
    int b = atoi(argv[5]);
    int res = 0;
    if (rpc_add(ip, port, a, b, &res) == 0){
      printf("%d + %d = %d\n", a, b, res);
      return 0;
    } else {
      fprintf(stderr, "falha na chamada rpc_add\n");
      return 2;
    }
  } else if (strcmp(cmd, "echo") == 0){
    // junta todos os argumentos restantes em uma única mensagem com espaços
    if (argc < 5){
      fprintf(stderr, "echo requer uma mensagem\n");
      usage(argv[0]);
      return 1;
    }
    // monta buffer com os args [4..argc-1] separados por espaço
    char msg[BUFSZ];
    msg[0] = '\0';
    size_t used = 0;
    for (int i = 4; i < argc; i++){
      const char* part = argv[i];
      size_t plen = strlen(part);
      if (used && used + 1 < sizeof msg){ msg[used++] = ' '; }
      size_t room = sizeof msg - used - 1;
      size_t ncopy = (plen < room) ? plen : room;
      if (ncopy > 0){ memcpy(msg + used, part, ncopy); used += ncopy; }
      msg[used] = '\0';
      if (ncopy < plen){ break; } // truncou
    }

    char out[BUFSZ];
    size_t outlen = 0;
    if (rpc_echo(ip, port, msg, out, sizeof out, &outlen) == 0){
      printf("echo(%zu bytes): %s\n", outlen, out);
      return 0;
    } else {
      fprintf(stderr, "falha na chamada rpc_echo\n");
      return 2;
    }
  } else {
    fprintf(stderr, "comando desconhecido: %s\n", cmd);
    usage(argv[0]);
    return 1;
  }
}
