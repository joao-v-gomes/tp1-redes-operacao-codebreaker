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
HackerMessage msg_to_send;

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

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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

int readMessageFromClient(int client_socket, HackerMessage *msg) {
    int total = 0;
    while (total < sizeof(HackerMessage)) {
        int n = read(client_socket, ((char*)msg) + total, sizeof(HackerMessage) - total);

        printf("Read %d bytes from client socket\n", n);

        if (n <= 0){
            return ERROR;
        }
        total += n;
    }
    return OK;
}

int checkGuess(char *guess, int code, int *feedback) {
    return OK;
};

int main(int argc, char **argv) {

    state = START_SERVER_STATE;
    printf("Starting server...\n");

    argc_counter = argc;
    code = atoi(argv[3]);
    while (1)
    {
        switch (state)
        {
            case START_SERVER_STATE:
                // printf("Starting server...\n");
                state = SETTING_UP_SERVER_STATE;
                printf("Setting up server...\n");
                break;
            
            //Precisei colocar o case dentro de chaves para declarar variáveis locais
            case SETTING_UP_SERVER_STATE: {
                // printf("Setting up server...\n");

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
                    printf("Waiting for client connection...\n");
                }

                break;
            }

            case WAIT_FOR_CONNECTION_STATE:
                // printf("Waiting for client connection...\n");

                client_socket = waitForClientConnection(server_socket);
                
                if (client_socket < 0) {
                    printf("Error waiting for client connection");
                    return ERROR;
                }
                else{
                    printf("Client connected successfully \n");
                    state = WAIT_FOR_START_MESSAGE_STATE;
                    printf("Waiting for start message from client...\n");
                }

                break;

            case WAIT_FOR_START_MESSAGE_STATE:
                // printf("Waiting for start message from client...\n");
                // state = RECEIVED_START_MESSAGE_STATE;

                readMessageFromClient(client_socket, &msg_received);

                if(msg_received.type == MSG_START) {
                    printf("Start message received from client...\n");
                    state = RECEIVED_START_MESSAGE_STATE;
                }
                else{
                    printf("Invalid message type received from client. Expected MSG_START.\n");
                    return ERROR;
                }

                break;

            case RECEIVED_START_MESSAGE_STATE:
                // printf("Start message received from client...\n");
                state = WAIT_FOR_GUESS_MESSAGE_STATE;
                printf("Waiting for guess message from client...\n");

                break;    

            case WAIT_FOR_GUESS_MESSAGE_STATE:
                // printf("Waiting for guess message from client...\n");

                if (readMessageFromClient(client_socket, &msg_received) == ERROR) {
                    printf("Error reading message from client");
                    return ERROR;
                }
                else if (msg_received.type == MSG_GUESS) {
                    state = RECEIVED_GUESS_MESSAGE_STATE;
                    printf("Guess message received from client...\n");
                }
                else{
                    printf("Invalid message type received from client. Expected MSG_GUESS.\n");
                    return ERROR;
                }

                break;
            
            case RECEIVED_GUESS_MESSAGE_STATE: {
                // printf("Guess message received from client...\n");

                char guess_received[6];

                for(int i = 0; i < 5; i++) {
                    guess_received[i] = msg_received.guess[i] + '0';
                }
                guess_received[5] = '\0';

                printf("Guess received from client: %s \n", guess_received);

                checkGuess(guess_received, code, msg_to_send.feedback);

                state = SEND_FEEDBACK_STATE;
                printf("Sending feedback to client...\n");

                break;
            }

            case SEND_FEEDBACK_STATE:
                // printf("Sending feedback to client...\n");

                printf("Calculating feedback for guess...\n");
                printf("This was the guess received: %d %d %d %d %d \n", msg_received.guess[0], msg_received.guess[1], msg_received.guess[2], msg_received.guess[3], msg_received.guess[4]);

                msg_to_send.type = MSG_FEEDBACK;

                msg_to_send.feedback[0] = msg_received.guess[0];
                msg_to_send.feedback[1] = msg_received.guess[1];
                msg_to_send.feedback[2] = msg_received.guess[2];
                msg_to_send.feedback[3] = msg_received.guess[3];
                msg_to_send.feedback[4] = msg_received.guess[4];

                write(client_socket, &msg_to_send, sizeof(msg_to_send));
                state = CHECK_WIN_STATUS_STATE;
                printf("Checking win status...\n");
                break;

            case CHECK_WIN_STATUS_STATE:
                // printf("Checking win status...\n");
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