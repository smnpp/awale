#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server.h"
#include "client.h"

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
   int actual = 0;
   int max = sock;
   Client clients[MAX_CLIENTS];
   fd_set rdfs;

   while (1)
   {
      FD_ZERO(&rdfs);
      FD_SET(STDIN_FILENO, &rdfs);
      FD_SET(sock, &rdfs);

      for (int i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      if (FD_ISSET(STDIN_FILENO, &rdfs))
      {
         break;
      }
      else if (FD_ISSET(sock, &rdfs))
      {
         SOCKADDR_IN csin = {0};
         socklen_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if (csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         if (read_client(csock, buffer) > 0)
         {
            if (inscrireClient(buffer) == 1)
            {

               max = csock > max ? csock : max;
               Client c = {csock};
               strncpy(c.name, buffer, BUF_SIZE - 1);
               clients[actual++] = c;
               write_client(csock, "Vous êtes connecté au serveur de chat\n 1: jouer avec un autre client\n 2: se deconnecter\n");
               read_client(csock, buffer);

               if (strcmp(buffer, "1") == 0)
               {
                  write_client(csock, "Vous avez choisi de jouer avec un autre client\nVoici la liste des clients connectés:\n");

                  char **clientsList = listClients(); // Récupère la liste des clients connectés
                  if (clientsList != NULL)            // Vérifie si la liste n'est pas NULL
                  {
                     int i = 0;
                     while (clientsList[i] != NULL)
                     {
                        // Envoie chaque client avec une nouvelle ligne
                        if (strcmp(clientsList[i], c.name) != 0)
                        {
                           write_client(csock, clientsList[i]);
                           write_client(csock, "\n");
                        } // Ajouter un saut de ligne entre les noms
                        i++;
                     }

                     // Libération de la mémoire allouée pour clientsList
                     for (int i = 0; clientsList[i] != NULL; i++)
                     {
                        free(clientsList[i]); // Libérer chaque chaîne allouée
                     }
                     free(clientsList); // Libérer le tableau de pointeurs
                  }
                  else
                  {
                     write_client(csock, "Erreur lors de la récupération de la liste des clients\n");
                  }
               }

               else if (strcmp(buffer, "2") == 0)
               {
                  write_client(csock, "Vous avez choisi de vous deconnecter\n");
                  closesocket(csock);
                  deconnecterClient(clients, actual - 1, &actual);
               }
               else
               {
                  write_client(csock, "Choix invalide\n");
                  closesocket(csock);
                  remove_client(clients, actual - 1, &actual);
               }
            }
            else
            {
               write_client(csock, "Client déjà connecté ou erreur d'inscription");
               closesocket(csock);
            }
         }
         else
         {
            perror("Erreur lors de la réception du nom du client");
            closesocket(csock);
         }
      }
      else
      {
         for (int i = 0; i < actual; i++)
         {
            if (FD_ISSET(clients[i].sock, &rdfs))
            {
               Client client = clients[i];
               if (read_client(clients[i].sock, buffer) == 0)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  snprintf(buffer, BUF_SIZE, "%s disconnected !", client.name);
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
               }
               else if (read_client(clients[i].sock, buffer) == -9)
               {
                  deconnecterClient(clients, i, &actual);
               } // Si le client envoie "/disconnect"
               else
               {
                  send_message_to_all_clients(clients, client, actual, buffer, 0);
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
   for (int i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   (*actual)--;
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   char message[BUF_SIZE];
   message[0] = 0;
   for (int i = 0; i < actual; i++)
   {
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
   int n = recv(sock, buffer, BUF_SIZE - 1, 0);
   if (n < 0)
   {
      perror("recv()");
      n = 0;
   }
   if (strncmp(buffer, "/disconnect", 11) == 0)
   {
      return -9;
   }
   buffer[n] = 0;
   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if (send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
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
static char **listClients()
{
   FILE *fichier = fopen("./data/clients.csv", "r");
   if (!fichier)
   {
      perror("Erreur d'ouverture du fichier");
      return NULL;
   }

   // Allouer un tableau de chaînes initial, mais le redimensionner si nécessaire
   char **clientsList = malloc(10 * sizeof(char *)); // Commence avec 10 éléments
   if (!clientsList)
   {
      perror("Erreur d'allocation de mémoire pour clientsList");
      fclose(fichier);
      return NULL;
   }

   char line[256];
   int i = 0;
   while (fgets(line, sizeof(line), fichier))
   {
      // Récupérer le nom du client (avant la virgule)
      char *name = strtok(line, ",");

      // Vérifier si le client est connecté
      char *status = strtok(NULL, ","); // Récupère le statut après la virgule
      if (name && status && strstr(status, "+"))
      {
         printf("Client %d: %s\n", i, name);
         fflush(stdout);

         // Vérifier si on doit réallouer plus de mémoire pour les clients
         if (i >= 10)
         {
            // Redimensionner le tableau (ajouter plus de place si nécessaire)
            char **temp = realloc(clientsList, (i + 10) * sizeof(char *));
            if (!temp)
            {
               perror("Erreur de réallocation de mémoire");
               // Libérer la mémoire allouée avant de quitter
               for (int j = 0; j < i; j++)
                  free(clientsList[j]);
               free(clientsList);
               fclose(fichier);
               return NULL;
            }
            clientsList = temp;
         }

         // Allouer de la mémoire pour le nom du client et l'ajouter à la liste
         clientsList[i] = malloc(256 * sizeof(char));
         if (!clientsList[i])
         {
            perror("Erreur d'allocation de mémoire pour le nom du client");
            // Libérer la mémoire allouée avant de quitter
            for (int j = 0; j < i; j++)
               free(clientsList[j]);
            free(clientsList);
            fclose(fichier);
            return NULL;
         }

         snprintf(clientsList[i], 256, "%s", name); // Copier le nom du client
         i++;
      }
   }

   // Marquer la fin de la liste
   clientsList[i] = NULL;

   fclose(fichier);
   return clientsList;
}
static int deconnecterClient(Client *clients, int to_remove, int *actual)
{
   // Nom du client à déconnecter
   const char *name = clients[to_remove].name;

   printf("Déconnexion du client %s\n", name);
   fflush(stdout);

   // Ouverture du fichier clients.csv en lecture et d'un fichier temporaire en écriture
   FILE *fichier = fopen("./data/clients.csv", "r");
   FILE *tempFile = fopen("./data/temp2.csv", "w");
   if (!fichier || !tempFile)
   {
      perror("Erreur d'ouverture des fichiers");
      return -1;
   }

   char line[256];
   char name_used[256];
   snprintf(name_used, sizeof(name_used), "%s,", name);

   // Lecture ligne par ligne pour mettre à jour le statut du client
   while (fgets(line, sizeof(line), fichier))
   {
      // Si le nom du client est trouvé dans la ligne, on le marque comme déconnecté
      if (strstr(line, name_used))
      {
         fprintf(tempFile, "%s, -\n", name);
      }
      else
      {
         // Copier les autres lignes telles quelles
         fputs(line, tempFile);
      }
   }

   // Fermeture des fichiers
   fclose(fichier);
   fclose(tempFile);

   // Remplacement de clients.csv par le fichier temporaire
   remove("./data/clients.csv");
   rename("./data/temp2.csv", "./data/clients.csv");

   // Retrait du client de la liste en mémoire
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   (*actual)--;
   closesocket(clients[to_remove].sock);

   printf("Client %s déconnecté avec succès\n", name);
   fflush(stdout);

   return 1;
}

int main(int argc, char **argv)
{
   init();
   app();
   end();
   return EXIT_SUCCESS;
}
