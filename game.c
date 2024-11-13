#include "game.h"
#include <stdio.h>
#include <stdlib.h>

void init_game(Game *game, Client *client1, Client *client2)
{
    game->player1 = client1;
    game->player2 = client2;
    game->current_turn = 0; // On commence avec le tour du Joueur 1
    game->game_over = 0;
    initialiserPlateau(&game->jeu); // Utilise initialiserPlateau pour configurer le plateau
}

// Gestion d'un coup joué par un joueur
void process_move(Game *game, Client *client, int move)
{
    if ((game->current_turn % 2 == 0 && client != game->player1) ||
        (game->current_turn % 2 != 0 && client != game->player2))
    {
        write_client(client->sock, "Ce n'est pas votre tour!\n");
        return;
    }

    int index;
    // Vérification de la validité du coup
    if ((game->current_turn % 2 == 0 && (move < 0 || move > 5 || game->jeu.trous[move] == 0)) ||
        (game->current_turn % 2 != 0 && (move < 6 || move > 11 || game->jeu.trous[move] == 0)))
    {
        write_client(client->sock, "Mouvement invalide! Réessayez.\n");
        return;
    }
    index = move;

    // Distribuer les graines et capturer
    int dernierTrou = distribuerGraines(&game->jeu, index);
    capturerGraines(&game->jeu, dernierTrou);

    // Vérifier si la partie est terminée
    game->game_over = partieTerminee(&game->jeu);
    if (game->game_over)
    {
        end_game(game);
    }
    else
    {
        game->current_turn++; // Alterner le tour
        send_update_to_players(game);
    }
}

// Fonction pour terminer la partie et envoyer le résultat aux joueurs
void end_game(Game *game)
{
    char result[BUF_SIZE];
    snprintf(result, BUF_SIZE, "END Game over! Scores: %s: %d, %s: %d",
             game->player1->name, game->jeu.scoreJoueur1,
             game->player2->name, game->jeu.scoreJoueur2);

    write_client(game->player1->sock, result);
    write_client(game->player2->sock, result);
    game->game_over = 1;
}

// Fonction pour envoyer l'état actuel du plateau aux joueurs
void send_update_to_players(Game *game)
{
    char board_state[BUF_SIZE];
    snprintf(board_state, BUF_SIZE, "Plateau: J1: ");
    for (int i = TROUS / 2 - 1; i >= 0; i--)
    {
        snprintf(board_state + strlen(board_state), BUF_SIZE - strlen(board_state), "%d ", game->jeu.trous[i]);
    }
    snprintf(board_state + strlen(board_state), BUF_SIZE - strlen(board_state), " | J2: ");
    for (int i = TROUS / 2; i < TROUS; i++)
    {
        snprintf(board_state + strlen(board_state), BUF_SIZE - strlen(board_state), "%d ", game->jeu.trous[i]);
    }
    snprintf(board_state + strlen(board_state), BUF_SIZE - strlen(board_state), "Scores - J1: %d, J2: %d",
             game->jeu.scoreJoueur1, game->jeu.scoreJoueur2);

    // Envoyer l'état à chaque joueur
    write_client(game->player1->sock, board_state);
    write_client(game->player2->sock, board_state);
}

// Fonction pour créer une nouvelle partie
Game *create_game(Client *client1, Client *client2)
{
    Game *game = (Game *)malloc(sizeof(Game));
    if (game == NULL)
    {
        perror("malloc");
        return NULL;
    }
    init_game(game, client1, client2);
    return game;
}