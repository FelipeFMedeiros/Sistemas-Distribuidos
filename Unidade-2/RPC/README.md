# RPC Server/Client - Operação ADD

Sistema RPC simples implementado em C para Linux, utilizando TCP e protocolo binário.

## Requisitos

- GCC
- Linux
- pthread

## Compilação

```bash
# Compilar o servidor
gcc rpc_server.c -o rpc_server -pthread

# Compilar o cliente
gcc rpc_client.c -o rpc_client
```

## Uso

### Iniciar o servidor

```bash
./rpc_server <PORTA>
```

Exemplo:
```bash
./rpc_server 5000
```

O servidor ficará escutando na porta especificada e processará requisições de forma concorrente (uma thread por cliente).

### Executar o cliente

```bash
./rpc_client <IP> <PORTA> add <A> <B>
```

Exemplo:
```bash
./rpc_client 10.10.0.11 5000 add 7 35
```

Saída:
```
7 + 35 = 42
```

## Características

- **Protocolo**: TCP com mensagens binárias (big-endian)
- **Operação**: ADD - soma dois inteiros
- **Multithread**: O servidor cria uma thread por conexão
- **Simulação**: Processamento lento de 3 segundos por requisição
- **Plataforma**: Linux

## Estrutura do Protocolo

**Header (8 bytes)**:
- `op` (4 bytes): Código da operação (1 = ADD)
- `len` (4 bytes): Tamanho do payload

**Payload ADD**:
- Request: 2 inteiros de 32 bits (8 bytes)
- Response: 1 inteiro de 32 bits (4 bytes)
