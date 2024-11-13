#ifndef GAME_H
#define GAME_H

#include <stdio.h>
#include <string.h>
#include "awale.h"
#include "Serveur/server.h"

typedef struct
{
    Client *player1;
    Client *player2;
    int current_turn;
    int game_over;
    Awale jeu; // Plateau de jeu utilisé par les fonctions de awale.c
} Game;

void init_game(Game *game, Client *client1, Client *client2);
void process_move(Game *game, Client *client, int move);
void end_game(Game *game);
void send_update_to_players(Game *game);
Game *create_game(Client *client1, Client *client2);

#endif /* GAME_H */