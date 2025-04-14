#define _GNU_SOURCE // Solução para hints

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "8008"
#define MAX 2000

void menu_principal()
{
    printf("\n===== Menu Principal =====\n");
    printf("1. Operacoes de Registro e Modificacao\n");
    printf("2. Operacoes de Consulta\n");
    printf("3. Sair\n");
    printf("Escolha uma opcao: ");
}

void menu_registro() {
    printf("\n--- Registro e Modificacao ---\n");
    printf("1. Cadastrar novo filme\n");
    printf("2. Adicionar novo genero a um filme existente\n");
    printf("3. Remover filme por ID\n");
    printf("4. Voltar ao menu principal\n");
    printf("Escolha uma opcao: ");
}

void menu_consulta()
{
    printf("\n--- Consultas ---\n");
    printf("1. Listar todos os filmes (ID e Titulo)\n");
    printf("2. Listar informacoes completas de todos os filmes\n");
    printf("3. Buscar filme por ID\n");
    printf("4. Listar filmes por genero\n");
    printf("5. Voltar ao menu principal\n");
    printf("Escolha uma opcao: ");
}

void func(int sockfd)
{
    char buff[MAX];
    int opcao_menu = 0;

    while (1)
    {
        menu_principal();
        fgets(buff, MAX, stdin);
        opcao_menu = atoi(buff);

        if (opcao_menu == 1)
        {
            menu_registro();
            fgets(buff, MAX, stdin);
            
            // Cadastrar novo filme
            if (strncmp(buff, "1", 1) == 0)
            {
                // Opção de cadastro no server.c
                write(sockfd, "1\n", 2);
                // Esperado Nome, Genero, Diretor e Ano
                for (int i = 0; i < 4; i++)
                {
                    bzero(buff, sizeof(buff));
                    read(sockfd, buff, sizeof(buff));
                    printf("%s", buff);
                    bzero(buff, sizeof(buff));
                    fgets(buff, MAX, stdin);
                    write(sockfd, buff, strlen(buff));
                }
                bzero(buff, sizeof(buff));
                read(sockfd, buff, sizeof(buff));
                printf("%s\n", buff);
            }

            // Adicionar novo genero a um filme existente
            else if (strncmp(buff, "2", 1) == 0)
            {
                //Envia pelo socket a opção
                write(sockfd, "8\n", 2);
                bzero(buff, sizeof(buff));
                read(sockfd, buff, sizeof(buff));
                printf("%s", buff);
                bzero(buff, sizeof(buff));
                fgets(buff, MAX, stdin);
                write(sockfd, buff, strlen(buff));

                bzero(buff, sizeof(buff));
                read(sockfd, buff, sizeof(buff));
                printf("%s", buff);
                bzero(buff, sizeof(buff));
                fgets(buff, MAX, stdin);
                write(sockfd, buff, strlen(buff));

                bzero(buff, sizeof(buff));
                read(sockfd, buff, sizeof(buff));
                printf("%s\n", buff);
            }
            // Remoção de filme por ID
            else if (strncmp(buff, "3", 1) == 0) {
                // Envio da opção pelo socket
                write(sockfd, "9\n", 2);
                bzero(buff, sizeof(buff));
                read(sockfd, buff, sizeof(buff));
                printf("%s", buff);
                bzero(buff, sizeof(buff));
                fgets(buff, MAX, stdin);
                write(sockfd, buff, strlen(buff));
                bzero(buff, sizeof(buff));
                read(sockfd, buff, sizeof(buff));
                printf("%s\n", buff);
            }
        }
        else if (opcao_menu == 2)
        {
            menu_consulta();
            fgets(buff, MAX, stdin);
            if (strncmp(buff, "1", 1) == 0)
            {
                write(sockfd, "4\n", 2);
                bzero(buff, sizeof(buff));
                read(sockfd, buff, sizeof(buff));
                printf("%s\n", buff);
            }
            else if (strncmp(buff, "2", 1) == 0)
            {
                write(sockfd, "5\n", 2);
                bzero(buff, sizeof(buff));
                read(sockfd, buff, sizeof(buff));
                printf("%s\n", buff);
            }
            else if (strncmp(buff, "3", 1) == 0)
            {
                write(sockfd, "6\n", 2);
                // Lê o prompt do servidor: "Digite o ID do filme"
                bzero(buff, sizeof(buff));
                read(sockfd, buff, sizeof(buff));
                printf("%s", buff);

                // Usuário envia ID
                bzero(buff, sizeof(buff));
                fgets(buff, MAX, stdin);
                write(sockfd, buff, strlen(buff));

                // Lê a resposta do servidor
                bzero(buff, sizeof(buff));
                read(sockfd, buff, sizeof(buff));
                printf("%s\n", buff);
            }
            else if (strncmp(buff, "4", 1) == 0)
            {
                write(sockfd, "7\n", 2);
                bzero(buff, sizeof(buff));
                read(sockfd, buff, sizeof(buff));
                printf("%s", buff);
                bzero(buff, sizeof(buff));
                fgets(buff, MAX, stdin);
                write(sockfd, buff, strlen(buff));
                bzero(buff, sizeof(buff));
                read(sockfd, buff, sizeof(buff));
                printf("%s\n", buff);
            }
        }
        else if (opcao_menu == 3)
        {
            write(sockfd, "exit\n", 5);
            printf("Encerrando cliente...\n");
            break;
        }
        else
        {
            printf("Opcao invalida. Tente novamente.\n");
        }
    }
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in *)sa)->sin_addr);
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s <hostname>\n", argv[0]);
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "client: falha ao conectar\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("Conectado ao servidor em %s\n", s);

    func(sockfd);

    close(sockfd);
    return 0;
}
