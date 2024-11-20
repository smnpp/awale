#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock.h>

#elif defined(linux) || defined(__APPLE__)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h>  /* gethostbyname */
#include "../3rdparty/cJSON/cJSON.h"
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

#define CRLF "\r\n"
#define PORT 5788
#define MAX_CLIENTS 100

#define BUF_SIZE 1024

#define CMD_PLAY "/play"
#define CMD_LIST "/list"
#define CMD_OBSERVE "/observe"
#define CMD_HELP "/help"
#define CMD_QUIT "/quit"
#define CMD_MSG "/msg"
#define CMD_ALL "/all"
#define CMD_GAMES "/games"
#define CMD_ACCEPT "/accept"
#define CMD_DECLINE "/decline"
#define CMD_LOGOUT "/logout"
#define CMD_WRITEBIO "/writebio"
#define CMD_READBIO "/readbio"
#define CMD_ADD_FRIEND "/addfriend"
#define CMD_REMOVE_FRIEND "/removefriend"
#define CMD_LIST_FRIENDS "/friends"
#define CMD_MATCHMAKING "/matchmaking"
#define CMD_PRIVATE "/private"
#define CMD_PUBLIC "/public"
#define CMD_HISTORY "/history"
#define CMD_WATCH "/watch"

#include "client.h"
#include "../game.h"

// static void init(void);
// static void end(void);
// static void app(void);
// static int init_connection(void);
// static void end_connection(int sock);
int read_client(SOCKET sock, char *buffer);
void write_client(SOCKET sock, const char *buffer);
// static void send_message_to_all_clients(Client *clients, Client client, int actual, const char *buffer, char from_server);
// static void remove_client(Client *clients, int to_remove, int *actual);
// static void clear_clients(Client *clients, int actual);
// static int inscrireClient(const char *name);
// static int listClients(Client clients[], int index, int *actual);
// static int deconnecterClient(Client *clients, int to_remove, int *actual);
// static int deconnecterServeur(Client *clients, int to_remove, int *actual);
void start_game(Client *client1, Client *client2);
void listParties(Client *clients, int index, int *actual);
int send_message_to_client_by_name(Client *clients, int actual, const char *sender_name, const char *target_name, const char *message);
void send_message_to_all_clients(Client *clients, const char *sender_name, int actual, const char *message);
void send_notification(Client *client, const char *format);
void display_help(Client *client);
void process_command(Client *client, char *buffer, Client *clients, int *actual);
cJSON *load_friends_json(void);
void load_friends_from_json(Client *client);
void save_friends_json(cJSON *root);
void add_friend(Client *client, const char *friend_name);
void remove_friend(Client *client, const char *friend_name);
void list_friends(Client *client);
bool is_friend(Client *client, const char *friend_name);
void write_bio(Client *client, const char *bio);
void read_bio(Client *client, Client *clients, int actual, const char *target_name);
void display_board_replay(Client *client, Game *game, const char *player1_name, const char *player2_name);
void watch_game(Client *client, const char *game_id_str);
void display_board_history(Client *client, cJSON *board_state, const char *player1_name, const char *player2_name, int score1, int score2);
void display_game_history(Client *client);
#endif /* guard */