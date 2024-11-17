#ifndef CLIENT_H
#define CLIENT_H

#include "server.h"
enum Etat
{
   Enattente,
   Initialisation,
   EnvoieReponse,
   DemandeDePartie,
   EnPartie
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
   Client *opponent; // Pointeur vers un autre Client
};

#endif /* guard */
