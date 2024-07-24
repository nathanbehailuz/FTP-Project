#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <ctype.h>

#define CTRL_PORT 21    // Control channel port
#define DATA_PORT 20    // Data channel port
#define INPUT_LEN 10    // Maximum size of username and password
#define MAX_CLIENTS 3   // Maximum number of clients

// Structure to store user information
struct users {
    char names[MAX_CLIENTS][INPUT_LEN];    // list of users names
    char passwords[MAX_CLIENTS][INPUT_LEN]; // list of users passwords
    int count;
};

// Structure to store state of each connected user
struct user_state {
    int fd;                    // File descriptor for the user socket
    char cwd[256];             // Current working directory of the user
    int valid_name;            // Flag to check if the username is valid
    int valid_password;        // Flag to check if the password is valid
    char username[INPUT_LEN];  // Username of the user
    char client_ip[16];        // IP address of the user
    unsigned short client_port;// Port number of the user
};

// Array to store the state of each connected client
struct user_state clients[MAX_CLIENTS];

typedef struct users Users;

// Function declarations
void load_users(const char* filename, Users* users);
int authenticate_user(char* name, Users* userslist);
int authenticate_password(char* password, Users* userslist, int idx);
void cwd(const char *path, char* cwd);
void printStructContents(struct user_state * user);
int change_directory(const char *user_cwd, char *original_cwd, char *new_cwd);
int store_file(int data_socket, char* filename);
int list_files(int data_socket);
void get_ip_and_port(const char* ip_port_str, int* ip_parts, int* port_parts);
