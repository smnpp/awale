#ifndef CLIENT_H
#define CLIENT_H

#include "server.h"

#define MAX_FRIENDS 50

enum Etat
{
   Enattente,
   Initialisation,
   EnvoieReponse,
   DemandeDePartie,
   EnPartie,
   Observateur,
   Matchmaking,
};
enum Tour
{
   yes,
   no
};
typedef struct Client Client; // Déclaration préalable de la structure Client
typedef struct Game Game;     // Déclaration préalable de la structure Game
struct Client
{
   SOCKET sock;
   char name[BUF_SIZE];
   enum Etat etat;
   enum Tour tour;
   Game *game;
   Client *opponent;     // Pointeur vers un autre Client
   time_t last_activity; // Pour détecter l'inactivité
   int ping_attempts;    // Pour la gestion de la connexion
   char friends[MAX_FRIENDS][BUF_SIZE];
   int nb_friends;
   char bio[BUF_SIZE];
};

#endif /* guard */
