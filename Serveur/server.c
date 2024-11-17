#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server.h"
#include "client.h"
#include "../game.h"

// DECLARATION DES FONCTIONS UTILISEES SEULEMENT DANS CE FICHIER
int init_connection(void);
int inscrireClient(const char *name);
int deconnecterClient(Client *clients, int i, int *actual);
void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server);
int listClients(Client clients[], int index, int *actual);
void clear_clients(Client *clients, int actual);
void remove_client(Client *clients, int to_remove, int *actual);
int deconnecterServeur(Client *clients, int i, int *actual);
void end_connection(int sock);

void init(void)
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

void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

void app(void)
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
               write_client(csock, "Vous êtes déjà connecté");
               closesocket(csock);
               continue;
            }
            else if (result == -1) // verifier le -1
            {
               write_client(csock, "Erreur lors de l'inscription");
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
               clients[actual].tour = no;
               write_client(csock, "Bienvenue dans le jeu Awale !\nVeuillez choisir une option :\n1. Jouer contre un adversaire en ligne\n2. Quitter le jeu");
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
               write_client(clients[i].sock, "\nUn nouveau joueur s'est connecté\n Tapez 1 pour jouer avec un autre joeur");
            }
            /* a client is talking */
            if (FD_ISSET(clients[i].sock, &rdfs))
            {

               int c = read_client(clients[i].sock, buffer);
               /* client disconnected */
               if (c == 0)
               {
                  if (clients[i].etat == EnPartie)
                  {

                     clients[i].opponent->etat = Initialisation;
                     clients[i].opponent->opponent = NULL;
                     clients[i].opponent->game = NULL;
                     write_client(clients[i].opponent->sock, "Votre adversaire s'est déconnecté\nVeuillez choisir une option :\n1. Jouer contre un adversaire en ligne\n2. Quitter le jeu");
                  }

                  deconnecterClient(clients, i, &actual);
               }

               else
               {

                  if (clients[i].etat == Initialisation)

                  {

                     if (strcmp(buffer, "1") == 0)
                     {
                        write_client(clients[i].sock, "\nVous avez choisi de jouer avec un autre joueur.\nVoici la liste des clients connectés:\n");
                        listClients(clients, i, &actual);
                     }
                     if (strcmp(buffer, "2") == 0)
                     {
                        write_client(clients[i].sock, "\nVous avez choisi de quitter");
                        deconnecterClient(clients, i, &actual);
                     }
                  }
                  else if (clients[i].etat == Enattente)
                  {
                     if (strcmp(buffer, "1") == 0)
                     {
                        write_client(clients[i].sock, "\nVous avez choisi de jouer avec un autre joueur.\nVoici la liste des clients connectés:\n");
                        listClients(clients, i, &actual);
                     }
                  }
                  else if (clients[i].etat == DemandeDePartie)
                  {
                     int opponent_index = atoi(buffer);
                     if (opponent_index < 0 || opponent_index >= actual)
                     {
                        write_client(clients[i].sock, "\nNuméro de joueur invalide");
                     }
                     else
                     {
                        if (clients[opponent_index].etat != EnPartie || clients[opponent_index].etat != EnvoieReponse || clients[opponent_index].etat != DemandeDePartie)
                        {
                           char message[256];
                           snprintf(message, sizeof(message), "\nDemande de partie de %s\nVeuillez répondre par Y ou N", clients[i].name);
                           write_client(clients[opponent_index].sock, message);
                           clients[opponent_index].etat = EnvoieReponse;
                           clients[i].opponent = &clients[opponent_index];
                           clients[opponent_index].opponent = &clients[i];
                        }
                        else
                        {
                           write_client(clients[i].sock, "\nLe joueur choisi est déjà en partie\nVeuillez chosir un autre joueur");
                           listClients(clients, i, &actual);
                        }
                     }
                  }
                  else if (clients[i].etat == EnvoieReponse)
                  {
                     if (strcmp(buffer, "Y") == 0)
                     {
                        write_client(clients[i].sock, "\nVous avez accepté la demande de partie\n");
                        write_client(clients[i].opponent->sock, "\nVotre demande de partie a été acceptée\n");
                        start_game(&clients[i], clients[i].opponent);
                     }
                     else if (strcmp(buffer, "N") == 0)
                     {
                        write_client(clients[i].sock, "\nVous avez refusé la demande de partie\n");
                        write_client(clients[i].opponent->sock, "\nVotre demande de partie a été refusée\n");
                        clients[i].etat = Initialisation;
                        clients[i].opponent->etat = Initialisation;
                     }
                     else
                     {
                        write_client(clients[i].sock, "\nVeuillez répondre par Y ou N");
                     }
                  }
                  else if (clients[i].etat == EnPartie && clients[i].tour == yes)
                  {
                     printf("Client %s joue\n", clients[i].name);
                     fflush(stdout);
                     jouerCoup(clients[i].game, buffer);
                     display_board(clients[i].game);
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

void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for (i = 0; i < actual; i++)
   {
      deconnecterServeur(clients, i, &actual);
   }
}

void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
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

int init_connection(void)
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

void end_connection(int sock)
{
   closesocket(sock);
}

int read_client(SOCKET sock, char *buffer)
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

int inscrireClient(const char *name)
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

   printf("Joueur %s inscrit et connecté\n", name);
   fflush(stdout);
   return 1;
}

int listClients(Client clients[], int index, int *actual)
{
   write_client(clients[index].sock, "==============================");

   int found = 0;
   for (int i = 0; i < *actual; i++)
   {
      if (i != index)
      {
         char message[256];
         if (clients[i].etat == EnPartie)
         {
            snprintf(message, sizeof(message), "\n%d: %s - En partie", i, clients[i].name);
         }
         else
         {
            snprintf(message, sizeof(message), "\n%d: %s - Disponible", i, clients[i].name);
         }
         write_client(clients[index].sock, message);
         found = 1;
      }
   }

   if (!found)
   {
      write_client(clients[index].sock, "\nAucun autre joueur n'est connecté.");
   }

   write_client(clients[index].sock, "\n==============================");
   write_client(clients[index].sock, "\nVeuillez choisir un numéro de joueur :");

   clients[index].etat = DemandeDePartie;
   return 1;
}

int deconnecterClient(Client *clients, int i, int *actual)
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

   printf("Joueur %s déconnecté\n", clients[i].name);
   fflush(stdout);
   closesocket(clients[i].sock);
   remove_client(clients, i, actual);

   return 1;
}

int deconnecterServeur(Client *clients, int i, int *actual)
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

   printf("Joueur %s déconnecté\n", clients[i].name);
   fflush(stdout);

   closesocket(clients[i].sock);
   return 1;
}

void start_game(Client *client1, Client *client2)
{
   Game *game = create_game(client1, client2);
   if (game)
   {
      client1->etat = EnPartie;
      client2->etat = EnPartie;
      client1->game = game;
      client2->game = game;
      initialiserGame(game, client1, client2);
      display_board(game);
   }
   else
   {
      write_client(client1->sock, "Erreur lors du démarrage de la partie.\n");
      write_client(client2->sock, "Erreur lors du démarrage de la partie.\n");
   }
}
