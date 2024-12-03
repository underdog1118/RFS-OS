/*
 * Name: Zhengpeng Qiu and Blake Koontz
 * Course: CS5600
 * Semester: Fall 2024
 * 
 * rfs.h - Header file for Remote File System (RFS) project
 * Defines constants, data structures, and function prototypes
 */

#ifndef RFS_H
#define RFS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>

// Network and system configuration constants
#define PORT 2024                // Port number for socket communication
#define SERVER_ROOT "./server_root/"  // Base directory for server-side file storage
#define BUFFER_SIZE 8192         // Standard buffer size for file transfers

/**
 * Enumeration of supported command types
 * Defines the operations that can be performed in the remote file system
 */
typedef enum {
    CMD_WRITE,   // Upload a file to the server
    CMD_GET,     // Download a file from the server
    CMD_RM,      // Remove a file from the server
    CMD_INVALID  // Invalid or unsupported command
} CommandType;

/**
 * Command structure to represent client file system operations
 * Stores command type and file paths for local and remote files
 */
typedef struct {
    CommandType type;             // Type of operation to be performed
    char local_path[PATH_MAX];    // Path to file on local filesystem
    char remote_path[PATH_MAX];   // Path to file on remote filesystem
} Command;

/**
 * Server thread data structure
 * Combines command information, client socket, and full file path
 * Used for passing data to server-side thread handling client requests
 */
typedef struct {
    Command cmd;                  // Command details
    int client_sock;              // Client socket descriptor
    char full_path[PATH_MAX];     // Fully resolved path on server
} ServerThreadData;

// Function prototypes for client-side operations
/**
 * Parse command-line arguments into a Command structure
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @param cmd Pointer to Command structure to be populated
 * @return 0 on success, -1 on invalid input
 */
int parse_command(int argc, char *argv[], Command *cmd);

/**
 * Send file contents over a socket
 * @param socket Connected socket descriptor
 * @param filepath Path to the file to be sent
 * @return 0 on success, -1 on error
 */
int send_file(int socket, const char *filepath);

/**
 * Receive file contents from a socket
 * @param socket Connected socket descriptor
 * @param filepath Path where file will be saved
 * @return 0 on success, -1 on error
 */
int receive_file(int socket, const char *filepath);

/**
 * Create directory structure for a given file path
 * Ensures all parent directories exist
 * @param filepath Full path for which directories should be created
 */
void create_server_directories(const char *filepath);

/**
 * Delete a file or directory
 * @param filepath Path to file or directory to be deleted
 * @return 0 on success, -1 on error
 */
int delete_file_or_directory(const char *filepath);

#endif // RFS_H