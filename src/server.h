#define START_SERVER_STATE 0
#define SETTING_UP_SERVER_STATE 1
#define WAIT_FOR_CONNECTION_STATE 2
#define WAIT_FOR_MESSAGE_STATE 3
#define RECEIVED_MESSAGE_STATE 4
#define SEND_FEEDBACK_STATE 5
#define CHECK_WIN_STATUS_STATE 6
#define EXIT_STATE 7



int setUpServer(char *ip, int port);
int validateInfoToSetUpServer(char *ip, int port);
int waitForClientConnection(int server_socket);