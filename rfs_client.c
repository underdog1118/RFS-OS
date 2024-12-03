/*
 * Name: Zhengpeng Qiu and Blake Koontz
 * Course: CS5600
 * Semester: Fall 2024
 *
 * rfs_client.c -- TCP Socket Client
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

/* 
 * Add multithreading support for concurrent client operations 
 */
#include <pthread.h>
#include <semaphore.h>

/**
 * parse_command - Parse and validate command-line arguments
 * @argc: Number of command-line arguments
 * @argv: Array of command-line argument strings
 * @cmd: Pointer to Command structure to be populated
 * 
 * Parses the command type (WRITE, GET, RM) and validates arguments
 * Returns 0 on success, -1 on invalid input
 */
int parse_command(int argc, char *argv[], Command *cmd) {
    // Validate minimum number of arguments
    if (argc < 3) return -1;

    // Reset command structure to ensure clean state
    memset(cmd, 0, sizeof(Command));

    // Identify and process command type
    if (strcmp(argv[1], "WRITE") == 0) {
        // WRITE command: local-file to remote-file
        cmd->type = CMD_WRITE;
        if (argc != 4) return -1;
        strncpy(cmd->local_path, argv[2], sizeof(cmd->local_path) - 1);
        strncpy(cmd->remote_path, argv[3], sizeof(cmd->remote_path) - 1);
    } else if (strcmp(argv[1], "GET") == 0) {
        // GET command: remote-file to local-file
        cmd->type = CMD_GET;
        if (argc != 4) return -1;
        strncpy(cmd->remote_path, argv[2], sizeof(cmd->remote_path) - 1);
        strncpy(cmd->local_path, argv[3], sizeof(cmd->local_path) - 1);
    } else if (strcmp(argv[1], "RM") == 0) {
        // RM command: remove remote-file
        cmd->type = CMD_RM;
        if (argc != 3) return -1;
        strncpy(cmd->remote_path, argv[2], sizeof(cmd->remote_path) - 1);
    } else {
        // Invalid command
        cmd->type = CMD_INVALID;
        return -1;
    }

    return 0;
}

/**
 * send_file - Send file contents over a socket
 * @socket: Connected socket descriptor
 * @filepath: Path to the file to be sent
 * 
 * Sends file size followed by file contents in chunks
 * Returns 0 on success, -1 on error
 */
int send_file(int socket, const char *filepath) {
    // Open file in binary read mode
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        perror("Error opening local file");
        return -1;
    }

    // Determine file size
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    rewind(file);

    // Send file size to receiver
    if (send(socket, &filesize, sizeof(filesize), 0) < 0) {
        perror("Error sending file size");
        fclose(file);
        return -1;
    }

    // Send file contents in chunks
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

/**
 * receive_file - Receive file contents from a socket
 * @socket: Connected socket descriptor
 * @filepath: Path where file will be saved
 * 
 * Receives file size and contents, creates directories if needed
 * Returns 0 on success, -1 on error
 */
int receive_file(int socket, const char *filepath) {
    // Create directory path if it doesn't exist
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

    // Open file for writing
    FILE *file = fopen(filepath, "wb");
    if (!file) {
        perror("Error creating local file");
        return -1;
    }

    // Receive file contents in chunks
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

/**
 * clientthread - Thread function to handle client socket operations
 * @args: Pointer to Command structure containing operation details
 * 
 * Establishes socket connection and performs file operations 
 * based on the command type (WRITE, GET, RM)
 */
void* clientthread(void* args){
    // Extract command details from thread argument
    Command cmd = *((Command*)args);
    printf("\n*Client clientthread func*");
    printf("\nCommand local path:%s",cmd.local_path);
    printf("\nCommand remote path:%s",cmd.remote_path);

    // Create socket
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc < 0) {
        perror("Unable to create socket");
        pthread_exit(NULL);
    }

    // Configure server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Establish connection to server
    if (connect(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Unable to connect to server");
        close(socket_desc);
        pthread_exit(NULL);
    }

    // Send command details to server
    if (send(socket_desc, &cmd, sizeof(cmd), 0) < 0) {
        perror("Error sending command");
        close(socket_desc);
        pthread_exit(NULL);
    }

    // Execute command based on type
    switch (cmd.type) {
        case CMD_WRITE:
            send_file(socket_desc, cmd.local_path);
            break;
        case CMD_GET:
            receive_file(socket_desc, cmd.local_path);
            break;
        case CMD_RM: {
            // Wait for server response about deletion
            int status;
            recv(socket_desc, &status, sizeof(status), 0);
            break;
        }
        default:
            break;
    }

    // Close socket and exit thread
    close(socket_desc);
    pthread_exit(NULL);
    return 0; 
}

/**
 * main - Entry point for the remote file system client
 * @argc: Number of command-line arguments
 * @argv: Array of command-line argument strings
 * 
 * Parses command, creates thread to handle socket operations
 */
int main(int argc, char *argv[]) {
    Command cmd;
    int result = 0;
    pthread_t tid;

    // Validate and parse command-line arguments
    if (parse_command(argc, argv, &cmd) < 0) { 
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  rfs WRITE local-file remote-file\n");
        fprintf(stderr, "  rfs GET remote-file local-file\n");
        fprintf(stderr, "  rfs RM remote-file\n");
        return -1;
    }

    // Print command details for debugging
    printf("\n*Client Main func*");
    printf("\nCommand local path:%s",cmd.local_path);
    printf("\nCommand remote path:%s",cmd.remote_path);

    // Create thread to handle client socket operations
    pthread_create(&tid,
                   NULL,
                   clientthread,
                   &cmd);
    sleep(1);
    pthread_join(tid, NULL);

    return result;
}