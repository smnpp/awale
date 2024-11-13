#ifndef GAME_H
#define GAME_H

#include <stdio.h>
#include <string.h>

#include "serveur/client.h"
#include "serveur/server.h"
#include "awale.h"

typedef struct
{
    Client *player1;
    Client *player2;
    int current_turn;
    int game_over;
    Awale jeu; // Plateau de jeu utilisé par les fonctions de awale.c
} Game;

#endif /* GAME_H */