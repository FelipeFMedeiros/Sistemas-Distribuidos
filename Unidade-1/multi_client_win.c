#define _WINSOCK_DEPRECATED_NO_WARNINGS  // Desabilita warnings de funções depreciadas do Winsock
#include <winsock2.h>   // Biblioteca para sockets no Windows
#include <windows.h>    // Biblioteca principal do Windows para threads
#include <stdio.h>
#include <stdlib.h>

/*
 * Cliente multi-thread para Windows (TCP e UDP)
 * - Cria N threads de cliente usando CreateThread.
 * - Cada thread envia "MSGBASE-<idx>" e tenta ler a resposta do servidor.
 * - Para UDP, configura timeout de recebimento (SO_RCVTIMEO).
 *
 * Uso:
 *   multi_client_win.exe tcp|udp IP PORTA N "MENSAGEM_BASE"
 *
 * Exemplos:
 *   multi_client_win.exe tcp 192.168.56.10 5000 20 "HELLO"
 *   multi_client_win.exe udp 192.168.56.10 6000 50 "PING"
 */

// Estrutura do job para cada thread
typedef struct { 
    int proto;      // Protocolo: 0=TCP, 1=UDP
    char ip[64];    // IP do servidor
    int port;       // Porta do servidor
    int idx;        // Índice da thread (identificador)
    char msg[512];  // Mensagem base a ser enviada
} job_t;

// Função executada por cada thread TCP
DWORD WINAPI run_tcp(LPVOID p) {
    job_t *j = (job_t *) p;  // Cast do parâmetro para job_t
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);  // Cria socket TCP
    
    // Configura estrutura do servidor
    SOCKADDR_IN srv;
    srv.sin_family = AF_INET;               // Família IPv4
    srv.sin_port = htons(j->port);          // Converte porta para network byte order
    srv.sin_addr.s_addr = inet_addr(j->ip); // Converte IP string para formato binário

    // Tenta conectar ao servidor
    if (connect(s, (SOCKADDR *) &srv, sizeof srv) == 0) {
        char buf[1024];
        // Formata mensagem com índice da thread
        _snprintf(buf, 1024, "%s-%d", j->msg, j->idx);
        send(s, buf, (int) strlen(buf), 0);     // Envia dados para servidor
        int n = recv(s, buf, 1023, 0);          // Recebe resposta do servidor
        if (n > 0) {
            buf[n] = '\0';  // Termina string com null
            printf("[TCP %d] %s\n", j->idx, buf);
        }
    }
    closesocket(s);  // Fecha socket
    free(j);         // Libera memória do job
    return 0;
}

// Função executada por cada thread UDP
DWORD WINAPI run_udp(LPVOID p) {
    job_t *j = (job_t *) p;  // Cast do parâmetro para job_t
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);  // Cria socket UDP
    
    int to = 10000;  // Timeout de 10 segundos
    // Define timeout para recepção de dados
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *) &to, sizeof to);
    
    // Configura estrutura do servidor (igual ao TCP)
    SOCKADDR_IN srv;
    srv.sin_family = AF_INET;
    srv.sin_port = htons(j->port);
    srv.sin_addr.s_addr = inet_addr(j->ip);

    char buf[1024];
    // Formata mensagem com índice da thread
    _snprintf(buf, 1024, "%s-%d", j->msg, j->idx);
    // Envia dados via UDP (sem conexão)
    sendto(s, buf, (int) strlen(buf), 0, (SOCKADDR *) &srv, sizeof srv);
    
    int sl = sizeof srv;
    // Recebe resposta via UDP
    int n = recvfrom(s, buf, 1023, 0, (SOCKADDR *) &srv, &sl);
    if (n > 0) {
        buf[n] = '\0';
        printf("[UDP %d] %s\n", j->idx, buf);  // Imprime resposta
    }
    else {
        printf("[UDP %d] timeout\n", j->idx);  // Imprime se houve timeout
    }
    closesocket(s);  // Fecha socket
    free(j);         // Libera memória
    return 0;
}

int main(int argc, char **argv) {
    // Verifica se tem argumentos suficientes
    if (argc < 6) {
        fprintf(stderr, "uso: %s tcp|udp IP PORTA N MSG\n", argv[0]);
        return 1;
    }

    WSADATA w; 
    WSAStartup(MAKEWORD(2, 2), &w);  // Inicializa Winsock versão 2.2
    
    // Processa argumentos da linha de comando
    int proto = _stricmp(argv[1], "udp") == 0 ? 1 : 0;  // Define protocolo
    const char *ip = argv[2];     // IP do servidor
    int port = atoi(argv[3]);     // Porta do servidor
    int N = atoi(argv[4]);        // Número de threads/clientes
    const char *base = argv[5];   // Mensagem base

    // Aloca array de handles para as threads
    HANDLE *th = (HANDLE *) malloc(sizeof(HANDLE) * N);
    
    // Cria N threads
    for (int i = 0; i < N; i++) {
        // Aloca e inicializa job para cada thread
        job_t *j = (job_t *) calloc(1, sizeof * j); 
        j->proto = proto; 
        strncpy(j->ip, ip, 63);      // Copia IP
        j->port = port; 
        j->idx = i + 1;              // Índice da thread (começa em 1)
        strncpy(j->msg, base, 511);  // Copia mensagem base
        
        // Cria thread TCP ou UDP baseado no protocolo
        th[i] = CreateThread(NULL, 0, proto ? run_udp : run_tcp, j, 0, NULL);
    }
    
    // Espera todas as threads terminarem
    WaitForMultipleObjects(N, th, TRUE, INFINITE);
    
    // Fecha handles das threads
    for (int i = 0; i < N; i++) CloseHandle(th[i]);
    free(th);        // Libera array de handles
    WSACleanup();    // Finaliza Winsock
    return 0;
}