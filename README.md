# TP1 - Operacao: Codebreaker

Sistema cliente-servidor de quebra de senhas implementado em C com POSIX sockets. O servidor guarda um código secreto de 5 dígitos e o cliente tenta descobri-lo através de palpites — a cada tentativa o servidor retorna dicas posicionais sobre os acertos.

TP1 da disciplina de Redes de Computadores — DCC/UFMG, 2026/1.

## Como compilar

```bash
make
```

Gera os binários `client` e `server` na raiz do projeto.

## Como usar

**Servidor:**
```bash
./server <protocolo> <porta> <senha>
```

- `<protocolo>`: `v4` para IPv4 ou `v6` para IPv6
- `<senha>`: sequência de 5 dígitos a ser descoberta

```bash
./server v4 51511 54321
```

**Cliente:**
```bash
./client <endereço ip> <porta>
```

```bash
./client 127.0.0.1 51511
```

## Exemplo de jogo

```
$ ./server v4 51511 54321
Servidor iniciado em modo IPv4 na porta 51511.

$ ./client 127.0.0.1 51511
Insira seu palpite:
> 92345
Dica: _ * 3 * *
Tentativas realizadas: 1
Insira seu palpite:
> 54321
Acesso concedido! Thaísa recuperou o sistema!
```

**Legenda das dicas:**
- `<dígito>` — número correto na posição certa
- `*` — número existe na senha, mas está na posição errada
- `_` — número não faz parte da senha

## Requisitos

- GCC
- Linux
- Make

## Estrutura

```
.
├── src/
│   ├── client.c
│   ├── client.h
│   ├── server.c
│   ├── server.h
│   └── util.h
└── Makefile
```
