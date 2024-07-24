#include "client.h"

int main() 
{
    int network_socket;
    network_socket = socket(AF_INET, SOCK_STREAM, 0); // Create a socket

    if (network_socket == -1) { // Check if socket creation failed
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client_address;
    client_address.sin_family = AF_INET;
    client_address.sin_port = htons(0); // Use any open port
    client_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(network_socket, (struct sockaddr*)&client_address, sizeof(client_address)) < 0) { // Bind the socket
        perror("bind failed");
        close(network_socket);
        exit(EXIT_FAILURE);
    }

    char local_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_address.sin_addr, local_ip, sizeof(local_ip)); // Convert IP to string

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(CTRL_PORT); // Control channel port
    server_address.sin_addr.s_addr = INADDR_ANY; 

    if (connect(network_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) { // Connect to server
        perror("connect");
        exit(EXIT_FAILURE);
    }
    else {
        char local_ip1[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &server_address.sin_addr, local_ip1, sizeof(local_ip1)); // Convert server IP to string
    }

    printCommands(); // Print available commands
    printf("220 Service ready for new user.\n");

    char buffer[256];
    int valid_name = 0;
    int valid_password = 0;

    chdir("../Client"); // Change to Client directory

    while (1)
    {
            bzero(buffer,sizeof(buffer)); // Clear buffer
            printf("ftp>");
            fgets(buffer, sizeof(buffer), stdin); // Get user input
            buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline character

            // check if username is valid
            if(valid_name == 0  && valid_password == 0)
            {
                if (strncmp(buffer,"USER ",5) == 0 && strlen(buffer) > 5 || strncmp(buffer,"QUIT",4) == 0) {
                    send(network_socket, buffer, sizeof(buffer), 0); // Send username or QUIT command
                    bzero(buffer,sizeof(buffer));
                    recv(network_socket, buffer, sizeof(buffer), 0); // Receive server response
                    
                    // user is registered in server
                    if(strncmp(buffer,"331",3) == 0) {
                        printf("%s \n", buffer);
                        valid_name = 1;
                    }
                    // user is not registered in server
                    else if (strncmp(buffer,"332",3) == 0) {
                        printf("%s \n", buffer);
                        valid_name = 0;
                    }
                    // close connection
                    else if (strncmp(buffer,"221",3) == 0) {
                        printf("%s \n", buffer); 
                        close(network_socket);
                        break;                
                    }
                }
                else if(strncmp(buffer,"PASS ",5) == 0){
                    printf("503 Bad sequence of commands.\n");
                }
                else {
                    printf("530 Not logged in.\n");
                }
            } 
            // check if password is valid
            else if(valid_name == 1 && valid_password == 0) 
            {
                if (strncmp(buffer, "PASS ", 5) == 0 && strlen(buffer) > 5 || strncmp(buffer, "QUIT", 4) == 0) {
                    send(network_socket, buffer, sizeof(buffer), 0); // Send password or QUIT command
                    bzero(buffer, sizeof(buffer));

                    recv(network_socket, buffer, sizeof(buffer), 0); // Receive server response

                    // valid password
                    if(strncmp(buffer, "230", 3) == 0) {
                        printf("%s", buffer);
                        valid_password = 1;
                    }

                    // invalid password
                    else if(strncmp(buffer, "530", 3) == 0) {
                        printf("%s", buffer);
                        valid_name = 0;
                    }

                    // quit command
                    else if (strncmp(buffer, "221", 3) == 0) {
                        printf("%s \n", buffer); // Server response
                        close(network_socket);
                        break;
                    }
                } else {
                    printf("530 Not logged in.\n");
                    valid_name = 0;
                }
            }

            // user authenticated -> do necessary commands
            else if (valid_name == 1  && valid_password == 1)
            {   
                // client side operations -> no need to send them to server
                if(strncmp(buffer,"!", 1) == 0){
                    
                    if (strncmp(buffer,"!LIST",5) == 0){
                        system("ls");
                    }
                    else if (strncmp(buffer,"!CWD",4) == 0){
                        char* arg_1 = strtok(buffer, " ");
                        char* arg_2 = strtok(NULL, " ");
                        cwd(arg_2);
                    }
                    else if (strncmp(buffer,"!PWD",4) == 0){
                        system("pwd");
                    }
                }
                else if (strncmp(buffer, "RETR ", 5) == 0 ) {
                    char cwd[PATH_MAX];
                    if (getcwd(cwd, sizeof(cwd)) == NULL) { // Get current directory
                        perror("getcwd failed");
                        continue;
                    }
                    
                    // Get the next open port 
                    int data_socket = socket(AF_INET, SOCK_STREAM, 0);
                    struct sockaddr_in data_address;
                    data_address.sin_family = AF_INET;
                    data_address.sin_port = 0; // Let the OS pick an available port
                    data_address.sin_addr.s_addr = INADDR_ANY;

                    if (bind(data_socket, (struct sockaddr*)&data_address, sizeof(data_address)) < 0) { // Bind data socket
                        perror("data socket bind failed");
                        close(data_socket);
                        continue;
                    }

                    // Listen on the data socket
                    if (listen(data_socket, 1) < 0) { // Listen for connection
                        perror("data socket listen failed");
                        close(data_socket);
                        continue;
                    }

                    
                    // Get the assigned port number
                    socklen_t addr_len = sizeof(data_address);
                    getsockname(data_socket, (struct sockaddr*)&data_address, &addr_len); // Get the port number assigned

                    // Send the PORT command to the server
                    char* ip_port_str = get_ip_and_port(&data_address);

                    char port_cmd[256];
                    snprintf(port_cmd, 256, "PORT %s", ip_port_str); // Prepare PORT command
                    send(network_socket, port_cmd, sizeof(port_cmd), 0); // Send PORT command
                    bzero(port_cmd, sizeof(port_cmd));
                    recv(network_socket, port_cmd, sizeof(port_cmd), 0); // Receive server response
                    if (strncmp("200", port_cmd, 3) == 0) {
                        printf("%s", port_cmd);
                        send(network_socket, buffer, sizeof(buffer), 0); // Send RETR command
                        // Accept the data connection from the server
                        int data_conn_socket = accept(data_socket, NULL, NULL);
                        if (data_conn_socket < 0) {
                            perror("data connection accept failed");
                        } else {
                            // Handle the data transfer
                            char *arg_1 = strtok(buffer, " ");
                            char *arg_2 = strtok(NULL, " ");
                            char filepath[PATH_MAX];
                            snprintf(filepath, sizeof(filepath), "%s/%s", cwd, arg_2); // Prepend current directory to filename
                            
                            recv(network_socket, buffer, sizeof(buffer), 0); // Receive server response
                            if (strncmp(buffer, "150", 3) == 0) {
                                printf("%s\n", buffer); 
                                if (receive_file(data_conn_socket, filepath)) {
                                    printf("226 Transfer completed.\n\n");
                                } else {
                                    printf("550 File not found.\n");
                                }
                            } else {
                                printf("550 File not found.\n");
                            }
                            close(data_conn_socket);
                        }
                    } else if (strncmp("550", port_cmd, 3) == 0) {
                        printf("550 File not found.\n");
                        char response[] = "550 File not found.\n";
                        send(network_socket, response, sizeof(response), 0);
                    }
                    close(data_socket);
                    free(ip_port_str);
                }
                else if (strncmp(buffer, "STOR", 4) == 0) {
                    char cwd[PATH_MAX];
                    if (getcwd(cwd, sizeof(cwd)) == NULL) { // Get current directory
                        perror("getcwd failed");
                        continue;
                    }

                    char *arg_1 = strtok(buffer, " ");
                    char *arg_2 = strtok(NULL, " ");
                    char file_name[50]; 
                    snprintf(file_name, sizeof(file_name), "%s", arg_2);
                    printf("%s \n",file_name);

                    char filepath[PATH_MAX];
                    snprintf(filepath, sizeof(filepath), "%s/%s", cwd, arg_2); // Prepend current directory to filename

                    printf("%s \n",file_name);
                    FILE *file = fopen(filepath, "rb");
                    if (file == NULL) { // Check if file exists
                        printf("550 File not found.\n");
                        continue;
                    }
                    fclose(file);

                    // Get the next open port
                    int data_socket = socket(AF_INET, SOCK_STREAM, 0);
                    struct sockaddr_in data_address;
                    data_address.sin_family = AF_INET;
                    data_address.sin_port = 0; // Let the OS pick an available port
                    data_address.sin_addr.s_addr = INADDR_ANY;

                    if (bind(data_socket, (struct sockaddr*)&data_address, sizeof(data_address)) < 0) { // Bind data socket
                        perror("data socket bind failed");
                        close(data_socket);
                        continue;
                    }

                    // Listen on the data socket
                    if (listen(data_socket, 1) < 0) { // Listen for connection
                        perror("data socket listen failed");
                        close(data_socket);
                        continue;
                    }

                    // Get the assigned port number
                    socklen_t addr_len = sizeof(data_address);
                    getsockname(data_socket, (struct sockaddr*)&data_address, &addr_len); // Get the port number assigned

                    // Send the PORT command to the server
                    char* ip_port_str = get_ip_and_port(&data_address);
                    char port_cmd[256];
                    snprintf(port_cmd, 256, "PORT %s", ip_port_str); // Prepare PORT command
                    send(network_socket, port_cmd, sizeof(port_cmd), 0); // Send PORT command
                    bzero(port_cmd, sizeof(port_cmd));
                    recv(network_socket, port_cmd, sizeof(port_cmd), 0); // Receive server response

                    if (strncmp("200", port_cmd, 3) == 0) {
                        send(network_socket, buffer, sizeof(buffer), 0); // Send STOR command
                        // Accept the data connection from the server
                        int data_conn_socket = accept(data_socket, NULL, NULL);
                        if (data_conn_socket < 0) {
                            perror("data connection accept failed");
                        } else {
                            // Handle the data transfer
                            recv(network_socket, buffer, sizeof(buffer), 0); // Receive server response
                            if (strncmp(buffer, "150", 3) == 0) {
                                printf("%s \n", buffer);
                                send(network_socket, file_name, strlen(file_name), 0); // Send filename
                                if (send_file(data_conn_socket, file_name)) {
                                    printf("226 Transfer completed.\n\n");                      
                                }
                            } else {
                                printf("Error: %s\n", buffer);
                            }
                            close(data_conn_socket);
                        }
                    } else {
                        printf("550 File not found.\n");
                    }

                    close(data_socket);
                    free(ip_port_str);
                }

                else if (strncmp(buffer, "LIST", 4) == 0) 
                {
                    // Get the next open port 
                    int data_socket = socket(AF_INET, SOCK_STREAM, 0);
                    struct sockaddr_in data_address;
                    data_address.sin_family = AF_INET;
                    data_address.sin_port = 0; // Let the OS pick an available port
                    data_address.sin_addr.s_addr = INADDR_ANY;

                    if (bind(data_socket, (struct sockaddr*)&data_address, sizeof(data_address)) < 0) { // Bind data socket
                        perror("data socket bind failed");
                        close(data_socket);
                        continue;
                    }

                    // Listen on the data socket
                    if (listen(data_socket, 1) < 0) { // Listen for connection
                        perror("data socket listen failed");
                        close(data_socket);
                        continue;
                    }

                    // Get the assigned port number
                    socklen_t addr_len = sizeof(data_address);
                    getsockname(data_socket, (struct sockaddr*)&data_address, &addr_len); // Get the port number assigned

                    // Send the PORT command to the server
                    char* ip_port_str = get_ip_and_port(&data_address);

                    char port_cmd[256];
                    snprintf(port_cmd, 256, "PORT %s", ip_port_str); // Prepare PORT command
                    send(network_socket, port_cmd, sizeof(port_cmd), 0); // Send PORT command
                    bzero(port_cmd, sizeof(port_cmd));
                    recv(network_socket, port_cmd, sizeof(port_cmd), 0); // Receive server response
                    

                    if (strncmp("200", port_cmd, 3) == 0) {
                        printf("%s", port_cmd);
                        send(network_socket, buffer, sizeof(buffer), 0); // Send LIST command

                        // Handle the server's response
                        char response[256];
                        bzero(response, sizeof(response));
                        recv(network_socket, response, sizeof(response), 0); // Receive server response
                        if (strncmp(response, "150", 3) == 0) {
                            printf("%s\n", response); 
                            // Accept the data connection from the server
                            int data_conn_socket = accept(data_socket, NULL, NULL);
                            if (data_conn_socket < 0) {
                                perror("data connection accept failed");
                            } else {
                                // Handle the directory listing
                                list_files(data_conn_socket);
                                close(data_conn_socket);
                            }
                        } else {
                            printf("%s\n", response);
                        } 
                    } else if (strncmp("550", port_cmd, 3) == 0) {
                        printf("Failed to list directory.\n");
                    }

                    close(data_socket);
                    free(ip_port_str);
                }

                else if (strncmp(buffer, "PWD", 3) == 0) 
                {
                    send(network_socket, buffer, sizeof(buffer), 0); // Send PWD command
                    bzero(buffer, sizeof(buffer));
                    recv(network_socket, buffer, sizeof(buffer), 0); // Receive server response
                    if (strncmp(buffer, "257", 3) == 0){
                        printf("%s \n", buffer);
                    }
                    else if (strncmp(buffer, "530", 3) == 0){
                        printf("%s \n", buffer);
                    }
                    
                }
                else if (strncmp(buffer, "CWD", 3) == 0) 
                {
                    send(network_socket, buffer, sizeof(buffer), 0); // Send CWD command
                    bzero(buffer, sizeof(buffer));
                    recv(network_socket, buffer, sizeof(buffer), 0); // Receive server response
                    printf("%s", buffer);
                }

                else if (strncmp(buffer, "USER", 4) == 0 || strncmp(buffer, "PASS", 4) == 0 )
                {
                    printf("User already logged in, enter QUIT to exit.\n");
                }
                else
                {
                    send(network_socket, buffer, sizeof(buffer), 0); // Send other commands
                    bzero(buffer, sizeof(buffer));
                    recv(network_socket, buffer, sizeof(buffer), 0); // Receive server response
                    printf("%s \n", buffer);
                    if (strncmp(buffer, "221", 3) == 0) {
                        close(network_socket); 
                        break;
                    }
                }
            }   
    }
    return 0;
}

