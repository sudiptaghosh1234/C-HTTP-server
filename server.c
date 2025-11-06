#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define WEB_ROOT "./www"

// Function to detect content type
const char* get_content_type(const char* path) {
    if (strstr(path, ".html")) return "text/html";
    if (strstr(path, ".css")) return "text/css";
    if (strstr(path, ".js")) return "application/javascript";
    if (strstr(path, ".png")) return "image/png";
    if (strstr(path, ".jpg")) return "image/jpeg";
    if (strstr(path, ".txt")) return "text/plain";
    return "application/octet-stream";
}

// Function to send HTTP response
void send_response(int client_socket, const char* header, const char* content_type, const char* body, size_t body_length) {
    char response_header[BUFFER_SIZE];
    snprintf(response_header, sizeof(response_header),
             "%s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n",
             header, content_type, body_length);

    send(client_socket, response_header, strlen(response_header), 0);
    send(client_socket, body, body_length, 0);
}

// Function to serve files from www/
void serve_file(int client_socket, const char* path) {
    char full_path[1024];
    if (strcmp(path, "/") == 0) {
        snprintf(full_path, sizeof(full_path), "%s/index.html", WEB_ROOT);
    } else {
        snprintf(full_path, sizeof(full_path), "%s%s", WEB_ROOT, path);
    }

    FILE* file = fopen(full_path, "rb");
    if (!file) {
        const char* not_found = "<h1>404 Not Found</h1>";
        send_response(client_socket, "HTTP/1.1 404 Not Found", "text/html", not_found, strlen(not_found));
        return;
    }

    // Get file size
    struct stat st;
    stat(full_path, &st);
    size_t file_size = st.st_size;

    // Read file content
    char* file_buffer = malloc(file_size);
    fread(file_buffer, 1, file_size, file);
    fclose(file);

    // Send file to browser
    send_response(client_socket, "HTTP/1.1 200 OK", get_content_type(full_path), file_buffer, file_size);

    free(file_buffer);
}

// Function to handle client requests (runs in a thread)
void* handle_client(void* arg) {
    int client_socket = *((int*)arg);
    free(arg);

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    read(client_socket, buffer, sizeof(buffer) - 1);

    printf("📩 Thread [%ld] handling request:\n%s\n", pthread_self(), buffer);

    char method[16], path[1024];
    sscanf(buffer, "%s %s", method, path);

    if (strcmp(method, "GET") == 0) {
        serve_file(client_socket, path);
    } else {
        const char* not_allowed = "<h1>405 Method Not Allowed</h1>";
        send_response(client_socket, "HTTP/1.1 405 Method Not Allowed", "text/html", not_allowed, strlen(not_allowed));
    }

    close(client_socket);
    printf("✅ Thread [%ld] finished.\n", pthread_self());
    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("🚀 Multi-threaded C Server running on http://localhost:%d\n", PORT);

    // Accept loop
    while (1) {
        int client_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        // Create a thread for each new connection
        pthread_t tid;
        int* pclient = malloc(sizeof(int));
        *pclient = client_socket;
        pthread_create(&tid, NULL, handle_client, pclient);
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}
