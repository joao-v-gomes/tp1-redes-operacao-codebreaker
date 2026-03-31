#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "util.h"
#include "server.h"

int state;

int argc_counter;

int server_socket = 0;
int client_socket = 0;
int code;

HackerMessage msg_received;

int setUpServer(char *ip, int port) {

    // Validar as informações de configuração do servidor
    if (validateInfoToSetUpServer(ip, port) != 0) {
        return ERROR;
    }

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_address.sin_addr);

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Error binding server socket");
        return ERROR;
    }

    if (listen(server_socket, 1) < 0) {
        perror("Error listening for connections");
        return ERROR;
    }

    return server_socket;
}

int validateInfoToSetUpServer(char *ip, int port) {

    // Verificar se o número de argumentos é correto
    if (argc_counter != 4) {
        printf("Usage: ./server <ip> <port> <code>\n");
        return ERROR;
    }

    // Verificar se porta e valida
    if (port <= 0 || port > 65535) {
        printf("Invalid port number. Please provide a port number between 1 and 65535.\n");
        return ERROR;
    }

    return OK;
}


int waitForClientConnection(int server_socket) {
    int client_socket = accept(server_socket, NULL, NULL);
    if (client_socket < 0) {
        printf("Error accepting client connection");
        return ERROR;
    }
    return client_socket;
}

int main(int argc, char **argv) {

    state = START_SERVER_STATE;
    argc_counter = argc;
    code = atoi(argv[3]);
    while (1)
    {
        switch (state)
        {
            case START_SERVER_STATE:
                printf("Starting server...\n");
                state = SETTING_UP_SERVER_STATE;
                break;
            
            case SETTING_UP_SERVER_STATE:
                printf("Setting up server...\n");

                char *ip = argv[1];
                int port = atoi(argv[2]);

                server_socket = setUpServer(ip, port);

                if (server_socket < 0) {
                    printf("Error setting up server");
                    return ERROR;
                }
                else{
                    printf("Server set up successfully on %s:%d \n", ip, port);
                    state = WAIT_FOR_CONNECTION_STATE;
                }
                
                break;

            case WAIT_FOR_CONNECTION_STATE:
                printf("Waiting for client connection...\n");

                client_socket = waitForClientConnection(server_socket);
                
                if (client_socket < 0) {
                    printf("Error waiting for client connection");
                    return ERROR;
                }
                else{
                    printf("Client connected successfully \n");
                    state = WAIT_FOR_MESSAGE_STATE;
                }

                break;

            case WAIT_FOR_MESSAGE_STATE:
                printf("Waiting for message from client...\n");
                // state = RECEIVED_MESSAGE_STATE;
                break;
            
            case RECEIVED_MESSAGE_STATE:
                printf("Message received from client...\n");
                // state = SEND_FEEDBACK_STATE;
                break;

            case SEND_FEEDBACK_STATE:
                printf("Sending feedback to client...\n");
                // state = CHECK_WIN_STATUS_STATE;
                break;

            case CHECK_WIN_STATUS_STATE:
                printf("Checking win status...\n");
                // state = EXIT_STATE;
                break;

            case EXIT_STATE:
                printf("Exiting server...\n");
                break;

            default:
                printf("INVALID SERVER STATE!!!\n");
                break;
        }
    }
    // int port = atoi(argv[2]);
    // char *code = argv[3];

    // printf("Server is running on %s:%d and the code is %s \n", ip, port, code);

    // //create a socket and bind it to the port
    // int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    // struct sockaddr_in server_address;
    // server_address.sin_family = AF_INET;
    // server_address.sin_port = htons(port);
    // inet_pton(AF_INET, ip, &server_address.sin_addr);

    // bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));

    // //listen for incoming connections
    // listen(server_socket, 1);

    // // char buffer[1024];

    // int client_socket = accept(server_socket, NULL, NULL);

    HackerMessage msg;
    int total = 0;
    while (1)
    {
        total = 0;
        while (total < sizeof(msg)) {
            int n = read(client_socket, ((char*)&msg) + total, sizeof(msg) - total);
            if (n <= 0){
                break;
            }
            total += n;
        }

        printf("Received message from client: %s \n", msg.type == MSG_START ? "MSG_START" : msg.type == MSG_GUESS ? "MSG_GUESS" : msg.type == MSG_FEEDBACK ? "MSG_FEEDBACK" : msg.type == MSG_WIN ? "MSG_WIN" : msg.type == MSG_ERROR ? "MSG_ERROR" : "MSG_EXIT");
        
        printf("Guess: %d %d %d %d %d \n", msg.guess[0], msg.guess[1], msg.guess[2], msg.guess[3], msg.guess[4]);
        // printf("Attempts: %d \n", msg.attempts);
        printf("Win status: %d \n", msg.win_status);



        //clear the struct
        memset(&msg, 0, sizeof(msg));

    } 

    return 0;
}