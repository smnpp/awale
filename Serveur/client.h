#ifndef CLIENT_H
#define CLIENT_H

#include "server.h"
enum Etat
{
   Enattente,
   Initialisation,
   EnvoieReponse,
   DemandeDePartie
};
enum Etatjeu
{
   EnCours,
   Rien
};
typedef struct Client Client; // Déclaration préalable de la structure Client

struct Client
{
   SOCKET sock;
   char name[BUF_SIZE];
   enum Etat etat;
   enum Etatjeu etatjeu;
   Client *opponent; // Pointeur vers un autre Client
};

#endif /* guard */