// Function to print available FTP commands
void printCommands() {
    printf("Hello!! Please Authenticate to run server commands \n");
    printf("1. type \"USER\" followed by a space and your username \n");
    printf("2. type \"PASS\" followed by a space and your password \n\n");
    printf("\"QUIT\" to close connection at any moment \n");
    printf("Once Authenticated this is the list of commands \n");
    printf("\"STOR\" + space + filename | to send a file to the server \n");
    printf("\"RETR\" + space + filename | to download a file from the server \n");
    printf("\"LIST\" | to list all the files under the current server directory \n");
    printf("\"CWD\" + space + directory | to change the current server directory \n");
    printf("\"PWD\" | to display the current server directory \n");
    printf("Add \"!\" before the last three commands to apply them locally \n\n");
}

// Function to change the current working directory
int cwd(const char *path) {
    struct stat info;
    
    // Check if the path is an empty space
    if (path == NULL || strcmp(path, "") == 0) {
        printf("Invalid directory path.\n");
        return 0;
    }

    // Check if the directory exists
    if (stat(path, &info) != 0) {
        perror("stat");
        return 0;
    } else if (info.st_mode & S_IFDIR) { // Check if it's a directory
        if (chdir(path) == 0) {
            printf("Changed directory to %s\n", path);
            return 1;
        } else {
            perror("chdir");
            return 0;
        }
    } else {
        printf("Not a directory.\n");
        return 0;
    }
}

