#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

void play_game(Game *game, Client *client1, Client *client2)
{
    game->player1 = client1;
    game->player2 = client2;
    game->current_turn = (rand() % 2 == 0) ? client1 : client2;

    initialiserPlateau(&game->jeu);

    char buffer[BUF_SIZE];
    char buffer_player1[BUF_SIZE];
    char buffer_player2[BUF_SIZE];
    char moves[BUF_SIZE] = ""; // Suivi des coups
    Client *current_client;

    while (!game->game_over)
    {
        current_client = game->current_turn;

        // Générer l'affichage du plateau pour chaque joueur
        generate_board_state(game, buffer_player1, buffer_player2);
        write_client(client1->sock, buffer_player1);
        write_client(client2->sock, buffer_player2);

        // Indiquer le tour actuel
        write_client(current_client->sock, "C'est votre tour !");
        write_client(current_client->sock, "Choisissez un trou entre 1 et 6 :");

        if (current_client == client1)
        {
            write_client(client2->sock, "C'est le tour de l'adversaire.\n");
        }
        else
        {
            write_client(client1->sock, "C'est le tour de l'adversaire.\n");
        }

        // Lecture du coup
        if (read_client(current_client->sock, buffer) > 0)
        {
            int move = atoi(buffer);
            if (current_client == client1)
            {
                move -= 1; // Ajuster l'index
            }
            else if (current_client == client2)
            {
                move += 5; // Ajuster l'index
            }

            // Valider le coup
            if (process_move(game, current_client, move, moves))
            {
                // Ajouter le coup au suivi
                char move_str[16];
                snprintf(move_str, sizeof(move_str), "%d ", move);
                strncat(moves, move_str, sizeof(moves) - strlen(moves) - 1);

                // Alterner les joueurs
                game->current_turn = (current_client == client1) ? client2 : client1;
            }
            else
            {
                write_client(current_client->sock, "Coup invalide, essayez à nouveau.");
            }
        }
        else
        {
            game->game_over = 1;
            snprintf(buffer, BUF_SIZE, "L'autre joueur a quitté la partie.");
            write_client(client1->sock, buffer);
            write_client(client2->sock, buffer);
        }

        // Vérification de fin de partie
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

    // Déterminer le gagnant
    const char *winner = (game->jeu.scoreJoueur1 > game->jeu.scoreJoueur2)
                             ? client1->name
                         : (game->jeu.scoreJoueur1 < game->jeu.scoreJoueur2)
                             ? client2->name
                             : "Égalité";

    // Enregistrer dans JSON
    log_game_to_json(game, winner, moves);
}

// Gestion d'un coup joué par un joueur
int process_move(Game *game, Client *client, int move, char *moves)
{
    Awale *jeu = &game->jeu;
    char buffer[BUF_SIZE];

    // Vérifiez si le mouvement est valide
    if ((client == game->player1 && (move < 0 || move > 5 || jeu->trous[move] == 0)) ||
        (client == game->player2 && (move < 6 || move > 11 || jeu->trous[move] == 0)))
    {
        return 0; // Mouvement invalide
    }

    // Ajout du coup dans l'historique
    char move_str[16];
    snprintf(move_str, sizeof(move_str), "%d ", move);
    strncat(moves, move_str, sizeof(moves) - strlen(moves) - 1);

    distribuerEtCapturer(jeu, move, buffer);

    // Vérifiez si le jeu est terminé
    if (partieTerminee(jeu, buffer))
    {
        game->game_over = 1;
    }

    return 1; // Mouvement valide
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
             "\n==============================\nScores - Vous : %d | Adversaire: %d\n",
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
             "\n==============================\nScores - Vous : %d | Adversaire: %d\n",
             game->jeu.scoreJoueur2, game->jeu.scoreJoueur1);
}

int get_next_game_id()
{
    FILE *id_file = fopen("data/game_id.txt", "r+"); // Ouvrir en lecture/écriture
    int id = 1;

    if (id_file == NULL)
    {
        // Si le fichier n'existe pas, créez-le et initialisez l'ID à 1
        id_file = fopen("data/game_id.txt", "w");
        if (id_file == NULL)
        {
            perror("Erreur d'ouverture du fichier d'ID");
            return -1;
        }
        fprintf(id_file, "%d\n", id);
    }
    else
    {
        // Lire l'ID courant depuis le fichier
        fscanf(id_file, "%d", &id);
        rewind(id_file);                  // Revenir au début pour écraser la valeur
        fprintf(id_file, "%d\n", id + 1); // Incrémenter et sauvegarder le nouvel ID
    }

    fclose(id_file);
    return id;
}

