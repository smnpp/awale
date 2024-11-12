#ifndef CLIENT_H
#define CLIENT_H

#include "server.h"
enum Etat
{
   EnPartie,
   Enattente
};
typedef struct
{
   SOCKET sock;
   char name[BUF_SIZE];
   enum Etat etat;
} Client;

#endif /* guard */
