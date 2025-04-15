# Projeto 1 - Sistema de cadastro e consulta de filmes 
## MC833 - Projeto de Redes de Computadores
Repositório do projeto 1 de programação de redes 

Este projeto implementa um sistema cliente-servidor para cadastro, modificação e consulta de filmes, utilizando sockets TCP em C e armazenamento local com SQLite. Ele permite múltiplos acessos simultâneos, com controle de concorrência via `fork()` no servidor.

# Estrutura

- `client.c` — Código do cliente com menus interativos em terminal.
- `server.c` — Código do servidor responsável pelas operações no banco.


# Compilação

Execute:

```bash
make

Abra dois terminais e execute:

./server

e 

./client localhost

