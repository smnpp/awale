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
int inscrireClient(const char *name, Client *clients, int *actual);
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
            int result = inscrireClient(buffer, clients, &actual);

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
               c.nb_friends = 0;
               clients[actual].etat = Initialisation;
               clients[actual].tour = no;

               load_friends_from_json(&clients[actual]);
               write_client(csock, "Bienvenue dans le jeu Awale !\n\n");
               display_help(&clients[actual]);
               actual++;
               for (int i = 0; i < (actual - 1); i++)
               {
                  if (clients[i].etat == Enattente)
                  {
                     printf("on est rentrer dans en attente\n");
                     fflush(stdout);
                     write_client(clients[i].sock, "Un nouveau joueur s'est connecté");
                     clients[i].etat = Initialisation;
                  }
               }
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
               /*
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
               }*/
               else // if (buffer[0] == '/')
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

int inscrireClient(const char *name, Client *clients, int *actual)
{
   printf("Inscription du client %s\n", name);
   fflush(stdout);
   for (int i = 0; i < *actual; i++)
   {
      if (strcmp(clients[i].name, name) == 0)
      {
         printf("Client %s déjà connecté\n", name);
         fflush(stdout);
         return 2;
      }
   }
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
   int clientExists = 0;

   while (fgets(line, sizeof(line), fichier))
   {
      if (strstr(line, name_used))
      {

         clientExists = 1;
      }
      else
      {
         fputs(line, tempFile);
      }
   }

   if (!clientExists)
   {
      fprintf(tempFile, "%s,\n", name);
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

   printf("Joueur %s déconnecté\n", clients[i].name);
   fflush(stdout);
   closesocket(clients[i].sock);
   remove_client(clients, i, actual);

   return 1;
}

int deconnecterServeur(Client *clients, int i, int *actual)
{

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

   if (*actual == 1)
   {
      printf("actual : %d\n", *actual);
      fflush(stdout);
      write_client(current_client->sock, "\nAucun autre joueur n'est connecté.");
      current_client->etat = Enattente;
   }
   else
   {
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
   }
   write_client(current_client->sock, "\n================\n");
}

void display_help(Client *client)
{
   write_client(client->sock, "\nCommandes disponibles:\n"
                              "/list - Afficher la liste des joueurs connectés\n"
                              "/play <nom> - Lancer une partie avec un joueur\n"
                              "/matchmaking - Rejoindre la file d'attente\n"
                              "/private - Changer le statut de la partie en privée\n"
                              "/public - Changer le statut de la partie en publique\n"
                              "/games - Voir les parties en cours\n"
                              "/observe <id> - Observer une partie\n"
                              "/msg <nom> <message> - Envoyer un message privé\n"
                              "/all <message> - Envoyer un message à tous\n"
                              "/writebio <bio> - Écrire une biographie\n"
                              "/readbio <nom> - Lire la biographie d'un joueur\n"
                              "/addfriend <nom> - Ajouter un ami\n"
                              "/removefriend <nom> - Retirer un ami\n"
                              "/friends - Voir sa liste d'amis\n"
                              "/history - Voir l'historique des parties\n"
                              "/watch <id> - Observer une partie\n"
                              "/help - Afficher l'aide\n"
                              "/quit - Quitter une partie\n"
                              "/logout - Se déconnecter\n");
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
   if (strcmp(buffer, CMD_LOGOUT) == 0)
   {

      write_client(client->sock, "Bonne continuation\n");
      if (client->etat == EnPartie)
      {
         client->opponent->etat = Initialisation;
         client->opponent->opponent = NULL;
         client->opponent->game = NULL;
         client->opponent->tour = no;
         write_client(client->opponent->sock, "Votre adversaire a quitté la partie.\n");
         display_help(client->opponent);

         for (int j = 0; j < client->game->nb_observers; j++)
         {

            write_client(client->game->observers[j]->sock, "La partie a été interrompue\n");
            display_help(client->game->observers[j]);
            client->game->observers[j]->etat = Initialisation;
         }
      }

      deconnecterClient(clients, client->sock - clients[0].sock, actual);

      return;
   }
   if (client->etat == EnPartie)
   {
      printf("buffer : %s\n", buffer);
      fflush(stdout);
      if (strcmp(buffer, CMD_QUIT) == 0)
      {
         // Gérer la déconnexion pendant une partie
         write_client(client->opponent->sock, "Votre adversaire a quitté la partie.\n");
         client->opponent->etat = Initialisation;
         client->etat = Initialisation;
         client->opponent->opponent = NULL;
         client->opponent->game = NULL;
         client->opponent->tour = no;

         // deconnecterClient(clients, client->sock - clients[0].sock, actual);
         display_help(client->opponent);
         printf("on est rentre pas dans le if\n");
         fflush(stdout);
         if (client->game->nb_observers > 0)
         {
            printf("on est rentrer dans le if\n");
            fflush(stdout);
            for (int j = 0; j < client->game->nb_observers; j++)
            {

               write_client(client->game->observers[j]->sock, "La partie a été interrompue\n");
               display_help(client->game->observers[j]);
               client->game->observers[j]->etat = Initialisation;
            }
         }
         client->game = NULL;
         printf("on est apres le if\n");
         fflush(stdout);
         client->opponent = NULL;
         display_help(client);

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
         return; // a quoi il sert le return
      }
      else if (strcmp(buffer, CMD_PRIVATE) == 0)
      {
         if (client->etat == EnPartie)
         {
            // Vérifier si la partie est déjà privée
            if (client->game->private == true)
            {
               write_client(client->sock, "La partie est déjà privée.\n");
               return;
            }

            // Rendre la partie privée
            client->game->private = true;

            // Notifier les deux joueurs
            write_client(client->sock, "La partie est maintenant privée.\n");
            write_client(client->opponent->sock, "La partie est maintenant privée.\n");

            // Retirer les observateurs qui ne sont pas amis
            for (int i = 0; i < client->game->nb_observers; i++)
            {
               Client *observer = client->game->observers[i];
               if (!is_friend(client, observer->name) && !is_friend(client->opponent, observer->name))
               {
                  // Notifier l'observateur qu'il ne peut plus regarder la partie
                  write_client(observer->sock, "La partie est devenue privée. Vous ne pouvez plus l'observer.\n");
                  display_help(observer);

                  // Retirer l'observateur
                  for (int j = i; j < client->game->nb_observers - 1; j++)
                  {
                     client->game->observers[j] = client->game->observers[j + 1];
                  }
                  client->game->nb_observers--;
                  observer->etat = Initialisation;
                  i--; // Ajuster l'index car nous avons déplacé les éléments
               }
            }
         }
         else
         {
            write_client(client->sock, "Vous devez être en partie pour utiliser cette commande.\n");
         }
      }
      else if (strcmp(buffer, CMD_PUBLIC) == 0)
      {
         if (client->etat == EnPartie)
         {
            // Vérifier si la partie est déjà privée
            if (client->game->private == false)
            {
               write_client(client->sock, "La partie est déjà publique.\n");
               return;
            }

            // Rendre la partie privée
            client->game->private = false;

            // Notifier les deux joueurs
            write_client(client->sock, "La partie est maintenant publique.\n");
            write_client(client->opponent->sock, "La partie est maintenant publique.\n");
         }
         else
         {
            write_client(client->sock, "Vous devez être en partie pour utiliser cette commande.\n");
         }
      }
      else if (client->tour == yes)
      {
         // Si c'est le tour du joueur, traiter comme un coup de jeu
         jouerCoup(client->game, buffer);

         if (client->game->game_over != 1)
         {
            display_board(client->game);

            // Mise à jour des observateurs
            for (int j = 0; j < client->game->nb_observers; j++)
            {

               display_board_Observateur(client->game->observers[j]);
            }
         }

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
      if (client->etat == EnPartie || client->etat == EnvoieReponse || client->etat == DemandeDePartie)
      {
         write_client(client->sock, "Vous êtes déjà en attente d'une partie.");
         return;
      }
      char *target = buffer + strlen(CMD_PLAY) + 1;
      if (target && *target)
      {
         if (strcmp(client->name, target) == 0)
         {
            write_client(client->sock, "Vous ne pouvez pas jouer contre vous-même.");
            return;
         }
         for (int i = 0; i < *actual; i++)
         {
            if (strcmp(clients[i].name, target) == 0)
            {
               if (clients[i].etat != EnPartie && clients[i].etat != EnvoieReponse)
               {
                  char message[256];
                  snprintf(message, sizeof(message), "\nDemande de partie de %s\nRépondez /accept ou /decline", client->name);
                  write_client(clients[i].sock, message);
                  client->etat = DemandeDePartie;
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
   else if (strcmp(buffer, CMD_MATCHMAKING) == 0)
   {
      client->etat = Matchmaking;
      for (int i = 0; i < *actual; i++)
      {
         if (strcmp(clients[i].name, client->name) != 0)
         {
            if (clients[i].etat == Matchmaking)
            {
               client->opponent = &clients[i];
               clients[i].opponent = client;
               start_game(client, &clients[i]);
               return;
            }
         }
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
         char *endptr;
         int game_id = strtol(id_str, &endptr, 10);

         // Vérifier si la conversion est invalide
         if (endptr == buffer || *endptr != '\0')
         {
            write_client(client->sock, "ID de partie invalide.");
            return;
         }

         if (game_id >= 0 && game_id < *actual && clients[game_id].etat == EnPartie)
         {
            if (clients[game_id].game->private == false)
            {
               client->etat = Observateur;
               client->game = clients[game_id].game;
               add_observer(client->game, client);
               display_board_Observateur(client);
            }
            else if (is_friend(clients[game_id].game->player1, client->name) || is_friend(clients[game_id].game->player2, client->name))
            {
               client->etat = Observateur;
               client->game = clients[game_id].game;
               add_observer(client->game, client);
               display_board_Observateur(client);
            }
            else
            {
               write_client(client->sock, "Vous ne pouvez pas observer cette partie.");
            }
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
   else if (strncmp(buffer, CMD_ADD_FRIEND, strlen(CMD_ADD_FRIEND)) == 0)
   {
      char *friend_name = buffer + strlen(CMD_ADD_FRIEND) + 1;
      if (friend_name && *friend_name)
      {
         add_friend(client, friend_name);
      }
      else
      {
         write_client(client->sock, "Usage: /addfriend <nom>");
      }
   }
   else if (strncmp(buffer, CMD_WRITEBIO, strlen(CMD_WRITEBIO)) == 0)
   {
      char *bio = buffer + strlen(CMD_WRITEBIO) + 1;
      if (bio && *bio)
      {
         write_bio(client, bio);
      }
      else
      {
         write_client(client->sock, "Usage: /writebio <votre biographie>");
      }
   }
   else if (strncmp(buffer, CMD_READBIO, strlen(CMD_READBIO)) == 0)
   {
      char *target = buffer + strlen(CMD_READBIO) + 1;
      if (target && *target)
      {
         read_bio(client, clients, *actual, target);
      }
      else
      {
         write_client(client->sock, "Usage: /readbio <nom_joueur>");
      }
   }
   else if (strncmp(buffer, CMD_REMOVE_FRIEND, strlen(CMD_REMOVE_FRIEND)) == 0)
   {
      char *friend_name = buffer + strlen(CMD_REMOVE_FRIEND) + 1;
      if (friend_name && *friend_name)
      {
         remove_friend(client, friend_name);
      }
      else
      {
         write_client(client->sock, "Usage: /removefriend <nom>");
      }
   }
   else if (strcmp(buffer, CMD_LIST_FRIENDS) == 0)
   {
      list_friends(client);
   }
   else if (strcmp(buffer, CMD_HISTORY) == 0)
   {
      display_game_history(client);
   }
   else if (strncmp(buffer, CMD_WATCH, strlen(CMD_WATCH)) == 0)
   {
      char *game_id = buffer + strlen(CMD_WATCH) + 1;
      if (game_id && *game_id)
      {
         watch_game(client, game_id);
      }
      else
      {
         write_client(client->sock, "Usage: /watch <numéro de partie>\n");
      }
   }
   else
   {
      write_client(client->sock, "Commande non reconnue. Tapez /help pour voir les commandes disponibles.");
   }
}

bool is_friend(Client *client, const char *friend_name)
{
   for (int i = 0; i < client->nb_friends; i++)
   {
      if (strcmp(client->friends[i], friend_name) == 0)
      {
         return true;
      }
   }
   return false;
}

// Fonction pour charger les amis du client à partir du JSON
void load_friends_from_json(Client *client)
{
   FILE *file = fopen("./data/friends.json", "r");
   if (!file)
   {
      // Si le fichier n'existe pas, créer un nouveau fichier JSON avec une structure de base
      cJSON *root = cJSON_CreateObject();
      cJSON *friendships = cJSON_CreateObject();
      cJSON_AddItemToObject(root, "friendships", friendships);

      FILE *newfile = fopen("./data/friends.json", "w");
      if (newfile)
      {
         char *json_str = cJSON_Print(root);
         fprintf(newfile, "%s", json_str);
         free(json_str);
         fclose(newfile);
      }
      cJSON_Delete(root);
      client->nb_friends = 0;
      return;
   }

   // Lire le contenu du fichier existant
   fseek(file, 0, SEEK_END);
   long size = ftell(file);
   fseek(file, 0, SEEK_SET);

   if (size == 0)
   {
      fclose(file);
      client->nb_friends = 0;
      return;
   }

   char *content = malloc(size + 1);
   fread(content, 1, size, file);
   content[size] = '\0';
   fclose(file);

   cJSON *root = cJSON_Parse(content);
   free(content);

   if (!root)
   {
      client->nb_friends = 0;
      return;
   }

   cJSON *friendships = cJSON_GetObjectItem(root, "friendships");
   if (!friendships)
   {
      cJSON_Delete(root);
      client->nb_friends = 0;
      return;
   }

   cJSON *user_friends = cJSON_GetObjectItem(friendships, client->name);
   client->nb_friends = 0;

   if (user_friends && cJSON_IsArray(user_friends))
   {
      int array_size = cJSON_GetArraySize(user_friends);
      for (int i = 0; i < array_size && i < MAX_FRIENDS; i++)
      {
         cJSON *friend_item = cJSON_GetArrayItem(user_friends, i);
         if (cJSON_IsString(friend_item))
         {
            strncpy(client->friends[client->nb_friends], friend_item->valuestring, BUF_SIZE - 1);
            client->nb_friends++;
         }
      }
   }

   cJSON *bios = cJSON_GetObjectItem(root, "bios");
   if (bios)
   {
      cJSON *bio = cJSON_GetObjectItem(bios, client->name);
      if (bio && cJSON_IsString(bio))
      {
         strncpy(client->bio, bio->valuestring, BUF_SIZE - 1);
         client->bio[BUF_SIZE - 1] = '\0';
      }
      else
      {
         strcpy(client->bio, "Pas de biographie");
      }
   }
   else
   {
      strcpy(client->bio, "Pas de biographie");
   }

   cJSON_Delete(root);
}

// Fonction pour charger le JSON des amis
cJSON *load_friends_json(void)
{
   FILE *file = fopen("./data/friends.json", "r");
   if (!file)
   {
      // Créer un nouveau fichier JSON avec une structure de base
      cJSON *root = cJSON_CreateObject();
      cJSON_AddObjectToObject(root, "friendships");

      FILE *newfile = fopen("./data/friends.json", "w");
      if (newfile)
      {
         char *json_str = cJSON_Print(root);
         fprintf(newfile, "%s", json_str);
         free(json_str);
         fclose(newfile);
      }
      return root;
   }

   // Lire le fichier existant
   fseek(file, 0, SEEK_END);
   long size = ftell(file);
   fseek(file, 0, SEEK_SET);

   char *content = malloc(size + 1);
   fread(content, 1, size, file);
   content[size] = '\0';
   fclose(file);

   cJSON *root = cJSON_Parse(content);
   free(content);

   if (!root)
   {
      root = cJSON_CreateObject();
      cJSON_AddObjectToObject(root, "friendships");
   }

   return root;
}

// Fonction pour sauvegarder le JSON des amis
void save_friends_json(cJSON *root)
{
   FILE *file = fopen("./data/friends.json", "w");
   if (file)
   {
      char *json_str = cJSON_Print(root);
      fprintf(file, "%s", json_str);
      free(json_str);
      fclose(file);
   }
}

// Fonction pour ajouter un ami
void add_friend(Client *client, const char *friend_name)
{
   // Vérifier si on essaie de s'ajouter soi-même
   if (strcmp(client->name, friend_name) == 0)
   {
      write_client(client->sock, "Vous ne pouvez pas vous ajouter vous-même comme ami.\n");
      return;
   }

   // Vérifier si le nom d'ami existe dans clients.csv
   FILE *check_file = fopen("./data/clients.csv", "r");
   if (check_file)
   {
      char line[256];
      int friend_exists = 0;
      while (fgets(line, sizeof(line), check_file))
      {
         char username[BUF_SIZE];
         sscanf(line, "%[^,]", username);
         if (strcmp(username, friend_name) == 0)
         {
            friend_exists = 1;
            break;
         }
      }
      fclose(check_file);

      if (!friend_exists)
      {
         write_client(client->sock, "Cet utilisateur n'existe pas.\n");
         return;
      }
   }

   // Charger le fichier JSON
   cJSON *root = load_friends_json();
   cJSON *friendships = cJSON_GetObjectItem(root, "friendships");
   if (!friendships)
   {
      friendships = cJSON_CreateObject();
      cJSON_AddItemToObject(root, "friendships", friendships);
   }

   // Obtenir ou créer la liste d'amis du client
   cJSON *user_friends = cJSON_GetObjectItem(friendships, client->name);
   if (!user_friends)
   {
      user_friends = cJSON_CreateArray();
      cJSON_AddItemToObject(friendships, client->name, user_friends);
   }

   // Vérifier si l'ami est déjà dans la liste
   int array_size = cJSON_GetArraySize(user_friends);
   for (int i = 0; i < array_size; i++)
   {
      cJSON *friend_item = cJSON_GetArrayItem(user_friends, i);
      if (strcmp(friend_item->valuestring, friend_name) == 0)
      {
         write_client(client->sock, "Cette personne est déjà dans votre liste d'amis.\n");
         cJSON_Delete(root);
         return;
      }
   }

   // Ajouter le nouvel ami
   cJSON_AddItemToArray(user_friends, cJSON_CreateString(friend_name));

   // Sauvegarder le fichier
   save_friends_json(root);

   // Mettre à jour la liste locale du client
   if (client->nb_friends < MAX_FRIENDS)
   {
      strncpy(client->friends[client->nb_friends], friend_name, BUF_SIZE - 1);
      client->nb_friends++;
   }

   char message[BUF_SIZE];
   snprintf(message, BUF_SIZE, "%s a été ajouté à votre liste d'amis.\n", friend_name);
   write_client(client->sock, message);

   cJSON_Delete(root);
}

// Fonction pour retirer un ami
void remove_friend(Client *client, const char *friend_name)
{
   // Charger le fichier JSON
   cJSON *root = load_friends_json();
   cJSON *friendships = cJSON_GetObjectItem(root, "friendships");
   if (!friendships)
   {
      write_client(client->sock, "Vous n'avez pas d'amis dans votre liste.\n");
      cJSON_Delete(root);
      return;
   }

   // Obtenir la liste d'amis du client
   cJSON *user_friends = cJSON_GetObjectItem(friendships, client->name);
   if (!user_friends)
   {
      write_client(client->sock, "Vous n'avez pas d'amis dans votre liste.\n");
      cJSON_Delete(root);
      return;
   }

   // Rechercher et supprimer l'ami
   int found = 0;
   int array_size = cJSON_GetArraySize(user_friends);
   for (int i = 0; i < array_size; i++)
   {
      cJSON *friend_item = cJSON_GetArrayItem(user_friends, i);
      if (strcmp(friend_item->valuestring, friend_name) == 0)
      {
         // Supprimer du JSON
         cJSON_DeleteItemFromArray(user_friends, i);

         // Supprimer de la liste locale
         for (int j = 0; j < client->nb_friends; j++)
         {
            if (strcmp(client->friends[j], friend_name) == 0)
            {
               // Décaler les éléments restants
               for (int k = j; k < client->nb_friends - 1; k++)
               {
                  strcpy(client->friends[k], client->friends[k + 1]);
               }
               client->nb_friends--;
               break;
            }
         }

         found = 1;
         break;
      }
   }

   if (found)
   {
      save_friends_json(root);
      char message[BUF_SIZE];
      snprintf(message, BUF_SIZE, "%s a été retiré de votre liste d'amis.\n", friend_name);
      write_client(client->sock, message);
   }
   else
   {
      write_client(client->sock, "Cette personne n'est pas dans votre liste d'amis.\n");
   }

   cJSON_Delete(root);
}

// Fonction pour lister les amis
void list_friends(Client *client)
{
   cJSON *root = load_friends_json();
   cJSON *friendships = cJSON_GetObjectItem(root, "friendships");
   cJSON *user_friends = cJSON_GetObjectItem(friendships, client->name);

   if (!user_friends || cJSON_GetArraySize(user_friends) == 0)
   {
      write_client(client->sock, "Votre liste d'amis est vide.\n");
      cJSON_Delete(root);
      return;
   }

   write_client(client->sock, "\nVos amis :\n================\n");

   int array_size = cJSON_GetArraySize(user_friends);
   for (int i = 0; i < array_size; i++)
   {
      cJSON *friend_item = cJSON_GetArrayItem(user_friends, i);
      char message[BUF_SIZE];
      snprintf(message, BUF_SIZE, "%s\n", friend_item->valuestring);
      write_client(client->sock, message);
   }

   write_client(client->sock, "================\n");
   cJSON_Delete(root);
}

void write_bio(Client *client, const char *new_bio)
{
   // Charger le fichier JSON existant
   cJSON *root = load_friends_json();

   // Ajouter une nouvelle section "bios" si elle n'existe pas
   cJSON *bios = cJSON_GetObjectItem(root, "bios");
   if (!bios)
   {
      bios = cJSON_CreateObject();
      cJSON_AddItemToObject(root, "bios", bios);
   }

   // Mettre à jour ou ajouter la bio
   cJSON_DeleteItemFromObject(bios, client->name);
   cJSON_AddStringToObject(bios, client->name, new_bio);

   // Sauvegarder le fichier
   save_friends_json(root);

   // Mettre à jour la bio locale du client
   strncpy(client->bio, new_bio, BUF_SIZE - 1);
   client->bio[BUF_SIZE - 1] = '\0';

   write_client(client->sock, "Votre biographie a été mise à jour.\n");

   cJSON_Delete(root);
}

void read_bio(Client *client, Client *clients, int actual, const char *target_name)
{
   for (int i = 0; i < actual; i++)
   {
      if (strcmp(clients[i].name, target_name) == 0)
      {
         char message[BUF_SIZE * 2];
         snprintf(message, sizeof(message), "\nBiographie de %s:\n================\n%s\n================\n",
                  target_name, clients[i].bio);
         write_client(client->sock, message);
         return;
      }
   }
   write_client(client->sock, "Utilisateur non trouvé.\n");
}

void display_board_history(Client *client, cJSON *board_state, const char *player1_name, const char *player2_name, int score1, int score2)
{
   char buffer[BUF_SIZE];

   // En-tête avec noms des joueurs
   snprintf(buffer, BUF_SIZE,
            "\nPlateau:\n"
            "==============================\n"
            "%s : ",
            player2_name);
   write_client(client->sock, buffer);

   // Afficher les trous du joueur 2 (haut)
   for (int i = TROUS - 1; i >= TROUS / 2; i--)
   {
      cJSON *trou = cJSON_GetArrayItem(board_state, i);
      char trou_str[8];
      snprintf(trou_str, sizeof(trou_str), "%02d ", trou->valueint);
      write_client(client->sock, trou_str);
   }

   // Afficher les trous du joueur 1 (bas)
   snprintf(buffer, BUF_SIZE, "\n%s : ", player1_name);
   write_client(client->sock, buffer);

   for (int i = 0; i < TROUS / 2; i++)
   {
      cJSON *trou = cJSON_GetArrayItem(board_state, i);
      char trou_str[8];
      snprintf(trou_str, sizeof(trou_str), "%02d ", trou->valueint);
      write_client(client->sock, trou_str);
   }

   // Afficher les scores
   snprintf(buffer, BUF_SIZE,
            "\n==============================\n"
            "Scores - %s: %d | %s: %d\n"
            "==============================\n",
            player1_name, score1,
            player2_name, score2);
   write_client(client->sock, buffer);
}

void display_board_replay(Client *client, Game *game, const char *player1_name, const char *player2_name)
{
   char buffer[BUF_SIZE];

   // En-tête avec noms des joueurs
   snprintf(buffer, BUF_SIZE,
            "\nPlateau actuel:\n"
            "==============================\n"
            "%s : ",
            player2_name);
   write_client(client->sock, buffer);

   // Trous du joueur 2 (haut) avec compteur pour l'alignement
   int count = 0;
   for (int i = TROUS - 1; i >= TROUS / 2; i--)
   {
      char trou[8];
      if (game->jeu.trous[i] < 10)
         snprintf(trou, sizeof(trou), "0%d ", game->jeu.trous[i]);
      else
         snprintf(trou, sizeof(trou), "%d ", game->jeu.trous[i]);
      write_client(client->sock, trou);
      count++;
      if (count == 6)
         write_client(client->sock, "\n");
   }

   // Trous du joueur 1 (bas)
   snprintf(buffer, BUF_SIZE, "%s : ", player1_name);
   write_client(client->sock, buffer);

   count = 0;
   for (int i = 0; i < TROUS / 2; i++)
   {
      char trou[8];
      if (game->jeu.trous[i] < 10)
         snprintf(trou, sizeof(trou), "0%d ", game->jeu.trous[i]);
      else
         snprintf(trou, sizeof(trou), "%d ", game->jeu.trous[i]);
      write_client(client->sock, trou);
      count++;
      if (count == 6)
         write_client(client->sock, "\n");
   }

   // Scores
   snprintf(buffer, BUF_SIZE,
            "==============================\n"
            "Scores - %s: %d | %s: %d\n",
            player1_name, game->jeu.scoreJoueur1,
            player2_name, game->jeu.scoreJoueur2);
   write_client(client->sock, buffer);
}

void watch_game(Client *client, const char *game_id_str)
{
   int game_id = atoi(game_id_str);

   FILE *file = fopen("data/games.json", "r");
   if (!file)
   {
      write_client(client->sock, "Partie non trouvée.\n");
      return;
   }

   fseek(file, 0, SEEK_END);
   long size = ftell(file);
   fseek(file, 0, SEEK_SET);

   char *content = malloc(size + 1);
   fread(content, 1, size, file);
   content[size] = '\0';
   fclose(file);

   cJSON *games = cJSON_Parse(content);
   free(content);

   // Trouver la partie avec l'ID correspondant
   cJSON *selected_game = NULL;
   int game_count = cJSON_GetArraySize(games);
   for (int i = 0; i < game_count; i++)
   {
      cJSON *game = cJSON_GetArrayItem(games, i);
      cJSON *id = cJSON_GetObjectItem(game, "id");
      if (id->valueint == game_id)
      {
         selected_game = game;
         break;
      }
   }

   if (!selected_game)
   {
      write_client(client->sock, "Partie non trouvée.\n");
      cJSON_Delete(games);
      return;
   }

   // Récupérer les informations de la partie
   cJSON *player1 = cJSON_GetObjectItem(selected_game, "player1");
   cJSON *player2 = cJSON_GetObjectItem(selected_game, "player2");
   cJSON *winner = cJSON_GetObjectItem(selected_game, "winner");
   cJSON *date = cJSON_GetObjectItem(selected_game, "date");
   cJSON *boards = cJSON_GetObjectItem(selected_game, "boards");
   cJSON *score1 = cJSON_GetObjectItem(selected_game, "score1");
   cJSON *score2 = cJSON_GetObjectItem(selected_game, "score2");

   // Afficher les informations de la partie
   char header[BUF_SIZE];
   snprintf(header, sizeof(header),
            "\n=== Partie #%d ===\n"
            "Date : %s\n"
            "Joueurs : %s VS %s\n"
            "Gagnant : %s\n"
            "\nAppuyez sur Entrée pour voir chaque état du plateau...\n",
            game_id,
            date->valuestring,
            player1->valuestring,
            player2->valuestring,
            winner->valuestring);
   write_client(client->sock, header);

   // Parcourir tous les états du plateau
   int nb_states = cJSON_GetArraySize(boards);
   for (int i = 0; i < nb_states; i++)
   {
      char input[BUF_SIZE];
      read_client(client->sock, input); // Attendre que l'utilisateur appuie sur Entrée

      cJSON *board_state = cJSON_GetArrayItem(boards, i);

      // Afficher le numéro de l'état
      char state_info[64];
      snprintf(state_info, sizeof(state_info),
               "\nÉtat %d/%d :\n",
               i + 1, nb_states);
      write_client(client->sock, state_info);

      // Gérer les scores
      int current_score1 = 0, current_score2 = 0;
      if (i == nb_states - 1) // Pour le dernier état, utiliser les scores finaux
      {
         current_score1 = score1->valueint;
         current_score2 = score2->valueint;
      }

      display_board_history(client, board_state,
                            player1->valuestring,
                            player2->valuestring,
                            current_score1, current_score2);

      // Message d'instruction pour continuer
      if (i < nb_states - 1)
      {
         write_client(client->sock, "\nAppuyez sur Entrée + Espace pour continuer...");
      }
   }

   // Message de fin
   char final_message[BUF_SIZE];
   snprintf(final_message, sizeof(final_message),
            "\nFin de la partie\n"
            "Score final : %s (%d) - (%d) %s\n"
            "Gagnant : %s\n",
            player1->valuestring, score1->valueint,
            score2->valueint, player2->valuestring,
            winner->valuestring);
   write_client(client->sock, final_message);

   cJSON_Delete(games);
}

void display_game_history(Client *client)
{
   FILE *file = fopen("data/games.json", "r");
   if (!file)
   {
      write_client(client->sock, "Aucune partie enregistrée.\n");
      return;
   }

   fseek(file, 0, SEEK_END);
   long size = ftell(file);
   fseek(file, 0, SEEK_SET);

   char *content = malloc(size + 1);
   fread(content, 1, size, file);
   content[size] = '\0';
   fclose(file);

   cJSON *games = cJSON_Parse(content);
   free(content);

   if (!games || !cJSON_IsArray(games))
   {
      write_client(client->sock, "Aucune partie enregistrée.\n");
      if (games)
         cJSON_Delete(games);
      return;
   }

   write_client(client->sock, "\nHistorique des parties:\n================\n");

   int game_count = cJSON_GetArraySize(games);
   for (int i = 0; i < game_count; i++)
   {
      cJSON *game = cJSON_GetArrayItem(games, i);
      cJSON *id = cJSON_GetObjectItem(game, "id");
      cJSON *player1 = cJSON_GetObjectItem(game, "player1");
      cJSON *player2 = cJSON_GetObjectItem(game, "player2");
      cJSON *winner = cJSON_GetObjectItem(game, "winner");
      cJSON *date = cJSON_GetObjectItem(game, "date");
      cJSON *score1 = cJSON_GetObjectItem(game, "score1");
      cJSON *score2 = cJSON_GetObjectItem(game, "score2");

      char message[BUF_SIZE];
      snprintf(message, sizeof(message),
               "[%d] %s | %s (%d) VS %s (%d) - Gagnant: %s\n",
               id->valueint,
               date->valuestring,
               player1->valuestring, score1->valueint,
               player2->valuestring, score2->valueint,
               winner->valuestring);
      write_client(client->sock, message);
   }

   write_client(client->sock, "\n================\n");
   write_client(client->sock, "Pour voir une partie: /watch <numéro>\n");

   cJSON_Delete(games);
}