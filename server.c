/*
 * Servidor TCP para Streaming de Filmes com ID baseado em hash do título
 * Verifica duplicidade de ID antes de cadastrar
 * Permite adicionar novo gênero a um filme existente
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <arpa/inet.h>
 #include <pthread.h>
 #include <json-c/json.h>
 
 #define PORT 8080
 #define BUFFER_SIZE 1024
 #define FILE_NAME "filmes.json"
 
 pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
 
 unsigned long hash_title(const char *str) {
     unsigned long hash = 5381;
     int c;
     while ((c = *str++)) {
         hash = ((hash << 5) + hash) + c;
     }
     return hash;
 }
 
 int id_exists(struct json_object *movies_array, unsigned long id) {
     int length = json_object_array_length(movies_array);
     for (int i = 0; i < length; i++) {
         struct json_object *movie = json_object_array_get_idx(movies_array, i);
         struct json_object *movie_id;
         if (json_object_object_get_ex(movie, "id", &movie_id)) {
             if ((unsigned long)json_object_get_int64(movie_id) == id) {
                 return 1;
             }
         }
     }
     return 0;
 }
 
 void add_movie(const char *title, const char *genre, const char *director, int year) {
     pthread_mutex_lock(&lock);
     FILE *file = fopen(FILE_NAME, "r");
     struct json_object *json, *movies_array;
 
     if (file) {
         fseek(file, 0, SEEK_END);
         long size = ftell(file);
         rewind(file);
         char *content = malloc(size + 1);
         fread(content, 1, size, file);
         fclose(file);
         content[size] = '\0';
         json = json_tokener_parse(content);
         free(content);
     } else {
         json = json_object_new_object();
     }
 
     if (!json_object_object_get_ex(json, "movies", &movies_array)) {
         movies_array = json_object_new_array();
         json_object_object_add(json, "movies", movies_array);
     }
 
     unsigned long id = hash_title(title);
     if (id_exists(movies_array, id)) {
         printf("Filme com ID %lu ja existe, ignorando cadastro.\n", id);
         pthread_mutex_unlock(&lock);
         json_object_put(json);
         return;
     }
 
     struct json_object *new_movie = json_object_new_object();
     json_object_object_add(new_movie, "id", json_object_new_int64(id));
     json_object_object_add(new_movie, "title", json_object_new_string(title));
     json_object_object_add(new_movie, "genre", json_object_new_string(genre));
     json_object_object_add(new_movie, "director", json_object_new_string(director));
     json_object_object_add(new_movie, "year", json_object_new_int(year));
     json_object_array_add(movies_array, new_movie);
 
     file = fopen(FILE_NAME, "w");
     fprintf(file, "%s", json_object_to_json_string_ext(json, JSON_C_TO_STRING_PRETTY));
     fclose(file);
     json_object_put(json);
     pthread_mutex_unlock(&lock);
 }
 
 void add_genre_to_movie(unsigned long id, const char *new_genre, int client_sock) {
     pthread_mutex_lock(&lock);
     FILE *file = fopen(FILE_NAME, "r");
     if (!file) {
         send(client_sock, "Arquivo de filmes nao encontrado!\n", 34, 0);
         pthread_mutex_unlock(&lock);
         return;
     }
 
     fseek(file, 0, SEEK_END);
     long size = ftell(file);
     rewind(file);
     char *content = malloc(size + 1);
     fread(content, 1, size, file);
     fclose(file);
     content[size] = '\0';
 
     struct json_object *json = json_tokener_parse(content);
     free(content);
 
     struct json_object *movies_array;
     if (!json_object_object_get_ex(json, "movies", &movies_array)) {
         send(client_sock, "Nenhum filme cadastrado!\n", 27, 0);
         json_object_put(json);
         pthread_mutex_unlock(&lock);
         return;
     }
 
     int updated = 0;
     int len = json_object_array_length(movies_array);
     for (int i = 0; i < len; i++) {
         struct json_object *movie = json_object_array_get_idx(movies_array, i);
         struct json_object *movie_id;
         if (json_object_object_get_ex(movie, "id", &movie_id) && (unsigned long)json_object_get_int64(movie_id) == id) {
             struct json_object *genre_obj;
             if (json_object_object_get_ex(movie, "genre", &genre_obj)) {
                 const char *current = json_object_get_string(genre_obj);
                 char new_genre_combined[256];
                 snprintf(new_genre_combined, sizeof(new_genre_combined), "%s,%s", current, new_genre);
                 json_object_object_set(movie, "genre", json_object_new_string(new_genre_combined));
                 updated = 1;
                 break;
             }
         }
     }
 
     if (updated) {
         file = fopen(FILE_NAME, "w");
         fprintf(file, "%s", json_object_to_json_string_ext(json, JSON_C_TO_STRING_PRETTY));
         fclose(file);
         send(client_sock, "Genero adicionado ao filme.\n", 29, 0);
     } else {
         send(client_sock, "Filme nao encontrado!\n", 25, 0);
     }
 
     json_object_put(json);
     pthread_mutex_unlock(&lock);
 }
 
 void list_movies(int client_sock) {
     pthread_mutex_lock(&lock);
     FILE *file = fopen(FILE_NAME, "r");
     if (!file) {
         send(client_sock, "Nenhum filme cadastrado!\n", 27, 0);
         pthread_mutex_unlock(&lock);
         return;
     }
 
     fseek(file, 0, SEEK_END);
     long size = ftell(file);
     rewind(file);
     char *content = malloc(size + 1);
     fread(content, 1, size, file);
     fclose(file);
     content[size] = '\0';
 
     struct json_object *json = json_tokener_parse(content);
     free(content);
     struct json_object *movies_array;
 
     if (!json_object_object_get_ex(json, "movies", &movies_array)) {
         send(client_sock, "Nenhum filme cadastrado!\n", 27, 0);
         json_object_put(json);
         pthread_mutex_unlock(&lock);
         return;
     }
 
     const char *movies_str = json_object_to_json_string_ext(movies_array, JSON_C_TO_STRING_PRETTY);
     send(client_sock, movies_str, strlen(movies_str), 0);
     json_object_put(json);
     pthread_mutex_unlock(&lock);
 }
 
 void *handle_client(void *socket_desc) {
     int client_sock = *(int*)socket_desc;
     char buffer[BUFFER_SIZE];
     int read_size;
 
     while ((read_size = recv(client_sock, buffer, BUFFER_SIZE, 0)) > 0) {
         buffer[read_size] = '\0';
         printf("Cliente: %s\n", buffer);
 
         if (strncmp(buffer, "ADD", 3) == 0) {
             char title[100], genre[50], director[100];
             int year;
             sscanf(buffer + 4, "%99[^,],%49[^,],%99[^,],%d", title, genre, director, &year);
             add_movie(title, genre, director, year);
             send(client_sock, "Filme adicionado (ou ja existente).\n", 35, 0);
         } else if (strncmp(buffer, "LIST", 4) == 0) {
             list_movies(client_sock);
         } else if (strncmp(buffer, "ADDGENRE", 8) == 0) {
             unsigned long id;
             char genre[50];
             sscanf(buffer + 9, "%lu,%49[^\n]", &id, genre);
             add_genre_to_movie(id, genre, client_sock);
         } else {
             send(client_sock, "Comando invalido!\n", 20, 0);
         }
     }
 
     close(client_sock);
     free(socket_desc);
     pthread_exit(NULL);
 }
 
 int main() {
     int server_sock, client_sock, *new_sock;
     struct sockaddr_in server, client;
     socklen_t c = sizeof(struct sockaddr_in);
 
     server_sock = socket(AF_INET, SOCK_STREAM, 0);
     if (server_sock == -1) {
         perror("Erro ao criar socket");
         return 1;
     }
 
     server.sin_family = AF_INET;
     server.sin_addr.s_addr = INADDR_ANY;
     server.sin_port = htons(PORT);
 
     if (bind(server_sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
         perror("Erro ao fazer bind");
         return 1;
     }
 
     listen(server_sock, 5);
     printf("Servidor rodando na porta %d...\n", PORT);
 
     while ((client_sock = accept(server_sock, (struct sockaddr *)&client, &c))) {
         pthread_t thread;
         new_sock = malloc(sizeof(int));
         *new_sock = client_sock;
         pthread_create(&thread, NULL, handle_client, (void*) new_sock);
         pthread_detach(thread);
     }
 
     if (client_sock < 0) {
         perror("Erro ao aceitar conexao");
         return 1;
     }
 
     close(server_sock);
     return 0;
 }
 