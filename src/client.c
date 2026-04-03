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

int readMessageFromServer(int client_socket, HackerMessage *msg) {
    int total = 0;
    while (total < sizeof(HackerMessage)) {
        // int n = read(client_socket, ((char*)msg) + total, sizeof(HackerMessage) - total);
        int n = recv(client_socket, ((char*)msg) + total, sizeof(HackerMessage) - total, 0);

        // printf("Read %d bytes from server socket\n", n);

        if (n <= 0){
            return ERROR;
        }
        total += n;
    }
    return OK;
}

void convertFeedback(HackerMessage *msg_received, char *feedback) {

    // printf("Guess: %d %d %d %d %d \n", msg_received->guess[0], msg_received->guess[1], msg_received->guess[2], msg_received->guess[3], msg_received->guess[4]);
    // printf("Feedback: %d %d %d %d %d \n", msg_received->feedback[0], msg_received->feedback[1], msg_received->feedback[2], msg_received->feedback[3], msg_received->feedback[4]);

    for(int i = 0; i < 5; i++) {
        if (msg_received->feedback[i] == RIGHT_POSITION) {
            feedback[i] = msg_received->guess[i] + '0';
        }
        else if (msg_received->feedback[i] == WRONG_POSITION) {
            feedback[i] = '*';
        }
        else if (msg_received->feedback[i] == NOT_IN_CODE) {
            feedback[i] = '-';
        }
    }

    feedback[5] = '\0';

    printf("Feedback converted: %s \n", feedback);
}

int main(int argc, char **argv) {

    argc_counter = argc;

    state = START_CLIENT_STATE;
    printf("Starting client...\n");

    while(1)
    {
        switch (state)
        {
            case START_CLIENT_STATE:
                // printf("Starting client...\n");
                state = CONNECT_TO_SERVER_STATE;
                printf("Connecting to server...\n");
                break;
            
            case CONNECT_TO_SERVER_STATE: {
                // printf("Connecting to server...\n");

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
                    state = SEND_START_MESSAGE_STATE;
                    printf("Sending start message to server...\n");
                }        
                break;
            }

            case SEND_START_MESSAGE_STATE:
                // printf("Sending start message to server...\n");

                memset(&msg_sent, 0, sizeof(msg_sent));

                msg_sent.type = MSG_START;

                // write(client_socket, &msg_sent, sizeof(msg_sent));
                send(client_socket, &msg_sent, sizeof(msg_sent), 0);

                state = SEND_GUESS_STATE;
                printf("Sending guess to server...\n");
                break;

            case SEND_GUESS_STATE:
                // printf("Sending guess to server...\n");
                printf("Enter a message to send to the server: \n");

                char guess_string[16];

                memset(&guess_string, 0, sizeof(guess_string));

                fgets(guess_string, sizeof(guess_string), stdin);

                memset(&msg_sent, 0, sizeof(msg_sent));

                msg_sent.type = MSG_GUESS;

                for(int i = 0; i < 5; i++) {
                    msg_sent.guess[i] = guess_string[i] - '0';
                }

                // write(client_socket, &msg_sent, sizeof(msg_sent));
                send(client_socket, &msg_sent, sizeof(msg_sent), 0);

                state = WAIT_FOR_FEEDBACK_STATE;
                printf("Waiting for feedback from server...\n");

                break;

            case WAIT_FOR_FEEDBACK_STATE:
                // printf("Receiving feedback from server...\n");
                // state = CHECK_WIN_STATUS_STATE;

                if(readMessageFromServer(client_socket, &msg_received) == ERROR) {
                    printf("Error reading message from server");
                    return ERROR;
                }

                if(msg_received.type == MSG_FEEDBACK){
                    switch (msg_received.win_status)
                    {
                        case IN_GAME:
                            state = RECEIVED_IN_GAME_FEEDBACK_STATE;
                            printf("Feedback received from server. Game is still in progress...\n");
                            break;
                        
                        case WIN:
                            state = RECEIVED_WIN_FEEDBACK_STATE;
                            printf("Feedback received from server. You win!\n");
                            break;
                        
                        case ERROR:
                            state = RECEIVED_ERROR_FEEDBACK_STATE;
                            printf("Feedback received from server. An error occurred...\n");
                            break;

                        default:
                            printf("Invalid win status received from server. Expected WIN, IN_GAME or ERROR.\n");
                            return ERROR;
                            break;
                    }
                }
                else{
                    printf("Invalid message type received from server. Expected MSG_FEEDBACK.\n");
                    return ERROR;
                }
                // else if (msg_received.type == MSG_FEEDBACK) {
                //     state = RECEIVE_FEEDBACK_STATE;
                //     printf("Feedback received from server...\n");
                // }
                // else{
                //     printf("Invalid message type received from server. Expected MSG_FEEDBACK.\n");
                //     return ERROR;
                // }
                break;

            case RECEIVED_IN_GAME_FEEDBACK_STATE:
                // printf("Feedback received from server. Game is still in progress...\n");

                printf("Feedback received from server. Dicas: %d %d %d %d %d \n", msg_received.feedback[0], msg_received.feedback[1], msg_received.feedback[2], msg_received.feedback[3], msg_received.feedback[4]);

                char *feedbackConverted = (char*) malloc(16 * sizeof(char));

                convertFeedback(&msg_received, feedbackConverted);

                printf("Feedback received from server. Dicas convertidas: %s \n", feedbackConverted);
                printf("Tentativas: %d \n", msg_received.attempts);

                state = SEND_GUESS_STATE;
                printf("Sending guess to server...\n");
                break;

            case RECEIVED_WIN_FEEDBACK_STATE:
                // printf("Feedback received from server. You win!\n");
                state = WIN_STATE;

                memset(&msg_sent, 0, sizeof(msg_sent));
                msg_sent.type = MSG_EXIT;
                
                send(client_socket, &msg_sent, sizeof(msg_sent), 0);
                
                break;
            
            case WIN_STATE:
                printf("You win!\n");

                close(client_socket);
                // state = EXIT_STATE;

                state = EXIT_STATE;
                printf("Exiting client...\n");

                break;

            case EXIT_STATE:
                printf("Exited client\n");

                exit(0);
                break;

            default:
                printf("INVALID CLIENT STATE!!!\n");
                break;
        }
    }
}