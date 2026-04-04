#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "util.h"
#include "server.h"
#include "ifaddrs.h"

// #define DEBUG

int state;

int argc_counter;

int server_socket = 0;
int client_socket = 0;
int code;
int attempts_counter = 0;

HackerMessage msg_received;
HackerMessage msg_to_send;

static const char *getProtocolType(const char *protocol) {
    if (strcmp(protocol, "v6") == 0) {
        return "IPv6";
    }

    return "IPv4";
}

int isValidGuess(const int *guess) {
    for (int i = 0; i < 5; i++) {
        if (guess[i] < 0 || guess[i] > 9) {
            return 0;
        }
    }

    return 1;
}

int setServerSocketForIPv4(int *server_socket, struct sockaddr_in *server_address) {
    *server_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_address->sin_family = AF_INET;
    return OK;
}

int setServerSocketForIPv6(int *server_socket, struct sockaddr_in *server_address) {
    *server_socket = socket(AF_INET6, SOCK_STREAM, 0);
    server_address->sin_family = AF_INET6;
    return OK;
}

int setUpServer(char *ip, int port, int code) {

    // Validar as informações de configuração do servidor
    if (validateInfoToSetUpServer(ip,port,code) != 0) {
        return ERROR;
    }

    int server_socket = 0;
    struct sockaddr_in server_address;
    
    //Set ipv4 or ipv6
    if (strcmp(ip, "v4") == 0) {
        setServerSocketForIPv4(&server_socket, &server_address);
    }
    else if (strcmp(ip, "v6") == 0) {
        setServerSocketForIPv6(&server_socket, &server_address);
    }
    else{
        return ERROR;
    }

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
#ifdef DEBUG
        printf("Uso: ./server <protocolo> <porta> <senha>\n");
#endif
        return ERROR;
    }

    if(strcmp(ip, "v4") != 0 && strcmp(ip, "v6") != 0) {
#ifdef DEBUG
        printf("Protocolo inválido. Use v4 ou v6.\n");
#endif
        return ERROR;
    }

    // Verificar se porta e valida
    if (port <= 0 || port > 65535) {
#ifdef DEBUG
        printf("Porta inválida. Use um valor entre 1 e 65535.\n");
#endif
        return ERROR;
    }

    if(code < 0 || code > 99999) {
#ifdef DEBUG
        printf("Senha inválida. Use um valor entre 00000 e 99999.\n");
#endif
        return ERROR;
    }

    return OK;
}

int waitForClientConnection(int server_socket) {
    int client_socket = accept(server_socket, NULL, NULL);
    if (client_socket < 0) {
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

    argc_counter = argc;
    code = atoi(argv[3]);

    // printf("Code to guess: %d \n", code);

    while (1)
    {
        switch (state)
        {
            case START_SERVER_STATE:
                state = SETTING_UP_SERVER_STATE;
                break;
            
            //Precisei colocar o case dentro de chaves para declarar variáveis locais
            case SETTING_UP_SERVER_STATE: {
                char *ip_type = argv[1];
                int port = atoi(argv[2]);

                server_socket = setUpServer(ip_type, port, code);

                if (server_socket < 0) {
                    return ERROR;
                }
                else{
                    printf("Servidor iniciado em modo %s na porta %d.\n", getProtocolType(ip_type), port);
                    state = WAIT_FOR_CONNECTION_STATE;
                }

                break;
            }

            case WAIT_FOR_CONNECTION_STATE:
                client_socket = waitForClientConnection(server_socket);
                
                if (client_socket < 0) {
                    return ERROR;
                }
                else{
                    printf("Cliente conectado\n");
                    state = WAIT_FOR_START_MESSAGE_STATE;
                }

                break;

            case WAIT_FOR_START_MESSAGE_STATE:
                memset(&msg_received, 0, sizeof(msg_received));

                if (readMessageFromClient(client_socket, &msg_received) == ERROR) {
                    return ERROR;
                }

                if(msg_received.type == MSG_START) {
                    state = RECEIVED_START_MESSAGE_STATE;
                }
                else{
                    return ERROR;
                }

                break;

            case RECEIVED_START_MESSAGE_STATE:
                state = WAITING_FOR_MESSAGE_STATE;
                attempts_counter = 1;

                memset(&msg_received, 0, sizeof(msg_received));
                memset(&msg_to_send, 0, sizeof(msg_to_send));

                break;    

            case WAITING_FOR_MESSAGE_STATE:
                memset(&msg_received, 0, sizeof(msg_received));
                if (readMessageFromClient(client_socket, &msg_received) == ERROR) {
                    return ERROR;
                }
                else if (msg_received.type == MSG_GUESS) {
                    state = RECEIVED_GUESS_MESSAGE_STATE;
                }
                else{
                    return ERROR;
                }

                break;
            
            case RECEIVED_GUESS_MESSAGE_STATE: {
                state = SEND_FEEDBACK_STATE;

                break;
            }

            case SEND_FEEDBACK_STATE:
                memset(&msg_to_send, 0, sizeof(msg_to_send));
                msg_to_send.type = MSG_FEEDBACK;

                if (!isValidGuess(msg_received.guess)) {
                    msg_to_send.win_status = ERROR;
                    msg_to_send.attempts = attempts_counter;
                    send(client_socket, &msg_to_send, sizeof(msg_to_send), 0);

                    state = WAITING_FOR_MESSAGE_STATE;
                    break;
                }

                fillFeedbackWithGuess(msg_received.guess, &msg_to_send);

                calculateFeedback(msg_received.guess, code, &msg_to_send);

                msg_to_send.attempts = attempts_counter++;

                send(client_socket, &msg_to_send, sizeof(msg_to_send), 0);

                if(msg_to_send.win_status == IN_GAME) {
                    state = WAITING_FOR_MESSAGE_STATE;
                }
                else if (msg_to_send.win_status == WIN) {
                    state = WAIT_FOR_EXIT_MESSAGE_STATE;
                }
                else{
                    return ERROR;
                }

                break;

            case WAIT_FOR_EXIT_MESSAGE_STATE:
                memset(&msg_received, 0, sizeof(msg_received));
                if(readMessageFromClient(client_socket, &msg_received) == ERROR) {
                    return ERROR;
                }

                if (msg_received.type == MSG_EXIT) {
                    state = EXIT_STATE;
                    printf("Cliente desconectado\n");
                    close(client_socket);
                    close(server_socket);

                }
                else{
                    return ERROR;
                }

                break;

            case EXIT_STATE:
                exit(0);
                break;

            default:
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