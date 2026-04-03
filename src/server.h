#define START_SERVER_STATE 0
#define SETTING_UP_SERVER_STATE 1
#define WAIT_FOR_CONNECTION_STATE 2
#define WAIT_FOR_START_MESSAGE_STATE 3
#define RECEIVED_START_MESSAGE_STATE 4
#define WAITING_FOR_MESSAGE_STATE 5
#define RECEIVED_GUESS_MESSAGE_STATE 6
// #define CHECK_GUESS_AND_PREPARE_FEEDBACK_STATE 7
#define SEND_FEEDBACK_STATE 8
#define WAIT_FOR_EXIT_MESSAGE_STATE 9
#define EXIT_STATE 10



int setUpServer(char *ip, int port, int code);
int validateInfoToSetUpServer(char *ip, int port, int code);
int waitForClientConnection(int server_socket);