// Function to extract IP address and port number from a socket address
char* get_ip_and_port(struct sockaddr_in *addr) {
    char *result = (char *)malloc(50 * sizeof(char));
    if (result == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    // Extract the IP address and port number
    unsigned char *ip = (unsigned char *)&(addr->sin_addr.s_addr);
    unsigned short port = ntohs(addr->sin_port);

    // Format the IP address and port into a string
    snprintf(result, 50, "%d,%d,%d,%d,%d,%d", ip[0], ip[1], ip[2], ip[3], port / 256, port % 256);

    return result;
}

// Function to receive a file from the data socket and save it to the given filepath
int receive_file(int data_socket, char *filepath) {
    int ret = 1;
    FILE *file = fopen(filepath, "wb");
    if (file == NULL) {
        perror("fopen failed");
        return 0;
    }

    char buffer[1024];
    int bytes_received;
    while ((bytes_received = recv(data_socket, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
    }

    if (bytes_received < 0) {
        perror("recv failed");
        ret = 0;
    }

    fclose(file);
    return ret;
}

// Function to send a file from the given filename over the data socket
int send_file(int data_socket, char* filename) {
    int ret = 1;
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        return 0;
    }

    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(data_socket, buffer, bytes_read, 0) < 0) {
            ret = 0;
            break;
        }
    }

    fclose(file);
    return ret;
}

// Function to list files received from the data socket
void list_files(int data_socket) {
    char buffer[1024];
    int bytes_received;
    while ((bytes_received = recv(data_socket, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, stdout);
    }
    if (bytes_received < 0) {
        perror("recv failed");
    } else {
        printf("226 Transfer Complete \n\n");
    }
}