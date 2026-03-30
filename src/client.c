#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "util.h"

int main(int argc, char **argv) {
    
    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    printf("Client will connect to server at %s:%d \n", server_ip, server_port);

    //create a socket and connect to the server
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_address.sin_addr);

    connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address));
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