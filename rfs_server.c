/*
 * server.c -- TCP Socket Server
 *
 * adapted from:
 *   https://www.educative.io/answers/how-to-implement-tcp-sockets-in-c
 */

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "rfs.h"
#include <signal.h>
#include <sys/stat.h>
#include <limits.h>

int socket_desc;  // Global socket descriptor for signal handling

void signal_handler(int sig) {
    write(STDERR_FILENO, "Caught SIGINT!\n", 15);
    close(socket_desc);
    exit(0);
}

void create_server_directories(const char *filepath) {
    char dir_path[PATH_MAX];
    strncpy(dir_path, filepath, sizeof(dir_path));

    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';

        // Use mkdir -p equivalent in C
        char tmp_path[PATH_MAX];
        snprintf(tmp_path, sizeof(tmp_path), "%s", dir_path);
        char *p = NULL;
        size_t len = strlen(tmp_path);

        // Remove trailing slash if it exists
        if (tmp_path[len - 1] == '/')
            tmp_path[len - 1] = '\0';

        for (p = tmp_path + 1; *p; p++) {
            if (*p == '/') {
                *p = '\0';
                mkdir(tmp_path, 0755);
                *p = '/';
            }
        }
        mkdir(tmp_path, 0755);
    }
}

int delete_file_or_directory(const char *filepath) {
    struct stat path_stat;
    if (stat(filepath, &path_stat) != 0) {
        perror("File/directory does not exist");
        return -1;
    }

    if (S_ISDIR(path_stat.st_mode)) {
        // Attempt to remove directory
        return rmdir(filepath);
    } else {
        // Attempt to delete file
        return unlink(filepath);
    }
}

int main(void) {
    int client_sock;
    socklen_t client_size;
    struct sockaddr_in server_addr, client_addr;

    // Set up the signal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction failed");
        return -1;
    }

    // Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc < 0) {
        perror("Error creating socket");
        return -1;
    }

    // Reuse port to prevent "Address already in use" error
    int enable = 1;
    if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        return -1;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket
    if (bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Couldn't bind to port");
        return -1;
    }

    // Create server root directory if it doesn't exist
    mkdir(SERVER_ROOT, 0755);

    // Listen for clients
    if (listen(socket_desc, 5) < 0) {
        perror("Error while listening");
        close(socket_desc);
        return -1;
    }

    printf("Remote File System Server started. Listening on port %d...\n", PORT);

    while (1) {
        // Accept incoming connection
        client_size = sizeof(client_addr);
        client_sock = accept(socket_desc, (struct sockaddr*)&client_addr, &client_size);

        if (client_sock < 0) {
            perror("Can't accept connection");
            continue;
        }

        // Receive command
        Command cmd;
        if (recv(client_sock, &cmd, sizeof(cmd), 0) <= 0) {
            perror("Error receiving command");
            close(client_sock);
            continue;
        }

        // Construct full server-side path
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s%s", SERVER_ROOT, cmd.remote_path);

        int result = 0;
        switch (cmd.type) {
            case CMD_WRITE: {
                // Ensure server directory exists
                create_server_directories(full_path);

                // Receive file
                FILE *file = fopen(full_path, "wb");
                if (!file) {
                    perror("Error creating server file");
                    result = -1;
                    break;
                }

                // Receive file size
                long filesize;
                if (recv(client_sock, &filesize, sizeof(filesize), 0) <= 0) {
                    perror("Error receiving file size");
                    fclose(file);
                    result = -1;
                    break;
                }

                // Receive file contents
                char buffer[BUFFER_SIZE];
                long bytes_received = 0;
                ssize_t received;
                while (bytes_received < filesize) {
                    received = recv(client_sock, buffer, BUFFER_SIZE, 0);
                    if (received <= 0) {
                        perror("Error receiving file data");
                        fclose(file);
                        result = -1;
                        break;
                    }
                    fwrite(buffer, 1, received, file);
                    bytes_received += received;
                }

                fclose(file);
                break;
            }
            case CMD_GET: {
                // Open file to send
                FILE *file = fopen(full_path, "rb");
                if (!file) {
                    perror("Error opening server file");
                    result = -1;
                    break;
                }

                // Get file size
                fseek(file, 0, SEEK_END);
                long filesize = ftell(file);
                rewind(file);

                // Send file size
                if (send(client_sock, &filesize, sizeof(filesize), 0) <= 0) {
                    perror("Error sending file size");
                    fclose(file);
                    result = -1;
                    break;
                }

                // Send file contents
                char buffer[BUFFER_SIZE];
                size_t bytes_read;
                while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                    if (send(client_sock, buffer, bytes_read, 0) <= 0) {
                        perror("Error sending file data");
                        fclose(file);
                        result = -1;
                        break;
                    }
                }

                fclose(file);
                break;
            }
            case CMD_RM: {
                // Delete file or directory
                result = delete_file_or_directory(full_path);

                // Send deletion status back to client
                send(client_sock, &result, sizeof(result), 0);
                break;
            }
            default:
                result = -1;
        }

        // Close client connection
        close(client_sock);
    }

    // Close server socket (this will never be reached in this implementation)
    close(socket_desc);

    return 0;
}