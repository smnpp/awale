#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server.h"
#include "client.h"
#include "../game.h"

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if (err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

static void app(void)
{
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE];
   /* the index for the array */
   int actual = 0;
   int max = sock;
   /* an array for all clients */
   Client clients[MAX_CLIENTS];

   fd_set rdfs;

   while (1)
   {
      int i = 0;
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for (i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if (FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if (FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = {0};
         socklen_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if (csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* after connecting the client sends its name */
         if (read_client(csock, buffer) > 0)
         {
            int result = inscrireClient(buffer);

            if (result == 2)
            {
               write_client(csock, "Vous êtes déjà connecté\n");
               closesocket(csock);
               continue;
            }
            else if (result == -1) // verifier le -1
            {
               write_client(csock, "Erreur lors de l'inscription\n");
               closesocket(csock);
               continue;
            }
            else
            {
               max = csock > max ? csock : max;

               FD_SET(csock, &rdfs);

               Client c = {csock};
               strncpy(c.name, buffer, BUF_SIZE - 1);
               clients[actual] = c;
               clients[actual].etat = Initialisation;
               write_client(csock, "Veuillez choisir:\n 1. Jouer avec un autre client\n 2. Quitter\n");
               actual++;
            }
         }
      }
      else
      {
         int i = 0;
         for (i = 0; i < actual; i++)
         {
            if (clients[i].etat == Enattente)
            {
               write_client(clients[i].sock, "Un nouveau client s'est connecter\n Tapez 1 pour jouer avec un autre client\n");
            }
            /* a client is talking */
            if (FD_ISSET(clients[i].sock, &rdfs))
            {
               printf("test2 ");
               fflush(stdout);
               Client client = clients[i];

               int c = read_client(clients[i].sock, buffer);
               /* client disconnected */
               if (c == 0)
               {

                  deconnecterClient(clients, i, &actual);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
               }

               else
               {
                  printf("on es arrive %s", buffer);
                  fflush(stdout);

                  if (clients[i].etat == Initialisation)

                  {

                     if (strcmp(buffer, "1") == 0)
                     {
                        write_client(clients[i].sock, "Vous avez choisi de jouer avec un autre client\nVoici la liste des clients connectés:\n");
                        listClients(clients, i, actual);
                     }
                     if (strcmp(buffer, "2") == 0)
                     {
                        write_client(clients[i].sock, "Vous avez choisi de quitter\n");
                        deconnecterClient(clients, i, &actual);
                     }
                  }
                  else if (clients[i].etat == Enattente)
                  {
                     if (strcmp(buffer, "1") == 0)
                     {
                        write_client(clients[i].sock, "Vous avez choisi de jouer avec un autre client\nVoici la liste des clients connectés:\n");
                        listClients(clients, i, actual);
                     }
                  }
               }
               break;
            }
         }
      }
   }
   clear_clients(clients, actual);
   end_connection(sock);
}

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for (i = 0; i < actual; i++)
   {
      deconnecterServeur(clients, i, &actual);
   }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for (i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if (sender.sock != clients[i].sock)
      {
         if (from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = {0};

   if (sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if (bind(sock, (SOCKADDR *)&sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

void write_client(SOCKET sock, const char *buffer)
{
   if (send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}

static int inscrireClient(const char *name)
{
   printf("Inscription du client %s\n", name);
   fflush(stdout);
   FILE *fichier = fopen("./data/clients.csv", "r+");

   FILE *tempFile = fopen("./data/temp.csv", "w");
   if (!fichier || !tempFile)
   {
      perror("Erreur d'ouverture des fichiers");
      return -1;
   }
   char name_used[256];
   snprintf(name_used, sizeof(name_used), "%s,", name);
   char line[256];
   int clientExists = 0, clientConnected = 0;
   while (fgets(line, sizeof(line), fichier))
   {
      if (strstr(line, name_used))
      {

         clientExists = 1;
         if (strstr(line, "+"))
         {
            clientConnected = 1;
         }
         else
         {
            fprintf(tempFile, "%s, +\n", name);
         }
      }
      else
      {
         fputs(line, tempFile);
      }
   }

   if (!clientExists)
   {
      fprintf(tempFile, "%s, +\n", name);
   }
   else if (clientConnected)
   {
      printf("Client %s déjà connecté\n", name);
      fflush(stdout);
      fclose(fichier);
      fclose(tempFile);
      remove("./data/temp.csv");
      return 2;
   }

   fclose(fichier);
   fclose(tempFile);
   rename("./data/temp.csv", "./data/clients.csv");

   printf("Client %s inscrit et connecté\n", name);
   fflush(stdout);
   return 1;
}
static int listClients(Client clients[], int index, int actual)
{
   for (int i = 0; i < actual; i++)
   {
      if (actual == 1)
      {
         clients[index].etat = Enattente;
         write_client(clients[index].sock, "Aucun autre client connecté\n");
         return 0;
      }
      if (i != index)
      {

         char message[256];
         printf("Client %s\n", clients[i].name);
         fflush(stdout);
         snprintf(message, sizeof(message), "%s\n", clients[i].name);
         write_client(clients[index].sock, message);
      }
   }
   return 1;
}

static int deconnecterClient(Client *clients, int i, int *actual)
{

   FILE *fichier = fopen("./data/clients.csv", "r+");
   FILE *tempFile = fopen("./data/tempD.csv", "w");
   if (!fichier)
   {
      perror("Erreur d'ouverture des fichiers");
      return -1;
   }
   char line[256];
   char name_used[256];
   snprintf(name_used, sizeof(name_used), "%s,", clients[i].name);
   while (fgets(line, sizeof(line), fichier))
   {
      if (strstr(line, name_used))
      {
         fprintf(tempFile, "%s, -\n", clients[i].name);
      }
      else
      {
         fputs(line, tempFile);
      }
   }
   fclose(fichier);
   fclose(tempFile);
   rename("./data/tempD.csv", "./data/clients.csv");

   printf("Client %s déconnecté\n", clients[i].name);
   fflush(stdout);
   remove_client(clients, i, actual);
   closesocket(clients[i].sock);
   return 1;
}
static int deconnecterServeur(Client *clients, int i, int *actual)
{

   FILE *fichier = fopen("./data/clients.csv", "r+");
   FILE *tempFile = fopen("./data/tempD.csv", "w");
   if (!fichier)
   {
      perror("Erreur d'ouverture des fichiers");
      return -1;
   }
   char line[256];
   char name_used[256];
   snprintf(name_used, sizeof(name_used), "%s,", clients[i].name);
   while (fgets(line, sizeof(line), fichier))
   {
      if (strstr(line, name_used))
      {
         fprintf(tempFile, "%s, -\n", clients[i].name);
      }
      else
      {
         fputs(line, tempFile);
      }
   }
   fclose(fichier);
   fclose(tempFile);
   rename("./data/tempD.csv", "./data/clients.csv");

   printf("Client %s déconnecté\n", clients[i].name);
   fflush(stdout);

   closesocket(clients[i].sock);
   return 1;
}

void start_game(Client *client1, Client *client2)
{
   Game *game = create_game(client1, client2);
   if (game)
   {
      printf("Nouvelle partie démarrée entre %s et %s\n", client1->name, client2->name);
   }
   else
   {
      printf("Limite de parties atteinte\n");
   }
}

void parse_command(Client *client, const char *command, Client *clients, int actual)
{
   if (strncmp(command, "DEFY", 4) == 0)
   {
      char opponent_name[BUF_SIZE];
      sscanf(command + 5, "%s", opponent_name);
      Client *opponent = find_client_by_name(clients, actual, opponent_name);

      if (opponent)
      {
         start_game(client, opponent);
      }
      else
      {
         write_client(client->sock, "Adversaire non trouvé\n");
      }
   }
}

#include <string.h>
#include "client.h"

Client *find_client_by_name(Client *clients, int actual, const char *name)
{
   for (int i = 0; i < actual; i++)
   {
      if (strcmp(clients[i].name, name) == 0)
      {
         return &clients[i];
      }
   }
   return NULL; // Retourne NULL si le client n'est pas trouvé
}
