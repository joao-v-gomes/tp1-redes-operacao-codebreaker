//Defines para a FSM

#define START_CLIENT_STATE 0
#define CONNECT_TO_SERVER_STATE 1
#define SEND_START_MESSAGE_STATE 2
#define SEND_GUESS_STATE 3
#define WAIT_FOR_FEEDBACK_STATE 4
#define RECEIVE_FEEDBACK_STATE 5
#define CHECK_WIN_STATUS_STATE 6
#define WIN_STATE 7
#define EXIT_STATE 8




int connectToServer(char *server_ip, int server_port);
int validateInfoToConnectToServer(char *server_ip, int server_port);