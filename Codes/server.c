#include "server.h"

int main() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0); // Create a socket for the server

    if (server_socket < 0) {
        perror("socket:");
        exit(EXIT_FAILURE);
    }

    // Enable the socket to reuse the address
    int value = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));

    // Define server address structure
    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(CTRL_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the specified address and port
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) < 0) {
        perror("listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Declare file descriptor sets
    fd_set all_sockets;
    fd_set ready_sockets;

    // Initialize the set of all sockets
    FD_ZERO(&all_sockets);

    // Add the server socket to the set of all sockets
    FD_SET(server_socket, &all_sockets);

    Users users;
    load_users("../Server/users.csv", &users); // Load user data from a file

    // Define original_cwd to store the parent directory of the 'code' folder
    char original_cwd[256];
    if (getcwd(original_cwd, sizeof(original_cwd)) == NULL) {
        perror("getcwd() error");
        exit(EXIT_FAILURE);
    }

    // Move up one directory level and change to Server directory
    if (chdir("../Server") != 0) {
        perror("chdir() error");
        exit(EXIT_FAILURE);
    }

    // Get the parent directory
    if (getcwd(original_cwd, sizeof(original_cwd)) == NULL) {
        perror("getcwd() error");
        exit(EXIT_FAILURE);
    }

    int valid_name = 0; // Flag to track if a valid username has been provided
    char username[INPUT_LEN];
    int idx = -1;

    // Initialize user states
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;  // -1 indicates an unused slot
    }

    while (1) {
        ready_sockets = all_sockets; // Copy all_sockets to ready_sockets

        if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0) {
            perror("select error");
            exit(EXIT_FAILURE);
        }

        for (int fd = 0; fd < FD_SETSIZE; fd++) {
            // Check if that fd is set
            if (FD_ISSET(fd, &ready_sockets)) {
                if (fd == server_socket) {
                    struct sockaddr_in client_address;
                    socklen_t client_len = sizeof(client_address);
                    
                    // Accept new connection
                    int client_sd = accept(server_socket, (struct sockaddr*)&client_address, &client_len);
                    if (client_sd >= 0) {
                        printf("Connection established with user %d\n", client_sd);
                        printf("Their Port: %d\n\n", ntohs(client_address.sin_port));

                        // Find an empty slot in clients array
                        for (int i = 0; i < MAX_CLIENTS; i++) {
                            if (clients[i].fd == -1) {
                                clients[i].fd = client_sd;
                                strcpy(clients[i].cwd, "/");  // Set initial directory
                                clients[i].valid_name = 0;
                                clients[i].valid_password = 0;
                                strcpy(clients[i].client_ip, inet_ntoa(client_address.sin_addr));
                                clients[i].client_port = ntohs(client_address.sin_port);
                                FD_SET(client_sd, &all_sockets);
                                break;
                            }
                        }
                    }
                } else {
                    // Handle data from an existing client
                    char buffer[256];
                    bzero(buffer, sizeof(buffer));
                    int bytes = recv(fd, buffer, sizeof(buffer), 0);
                    if (bytes <= 0) {
                        // Close connection and remove from clients array
                        close(fd);
                        FD_CLR(fd, &all_sockets);
                        for (int i = 0; i < MAX_CLIENTS; i++) {
                            if (clients[i].fd == fd) {
                                clients[i].fd = -1;
                                break;
                            }
                        }
                        continue;
                    }

                    buffer[strcspn(buffer, "\n")] = 0; // Remove newline character
                    char* arg_1 = strtok(buffer, " ");
                    char* arg_2 = strtok(NULL, " ");

                    // Find the corresponding user state
                    struct user_state* user = NULL;
                    for (int i = 0; i < MAX_CLIENTS; i++) {
                        if (clients[i].fd == fd) {
                            user = &clients[i];
                            break;
                        }
                    }

                    if (user == NULL) continue;

                    // Handle commands
                    // handle user name input and verifiacation 
                    if (strcmp(arg_1, "USER") == 0) {
                        int idx = authenticate_user(arg_2, &users);
                        if (idx != -1) {
                            strcpy(user->username, users.names[idx]);
                            user->valid_name = 1;
                            printf("Successful username verification\n\n");
                            char response[] = "331 Username OK, need password.\n";
                            send(fd, response, strlen(response), 0);
                        } else {
                            char response[] = "332 Need account for login.\n";
                            send(fd, response, strlen(response), 0);
                        }
                    } 
                    // handle user password verification 
                    else if (strcmp(arg_1, "PASS") == 0) {
                        if (user->valid_name) {
                            int idx = authenticate_user(user->username, &users);
                            if (authenticate_password(arg_2, &users, idx)) {
                                user->valid_password = 1;
                                snprintf(user->cwd, sizeof(user->cwd), "%s", user->username);
                                char user_dir[512];
                                snprintf(user_dir, sizeof(user_dir), "/%s", user->cwd);
                                chdir(user_dir);
                                printf("Successful login\n\n");
                                char response[] = "230 User logged in, proceed.\n";
                                send(fd, response, strlen(response), 0);
                            } else {
                                user->valid_name = 0;
                                char response[] = "530 Not logged in.\n";
                                send(fd, response, strlen(response), 0);
                            }
                        } else {
                            char response[] = "503 Bad sequence of commands.\n";
                            send(fd, response, strlen(response), 0);
                        }
                    } 
                    // changing working directory
                    else if (strcmp(arg_1, "CWD") == 0) {
                        if (user->valid_password) {
                            if (arg_2 && strlen(arg_2) > 0) {
                                char new_dir[512];
                                int pathdiff;
                        
                                if (arg_2[0] != '/' || strcmp(arg_2, "..") != 0 ) { 
                                    snprintf(new_dir, sizeof(new_dir), "/%s/%s/%s", original_cwd, user->cwd, arg_2);
                                    pathdiff = strlen(new_dir) - strlen(original_cwd);
                                }
                                
                                struct stat sb;
                                if (stat(new_dir, &sb) == 0 && S_ISDIR(sb.st_mode)) {
                                    if (chdir(new_dir) == 0) {
                                    // Update the user's current working directory
                                    char response[256];
                                    snprintf(user->cwd, sizeof(user->cwd), "%s/%s", user->cwd, arg_2);
                                    snprintf(response, sizeof(response), "200 directory successfully changed to %s \n", user->cwd);
                                    printf("%s \n", user->cwd);
                                    send(fd, response, strlen(response), 0);
                                } else {
                                    char response[] = "550 Failed to change directory.\n";
                                    send(fd, response, strlen(response), 0);
                                }

                                } else {
                                    char response[] = "550 Failed to change directory. Directory does not exist.\n";
                                    send(fd, response, strlen(response), 0);
                                }
                            } else {
                                char response[] = "550 Failed to change directory. No directory specified.\n";
                                send(fd, response, strlen(response), 0);
                            }
                        } else {
                            char response[] = "530 Not logged in.\n";
                            send(fd, response, strlen(response), 0);
                        }
                    }
                    // printing elements inside a folder
                   else if (strcmp(arg_1, "LIST") == 0) {
                        // Handle LIST command to list directory contents
                        if (user->valid_password) {
                            char new_cwd[512];

                            // Get current working directory
                            if (getcwd(new_cwd, sizeof(new_cwd)) == NULL) {
                                perror("getcwd() error");
                                continue;
                            }

                            // Construct user's directory path
                            char user_dir[512];
                            snprintf(user_dir, sizeof(user_dir), "/%s", user->cwd);
                            if(change_directory(user_dir, original_cwd, new_cwd)){
                                printf("File okay, beginning data connections.\n");
                            }

                            // Change to the new directory
                            chdir(new_cwd);

                            printf("Connecting to Client Transfer Socket...\n");
                            int pid = fork();
                            if (pid == 0) {
                                // Child process to handle data transfer
                                int data_socket = socket(AF_INET, SOCK_STREAM, 0);
                                if (data_socket < 0) {
                                    perror("data socket creation failed");
                                    exit(EXIT_FAILURE);
                                }

                                struct sockaddr_in client_address;
                                bzero(&client_address, sizeof(client_address));
                                client_address.sin_family = AF_INET;
                                client_address.sin_port = htons(user->client_port);
                                if (inet_pton(AF_INET, user->client_ip, &client_address.sin_addr) <= 0) {
                                    perror("inet_pton failed");
                                    exit(EXIT_FAILURE);
                                }

                                if (connect(data_socket, (struct sockaddr*)&client_address, sizeof(client_address)) < 0) {
                                    perror("data socket connection failed");
                                    exit(EXIT_FAILURE);
                                }

                                printf("Connection Successful\n");

                                // Send response to client
                                char response[] = "150 file status okay; about to open data connection.\n";
                                send(fd, response, strlen(response), 0);

                                // List files and send to client
                                if(list_files(data_socket)){
                                    printf("226 Transfer Complete \n\n");
                                }
                                close(data_socket);
                                exit(EXIT_SUCCESS);
                            }

                            // Restore original working directory
                            if (chdir(original_cwd) != 0) {
                                perror("chdir() restore error");
                            }

                        } else {
                            // Send error if not logged in
                            char response[] = "530 Not logged in.\n";
                            send(fd, response, strlen(response), 0);
                        }
                    } 

                    // store a file in server
                    else if (strcmp(arg_1, "RETR") == 0) {
                        // Handle RETR command to retrieve a file
                        if (user->valid_password) {
                            char new_cwd[512];

                            // Get current working directory
                            if (getcwd(new_cwd, sizeof(new_cwd)) == NULL) {
                                perror("getcwd() error");
                                continue;
                            }

                            // Construct user's directory path
                            char user_dir[512];
                            snprintf(user_dir, sizeof(user_dir), "/%s", user->cwd);
                            change_directory(user_dir, original_cwd, new_cwd);

                            // Change to the new directory
                            chdir(new_cwd);
                            printf("Connecting to Client Transfer Socket...\n");

                            int pid = fork();
                            if (pid == 0) {
                                // Child process to handle data transfer
                                char* filename = arg_2;
                                int data_socket = socket(AF_INET, SOCK_STREAM, 0);
                                if (data_socket < 0) {
                                    perror("data socket creation failed");
                                    exit(EXIT_FAILURE);
                                }

                                struct sockaddr_in client_address;
                                bzero(&client_address, sizeof(client_address));
                                client_address.sin_family = AF_INET;
                                client_address.sin_port = htons(user->client_port);
                                if (inet_pton(AF_INET, user->client_ip, &client_address.sin_addr) <= 0) {
                                    perror("inet_pton failed");
                                    exit(EXIT_FAILURE);
                                }

                                if (connect(data_socket, (struct sockaddr*)&client_address, sizeof(client_address)) < 0) {
                                    perror("data socket connection failed");
                                    exit(EXIT_FAILURE);
                                }
                                printf("Connection Successful\n");

                                // Open the file to be sent
                                FILE *file = fopen(filename, "rb");
                                if (file == NULL) {
                                    perror("fopen failed");
                                    char response[] = "550 File not found\n";
                                    send(fd, response, strlen(response), 0);
                                    close(data_socket);
                                    exit(EXIT_FAILURE);
                                } else {
                                    char response[] = "150 file status okay; about to open data connection.\n";
                                    send(fd, response, strlen(response), 0);

                                    char file_buffer[1024];
                                    size_t bytes_read;
                                    int sent_succesfully = 1;
                                    // Read file and send it to the client
                                    while ((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), file)) > 0) {
                                        if (send(data_socket, file_buffer, bytes_read, 0) < 0) {
                                            perror("send failed");
                                            sent_succesfully = 0;
                                            break;
                                        }
                                    }
                                    if(sent_succesfully){
                                        printf("226 Transfer Complete \n\n");                     
                                    }
                                }

                                fclose(file);
                                close(data_socket);
                                exit(EXIT_SUCCESS);
                            }

                            // Restore original working directory
                            if (chdir(original_cwd) != 0) {
                                perror("chdir() restore error");
                            }

                        } 
                        else {
                            // Send error if not logged in
                            char response[] = "530 Not logged in.\n";
                            send(fd, response, strlen(response), 0);
                        }
                    } 

                    // upload a file to server
                    else if (strcmp(arg_1, "STOR") == 0) {
                        if (user->valid_password) {
                            char new_cwd[512];

                            if (getcwd(new_cwd, sizeof(new_cwd)) == NULL) {
                                perror("getcwd() error");
                                continue;
                            }

                            char user_dir[512];
                            snprintf(user_dir, sizeof(user_dir), "/%s", user->cwd);
                            change_directory(user_dir, original_cwd, new_cwd);
                            // printf("1.%s\n",user_dir);
                            // printf("2.%s\n",original_cwd);
                            // printf("3.%s\n",new_cwd);
                            

                            chdir(new_cwd);
                            printf("Connecting to Client Transfer Socket...\n");

                            int pid = fork();
                            if (pid == 0) {
                                int data_socket = socket(AF_INET, SOCK_STREAM, 0);
                                if (data_socket < 0) {
                                    perror("data socket creation failed");
                                    exit(EXIT_FAILURE);
                                }

                                struct sockaddr_in client_address;
                                bzero(&client_address, sizeof(client_address));
                                client_address.sin_family = AF_INET;
                                client_address.sin_port = htons(user->client_port);
                                if (inet_pton(AF_INET, user->client_ip, &client_address.sin_addr) <= 0) {
                                    perror("inet_pton failed");
                                    exit(EXIT_FAILURE);
                                }

                                if (connect(data_socket, (struct sockaddr*)&client_address, sizeof(client_address)) < 0) {
                                    perror("data socket connection failed");
                                    exit(EXIT_FAILURE);
                                }

                                printf("Connection Successful\n");
                                char response[] = "150 File status okay; about to open data connection.\n";
                                send(fd, response, strlen(response), 0);

                                // Receive the filename from the client
                                char filename[256];
                                recv(fd, filename, sizeof(filename), 0);

                                if (store_file(data_socket, filename)) {
                                    printf("226 Transfer completed.\n");
                                } else {
                                    printf("550 File not found.\n");
                                }

                                close(data_socket);
                                exit(EXIT_SUCCESS);
                            }

                            if (chdir(original_cwd) != 0) {
                                perror("chdir() restore error");
                            }
                        } else {
                            char response[] = "530 Not logged in.\n";
                            send(fd, response, strlen(response), 0);
                        }
                    }

                    // handles recieved port number and ip address to create a new data transfer connection 
                    else if (strcmp(arg_1, "PORT") == 0) {
                        int ip_parts[4], port_parts[2];
                        get_ip_and_port(arg_2, ip_parts, port_parts);

                        // Print the received port parts for debugging
                        printf("Port received: %d,%d,%d,%d,%d,%d \n", 
                               ip_parts[0], ip_parts[1], ip_parts[2], ip_parts[3], 
                               port_parts[0], port_parts[1]);

                        // Construct the IP address string
                        snprintf(user->client_ip, sizeof(user->client_ip), "%d.%d.%d.%d", 
                                 ip_parts[0], ip_parts[1], ip_parts[2], ip_parts[3]);

                        // Construct the port number
                        user->client_port = (unsigned short)(port_parts[0] * 256 + port_parts[1]);

                        char response[] = "200 PORT command successful.\n";
                        send(fd, response, strlen(response), 0);
                    } 

                    // prints working directory 
                    else if (strcmp(arg_1, "PWD") == 0) {
                        if (user->valid_password) {
                            char response[256];
                            snprintf(response, sizeof(response), "257 %s/\n", user->cwd);
                            send(fd, response, strlen(response), 0);
                        } else {
                            char response[] = "530 Not logged in.\n";
                            send(fd, response, strlen(response), 0);
                        }
                    } else if (strcmp(arg_1, "QUIT") == 0) {
                        char response[] = "221 Service closing control connection.\n";
                        send(fd, response, strlen(response), 0);
                        close(fd);
                        FD_CLR(fd, &all_sockets);
                        for (int i = 0; i < MAX_CLIENTS; i++) {
                            if (clients[i].fd == fd) {
                                clients[i].fd = -1;

                                break;
                            }
                        }
                    } else {
                        char response[] = "502 Command not implemented.\n";
                        send(fd, response, strlen(response), 0);
                    }
                }
            }
        }
    }

    close(server_socket);
    return 0;
}

