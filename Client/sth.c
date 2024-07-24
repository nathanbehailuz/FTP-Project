#include <stdio.h>


 

int main(){
    char input[256];
    char* arg_1;
    char* arg_2;

    printf("ftp>");
    fgets(input, sizeof(input), stdin);

    input[strcspn(input, "\n")] = '\0';
    arg_1 = strtok(input, " ");
    arg_2 = strtok(NULL, " ");
     // if(strncmp(buffer,"33",3) == 0)
        

       // if(strncmp(buffer,"!",1) != 0 || strncmp(buffer,"QUIT",4) != 0){
        //     send(network_socket, buffer, sizeof(buffer), 0);
        // }
       
        // else if(strncmp(buffer,"QUIT",4) == 0){
        //     printf("Closing the connection to server\n");
        //     close(network_socket);
        //     break;
        // }

        // else if (strncmp(buffer,"USER",4) == 0){
        //     send(network_socket, buffer, sizeof(buffer), 0);
        // }
        // else if (strcmp(buffer,"PASS") == 0){
        //     continue;
        // }
        // else if (strncmp(buffer,"STOR",4) == 0){
        //     continue;
        // }
        // else if (strncmp(buffer,"RETR",4) == 0){
        //     handle_RETR(network_socket, arg_2);
        //     continue;
        // }
        // else if (strcmp(arg_1,"LIST") == 0){
        //     send(network_socket, arg_1, sizeof(arg_1), 0);
        // }
        // else if (strcmp(arg_1,"CWD") == 0){
        //     send(network_socket, input, sizeof(input), 0);
        // }
        // else if (strcmp(arg_1,"PWD") == 0){
        //     send(network_socket, arg_1, sizeof(arg_1), 0);
        // }
        // else if (strcmp(arg_1,"!LIST") == 0){
        //     system("ls");
        // }
        // else if (strcmp(arg_1,"!CWD") == 0){
        //     cwd(arg_2);
        // }
        // else if (strcmp(arg_1,"!PWD") == 0){
        //     system("pwd");
        // }
return 0;
}


 else if (strcmp(arg_1, "RETR") == 0) {
                    if (user->valid_password) {
                        // Ensure cwd is restored for each operation
                        // printStructContents(user);
                        // printf("1%s \n",user->cwd);
                        // chdir(system("pwd")+user->cwd);
                        // system("pwd");

                        change_directory(user->cwd);
                        
                        
                        // Fork to handle file transfer separately
                        int pid = fork();
                        if (pid == 0) { // Child process
                            // Data transfer logic
                            char* filename = arg_2;
                            // printf("filename: %s \n",filename);
                            // // chdir(user->cwd);
                            // printf("2%s \n",user->cwd );
                            // system("pwd");

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

                            // Handle file transfer
                            FILE *file = fopen(filename, "rb");
                            if (file == NULL) {
                                perror("fopen failed");
                                close(data_socket);
                                exit(EXIT_FAILURE);
                            }

                            char file_buffer[1024];
                            size_t bytes_read;
                            while ((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), file)) > 0) {
                                if (send(data_socket, file_buffer, bytes_read, 0) < 0) {
                                    perror("send failed");
                                    break;
                                }
                            }

                            fclose(file);
                            close(data_socket);
                            exit(EXIT_SUCCESS);
                        }
                    } else {
                        char response[] = "530 Not logged in.\n";
                        send(fd, response, strlen(response), 0);
                    }
                }