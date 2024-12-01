 /*
 * Name: Zhengpeng Qui and Blake Koontz
 * Course: CS5600
 * Semester: Fall 2024
 *
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
/*
* Multithread change
*/
#include <pthread.h>
#include <semaphore.h>

sem_t x, y;
pthread_t tid;
pthread_t writethreads[100];
pthread_t getthreads[100];
pthread_t deletethreads[100];
int readercount = 0;

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
        if (tmp_path[len - 1] == '/') {
            tmp_path[len - 1] = '\0';
        }

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

//void* process_get(void* server_thread_data){
void* process_get(void* server_thread_data) {
  printf("\nRead is trying to enter");
  //Lock semaphore
  sem_wait(&x);

  //ServerThreadData* data = server_thread_data;
  ServerThreadData data = *((ServerThreadData*)server_thread_data);
  printf("\nRead entered");
  //readercount++;
  //if (readercount == 1){
 //   sem_wait(&y);
 // }
  // Open file to send
  FILE *file = fopen(data.full_path, "rb");
  //FILE *file = fopen(full_path, "rb");
  if (!file) {
    perror("Error opening server file");
    //result = -1;
    //return -1;
  }

  // Get file size
  fseek(file, 0, SEEK_END);
  long filesize = ftell(file);
  rewind(file);

  // Send file size
  //if (send(client_sock, &filesize, sizeof(filesize), 0) <= 0) {
  if (send(data.client_sock, &filesize, sizeof(filesize), 0) <= 0) {
    perror("Error sending file size");
    fclose(file);
    //result = -1;
    //break;
    //return;
  }

  // Send file contents
  char buffer[BUFFER_SIZE];
  size_t bytes_read;
  while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
    //if (send(client_sock, buffer, bytes_read, 0) <= 0) {
    if (send(data.client_sock, buffer, bytes_read, 0) <= 0) {
      perror("Error sending file data");
      fclose(file);
      //result = -1;
      //break;
      //return;
    }
  }

  fclose(file);

  //Unlock semaphore
  sem_post(&x);
  //printf("\n%d Reader is inside",readercount);
  sleep(1);

  //Lock semaphore
  /*
  sem_wait(&x);
  readercount--;
  if (readercount == 0){
    sem_post(&y);
  }
  */

  /*
  //Lock semaphore
  sem_post(&x);
  printf("\n%d Reader is leaving",readercount + 1);
  */
  printf("\nRead is leaving");
  pthread_exit(NULL);
  //return;
}

// Writer Function
//void* process_write(void* param)
void* process_write(void* server_thread_data) {
    printf("\nWriter is trying to enter");

    // Lock the semaphore
    sem_wait(&y);


    //ServerThreadData* data = server_thread_data;
    ServerThreadData data = *((ServerThreadData*)server_thread_data);
    printf("\nprocess_write func");
    printf("\ndata client_sock:%d",data.client_sock);


    printf("\nWriter has entered");

   // Receive file
   //FILE *file = fopen(server_thread_data->full_path, "wb");
   FILE *file = fopen(data.full_path, "wb");
   if (!file) {
     //perror("Error creating server file");
     //result = -1;
     //break;
     //return;
   }

   // Receive file size
   long filesize;
   //if (recv(client_sock, &filesize, sizeof(filesize), 0) <= 0) {
   if (recv(data.client_sock, &filesize, sizeof(filesize), 0) <= 0) {
     perror("Error receiving file size");
     fclose(file);
     //result = -1;
     //break;
   }

    // Unlock the semaphore
    sem_post(&y);

    printf("\nWriter is leaving");
    pthread_exit(NULL);
    //return;
}

//void* process_delete(void* param) {
void* process_delete(void* server_thread_data) {
  printf("\nDelete started");

  // Lock the semaphore
  sem_wait(&y);

  //ServerThreadData* data = server_thread_data;
  ServerThreadData data = *((ServerThreadData*)server_thread_data);
  // Delete file or directory
  //result = delete_file_or_directory(full_path);
  int result = 0;
  result = delete_file_or_directory(data.full_path);

  // Send deletion status back to client
  //send(client_sock, &result, sizeof(result), 0);
  send(data.client_sock, &result, sizeof(result), 0);

  printf("\nDelete has entered");
  // Lock the semaphore
  sem_post(&y);

  printf("\nDelete is leaving");
  pthread_exit(NULL);
  //return;
}

/*
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
*/

int main(void) {
    int client_sock;
    socklen_t client_size;
    struct sockaddr_in server_addr, client_addr;

    /*
    * Multithread Change
    */
    sem_init(&x, 0, 1);
    sem_init(&y, 0, 1);

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


    int i = 0;
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
        int max_connections = 50;


        // Assemble for thread
        ServerThreadData server_thread_data;
        //server_thread_data->cmd = cmd;
        //server_thread_data.cmd = cmd;
        memcpy(&server_thread_data.cmd,&cmd,sizeof(cmd));
        memcpy(&server_thread_data.client_sock,&client_sock,sizeof(client_sock));
        //server_thread_data.full_path = full_path;
        //strcpy(server_thread_data.full_path, full_path);
        snprintf(server_thread_data.full_path, sizeof(server_thread_data.full_path), "%s%s", SERVER_ROOT, cmd.remote_path);

        printf("\nServer main func");
        printf("\nserver thread local path:%s",server_thread_data.cmd.local_path);
        printf("\nserver thread remote path:%s",server_thread_data.cmd.remote_path);
        printf("\nserver thread full_path:%s",server_thread_data.full_path);
        printf("\nserver thread client sock:%d",server_thread_data.client_sock);

        switch (cmd.type) {
            case CMD_WRITE: {

                /*
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
                */
                //if (pthread_create(&writethreads[i++], NULL, process_write, &client_sock) != 0) {
                if (pthread_create(&writethreads[i++], NULL, process_write, &server_thread_data) != 0) {
                  printf("\nFailed to create write thread");
                  result = -1;
                }
                break;
            }
            case CMD_GET: {
                /*
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
                */

                //if (pthread_create(&getthreads[i++], NULL, process_get, &client_sock) != 0) {
                if (pthread_create(&getthreads[i++], NULL, process_get, &server_thread_data) != 0) {
                  printf("\nFailed to create get thread");
                  result = -1;
                }
                break;
            }
            case CMD_RM: {

                /*
                // Delete file or directory
                result = delete_file_or_directory(full_path);

                // Send deletion status back to client
                send(client_sock, &result, sizeof(result), 0);

                */
                //if (pthread_create(&deletethreads[i++], NULL, process_delete, &client_sock) != 0) {
                if (pthread_create(&deletethreads[i++], NULL, process_delete, &server_thread_data) != 0) {
                  printf("\nFailed to create delete thread");
                  result = -1;
                }
                break;
            }
            default:
                result = -1;
        }
        if(i >= max_connections) {
          i = 0;
          while (i < max_connections) {
            pthread_join(writethreads[i++],NULL);
            pthread_join(getthreads[i++],NULL);
            pthread_join(deletethreads[i++],NULL);
          }
          i = 0;
        }

        printf("\nresult:%d",result);
        // Close client connection
        close(client_sock);
    }

    // Close server socket (this will never be reached in this implementation)
    close(socket_desc);

    return 0;
}
