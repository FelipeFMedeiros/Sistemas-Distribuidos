# TRABALHO SISTEMAS DISTRIBUÍDOS - CENÁRIO 2
**Disciplina:** Sistemas Distribuídos  
**Cenário Implementado:** 2 (Cliente multithreads + Servidor)  
**Ambiente:** VPS Ubuntu (servidor) + Windows (cliente)  

## DESCRIÇÃO DO PROJETO

Este projeto implementa o **Cenário 2** do trabalho de Sistemas Distribuídos, onde temos:
- **1 máquina servidora** (VPS Ubuntu) executando servidores TCP e UDP
- **1 máquina cliente** (Windows) executando cliente multithread que simula múltiplos clientes simultâneos

### Arquitetura Implementada
```
[Windows - Cliente Multithread]  ←→  [VPS Ubuntu - Servidores TCP/UDP]
     multi_client_win.exe              tcp_server + udp_server
```

## ARQUIVOS DO PROJETO

### 1. `tcp_server.c` - Servidor TCP
**Executa em:** VPS Ubuntu  
**Funcionalidade:**
- Escuta conexões TCP na porta especificada
- Para cada cliente conectado, cria uma thread separada
- Simula processamento com `sleep(5)` para demonstrar concorrência
- Retorna mensagem de eco com ID da thread

**Como funciona:**
1. Aceita conexão TCP do cliente
2. Cria thread worker para processar a requisição
3. Recebe mensagem do cliente
4. Processa por 5 segundos (simulando trabalho)
5. Retorna: `"OK TCP thr=<thread_id> eco: <mensagem_recebida>"`

### 2. `udp_server.c` - Servidor UDP
**Executa em:** VPS Ubuntu  
**Funcionalidade:**
- Escuta datagramas UDP na porta especificada
- Para cada datagram recebido, cria uma thread separada
- Simula processamento com `sleep(5)` para demonstrar concorrência
- Retorna mensagem de eco com ID da thread

**Como funciona:**
1. Recebe datagram UDP do cliente
2. Cria thread worker para processar a requisição
3. Processa por 5 segundos (simulando trabalho)
4. Retorna: `"OK UDP thr=<thread_id> eco: <mensagem_recebida>"`

### 3. `multi_client_win.c` - Cliente Multithread
**Executa em:** Windows  
**Funcionalidade:**
- Cria N threads simultaneamente (especificado pelo usuário)
- Cada thread simula um cliente independente
- Suporta tanto TCP quanto UDP
- Envia mensagens numeradas para identificar cada cliente

**Como funciona:**
1. Cria N threads simultâneas
2. Cada thread conecta ao servidor (TCP ou UDP)
3. Envia mensagem: `"<base_message>-<numero_thread>"`
4. Aguarda resposta e exibe resultado
5. Aguarda todas as threads terminarem

## DEMONSTRAÇÃO DE CONCORRÊNCIA

### Por que o `sleep(5)` é importante?
O `sleep(5)` nos servidores simula um processamento demorado. Isso permite observar que:
- **Múltiplos clientes são atendidos simultaneamente**
- **Cada cliente é processado em uma thread separada**
- **O servidor não bloqueia enquanto processa um cliente**

### Identificação de Threads
Cada resposta inclui o ID da thread (`thr=<id>`), permitindo verificar que diferentes clientes são processados por threads diferentes.

## COMO TESTAR E VERIFICAR RESULTADOS

### Passo 1: Iniciar Servidores (VPS Ubuntu)
```bash
# Terminal 1 - Servidor TCP
gcc tcp_server.c -o tcp_server -pthread
./tcp_server 5000

# Terminal 2 - Servidor UDP  
gcc udp_server.c -o udp_server -pthread
./udp_server 6000
```

### Passo 2: Executar Clientes (Windows)
```cmd
# Compilar cliente
cl multi_client_win.c /Fe:multi_client_win.exe ws2_32.lib

# Teste TCP com 5 clientes simultâneos
multi_client_win.exe tcp <IP_VPS> 5000 5 "mensagem_teste"

# Teste UDP com 5 clientes simultâneos  
multi_client_win.exe udp <IP_VPS> 6000 5 "ping_teste"
```

### Passo 3: Observar Resultados

**No cliente (Windows):**
```
[TCP 1] OK TCP thr=140234567890 eco: mensagem_teste-1
[TCP 2] OK TCP thr=140234567891 eco: mensagem_teste-2
[TCP 3] OK TCP thr=140234567892 eco: mensagem_teste-3
[TCP 4] OK TCP thr=140234567893 eco: mensagem_teste-4
[TCP 5] OK TCP thr=140234567894 eco: mensagem_teste-5
```

**No servidor (Ubuntu):**
```
[TCP] escutando 0.0.0.0:5000
[TCP] conexão 203.0.113.10:54321
[TCP] conexão 203.0.113.10:54322
[TCP] conexão 203.0.113.10:54323
[TCP] recebido de 203.0.113.10:54321: mensagem_teste-1
[TCP] recebido de 203.0.113.10:54322: mensagem_teste-2
[TCP] processando 203.0.113.10:54321...
[TCP] processando 203.0.113.10:54322...
```

## EVIDÊNCIAS DE CONCORRÊNCIA

### 1. **Múltiplas Conexões Simultâneas**
- Servidor aceita várias conexões ao mesmo tempo
- Logs mostram conexões sendo estabelecidas rapidamente

### 2. **Processamento Paralelo**
- Diferentes IDs de thread nas respostas
- Múltiplas mensagens "processando..." aparecem simultaneamente

### 3. **Tempos de Resposta**
- Com 5 clientes: ~5 segundos total (não 25 segundos sequenciais)
- Demonstra que o processamento é paralelo, não sequencial

### 4. **Comportamento TCP vs UDP**
- **TCP:** Conexão mantida, entrega garantida
- **UDP:** Sem conexão, mais rápido, com timeout de segurança

## TECNOLOGIAS UTILIZADAS

- **Linguagem:** C
- **Threads:** pthread (Linux), Windows Threads (Windows)
- **Sockets:** BSD Sockets (Linux), Winsock2 (Windows)
- **Compilação:** GCC (Linux), MSVC (Windows)

## ESTRUTURA DO CENÁRIO 2

✅ **Servidor único** rodando em VPS Ubuntu  
✅ **Cliente multithread** simulando múltiplos clientes  
✅ **Comunicação TCP e UDP**  
✅ **Demonstração de concorrência com sleep(5)**  
✅ **Threads separadas para cada requisição**  
✅ **Logs detalhados para verificação**  

Este projeto demonstra efetivamente o atendimento simultâneo de múltiplos clientes através de programação multithread, cumprindo todos os requisitos do Cenário 2.