#define _GNU_SOURCE // Solução para hints

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sqlite3.h>

#define PORT "8008"
#define BACKLOG 10
#define MAX 1024

sqlite3 *db;

void sigchld_handler(int s) {
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) return &(((struct sockaddr_in*)sa)->sin_addr);
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void init_db() {
    char *err_msg = NULL;

    const char *drop_sql = "DROP TABLE IF EXISTS filmes;";
    if (sqlite3_exec(db, drop_sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "Erro ao remover tabela: %s\n", err_msg);
        sqlite3_free(err_msg);
        exit(1);
    }

    const char *create_sql = "CREATE TABLE filmes ("
                             "id INTEGER PRIMARY KEY,"
                             "titulo TEXT NOT NULL,"
                             "genero TEXT NOT NULL,"
                             "diretor TEXT NOT NULL,"
                             "ano INTEGER NOT NULL"
                             ");";
    if (sqlite3_exec(db, create_sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "Erro ao criar tabela: %s\n", err_msg);
        sqlite3_free(err_msg);
        exit(1);
    }
}

void func(int connfd) {
    char buff[MAX];

    for (;;) {
        bzero(buff, MAX);
        read(connfd, buff, sizeof(buff));

        if (strncmp("exit", buff, 4) == 0) break;

        if (strncmp("1", buff, 1) == 0) {  // Cadastrar
            char titulo[MAX], genero[MAX], diretor[MAX], ano_str[MAX];
            write(connfd, "Titulo:\n", 8);
            read(connfd, titulo, sizeof(titulo));
            write(connfd, "Genero:\n", 8);
            read(connfd, genero, sizeof(genero));
            write(connfd, "Diretor:\n", 9);
            read(connfd, diretor, sizeof(diretor));
            write(connfd, "Ano:\n", 5);
            read(connfd, ano_str, sizeof(ano_str));

            int ano = atoi(ano_str);
            sqlite3_stmt *stmt;
            const char *sql = "INSERT INTO filmes (titulo, genero, diretor, ano) VALUES (?, ?, ?, ?);";
            sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
            sqlite3_bind_text(stmt, 1, titulo, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, genero, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, diretor, -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 4, ano);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            // Envia ao cliente mensagem de que o filme foi corretamente cadastrado
            char msg[] = "Filme cadastrado.\n";
            write(connfd, msg, sizeof(msg));

        } else if (strncmp("4", buff, 1) == 0) {  // Listar todos
            sqlite3_stmt *stmt;
            const char *sql = "SELECT id, titulo FROM filmes;";
            sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

            char response[MAX] = "";
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                char row[MAX];
                snprintf(row, MAX, "ID: %d | Titulo: %s\n",
                         sqlite3_column_int(stmt, 0),
                         sqlite3_column_text(stmt, 1));
                strcat(response, row);
            }
            sqlite3_finalize(stmt);
            strcat(response, "Digite a operacao desejada:\n");
            write(connfd, response, strlen(response));
        }
        else if (strncmp("5", buff, 1) == 0) {  // Listar todas as informações dos filmes
            sqlite3_stmt *stmt;
            const char *sql = "SELECT titulo, genero, diretor, ano FROM filmes;";
            sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        
            char response[MAX] = "";
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                char row[MAX];
                snprintf(row, MAX, "Titulo: %s | Genero: %s | Diretor: %s | Ano: %d\n",
                         sqlite3_column_text(stmt, 0),
                         sqlite3_column_text(stmt, 1),
                         sqlite3_column_text(stmt, 2),
                         sqlite3_column_int(stmt, 3));
                strcat(response, row);
            }
            sqlite3_finalize(stmt);
            strcat(response, "Digite a operacao desejada:\n");
            write(connfd, response, strlen(response));
        }
        else if (strncmp("6", buff, 1) == 0) {  // Buscar filme por ID
            char id_str[MAX];
            int id;
            sqlite3_stmt *stmt;
        
            write(connfd, "Digite o ID do filme:\n", 23);
            read(connfd, id_str, sizeof(id_str));
            id = atoi(id_str);
        
            const char *sql = "SELECT titulo, genero, diretor, ano FROM filmes WHERE id = ?;";
            sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
            sqlite3_bind_int(stmt, 1, id);
        
            char response[MAX] = "";
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                snprintf(response, MAX,
                         "Titulo: %s\nGenero: %s\nDiretor: %s\nAno: %d\n",
                         sqlite3_column_text(stmt, 0),
                         sqlite3_column_text(stmt, 1),
                         sqlite3_column_text(stmt, 2),
                         sqlite3_column_int(stmt, 3));
            } else {
                snprintf(response, MAX, "Filme com ID %d nao encontrado.\n", id);
            }
        
            sqlite3_finalize(stmt);
            write(connfd, response, strlen(response));
        }
        
        else {
            write(connfd, "Comando invalido.\n", 45);
        }
    }
}

int main(void) {
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    struct sigaction sa;
    int sockfd, new_fd, yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    sqlite3_open("filmes.db", &db);
    init_db();

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("Servidor aguardando conexoes na porta %s...\n", PORT);

    while (1) {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("Servidor: conexao de %s\n", s);

        if (!fork()) {
            close(sockfd);
            func(new_fd);
            close(new_fd);
            exit(0);
        }
        close(new_fd);
    }

    sqlite3_close(db);
    return 0;
}
