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
    Client *observers[MAX_CLIENTS];
    int nb_observers;
    bool private;

    Awale jeu; // Plateau de jeu utilisé par les fonctions de awale.c
} Game;

int process_move(Game *game, Client *client, int move, char *moves);
void generate_board_state(Game *game, char *buffer_client1, char *buffer_client2);
void generate_board_state_Observateur(Game *game, char *bufferObservateur);
Game *create_game(Client *client1, Client *client2);
void log_game_to_json(Game *game, const char *winner_name, const char *moves);
void initialiserGame(Game *game, Client *client1, Client *client2);
void jouerCoup(Game *game, char *buffer);
void display_board(Game *game);
void end_game(Game *game);
void display_board_Observateur(Client *client);
void add_observer(Game *game, Client *observer);
void remove_observer(Game *game, Client *observer);
#endif /* GAME_H */