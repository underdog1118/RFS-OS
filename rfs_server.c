/*
 * Name: Zhengpeng Qiu and Blake Koontz
 * Course: CS5600
 * Semester: Fall 2024
 *
 * rfs_server.c -- TCP Socket Server
 *
 * Multithreaded remote file system server implementation
 * Supports write, get, and delete operations with reader-writer synchronization
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

/* Multithreading and synchronization libraries */
#include <pthread.h>
#include <semaphore.h>

// Semaphores for reader-writer synchronization
sem_t x, y;  // x: read lock, y: write lock
pthread_t tid;

// Thread arrays for different operation types
pthread_t writethreads[100];
pthread_t getthreads[100];
pthread_t deletethreads[100];

int readercount = 0;  // Number of concurrent readers

// Global socket descriptor for signal handling
int socket_desc;

/**
 * Signal handler for graceful server shutdown
 * Closes socket and exits on SIGINT (Ctrl+C)
 * 
 * @param sig Signal number received
 */
void signal_handler(int sig) {
    write(STDERR_FILENO, "Caught SIGINT!\n", 15);
    close(socket_desc);
    exit(0);
}

/**
 * Recursively create directory structure for a given file path
 * Ensures all parent directories exist before file creation
 * 
 * @param filepath Full path for which directories should be created
 */
void create_server_directories(const char *filepath) {
    char dir_path[PATH_MAX];
    strncpy(dir_path, filepath, sizeof(dir_path));

    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';

        // Implement mkdir -p equivalent
        char tmp_path[PATH_MAX];
        snprintf(tmp_path, sizeof(tmp_path), "%s", dir_path);
        char *p = NULL;
        size_t len = strlen(tmp_path);

        // Remove trailing slash if it exists
        if (tmp_path[len - 1] == '/') {
            tmp_path[len - 1] = '\0';
        }

        // Create directories recursively
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

/**
 * Delete a file or directory
 * 
 * @param filepath Path to file or directory to be deleted
 * @return 0 on success, -1 on error
 */
int delete_file_or_directory(const char *filepath) {
    struct stat path_stat;
    if (stat(filepath, &path_stat) != 0) {
        perror("File/directory does not exist");
        return -1;
    }

    // Check if path is a directory or file and delete accordingly
    if (S_ISDIR(path_stat.st_mode)) {
        return rmdir(filepath);
    } else {
        return unlink(filepath);
    }
}

/**
 * Thread function to handle file retrieval (GET) operation
 * Implements reader synchronization using semaphores
 * 
 * @param server_thread_data Pointer to ServerThreadData containing operation details
 * @return 0 on completion
 */
void* process_get(void* server_thread_data) {   
    printf("\nRead is trying to enter");
    
    // Acquire read lock
    sem_wait(&x);

    ServerThreadData data = *((ServerThreadData*)server_thread_data);
    printf("\nRead entered");

    int open = 1;
    FILE *file = fopen(data.full_path, "rb");
    if (!file) {
        perror("Error opening server file");
    } else {
        // Get file size
        fseek(file, 0, SEEK_END);
        long filesize = ftell(file);
        rewind(file);

        // Send file size to client
        if (send(data.client_sock, &filesize, sizeof(filesize), 0) <= 0) {
            perror("Error sending file size");
            open = 0;
            fclose(file);
        } else {
            // Send file contents in chunks
            char buffer[BUFFER_SIZE];
            size_t bytes_read;
            while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                if (send(data.client_sock, buffer, bytes_read, 0) <= 0) {
                    perror("Error sending file data");
                    open = 0;
                    fclose(file);
                    break;
                }
            }
        }
    }

    // Close file if still open
    if(open == 1) {
        fclose(file);
    }

    // Release read lock
    sem_post(&x);
    
    printf("\nRead is leaving");
    close(data.client_sock);
    pthread_exit(NULL);
    return 0;
}

/**
 * Thread function to handle file write (WRITE) operation
 * Implements writer synchronization using semaphores
 * 
 * @param server_thread_data Pointer to ServerThreadData containing operation details
 * @return 0 on completion
 */
