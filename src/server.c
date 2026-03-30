#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "util.h"

int main(int argc, char **argv) {

    char *ip = argv[1];
    int port = atoi(argv[2]);
    char *code = argv[3];

    printf("Server is running on %s:%d and the code is %s \n", ip, port, code);

    //create a socket and bind it to the port
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_address.sin_addr);

    bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));

    //listen for incoming connections
    listen(server_socket, 1);

    // char buffer[1024];

    int client_socket = accept(server_socket, NULL, NULL);

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