void log_game_to_json(Game *game, const char *winner_name, const char *moves)
{
    int game_id = get_next_game_id();
    if (game_id < 0)
    {
        return; // Gestion d'erreur si l'ID ne peut pas être généré
    }

    FILE *file = fopen("data/games.json", "a");
    if (file == NULL)
    {
        perror("Erreur d'ouverture du fichier JSON");
        return;
    }

    // Obtenir la date et l'heure actuelles
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

    // Ajouter les informations de la partie en format JSON
    if (game_id > 1)
    {
        fprintf(file, ",\n{\n");
    }
    else
    {
        fprintf(file, "{\n");
    }
    fprintf(file, "  \"id\": %d,\n", game_id);
    fprintf(file, "  \"date\": \"%s\",\n", timestamp);
    fprintf(file, "  \"player1\": \"%s\",\n", game->player1->name);
    fprintf(file, "  \"player2\": \"%s\",\n", game->player2->name);
    fprintf(file, "  \"first_player\": \"%s\",\n", game->current_turn == game->player1 ? game->player1->name : game->player2->name);
    fprintf(file, "  \"winner\": \"%s\",\n", winner_name);
    fprintf(file, "  \"moves\": \"%s\"\n", moves);
    fprintf(file, "}");

    fclose(file);
}
void initialiserGame(Game *game, Client *client1, Client *client2)
{
    game->player1 = client1;
    game->player2 = client2;
    if (rand() % 2 == 0)
    {
        game->current_turn = client1;
        client1->tour = yes;
    }
    else
    {
        game->current_turn = client2;
        client2->tour = yes;
    }

    strcpy(game->moves, "");
    game->game_over = 0;
    initialiserPlateau(&game->jeu);
}

void afficherplateau(Game *game)
{

    char buffer_player1[BUF_SIZE];
    char buffer_player2[BUF_SIZE];
    generate_board_state(game, buffer_player1, buffer_player2);
    write_client(game->player1->sock, buffer_player1);
    write_client(game->player2->sock, buffer_player2);

    // Indiquer le tour actuel
    write_client(game->current_turn->sock, "C'est votre tour !");
    write_client(game->current_turn->sock, "Choisissez un trou entre 1 et 6 :");
    if (game->current_turn == game->player1)
    {
        write_client(game->player2->sock, "C'est le tour de l'adversaire.\n");
    }
    else
    {
        write_client(game->player1->sock, "C'est le tour de l'adversaire.\n");
    }
}

void jouerCoup(Game *game, char *buffer)
{

    int move = atoi(buffer);
    if (game == NULL)
    {
        fprintf(stderr, "Erreur : game est NULL dans jouerCoup.\n");
        return;
    }
    if (game->player1 == NULL)
    {
        fprintf(stderr, "Erreur : player1 est NULL dans jouerCoup.\n");
        return;
    }
    if (game->player2 == NULL)
    {
        fprintf(stderr, "Erreur : player2 est NULL dans jouerCoup.\n");
        return;
    }
    if (game->current_turn == NULL)
    {
        fprintf(stderr, "Erreur : current_turn est NULL dans jouerCoup.\n");
        return;
    }
    if (game->current_turn == game->player1)
    {
        move -= 1; // Ajuster l'index
        printf("on est arrivé ici client 1");
        fflush(stdout);
    }
    else if (game->current_turn == game->player2)
    {
        move += 5; // Ajuster l'index
        printf("on est arrivé ici client 2");
        fflush(stdout);
    }

    // Valider le coup
    if (process_move(game, game->current_turn, move, game->moves))
    {
        // Ajouter le coup au suivi
        char move_str[16];
        snprintf(move_str, sizeof(move_str), "%d ", move);
        strncat(game->moves, move_str, sizeof(game->moves) - strlen(game->moves) - 1);

        // Alterner les joueurs
        if (game->current_turn == game->player1)
        {
            game->current_turn = game->player2;
            game->player1->tour = no;
            game->player2->tour = yes;
        }
        else if (game->current_turn == game->player2)
        {
            game->current_turn = game->player1;
            game->player2->tour = no;
            game->player1->tour = yes;
        }
    }
    else
    {
        write_client(game->current_turn->sock, "Coup invalide, essayez à nouveau.");
    }
}