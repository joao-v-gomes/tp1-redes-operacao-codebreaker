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

// Conta os argumentos para validar a quantidade de argumentos passados na linha de comando
int argc_counter;

// Variáveis globais para o estado da FSM e o socket do cliente
int state;
int client_socket;

// Estruturas para armazenar as mensagens enviadas e recebidas. Uma de cada vez...
HackerMessage msg_sent;
HackerMessage msg_received;

// Valida a string do palpite.
// O palpite deve conter exatamente 5 caracteres, todos numéricos (0-9).
int isValidGuess(const char *guess_string) {
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

// Se conecta ao servidor usando o IP e a porta fornecidos. Retorna o socket do cliente ou ERROR em caso de falha.
// Testa a conexao IpV4 primeiro, se falhar tenta o IPv6. Se ambos falharem, retorna ERROR.
int connectToServer(char *server_ip, int server_port) {
    if(validateInfoToConnectToServer(server_ip, server_port) != 0) {
        return ERROR;
    }

    int client_socket = ERROR;

    struct sockaddr_in server_address_v4;
    memset(&server_address_v4, 0, sizeof(server_address_v4));
    server_address_v4.sin_family = AF_INET;
    server_address_v4.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &server_address_v4.sin_addr) == 1) {
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket < 0) {
            return ERROR;
        }

        if (connect(client_socket, (struct sockaddr *)&server_address_v4, sizeof(server_address_v4)) < 0) {
            close(client_socket);
            return ERROR;
        }

        return client_socket;
    }

    struct sockaddr_in6 server_address_v6;
    memset(&server_address_v6, 0, sizeof(server_address_v6));
    server_address_v6.sin6_family = AF_INET6;
    server_address_v6.sin6_port = htons(server_port);

    if (inet_pton(AF_INET6, server_ip, &server_address_v6.sin6_addr) == 1) {
        client_socket = socket(AF_INET6, SOCK_STREAM, 0);
        if (client_socket < 0) {
            return ERROR;
        }

        if (connect(client_socket, (struct sockaddr *)&server_address_v6, sizeof(server_address_v6)) < 0) {
            close(client_socket);
            return ERROR;
        }

        return client_socket;
    }

    return ERROR;
}

// Valida as informações de configuração do cliente. Retorna OK se as informações forem válidas ou ERROR caso contrário.
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

    // Verificar se o IP do servidor é válido (IPv4 ou IPv6 literal)
    struct in_addr sa_v4;
    struct in6_addr sa_v6;

    if (inet_pton(AF_INET, server_ip, &sa_v4) != 1 && inet_pton(AF_INET6, server_ip, &sa_v6) != 1) {
#ifdef DEBUG
        printf("Endereço IP inválido.\n");
#endif
        return ERROR;
    }

    return OK;
}


// Lê uma mensagem do servidor. Retorna OK se a leitura for bem-sucedida ou ERROR em caso de falha.
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

// Converte o feedback recebido do servidor em uma string legível para o usuário. 
// O feedback é composto por 5 caracteres, onde cada caractere representa a avaliação de um dígito do palpite:
// - Se o dígito está na posição correta, o caractere é o próprio dígito (0-9).
// - Se o dígito está presente no código, mas na posição errada, o caractere é '*'
// - Se o dígito não está presente no código, o caractere é '_'
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

    // Inicializa o estado da FSM
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

                // Antes de enviar uma msg, limpa a estrutura para evitar enviar lixo de memória
                memset(&msg_sent, 0, sizeof(msg_sent));

                msg_sent.type = MSG_START;

                send(client_socket, &msg_sent, sizeof(msg_sent), 0);

                state = SEND_GUESS_STATE;
                break;

            case SEND_GUESS_STATE:
                printf("Insira seu palpite:\n");

                // 5 dígitos + '\n' + '\0' = 7 caracteres
                char guess_string[7];

                // Limpa a string do palpite para evitar lixo de memória
                memset(guess_string, 0, sizeof(guess_string));

                // Comecei usando scanf, mas o fgets foi melhor
                if (fgets(guess_string, sizeof(guess_string), stdin) == NULL) {
                    return ERROR;
                }

                // Check na validade do palpite
                if (!isValidGuess(guess_string)) {
                    printf("Insira uma sequência válida!\n");
                    state = SEND_GUESS_STATE;
                    break;
                }

                memset(&msg_sent, 0, sizeof(msg_sent));

                // Seta o tipo da msg
                msg_sent.type = MSG_GUESS;

                // Converte para um array de int
                for(int i = 0; i < 5; i++) {
                    msg_sent.guess[i] = guess_string[i] - '0';
                }

                send(client_socket, &msg_sent, sizeof(msg_sent), 0);
                
                // Aguarda o feedback do servidor
                state = WAIT_FOR_FEEDBACK_STATE;

                break;

            case WAIT_FOR_FEEDBACK_STATE:

                // le a msg recebida em msg_received
                if(readMessageFromServer(client_socket, &msg_received) == ERROR) {
                    return ERROR;
                }

                // Classifica a msg de feedback
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

                // Verifica o feedback e converte para as dicas
                convertFeedback(&msg_received, feedbackConverted);

                printf("Dica: %s\n", feedbackConverted);
                printf("Tentativas realizadas: %d\n", msg_received.attempts);

                // Volta para o estado de enviar palpite
                state = SEND_GUESS_STATE;
                break;
            }

            case RECEIVED_WIN_FEEDBACK_STATE:
                printf("Acesso concedido! Thaísa recuperou o sistema!\n");

                // Envia a mensagem de saída para o servidor
                memset(&msg_sent, 0, sizeof(msg_sent));
                msg_sent.type = MSG_EXIT;
                
                send(client_socket, &msg_sent, sizeof(msg_sent), 0);

                // Fecha o socket do cliente
                close(client_socket);

                state = WIN_STATE;
                
                break;

            // Caso o palpite seja inválido, 
            // o servidor retorna um feedback com win_status = ERROR.
            //Nesse caso, o cliente deve informar o usuário e pedir um novo palpite.
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