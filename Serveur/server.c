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
               write_client(csock, "Bienvenue dans le jeu Awale !\nVeuillez choisir une option :\n1. Jouer contre un adversaire en ligne\n2.Observer une partie\n3.Quitter le jeu ");
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
               write_client(clients[i].sock, "\nUn nouveau joueur s'est connecté\n Tapez 1 pour jouer avec un autre joueur");
            }

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
                     clients[i].opponent->tour = no;
                     write_client(clients[i].opponent->sock, "Votre adversaire s'est déconnecté\nVeuillez choisir une option :\n1. Jouer contre un adversaire en ligne\n2.Observer une partie\n3. Quitter le jeu");
                     for (int j = 0; j < actual; j++)
                     {
                        if (clients[j].game == clients[i].game && clients[j].etat == Observateur)
                        {
                           write_client(clients[j].sock, "La partie a été interrompue\nVeuillez choisir une option :\n1. Jouer contre un adversaire en ligne\n2.Observer une partie\n3. Quitter le jeu");
                           clients[j].etat = Initialisation;
                        }
                     }
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
                     else if (strcmp(buffer, "2") == 0)
                     {
                        write_client(clients[i].sock, "\nVous avez choisi d'observer");
                        listParties(clients, i, &actual);
                        clients[i].etat = Observateur;
                     }
                     else if (strcmp(buffer, "3") == 0)
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
                  else if (clients[i].etat == Observateur)
                  {
                     char *endptr;
                     int player_index = strtol(buffer, &endptr, 10);

                     // Vérifier si la conversion est invalide ou hors limites
                     if (endptr == buffer || *endptr != '\0' || player_index < 0 || player_index >= actual || player_index == i)
                     {
                        write_client(clients[i].sock, "\nNuméro de partie invalide");
                     }
                     else if (clients[i].game == NULL) // A verifier en cas de pepin
                     {
                        if (clients[player_index].etat == EnPartie)
                        {

                           clients[i].game = clients[player_index].game;
                           display_board_Observateur(&clients[i]);
                        }
                        else
                        {
                           write_client(clients[i].sock, "\nLa partie choisie n'est plus en cours\nVeuillez chosir une autre partie");
                           listParties(clients, i, &actual);
                        }
                     }
                  }
                  else if (clients[i].etat == DemandeDePartie)
                  {
                     char *endptr;
                     int opponent_index = strtol(buffer, &endptr, 10);

                     // Vérifier si la conversion est invalide
                     if (endptr == buffer || *endptr != '\0')
                     {
                        write_client(clients[i].sock, "\nNuméro de joueur invalide");
                     }
                     else if (opponent_index < 0 || opponent_index >= actual || opponent_index == i)
                     {
                        write_client(clients[i].sock, "\nNuméro de joueur invalide");
                     }
                     else
                     {
                        if ((clients[opponent_index].etat != EnPartie) && (clients[opponent_index].etat != EnvoieReponse))
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

                     if (clients[i].game->game_over != 1)
                     {
                        display_board(clients[i].game);
                     }

                     for (int j = 0; j < actual; j++)
                     {
                        if (clients[j].game == clients[i].game && clients[j].etat == Observateur && clients[j].game->game_over != 1)
                        {
                           display_board_Observateur(&clients[j]);
                        }
                        if (clients[j].game == clients[i].game && clients[j].etat == Observateur && clients[j].game->game_over == 1)
                        {
                           write_client(clients[j].sock, "La partie est terminée.\nVeuillez choisir une option :\n1. Jouer contre un adversaire en ligne\n2.Observer une partie\n3. Quitter le jeu");
                        }
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
   int found = 0;
   write_client(clients[index].sock, "==============================");

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
            found += 1;
         }
         write_client(clients[index].sock, message);
      }
   }

   if (*actual == 1)
   {
      write_client(clients[index].sock, "\nAucun autre joueur n'est connecté.");
      clients[index].etat = Enattente;
      return 0;
   }
   else
   {
      if (found == 0)
      {
         write_client(clients[index].sock, "\nAucun autre joueur n'est disponible.");
         clients[index].etat = Enattente;
         return -1;
      }
      else
      {
         write_client(clients[index].sock, "\n==============================");
         write_client(clients[index].sock, "\nVeuillez choisir un numéro de joueur :");

         clients[index].etat = DemandeDePartie;
         return 1;
      }
   }
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

int name_exists(char **listname, int count, const char *name)
{
   for (int j = 0; j < count; j++)
   {
      if (strcmp(listname[j], name) == 0)
      {
         return 1; // Name found in list
      }
   }
   return 0; // Name not found
}

void listParties(Client *clients, int index, int *actual)
{
   int found = 0;
   write_client(clients[index].sock, "==============================");

   // Allocate memory for listname with a maximum size of *actual * 2
   char **listname = malloc((*actual * 2) * sizeof(char *));
   int name_count = 0;

   for (int i = 0; i < *actual; i++)
   {
      if (i != index)
      {
         char message[256];
         if (clients[i].etat == EnPartie)
         {
            // Check if either clients[i].name or clients[i].opponent->name is already in listname
            if (name_exists(listname, name_count, clients[i].name) ||
                name_exists(listname, name_count, clients[i].opponent->name))
            {
               // Skip this client, as they or their opponent are already in a game
               continue;
            }

            // Add clients[i].name to listname if not already present
            listname[name_count] = strdup(clients[i].name);
            name_count++;
            // Add clients[i].opponent->name to listname if not already present
            listname[name_count] = strdup(clients[i].opponent->name);
            name_count++;

            snprintf(message, sizeof(message), "\n%d: %s - En partie VS - %s", i, clients[i].name, clients[i].opponent->name);
            found += 1;

            write_client(clients[index].sock, message);
         }
      }
   }
   clients[index].etat = Observateur;

   if (found == 0)
   {
      write_client(clients[index].sock, "\nAucune partie en cours.");
      clients[index].etat = Initialisation;
   }

   // Free the memory allocated for listname
   for (int k = 0; k < name_count; k++)
   {
      free(listname[k]);
   }
   free(listname);
}
