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
#include <stdint.h>

/*
 * RPC CLIENT (TCP)
 * - Stubs de alto nível:
 *     int rpc_add(const char* ip, int port, int a, int b, int* result_out)
 * - Cada chamada abre uma conexão, envia request, lê resposta e fecha.
 * - Uso:
 *     ./rpc_client IP PORT add 7 35
 */

#define BUFSZ 4096
// Define os códigos de operação para identificar qual função remota chamar
enum { OP_ADD = 1 };

// Estrutura do cabeçalho da mensagem RPC
typedef struct {
  uint32_t op;   // big-endian - código da operação (ADD ou ECHO)
  uint32_t len;  // big-endian - tamanho do payload em bytes
} __attribute__((packed)) rpc_hdr_t;

// Eestabelece conexão TCP com o servidor
static int connect_tcp(const char* ip, int port){
  // Cria socket TCP
  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0){ perror("socket"); return -1; }
  
  // Configura endereço do servidor
  struct sockaddr_in srv;
  memset(&srv, 0, sizeof srv);
  srv.sin_family = AF_INET;
  srv.sin_port = htons(port); // Converte porta para network byte order
  
  // Converte IP string para formato binário
  if (inet_pton(AF_INET, ip, &srv.sin_addr) != 1){
    fprintf(stderr, "IP inválido: %s\n", ip); close(s); return -1;
  }
  
  // Conecta ao servidor
  if (connect(s, (struct sockaddr*)&srv, sizeof srv) < 0){
    perror("connect"); close(s); return -1;
  }
  return s;
}

// Garante a leitura completa de 'n' bytes (trata leituras parciais)
static ssize_t read_full(int fd, void *buf, size_t n){
  size_t got = 0; char *p = (char*)buf;
  while (got < n){
    ssize_t r = recv(fd, p + got, n - got, 0);
    if (r == 0) return 0;             // peer fechou
    if (r < 0){
      if (errno == EINTR) continue;   // Interrupção - tenta novamente
      return -1;
    }
    got += (size_t)r;
  }
  return (ssize_t)got;
}

// Garante o envio completo de 'n' bytes (trata envios parciais)
static ssize_t write_full(int fd, const void *buf, size_t n){
  size_t sent = 0; const char *p = (const char*)buf;
  while (sent < n){
    ssize_t r = send(fd, p + sent, n - sent, 0);
    if (r <= 0){
      if (r < 0 && errno == EINTR) continue; // Interrupção - tenta novamente
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
  // Conecta ao servidor
  int s = connect_tcp(ip, port);
  if (s < 0) return -1;

  // Prepara payload: dois inteiros em network byte order (big-endian)
  char payload[8];
  int32_t a_net = (int32_t)htonl((uint32_t)a); // Converte para network byte order
  int32_t b_net = (int32_t)htonl((uint32_t)b);
  memcpy(payload,     &a_net, 4);
  memcpy(payload + 4, &b_net, 4);

  // Monta cabeçalho: operação ADD com payload de 8 bytes
  rpc_hdr_t h;
  h.op  = htonl(OP_ADD);
  h.len = htonl(8);

  // Envia cabeçalho + payload
  if (write_full(s, &h, sizeof h) < 0 || write_full(s, payload, 8) < 0){
    perror("send"); close(s); return -1;
  }

  // Lê cabeçalho da resposta
  rpc_hdr_t rh;
  if (read_full(s, &rh, sizeof rh) <= 0){
    perror("recv header"); close(s); return -1;
  }
  
  // Valida a resposta (deve ser OP_ADD com 4 bytes de resultado)
  uint32_t rop  = ntohl(rh.op);  // Converte de network byte order
  uint32_t rlen = ntohl(rh.len);
  if (rop != OP_ADD || rlen != 4){
    fprintf(stderr, "resposta inválida (op=%u len=%u)\n", rop, rlen);
    close(s); return -1;
  }

  // Lê o resultado (4 bytes)
  int32_t ans_net;
  if (read_full(s, &ans_net, 4) <= 0){
    perror("recv body"); close(s); return -1;
  }
  close(s); // Fecha conexão

  // Converte resultado para host byte order e retorna
  int32_t ans = (int32_t)ntohl((uint32_t)ans_net);
  if (result_out) *result_out = ans;
  return 0;
}

/* ===========================
 * MAIN de utilitário
 * =========================== */
static void usage(const char* prog){
  fprintf(stderr,
    "Uso:\n"
    "  %s IP PORT add A B\n"
    "\nExemplo:\n"
    "  %s 192.168.56.102 5000 add 7 35\n",
    prog, prog);
}

int main(int argc, char** argv){
  // Valida número mínimo de argumentos
  if (argc != 6){
    usage(argv[0]);
    return 1;
  }
  
  // Extrai argumentos comuns
  const char* ip = argv[1];
  int port = atoi(argv[2]);
  const char* cmd = argv[3];

  // Processa comando ADD
  if (strcmp(cmd, "add") == 0){
    int a = atoi(argv[4]);
    int b = atoi(argv[5]);
    int res = 0;
    
    // Chama função RPC e exibe resultado
    if (rpc_add(ip, port, a, b, &res) == 0){
      printf("%d + %d = %d\n", a, b, res);
      return 0;
    } else {
      fprintf(stderr, "falha na chamada rpc_add\n");
      return 2;
    }
  } else {
    fprintf(stderr, "comando desconhecido: %s\n", cmd);
    usage(argv[0]);
    return 1;
  }
}
