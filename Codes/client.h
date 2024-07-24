#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>

#define CTRL_PORT 21  // Control channel port
#define DATA_PORT 20  // Data channel port
#define PATH_MAX 1000 // maximum path file size


// function declarations
void printCommands();
int cwd(const char *path);
char* get_ip_and_port(struct sockaddr_in *addr);
int receive_file(int data_socket, char *filepath);
int send_file(int data_socket, char* filename);
void list_files(int data_socket);