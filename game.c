#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void play_game(Game *game, Client *client1, Client *client2)
{
    game->player1 = client1;
    game->player2 = client2;

    // Déterminer qui commence aléatoirement
    game->current_turn = (rand() % 2 == 0) ? client1 : client2;

    initialiserPlateau(&game->jeu); // Initialisation du plateau

    char buffer[BUF_SIZE];
    char buffer_player1[BUF_SIZE];
    char buffer_player2[BUF_SIZE];

    Client *current_client;

    while (!game->game_over)
    {
        current_client = game->current_turn;

        // Générer l'affichage du plateau pour chaque joueur
        generate_board_state(game, buffer_player1, buffer_player2);
        write_client(client1->sock, buffer_player1);
        write_client(client2->sock, buffer_player2);

        // Affichage de l'invite pour le joueur actuel
        write_client(current_client->sock, "C'est votre tour !");
        write_client(current_client->sock, "Choisissez un trou entre 1 et 6 :");

        // Lire le choix du joueur
        if (read_client(current_client->sock, buffer) > 0)
        {
            int move = atoi(buffer);
            if (current_client == client1)
            {
                move -= 1; // Ajuster pour l'index
            }
            else if (current_client == client2)
            {
                move += 5; // Ajuster pour l'index
            }

            // Traiter le coup joué
            if (process_move(game, current_client, move))
            {
                // Si le coup est valide, alterner les joueurs
                game->current_turn = (current_client == client1) ? client2 : client1;
            }
            else
            {
                // Si le coup est invalide
                write_client(current_client->sock, "Coup invalide, essayez à nouveau.\n");
            }
        }
        else
        {
            // Si un joueur se déconnecte
            game->game_over = 1;
            snprintf(buffer, BUF_SIZE, "L'autre joueur a quitté la partie.");
            write_client(client1->sock, buffer);
            write_client(client2->sock, buffer);
        }

        // Vérifier si le jeu est terminé
        if (partieTerminee(&game->jeu, buffer))
        {
            game->game_over = 1;
            snprintf(buffer, BUF_SIZE, "Partie terminée ! Scores : %s: %d, %s: %d",
                     client1->name, game->jeu.scoreJoueur1,
                     client2->name, game->jeu.scoreJoueur2);
            write_client(client1->sock, buffer);
            write_client(client2->sock, buffer);
        }
    }
}

// Gestion d'un coup joué par un joueur
int process_move(Game *game, Client *client, int move)
{
    Awale *jeu = &game->jeu;
    int index = move;

    // Vérifie si le mouvement est valide pour le joueur actuel
    char buffer[BUF_SIZE];
    if ((client == game->player1 && (move < 0 || move > 5 || jeu->trous[move] == 0)) ||
        (client == game->player2 && (move < 6 || move > 11 || jeu->trous[move] == 0)))
    {
        return 0;
    }

    distribuerEtCapturer(jeu, index, buffer);
    write_client(client->sock, buffer);

    // Vérifie si le jeu est terminé
    if (partieTerminee(jeu, buffer))
    {
        game->game_over = 1;
        write_client(client->sock, buffer);
    }

    return 1; // Mouvement valide
}

// Fonction pour terminer la partie et envoyer le résultat aux joueurs
void end_game(Game *game)
{
    char result[BUF_SIZE];
    snprintf(result, BUF_SIZE, "END Game over! Scores: %s: %d, %s: %d \n",
             game->player1->name, game->jeu.scoreJoueur1,
             game->player2->name, game->jeu.scoreJoueur2);

    write_client(game->player1->sock, result);
    write_client(game->player2->sock, result);
    game->game_over = 1;
}

// Fonction pour créer une nouvelle partie
Game *create_game(Client *client1, Client *client2)
{
    Game *game = (Game *)malloc(sizeof(Game));
    if (game == NULL)
    {
        perror("Erreur d'allocation mémoire pour Game");
        return NULL;
    }

    game->player1 = client1;
    game->player2 = client2;
    game->current_turn = client1;
    game->game_over = 0;

    return game;
}

// Génère l'état du plateau sous forme de chaîne de caractères
void generate_board_state(Game *game, char *buffer_client1, char *buffer_client2)
{
    snprintf(buffer_client1, BUF_SIZE, "Plateau actuel:\n==============================\nAdversaire : ");
    for (int i = TROUS - 1; i >= TROUS / 2; i--)
    {
        snprintf(buffer_client1 + strlen(buffer_client1), BUF_SIZE - strlen(buffer_client1), "%d ", game->jeu.trous[i]);
    }

    snprintf(buffer_client1 + strlen(buffer_client1), BUF_SIZE, "\nVous       : ");
    for (int i = 0; i < TROUS / 2; i++)
    {
        snprintf(buffer_client1 + strlen(buffer_client1), BUF_SIZE - strlen(buffer_client1), "%d ", game->jeu.trous[i]);
    }
    snprintf(buffer_client1 + strlen(buffer_client1), BUF_SIZE - strlen(buffer_client1),
             "\n==============================\nScores - Joueur 1: %d | Joueur 2: %d\n",
             game->jeu.scoreJoueur1, game->jeu.scoreJoueur2);

    snprintf(buffer_client2, BUF_SIZE, "Plateau actuel:\n==============================\nAdversaire : ");
    for (int i = (TROUS / 2) - 1; i >= 0; i--)
    {
        snprintf(buffer_client2 + strlen(buffer_client2), BUF_SIZE - strlen(buffer_client2), "%d ", game->jeu.trous[i]);
    }

    snprintf(buffer_client2 + strlen(buffer_client2), BUF_SIZE, "\nVous       : ");
    for (int i = (TROUS / 2); i < TROUS; i++)
    {
        snprintf(buffer_client2 + strlen(buffer_client2), BUF_SIZE - strlen(buffer_client2), "%d ", game->jeu.trous[i]);
    }
    snprintf(buffer_client2 + strlen(buffer_client2), BUF_SIZE - strlen(buffer_client2),
             "\n==============================\nScores - Joueur 1: %d | Joueur 2: %d\n",
             game->jeu.scoreJoueur1, game->jeu.scoreJoueur2);
}