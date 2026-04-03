#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "util.h"
#include "server.h"
#include "ifaddrs.h"

int state;

int argc_counter;

int server_socket = 0;
int client_socket = 0;
int code;
int attempts_counter = 0;

HackerMessage msg_received;
HackerMessage msg_to_send;

int setUpServer(char *ip, int port, int code) {

    // Validar as informações de configuração do servidor
    if (validateInfoToSetUpServer(ip,port,code) != 0) {
        return ERROR;
    }

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;

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

char* getLocalIp() {
    struct ifaddrs *ifaddr, *ifa;
    static char localIp[INET_ADDRSTRLEN] = "0.0.0.0";

    if (getifaddrs(&ifaddr) == -1) {
        return localIp;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;

            inet_ntop(AF_INET, &sa->sin_addr, localIp, sizeof(localIp));

        }
    }
    freeifaddrs(ifaddr);

    return localIp;
}

int validateInfoToSetUpServer(char *ip, int port, int code) {

    // Verificar se o número de argumentos é correto
    if (argc_counter != 4) {
        printf("Usage: ./server <ip> <port> <code>\n");
        return ERROR;
    }

    if(strcmp(ip, "v4") != 0 && strcmp(ip, "v6") != 0) {
        printf("Invalid IP version. Please provide a valid IP address.\n");
        return ERROR;
    }

    // Verificar se porta e valida
    if (port <= 0 || port > 65535) {
        printf("Invalid port number. Please provide a port number between 1 and 65535.\n");
        return ERROR;
    }

    if(code < 0 || code > 99999) {
        printf("Invalid code. Please provide a code between 00000 and 99999.\n");
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
        // int n = read(client_socket, ((char*)msg) + total, sizeof(HackerMessage) - total);
        int n = recv(client_socket, ((char*)msg) + total, sizeof(HackerMessage) - total, 0);

        // printf("Read %d bytes from client socket\n", n);

        if (n <= 0){
            return ERROR;
        }
        total += n;
    }
    return OK;
}

int calculateFeedback(int *guess, int code, HackerMessage *msg) {
    int code_digits[5];
    int counter_right_position = 0;

    // printf("Code to guess: %d \n", code);
    
    for(int i = 4; i >= 0; i--) {
        code_digits[i] = code % 10;
        code /= 10;
    }

    printf("Code digits: %d %d %d %d %d \n", code_digits[0], code_digits[1], code_digits[2], code_digits[3], code_digits[4]);

    for(int i = 0; i < 5; i++) {
        if (guess[i] == code_digits[i]) {
            msg->feedback[i] = RIGHT_POSITION;
            counter_right_position++;
        }
        else if (guess[i] == code_digits[(i + 1) % 5] || guess[i] == code_digits[(i + 2) % 5] || guess[i] == code_digits[(i + 3) % 5] || guess[i] == code_digits[(i + 4) % 5]) {
            msg->feedback[i] = WRONG_POSITION;
        }
        else{
            msg->feedback[i] = NOT_IN_CODE;
        }
    }

    if(counter_right_position == 5) {
        msg->win_status = WIN;
    }
    else{
        msg->win_status = IN_GAME;
    }

    return OK;
}

int fillFeedbackWithGuess(int *guess, HackerMessage *msg) {
    for(int i = 0; i < 5; i++) {
        msg->guess[i] = guess[i];
    }
    return OK;
}

int main(int argc, char **argv) {

    state = START_SERVER_STATE;
    printf("Starting server...\n");

    argc_counter = argc;
    code = atoi(argv[3]);

    // printf("Code to guess: %d \n", code);

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

                char *ip_type = argv[1];
                int port = atoi(argv[2]);

                server_socket = setUpServer(ip_type, port, code);

                if (server_socket < 0) {
                    printf("Error setting up server");
                    return ERROR;
                }
                else{
                    
                    printf("Server set up successfully on %s:%d \n", getLocalIp(), port);
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
                state = WAITING_FOR_MESSAGE_STATE;
                attempts_counter = 1;

                memset(&msg_received, 0, sizeof(msg_received));
                memset(&msg_to_send, 0, sizeof(msg_to_send));

                printf("Waiting for guess message from client...\n");

                break;    

            case WAITING_FOR_MESSAGE_STATE:
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
                printf("This was the guess received: %d %d %d %d %d \n", msg_received.guess[0], msg_received.guess[1], msg_received.guess[2], msg_received.guess[3], msg_received.guess[4]);

                state = SEND_FEEDBACK_STATE;
                printf("Sending feedback to client...\n");

                break;
            }

            case SEND_FEEDBACK_STATE:
                // printf("Sending feedback to client...\n");

                memset(&msg_to_send, 0, sizeof(msg_to_send));

                printf("Calculating feedback for guess...\n");

                fillFeedbackWithGuess(msg_received.guess, &msg_to_send);

                // printf("Code to guess: %d \n", code);

                calculateFeedback(msg_received.guess, code, &msg_to_send);

                msg_to_send.type = MSG_FEEDBACK;

                msg_to_send.attempts = attempts_counter++;

                // write(client_socket, &msg_to_send, sizeof(msg_to_send));
                send(client_socket, &msg_to_send, sizeof(msg_to_send), 0);

                if(msg_to_send.win_status == IN_GAME) {
                    state = WAITING_FOR_MESSAGE_STATE;
                    printf("Feedback sent to client. Waiting for next guess...\n");
                }
                else if (msg_to_send.win_status == WIN) {
                    state = WAIT_FOR_EXIT_MESSAGE_STATE;
                    printf("Feedback sent to client. Waiting for exit message...\n");
                }
                else{
                    printf("Invalid win status calculated. Expected IN_GAME or WIN.\n");
                    return ERROR;
                }

                break;

            case WAIT_FOR_EXIT_MESSAGE_STATE:
                // printf("Waiting for exit message from client...\n");
                // state = EXIT_STATE;

                memset(&msg_received, 0, sizeof(msg_received));
                if(readMessageFromClient(client_socket, &msg_received) == ERROR) {
                    printf("Error reading message from client");
                    return ERROR;
                }

                if (msg_received.type == MSG_EXIT) {
                    state = EXIT_STATE;
                    printf("Exit message received from client. Closing connection...\n");

                    close(client_socket);
                    close(server_socket);

                }
                else{
                    printf("Invalid message type received from client. Expected MSG_EXIT.\n");
                    return ERROR;
                }

                break;

            case EXIT_STATE:
                printf("Exiting server...\n");
                exit(0);
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

    // HackerMessage msg;
    // int total = 0;
    // while (1)
    // {
    //     total = 0;
    //     while (total < sizeof(msg)) {
    //         int n = read(client_socket, ((char*)&msg) + total, sizeof(msg) - total);
    //         if (n <= 0){
    //             break;
    //         }
    //         total += n;
    //     }

    //     printf("Received message from client: %s \n", msg.type == MSG_START ? "MSG_START" : msg.type == MSG_GUESS ? "MSG_GUESS" : msg.type == MSG_FEEDBACK ? "MSG_FEEDBACK" : msg.type == MSG_WIN ? "MSG_WIN" : msg.type == MSG_ERROR ? "MSG_ERROR" : "MSG_EXIT");
        
    //     printf("Guess: %d %d %d %d %d \n", msg.guess[0], msg.guess[1], msg.guess[2], msg.guess[3], msg.guess[4]);
    //     // printf("Attempts: %d \n", msg.attempts);
    //     printf("Win status: %d \n", msg.win_status);



    //     //clear the struct
    //     memset(&msg, 0, sizeof(msg));

    // } 

    return 0;
}