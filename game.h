#ifndef GAME_H
#define GAME_H

#include <stdio.h>
#include <string.h>
#include "awale.h"
#include "Serveur/server.h"

typedef struct Game
{
    Client *player1;
    Client *player2;
    Client *current_turn;
    char moves[BUF_SIZE];
    int game_over;
    Awale jeu; // Plateau de jeu utilisé par les fonctions de awale.c
} Game;

void play_game(Game *game, Client *client1, Client *client2);
int process_move(Game *game, Client *client, int move, char *moves);
// void end_game(Game *game);
void send_update_to_players(Game *game);
void generate_board_state(Game *game, char *buffer_client1, char *buffer_client2);
Game *create_game(Client *client1, Client *client2);
void log_game_to_json(Game *game, const char *winner_name, const char *moves);
void initialiserGame(Game *game, Client *client1, Client *client2);
void jouerCoup(Game *game, char *buffer);
void afficherplateau(Game *game);

#endif /* GAME_H */