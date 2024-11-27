/*
 * client.c -- TCP Socket Client
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

int parse_command(int argc, char *argv[], Command *cmd) {
    if (argc < 3) return -1;

    // Reset command structure
    memset(cmd, 0, sizeof(Command));

    // Parse command type
    if (strcmp(argv[1], "WRITE") == 0) {
        cmd->type = CMD_WRITE;
        if (argc != 4) return -1;
        strncpy(cmd->local_path, argv[2], sizeof(cmd->local_path) - 1);
        strncpy(cmd->remote_path, argv[3], sizeof(cmd->remote_path) - 1);
    } else if (strcmp(argv[1], "GET") == 0) {
        cmd->type = CMD_GET;
        if (argc != 4) return -1;
        strncpy(cmd->remote_path, argv[2], sizeof(cmd->remote_path) - 1);
        strncpy(cmd->local_path, argv[3], sizeof(cmd->local_path) - 1);
    } else if (strcmp(argv[1], "RM") == 0) {
        cmd->type = CMD_RM;
        if (argc != 3) return -1;
        strncpy(cmd->remote_path, argv[2], sizeof(cmd->remote_path) - 1);
    } else {
        cmd->type = CMD_INVALID;
        return -1;
    }

    return 0;
}

int send_file(int socket, const char *filepath) {
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        perror("Error opening local file");
        return -1;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    rewind(file);

    // Send file size
    if (send(socket, &filesize, sizeof(filesize), 0) < 0) {
        perror("Error sending file size");
        fclose(file);
        return -1;
    }

    // Send file contents
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(socket, buffer, bytes_read, 0) < 0) {
            perror("Error sending file data");
            fclose(file);
            return -1;
        }
    }

    fclose(file);
    return 0;
}

int receive_file(int socket, const char *filepath) {
    // Ensure directory exists
    char dir_path[PATH_MAX];
    strncpy(dir_path, filepath, sizeof(dir_path));
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        mkdir(dir_path, 0755);
    }

    // Receive file size
    long filesize;
    if (recv(socket, &filesize, sizeof(filesize), 0) < 0) {
        perror("Error receiving file size");
        return -1;
    }

    FILE *file = fopen(filepath, "wb");
    if (!file) {
        perror("Error creating local file");
        return -1;
    }

    // Receive file contents
    char buffer[BUFFER_SIZE];
    long bytes_received = 0;
    size_t chunk_size;

    while (bytes_received < filesize) {
        chunk_size = (filesize - bytes_received > BUFFER_SIZE) 
            ? BUFFER_SIZE : (filesize - bytes_received);
        
        ssize_t received = recv(socket, buffer, chunk_size, 0);
        if (received <= 0) {
            perror("Error receiving file data");
            fclose(file);
            return -1;
        }

        fwrite(buffer, 1, received, file);
        bytes_received += received;
    }

    fclose(file);
    return 0;
}

int main(int argc, char *argv[]) {
    Command cmd;
    // Usage Instrustions
    if (parse_command(argc, argv, &cmd) < 0) { 
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  rfs WRITE local-file remote-file\n");
        fprintf(stderr, "  rfs GET remote-file local-file\n");
        fprintf(stderr, "  rfs RM remote-file\n");
        return -1;
    }

    // Create socket
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc < 0) {
        perror("Unable to create socket");
        return -1;
    }

    // Configure server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    if (connect(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Unable to connect to server");
        close(socket_desc);
        return -1;
    }

    // Send command type
    if (send(socket_desc, &cmd, sizeof(cmd), 0) < 0) {
        perror("Error sending command");
        close(socket_desc);
        return -1;
    }

    // Handle specific command types
    int result = 0;
    switch (cmd.type) {
        case CMD_WRITE:
            result = send_file(socket_desc, cmd.local_path);
            break;
        case CMD_GET:
            result = receive_file(socket_desc, cmd.local_path);
            break;
        case CMD_RM: {
            // Wait for server response about deletion
            int status;
            recv(socket_desc, &status, sizeof(status), 0);
            result = status;
            break;
        }
        default:
            result = -1;
    }

    // Close socket
    close(socket_desc);

    return result;
}