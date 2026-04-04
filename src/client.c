#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>
#include "util.h"
#include "client.h"

// #define DEBUG

    
int argc_counter;

int state;
int client_socket;

HackerMessage msg_sent;
HackerMessage msg_received;

static int isValidGuess(const char *guess_string) {
    size_t length = strcspn(guess_string, "\n");

    if (length != 5) {
        return 0;
    }

    for (int i = 0; i < 5; i++) {
        if (!isdigit(guess_string[i])) {
            return 0;
        }
    }

    return 1;
}

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
        return ERROR;
    }

    return client_socket;
}

int validateInfoToConnectToServer(char *server_ip, int server_port) {

    // Verificar se o número de argumentos é correto
    if (argc_counter != 3) {
#ifdef DEBUG
        printf("Uso: ./client <endereço ip> <porta>\n");
#endif
        return ERROR;
    }

    // Verificar se porta e valida
    if (server_port <= 0 || server_port > 65535) {
#ifdef DEBUG
        printf("Porta inválida. Use um valor entre 1 e 65535.\n");
#endif
        return ERROR;
    }

    // TODO: Verificar se e IPv4 ou IPv6

    // Verificar se o IP do servidor é válido
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, server_ip, &(sa.sin_addr));
    if (result <= 0) {
#ifdef DEBUG
        printf("Endereço IP inválido.\n");
#endif
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
            feedback[i] = '_';
        }
    }

    feedback[5] = '\0';

}

int main(int argc, char **argv) {

    argc_counter = argc;

    state = START_CLIENT_STATE;

    while(1)
    {
        switch (state)
        {
            case START_CLIENT_STATE:
                state = CONNECT_TO_SERVER_STATE;
                break;
            
            case CONNECT_TO_SERVER_STATE: {
                char *server_ip = argv[1];
                int server_port = atoi(argv[2]);

                client_socket = connectToServer(server_ip, server_port);

                if (client_socket < 0) {
                    return ERROR;
                }

                state = SEND_START_MESSAGE_STATE;
                break;
            }

            case SEND_START_MESSAGE_STATE:
                memset(&msg_sent, 0, sizeof(msg_sent));

                msg_sent.type = MSG_START;

                send(client_socket, &msg_sent, sizeof(msg_sent), 0);

                state = SEND_GUESS_STATE;
                break;

            case SEND_GUESS_STATE:
                printf("Insira seu palpite:\n");

                char guess_string[16];

                memset(guess_string, 0, sizeof(guess_string));

                if (fgets(guess_string, sizeof(guess_string), stdin) == NULL) {
                    return ERROR;
                }

                if (!isValidGuess(guess_string)) {
                    printf("Insira uma sequência válida!\n");
                    state = SEND_GUESS_STATE;
                    break;
                }

                memset(&msg_sent, 0, sizeof(msg_sent));

                msg_sent.type = MSG_GUESS;

                for(int i = 0; i < 5; i++) {
                    msg_sent.guess[i] = guess_string[i] - '0';
                }

                send(client_socket, &msg_sent, sizeof(msg_sent), 0);

                state = WAIT_FOR_FEEDBACK_STATE;

                break;

            case WAIT_FOR_FEEDBACK_STATE:
                if(readMessageFromServer(client_socket, &msg_received) == ERROR) {
                    return ERROR;
                }

                if(msg_received.type == MSG_FEEDBACK){
                    switch (msg_received.win_status)
                    {
                        case IN_GAME:
                            state = RECEIVED_IN_GAME_FEEDBACK_STATE;
                            break;
                        
                        case WIN:
                            state = RECEIVED_WIN_FEEDBACK_STATE;
                            break;
                        
                        case ERROR:
                            state = RECEIVED_ERROR_FEEDBACK_STATE;
                            break;

                        default:
                            return ERROR;
                            break;
                    }
                }
                else{
                    return ERROR;
                }
                break;

            case RECEIVED_IN_GAME_FEEDBACK_STATE: {
                char feedbackConverted[6];

                convertFeedback(&msg_received, feedbackConverted);

                printf("Dica: %s\n", feedbackConverted);
                printf("Tentativas realizadas: %d\n", msg_received.attempts);

                state = SEND_GUESS_STATE;
                break;
            }

            case RECEIVED_WIN_FEEDBACK_STATE:
                printf("Acesso concedido! Thaísa recuperou o sistema!\n");

                memset(&msg_sent, 0, sizeof(msg_sent));
                msg_sent.type = MSG_EXIT;
                
                send(client_socket, &msg_sent, sizeof(msg_sent), 0);

                close(client_socket);

                state = WIN_STATE;
                
                break;

            case RECEIVED_ERROR_FEEDBACK_STATE:
                printf("Insira uma sequência válida!\n");
                state = SEND_GUESS_STATE;
                break;
            
            case WIN_STATE:
                state = EXIT_STATE;

                break;

            case EXIT_STATE:
                exit(0);
                break;

            default:
                break;
        }
    }
}