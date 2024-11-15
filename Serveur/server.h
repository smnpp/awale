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
void parse_command(Client *client, const char *command, Client *clients, int actual);
Client *find_client_by_name(Client *clients, int actual, const char *name);
#endif /* guard */