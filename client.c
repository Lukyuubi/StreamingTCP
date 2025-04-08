#include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <arpa/inet.h>
 
 #define SERVER_IP "127.0.0.1"
 #define PORT 8080
 #define BUFFER_SIZE 1024
 
 void menu() {
     printf("\n===== Menu do Cliente =====\n");
     printf("1. Cadastrar novo filme\n");
     printf("2. Listar todos os filmes\n");
     printf("3. Sair\n");
     printf("Escolha uma opcao: ");
 }
 
 int main() {
     int sock;
     struct sockaddr_in server;
     char buffer[BUFFER_SIZE];
 
     // Criando socket TCP
     sock = socket(AF_INET, SOCK_STREAM, 0);
     if (sock == -1) {
         perror("Erro ao criar socket");
         return 1;
     }
 
     server.sin_family = AF_INET;
     server.sin_addr.s_addr = inet_addr(SERVER_IP);
     server.sin_port = htons(PORT);
 
     // Conectando ao servidor
     if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
         perror("Erro ao conectar ao servidor");
         return 1;
     }
 
     printf("Conectado ao servidor TCP.\n");
 
     int opcao;
     while (1) {
         menu();
         scanf("%d", &opcao);
         getchar();
 
         if (opcao == 1) {
             char titulo[100], genero[50], diretor[100];
             int ano;
 
             printf("Titulo: ");
             fgets(titulo, sizeof(titulo), stdin);
             titulo[strcspn(titulo, "\n")] = 0;
 
             printf("Genero: ");
             fgets(genero, sizeof(genero), stdin);
             genero[strcspn(genero, "\n")] = 0;
 
             printf("Diretor: ");
             fgets(diretor, sizeof(diretor), stdin);
             diretor[strcspn(diretor, "\n")] = 0;
 
             printf("Ano de lancamento: ");
             scanf("%d", &ano);
             getchar();
 
             snprintf(buffer, sizeof(buffer), "ADD %s,%s,%s,%d", titulo, genero, diretor, ano);
             send(sock, buffer, strlen(buffer), 0);
 
             int recv_size = recv(sock, buffer, BUFFER_SIZE - 1, 0);
             if (recv_size > 0) {
                 buffer[recv_size] = '\0';
                 printf("\nServidor: %s\n", buffer);
             }
 
         } else if (opcao == 2) {
             strcpy(buffer, "LIST");
             send(sock, buffer, strlen(buffer), 0);
 
             int recv_size = recv(sock, buffer, BUFFER_SIZE - 1, 0);
             if (recv_size > 0) {
                 buffer[recv_size] = '\0';
                 printf("\nLista de Filmes:\n%s\n", buffer);
             }
 
         } else if (opcao == 3) {
             printf("Encerrando conexao.\n");
             break;
         } else {
             printf("Opcao invalida!\n");
         }
     }
 
     close(sock);
     return 0;
 }