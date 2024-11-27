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

#define PORT 2024
#define SERVER_ROOT "./server_root/"
#define BUFFER_SIZE 8192

// Command types
typedef enum {
    CMD_WRITE,
    CMD_GET,
    CMD_RM,
    CMD_INVALID
} CommandType;

// Command structure
typedef struct {
    CommandType type;
    char local_path[PATH_MAX];
    char remote_path[PATH_MAX];
} Command;

// Function prototypes
int parse_command(int argc, char *argv[], Command *cmd);
int send_file(int socket, const char *filepath);
int receive_file(int socket, const char *filepath);
void create_server_directories(const char *filepath);
int delete_file_or_directory(const char *filepath);

#endif // RFS_H