#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "server.h"
#include "client.h"
#include "../game.h"

// DECLARATION DES FONCTIONS UTILISEES SEULEMENT DANS CE FICHIER
int init_connection(void);
int inscrireClient(const char *name);
int deconnecterClient(Client *clients, int i, int *actual);
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
               write_client(csock, "Bienvenue dans le jeu Awale !\n\n"
                                   "Commandes disponibles:\n"
                                   "/list - Afficher la liste des joueurs connectés\n"
                                   "/play <nom> - Lancer une partie avec un joueur\n"
                                   "/games - Voir les parties en cours\n"
                                   "/observe <id> - Observer une partie\n"
                                   "/msg <nom> <message> - Envoyer un message privé\n"
                                   "/all <message> - Envoyer un message à tous\n"
                                   "/help - Afficher l'aide\n"
                                   "/quit - Quitter le jeu\n");
               actual++;
            }
         }
      }
      else
      {
         int i = 0;
         for (i = 0; i < actual; i++)
         {
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
                     display_help(clients[i].opponent);

                     for (int j = 0; j < actual; j++)
                     {
                        if (clients[j].game == clients[i].game && clients[j].etat == Observateur)
                        {
                           write_client(clients[j].sock, "La partie a été interrompue\n");
                           display_help(&clients[j]);
                           clients[j].etat = Initialisation;
                        }
                     }
                  }
                  deconnecterClient(clients, i, &actual);
               }
               else if (clients[i].etat == EnPartie && clients[i].tour == yes)
               {
                  // Si le client est en partie et c'est son tour, traiter le coup
                  jouerCoup(clients[i].game, buffer);

                  if (clients[i].game->game_over != 1)
                  {
                     display_board(clients[i].game);

                     // Mise à jour des observateurs
                     for (int j = 0; j < actual; j++)
                     {
                        if (clients[j].game == clients[i].game && clients[j].etat == Observateur)
                        {
                           display_board_Observateur(&clients[j]);
                        }
                     }
                  }
               }
               else if (buffer[0] == '/')
               {
                  // Traiter les commandes
                  process_command(&clients[i], buffer, clients, &actual);
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
   // Déterminer aléatoirement qui sera le joueur 1
   srand((unsigned int)time(NULL));
   int random = rand() % 2;

   Game *game;
   if (random == 0)
   {
      game = create_game(client1, client2); // client1 sera joueur 1
      write_client(client1->sock, "Vous êtes le Joueur 1");
      write_client(client2->sock, "Vous êtes le Joueur 2");
   }
   else
   {
      game = create_game(client2, client1); // client2 sera joueur 1
      write_client(client2->sock, "Vous êtes le Joueur 1");
      write_client(client1->sock, "Vous êtes le Joueur 2");
   }

   if (game)
   {
      client1->etat = EnPartie;
      client2->etat = EnPartie;
      client1->game = game;
      client2->game = game;
      initialiserGame(game, random == 0 ? client1 : client2, random == 0 ? client2 : client1);
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
   write_client(clients[index].sock, "\n==============================");

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

            snprintf(message, sizeof(message), "\n%d: %s -VS- %s", i, clients[i].name, clients[i].opponent->name);
            found += 1;

            write_client(clients[index].sock, message);
            write_client(clients[index].sock, "\n==============================");
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

int send_message_to_client_by_name(Client *clients, int actual, const char *sender_name, const char *target_name, const char *message)
{
   for (int i = 0; i < actual; i++)
   {
      if (strcmp(clients[i].name, target_name) == 0)
      {
         char full_message[BUF_SIZE];
         snprintf(full_message, sizeof(full_message), "[Message privé de %s] : %s", sender_name, message);
         write_client(clients[i].sock, full_message);

         // Confirmation à l'expéditeur
         char confirm_message[BUF_SIZE];
         snprintf(confirm_message, sizeof(confirm_message), "[Message envoyé à %s] : %s", target_name, message);
         for (int j = 0; j < actual; j++)
         {
            if (strcmp(clients[j].name, sender_name) == 0)
            {
               write_client(clients[j].sock, confirm_message);
               break;
            }
         }
         return 1;
      }
   }
   return 0;
}

void send_message_to_all_clients(Client *clients, const char *sender_name, int actual, const char *message)
{
   char full_message[BUF_SIZE];
   snprintf(full_message, sizeof(full_message), "[Message global de %s] : %s", sender_name, message);

   for (int i = 0; i < actual; i++)
   {
      write_client(clients[i].sock, full_message);
   }
}

void display_players_list(Client *clients, Client *current_client, int *actual)
{
   write_client(current_client->sock, "\nJoueurs connectés:");
   write_client(current_client->sock, "\n================");

   for (int i = 0; i < *actual; i++)
   {
      if (strcmp(clients[i].name, current_client->name) != 0)
      {
         char status[32];
         switch (clients[i].etat)
         {
         case EnPartie:
            strcpy(status, "En partie");
            break;
         case Observateur:
            strcpy(status, "Observateur");
            break;
         default:
            strcpy(status, "Disponible");
         }

         char message[256];
         snprintf(message, sizeof(message), "\n%s - %s", clients[i].name, status);
         write_client(current_client->sock, message);
      }
   }
   write_client(current_client->sock, "\n================\n");
}

void display_help(Client *client)
{
   write_client(client->sock, "\nCommandes disponibles:\n"
                              "/list - Afficher la liste des joueurs connectés\n"
                              "/play <nom> - Lancer une partie avec un joueur\n"
                              "/games - Voir les parties en cours\n"
                              "/observe <id> - Observer une partie\n"
                              "/msg <nom> <message> - Envoyer un message privé\n"
                              "/all <message> - Envoyer un message à tous\n"
                              "/help - Afficher l'aide\n"
                              "/quit - Quitter le jeu\n");
}

void send_notification(Client *client, const char *message)
{
   write_client(client->sock, message);
}

void display_games_list(Client *clients, Client *current_client, int *actual)
{
   int found = 0;
   write_client(current_client->sock, "\nParties en cours:");
   write_client(current_client->sock, "\n================");

   // Utiliser un tableau pour suivre les parties déjà affichées
   int games_shown[MAX_CLIENTS] = {0};

   for (int i = 0; i < *actual; i++)
   {
      if (clients[i].etat == EnPartie && !games_shown[i])
      {
         char message[256];
         snprintf(message, sizeof(message), "\n[%d] %s VS %s",
                  i,
                  clients[i].name,
                  clients[i].opponent->name);
         write_client(current_client->sock, message);

         // Marquer cette partie comme affichée
         games_shown[i] = 1;
         games_shown[clients[i].opponent->sock - clients[0].sock] = 1;
         found++;
      }
   }

   if (found == 0)
   {
      write_client(current_client->sock, "\nAucune partie en cours.");
   }
   write_client(current_client->sock, "\n================\n");
}

void process_command(Client *client, char *buffer, Client *clients, int *actual)
{
   // Commandes disponibles pendant une partie
   if (client->etat == EnPartie)
   {
      if (strcmp(buffer, CMD_QUIT) == 0)
      {
         // Gérer la déconnexion pendant une partie
         write_client(client->opponent->sock, "Votre adversaire a quitté la partie.\n");
         client->opponent->etat = Initialisation;
         client->opponent->opponent = NULL;
         client->opponent->game = NULL;
         deconnecterClient(clients, client->sock - clients[0].sock, actual);
         display_help(client->opponent);
         return;
      }
      else if (strncmp(buffer, CMD_MSG, strlen(CMD_MSG)) == 0 ||
               strncmp(buffer, CMD_ALL, strlen(CMD_ALL)) == 0)
      {
         // Permettre la messagerie pendant une partie
         if (strncmp(buffer, CMD_MSG, strlen(CMD_MSG)) == 0)
         {
            char *target = strtok(buffer + strlen(CMD_MSG) + 1, " ");
            char *message = strtok(NULL, "");
            if (target && message)
            {
               send_message_to_client_by_name(clients, *actual, client->name, target, message);
            }
         }
         else
         {
            char *message = buffer + strlen(CMD_ALL) + 1;
            if (message)
            {
               send_message_to_all_clients(clients, client->name, *actual, message);
            }
         }
         return;
      }
      else if (client->tour == yes)
      {
         // Si c'est le tour du joueur, traiter comme un coup de jeu
         jouerCoup(client->game, buffer);
         return;
      }
      else
      {
         write_client(client->sock, "Ce n'est pas votre tour.\n");
         return;
      }
   }

   // Commandes générales (hors partie)
   if (strncmp(buffer, CMD_PLAY, strlen(CMD_PLAY)) == 0)
   {
      char *target = buffer + strlen(CMD_PLAY) + 1;
      if (target && *target)
      {
         for (int i = 0; i < *actual; i++)
         {
            if (strcmp(clients[i].name, target) == 0)
            {
               if (clients[i].etat != EnPartie && clients[i].etat != EnvoieReponse)
               {
                  char message[256];
                  snprintf(message, sizeof(message), "\nDemande de partie de %s\nRépondez /accept ou /decline", client->name);
                  write_client(clients[i].sock, message);
                  clients[i].etat = EnvoieReponse;
                  client->opponent = &clients[i];
                  clients[i].opponent = client;
                  return;
               }
               else
               {
                  write_client(client->sock, "Ce joueur n'est pas disponible.");
                  return;
               }
            }
         }
         write_client(client->sock, "Joueur non trouvé.");
      }
      else
      {
         write_client(client->sock, "Usage: /play <nom_joueur>");
      }
   }
   else if (strcmp(buffer, CMD_LIST) == 0)
   {
      display_players_list(clients, client, actual);
   }
   else if (strcmp(buffer, CMD_GAMES) == 0)
   {
      display_games_list(clients, client, actual);
   }
   else if (strncmp(buffer, CMD_OBSERVE, strlen(CMD_OBSERVE)) == 0)
   {
      char *id_str = buffer + strlen(CMD_OBSERVE) + 1;
      if (id_str && *id_str)
      {
         int game_id = atoi(id_str);
         if (game_id >= 0 && game_id < *actual && clients[game_id].etat == EnPartie)
         {
            client->etat = Observateur;
            client->game = clients[game_id].game;
            display_board_Observateur(client);
            char message[256];
            snprintf(message, sizeof(message), "\nL'utilisateur %s observe maintenant votre partie.", client->name);
            write_client(clients[game_id].sock, message);
            write_client(clients[game_id].opponent->sock, message);
         }
         else
         {
            write_client(client->sock, "ID de partie invalide.");
         }
      }
      else
      {
         write_client(client->sock, "Usage: /observe <id_partie>");
      }
   }
   else if (strcmp(buffer, CMD_HELP) == 0)
   {
      display_help(client);
   }
   else if (strcmp(buffer, CMD_QUIT) == 0)
   {
      deconnecterClient(clients, client->sock - clients[0].sock, actual);
   }
   else if (strcmp(buffer, CMD_ACCEPT) == 0 && client->etat == EnvoieReponse)
   {
      write_client(client->sock, "Vous avez accepté la partie.\n");
      write_client(client->opponent->sock, "Votre demande a été acceptée.\n");
      start_game(client, client->opponent);
   }
   else if (strcmp(buffer, CMD_DECLINE) == 0 && client->etat == EnvoieReponse)
   {
      write_client(client->sock, "Vous avez refusé la partie.\n");
      write_client(client->opponent->sock, "Votre demande a été refusée.\n");
      client->etat = Initialisation;
      client->opponent->etat = Initialisation;
      client->opponent = NULL;
   }
   else if (strncmp(buffer, CMD_MSG, strlen(CMD_MSG)) == 0)
   {
      char *target = strtok(buffer + strlen(CMD_MSG) + 1, " ");
      char *message = strtok(NULL, "");
      if (target && message)
      {
         if (send_message_to_client_by_name(clients, *actual, client->name, target, message) == 0)
         {
            write_client(client->sock, "Destinataire non trouvé.");
         }
      }
      else
      {
         write_client(client->sock, "Usage: /msg <nom_joueur> <message>");
      }
   }
   else if (strncmp(buffer, CMD_ALL, strlen(CMD_ALL)) == 0)
   {
      char *message = buffer + strlen(CMD_ALL) + 1;
      if (message && strlen(message) > 0)
      {
         send_message_to_all_clients(clients, client->name, *actual, message);
      }
      else
      {
         write_client(client->sock, "Usage: /all <message>");
      }
   }
   else
   {
      write_client(client->sock, "Commande non reconnue. Tapez /help pour voir les commandes disponibles.");
   }
}