void* process_write(void* server_thread_data) {
    printf("\nWriter is trying to enter");

    // Acquire write lock
    sem_wait(&y);

    ServerThreadData data = *((ServerThreadData*)server_thread_data);
    printf("\nWriter has entered");

    // Ensure directory structure exists
    create_server_directories(data.full_path);

    // Open file for writing
    FILE *file = fopen(data.full_path, "wb");
    if (!file) {
        perror("Error creating server file");
    } else {
        // Receive file size
        long filesize;
        if (recv(data.client_sock, &filesize, sizeof(filesize), 0) <= 0) {
            perror("Error receiving file size");
            fclose(file);
        } else {
            // Receive file contents in chunks
            char buffer[BUFFER_SIZE];
            long bytes_received = 0;
            ssize_t received;
            while (bytes_received < filesize) {
                received = recv(data.client_sock, buffer, BUFFER_SIZE, 0);
                if (received <= 0) {
                    perror("Error receiving file data");
                    fclose(file);
                    break;
                }
                fwrite(buffer, 1, received, file);
                bytes_received += received;
            }
            fclose(file);
        }
    }

    // Release write lock
    sem_post(&y);

    printf("\nWriter is leaving");
    close(data.client_sock);
    pthread_exit(NULL);
    return 0;
}

/**
 * Thread function to handle file deletion (RM) operation
 * Uses write lock for exclusive access
 * 
 * @param server_thread_data Pointer to ServerThreadData containing operation details
 * @return 0 on completion
 */
void* process_delete(void* server_thread_data) {
    printf("\nDelete started");

    // Acquire write lock
    sem_wait(&y);

    ServerThreadData data = *((ServerThreadData*)server_thread_data);
    
    // Attempt to delete file or directory
    int result = delete_file_or_directory(data.full_path);

    // Send deletion status back to client
    send(data.client_sock, &result, sizeof(result), 0);

    // Release write lock
    sem_post(&y);

    printf("\nDelete is leaving");
    close(data.client_sock);
    pthread_exit(NULL);
    return 0;
}

/**
 * Main server function
 * Sets up socket, handles client connections, and spawns threads for operations
 * 
 * @return 0 on successful execution (never reaches in this implementation)
 */
int main(void) {
    int client_sock;
    socklen_t client_size;
    struct sockaddr_in server_addr, client_addr;

    // Initialize semaphores for synchronization
    sem_init(&x, 0, 1);
    sem_init(&y, 0, 1);

    // Set up signal handler for graceful shutdown
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

    // Allow socket reuse to prevent "Address already in use" errors
    int enable = 1;
    if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        return -1;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to address
    if (bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Couldn't bind to port");
        return -1;
    }

    // Create server root directory
    mkdir(SERVER_ROOT, 0755);

    // Start listening for connections
    if (listen(socket_desc, 50) < 0) {
        perror("Error while listening");
        close(socket_desc);
        return -1;
    }

    printf("Remote File System Server started. Listening on port %d...\n", PORT);

    int i = 0;
    int max_connections = 50;

    // Main server loop
    while (1) {
        // Accept incoming connection
        client_size = sizeof(client_addr);
        client_sock = accept(socket_desc, (struct sockaddr*)&client_addr, &client_size);

        if (client_sock < 0) {
            perror("Can't accept connection");
            continue;
        }

        // Receive command from client
        Command cmd;
        if (recv(client_sock, &cmd, sizeof(cmd), 0) <= 0) {
            perror("Error receiving command");
            close(client_sock);
            continue;
        }

        // Prepare thread data
        ServerThreadData server_thread_data;
        memcpy(&server_thread_data.cmd, &cmd, sizeof(cmd));
        memcpy(&server_thread_data.client_sock, &client_sock, sizeof(client_sock));
        snprintf(server_thread_data.full_path, sizeof(server_thread_data.full_path), "%s%s", SERVER_ROOT, cmd.remote_path);

        // Create thread based on command type
        int result = 0;
        switch (cmd.type) {
            case CMD_WRITE:
                pthread_create(&writethreads[i++], NULL, process_write, &server_thread_data);
                break;
            case CMD_GET:
                pthread_create(&getthreads[i++], NULL, process_get, &server_thread_data);
                break;
            case CMD_RM:
                pthread_create(&deletethreads[i++], NULL, process_delete, &server_thread_data);
                break;
            default:
                result = -1;
        }

        // Reset connection counter and join threads if max reached
        if(i >= max_connections) {
            i = 0;
            while (i < max_connections) {
                pthread_join(writethreads[i++], NULL);
                pthread_join(getthreads[i++], NULL);
                pthread_join(deletethreads[i++], NULL);
            }
            i = 0;
        }
    }

    // Close server socket (unreachable in this implementation)
    close(socket_desc);

    return 0;
}