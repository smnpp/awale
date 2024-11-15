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
enum EtatJeu
{
   EnJeu,
   Libre
};
typedef struct Client Client; // Déclaration préalable de la structure Client

struct Client
{
   SOCKET sock;
   char name[BUF_SIZE];
   enum Etat etat;
   enum EtatJeu etatjeu;
   Client *opponent; // Pointeur vers un autre Client
};

#endif /* guard */