// load user information in users.csv into users struct

void load_users(const char* filename, Users* users) {
    // check if file exisits
    FILE* fptr = fopen(filename, "r");
    if (fptr == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char line[100];
    int i = 0;
    while (fgets(line, sizeof(line), fptr) != NULL && i < MAX_CLIENTS) {
        char* token = strtok(line, ",");
        if (token == NULL) {
            fprintf(stderr, "Invalid file format\n");
            exit(EXIT_FAILURE);
        }
        strcpy(users->names[i], token);
        token = strtok(NULL, ",");
        if (token == NULL) {
            fprintf(stderr, "Invalid file format\n");
            exit(EXIT_FAILURE);
        }
        strcpy(users->passwords[i], token);
        i++;
    }
    users->count = i;

    fclose(fptr);
}

// changes the working directory by taking file name
void cwd(const char *path, char *cwd) {
    if (chdir(path) == 0) {
        snprintf(cwd, 256, "%s", path);
        printf("Changed directory to %s\n", path);
    } else {
        perror("chdir");
    }
}
// verifies if password is correct
int authenticate_user(char* name, Users* userslist) {
    // Check if the name starts with a space
    if(name == NULL){return -1;}
    if (isspace((unsigned char)name[0])) {
        return -1;
    }

    for (int i = 0; i < userslist->count; i++) {
        if (strcmp(name, userslist->names[i]) == 0) {
            return i;
        }
    }

    return -1;
}

// verifies if password is correct
int authenticate_password(char* password, Users* userslist, int idx) {
     if(password == NULL){return -1;}
    if (isspace((unsigned char)password[0])) {
        return -1;
    }
    // check if the password matches the one stored in users.csv
    if (strncmp(password, userslist->passwords[idx], strlen(password)) == 0 &&strlen(userslist->passwords[idx]) - strlen(password) <= 2){ 
        return 1;
    }
    return 0;
}

void get_ip_and_port(const char* ip_port_str, int* ip_parts, int* port_parts) {
    // Parse the input string
    sscanf(ip_port_str, "%d,%d,%d,%d,%d,%d", 
           &ip_parts[0], &ip_parts[1], &ip_parts[2], &ip_parts[3], 
           &port_parts[0], &port_parts[1]);
}

void printStructContents(struct user_state * user) {
    printf("fd: %d\n", user->fd);
    printf("cwd: %s\n", user->cwd);
    printf("is valid name: %d\n", user->valid_name);
    printf("have valid password: %d\n", user->valid_password);
    printf("username: %s\n", user->username);
    printf("--------\n");
}

int change_directory(const char *user_cwd, char *original_cwd, char *new_cwd) {
    // Use popen to get the output of the pwd command
    FILE *fp = popen("pwd", "r");
    if (fp == NULL) {
        perror("popen() error");
        return 0;
    }

    // Read the output of the pwd command into original_cwd
    if (fgets(original_cwd, 256, fp) == NULL) {
        perror("fgets() error");
        pclose(fp);
        return 0;
    }
    pclose(fp);

    // Remove the newline character from the end of original_cwd
    original_cwd[strcspn(original_cwd, "\n")] = '\0';

    // Task: concatenate original_cwd and user_cwd
    snprintf(new_cwd, 512, "%s%s", original_cwd, user_cwd);
    return 1;
}

int store_file(int data_socket, char* filename) {
    int ret = 1;
    printf("%s \n", filename);
    // Ensure the filename is not NULL and not empty
    if (filename == NULL || strlen(filename) == 0) {
        fprintf(stderr, "Invalid filename provided\n");
        return 0;
    }

    // check if file exists
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("fopen failed");
        return 0;
    }

    char buffer[1024];
    int bytes_received;

    // send files bit by bit manner
    while ((bytes_received = recv(data_socket, buffer, sizeof(buffer), 0)) > 0) {
        size_t bytes_written = fwrite(buffer, 1, bytes_received, file);
        if (bytes_written != bytes_received) {
            perror("fwrite failed");
            ret = 0;
            break;
        }
    }

    if (bytes_received < 0) {
        perror("recv failed");
        ret = 0;
    }

    fclose(file);
    return ret;
}


int list_files(int data_socket) {
    int ret = 1;
    FILE *fp = popen("ls", "r");
    if (fp == NULL) {
        perror("popen() error");
        return 0;
    }
    printf("Listing directory\n");
    char buffer[1024];
    //send name of files and folders in current directory
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (send(data_socket, buffer, strlen(buffer), 0) < 0) {
            perror("send failed");
            ret = 0;
            break;
        }
    }
    pclose(fp);
    return ret;
}
