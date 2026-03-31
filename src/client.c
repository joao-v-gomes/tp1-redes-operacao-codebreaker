#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "util.h"
#include "client.h"

    
int argc_counter;

int state;
int client_socket;

HackerMessage msg_sent;
HackerMessage msg_received;

int connectToServer(char *server_ip, int server_port) {

    if(validateInfoToConnectToServer(server_ip, server_port) != 0) {
        return ERROR;
    }

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_address.sin_addr);

    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        printf("Error connecting to server");
        return ERROR;
    }else{
        printf("Connected to server at %s:%d \n", server_ip, server_port);
    }

    return client_socket;
}

int validateInfoToConnectToServer(char *server_ip, int server_port) {

    // Verificar se o número de argumentos é correto
    if (argc_counter != 3) {
        printf("Usage: ./client <server_ip> <server_port>\n");
        return ERROR;
    }

    // Verificar se porta e valida
    if (server_port <= 0 || server_port > 65535) {
        printf("Invalid port number. Please provide a port number between 1 and 65535.\n");
        return ERROR;
    }

    // TODO: Verificar se e IPv4 ou IPv6

    // Verificar se o IP do servidor é válido
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, server_ip, &(sa.sin_addr));
    if (result <= 0) {
        printf("Invalid server IP address. Please provide a valid IPv4 address.\n");
        return ERROR;
    }

    return OK;
}

int main(int argc, char **argv) {

    argc_counter = argc;
    state = START_CLIENT_STATE;

    while(1)
    {
        switch (state)
        {
            case START_CLIENT_STATE:
                printf("Starting client...\n");
                state = CONNECT_TO_SERVER_STATE;
                break;
            
            case CONNECT_TO_SERVER_STATE:
                printf("Connecting to server...\n");

                char *server_ip = argv[1];
                int server_port = atoi(argv[2]);

                printf("Client will connect to server at %s:%d \n", server_ip, server_port);

                client_socket = connectToServer(server_ip, server_port);

                if (client_socket < 0) {
                    printf("Error connecting to server");
                    return ERROR;
                }
                else{
                    printf("Connected to server at %s:%d \n", server_ip, server_port);
                    state = SEND_GUESS_STATE;
                }        
                break;

            case SEND_GUESS_STATE:
                printf("Sending guess to server...\n");
                // state = RECEIVE_FEEDBACK_STATE;
                break;

            case RECEIVE_FEEDBACK_STATE:
                printf("Receiving feedback from server...\n");
                // state = CHECK_WIN_STATUS_STATE;
                break;

            case CHECK_WIN_STATUS_STATE:
                printf("Checking win status...\n");
                // state = EXIT_STATE;
                break;
            
            case WIN_STATE:
                printf("You win!\n");
                // state = EXIT_STATE;
                break;

            case EXIT_STATE:
                printf("Exiting client...\n");
                break;

            default:
                printf("INVALID CLIENT STATE!!!\n");
                break;
        }
    }


    HackerMessage msg;

    msg.type = MSG_START;
    msg.attempts = 0;
    msg.win_status = IN_GAME;

    char guess_string[16];


    while(1) {

        memset(&guess_string, 0, sizeof(guess_string));

        printf("Enter a message to send to the server: \n");
        // scanf("%d%d%d%d%d", &msg.guess[0], &msg.guess[1], &msg.guess[2], &msg.guess[3], &msg.guess[4]);
        fgets(guess_string, sizeof(guess_string), stdin);

        for(int i = 0; i < 5; i++) {
            msg.guess[i] = guess_string[i] - '0';
        }

        write(client_socket, &msg, sizeof(msg));

        //limpar o buffer
        memset(&msg, 0, sizeof(msg));
    }

    return 0;
}