#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include "3rdparty/cJSON/cJSON.h"

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

        // Déterminer les scores
        const char *winner = (jeu->scoreJoueur1 > jeu->scoreJoueur2) ? game->player1->name : (jeu->scoreJoueur1 < jeu->scoreJoueur2) ? game->player2->name
                                                                                                                                     : "Égalité";

        // Informer les joueurs
        char result_message[BUF_SIZE];
        snprintf(result_message, BUF_SIZE,
                 "Partie terminée ! Scores finaux : %s: %d, %s: %d. Gagnant : %s\n",
                 game->player1->name, jeu->scoreJoueur1,
                 game->player2->name, jeu->scoreJoueur2, winner);

        write_client(game->player1->sock, result_message);
        write_client(game->player2->sock, result_message);

        // Enregistrer dans un fichier si nécessaire (JSON par exemple)
        log_game_to_json(game, winner, game->moves);

        if (game->game_over)
        {
            end_game(game);

            // Ne pas retourner à du code qui utilise `game` ou ses champs
            return 1; // Fin du jeu
        }
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
void generate_board_state_Observateur(Game *game, char *bufferObservateur)
{
    snprintf(bufferObservateur, BUF_SIZE, "Plateau actuel:\n==============================\n%s : ", game->player2->name);
    for (int i = TROUS - 1; i >= TROUS / 2; i--)
    {
        snprintf(bufferObservateur + strlen(bufferObservateur), BUF_SIZE - strlen(bufferObservateur), "%d ", game->jeu.trous[i]);
    }

    snprintf(bufferObservateur + strlen(bufferObservateur), BUF_SIZE, "\n%s    : ", game->player1->name);
    for (int i = 0; i < TROUS / 2; i++)
    {
        snprintf(bufferObservateur + strlen(bufferObservateur), BUF_SIZE - strlen(bufferObservateur), "%d ", game->jeu.trous[i]);
    }
    snprintf(bufferObservateur + strlen(bufferObservateur), BUF_SIZE - strlen(bufferObservateur),
             "\n==============================\nScores - %s : %d | %s: %d\n",
             game->player1->name, game->jeu.scoreJoueur1, game->player2->name, game->jeu.scoreJoueur2);
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
    FILE *file = fopen("data/games.json", "r+");
    if (file == NULL)
    {
        perror("Erreur d'ouverture du fichier JSON");
        return;
    }

    // Charger le contenu existant (ou créer un tableau vide)
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *data = NULL;
    if (file_size > 0)
    {
        data = (char *)malloc(file_size + 1);
        fread(data, 1, file_size, file);
        data[file_size] = '\0';
    }

    cJSON *game_array = data ? cJSON_Parse(data) : cJSON_CreateArray();
    free(data);

    if (!game_array)
    {
        game_array = cJSON_CreateArray();
    }

    // Créer un nouvel objet JSON pour la partie
    cJSON *game_obj = cJSON_CreateObject();

    // Obtenir la date et l'heure actuelles
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

    // Ajouter les informations de la partie
    cJSON_AddNumberToObject(game_obj, "id", get_next_game_id());
    cJSON_AddStringToObject(game_obj, "date", timestamp);
    cJSON_AddStringToObject(game_obj, "player1", game->player1->name);
    cJSON_AddStringToObject(game_obj, "player2", game->player2->name);
    cJSON_AddStringToObject(game_obj, "first_player", game->current_turn == game->player1 ? game->player1->name : game->player2->name);
    cJSON_AddStringToObject(game_obj, "winner", winner_name);
    cJSON_AddStringToObject(game_obj, "moves", moves);

    // Ajouter l'objet au tableau
    cJSON_AddItemToArray(game_array, game_obj);

    // Sauvegarder dans le fichier
    char *json_string = cJSON_Print(game_array);
    freopen("data/games.json", "w", file); // Réouvrir en mode écriture
    fprintf(file, "%s", json_string);
    fclose(file);

    // Nettoyer
    cJSON_Delete(game_array);
    free(json_string);
}

void initialiserGame(Game *game, Client *client1, Client *client2)
{
    game->player1 = client1;
    game->player2 = client2;

    srand((unsigned int)(time(NULL) ^ (uintptr_t)&game));
    int random = rand() % 2;

    if (random == 0)
    {
        game->current_turn = client1;
        client1->tour = yes;
        client2->tour = no;
    }
    else
    {
        game->current_turn = client2;
        client2->tour = yes;
        client1->tour = no;
    }

    strcpy(game->moves, "");
    game->game_over = 0;
    initialiserPlateau(&game->jeu);
}

void display_board(Game *game)
{
    if (!game)
    {
        fprintf(stderr, "Erreur : tentative d'afficher un plateau pour un jeu inexistant.\n");
        return;
    }

    if (game->game_over)
    {
        fprintf(stderr, "Erreur : tentative d'afficher un plateau pour une partie terminée.\n");
        return;
    }

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
void display_board_Observateur(Client *client)
{
    if (!(client->game))
    {
        fprintf(stderr, "Erreur : tentative d'afficher un plateau pour un jeu inexistant.\n");
        return;
    }

    if (client->game->game_over)
    {
        fprintf(stderr, "Erreur : tentative d'afficher un plateau pour une partie terminée.\n");
        return;
    }

    char buffer_observateur[BUF_SIZE];
    generate_board_state_Observateur(client->game, buffer_observateur);
    write_client(client->sock, buffer_observateur);
}

void jouerCoup(Game *game, char *buffer)
{

    int move = atoi(buffer);
    if (!game)
    {
        fprintf(stderr, "Erreur : tentative d'accès à un jeu inexistant.\n");
        return;
    }

    if (game->game_over)
    {
        fprintf(stderr, "Erreur : tentative de jouer un coup sur une partie terminée.\n");
        return;
    }
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
        printf("on est arrivé ici client 1\n");
        fflush(stdout);
    }
    else if (game->current_turn == game->player2)
    {
        move += 5; // Ajuster l'index
        printf("on est arrivé ici client 2\n");
        fflush(stdout);
    }

    // Valider le coup
    if (process_move(game, game->current_turn, move, game->moves))
    {
        // Ajouter le coup au suivi
        char move_str[16];
        snprintf(move_str, sizeof(move_str), "%d ", move);
        strncat(game->moves, move_str, sizeof(game->moves) - strlen(game->moves) - 1);

        if (game->game_over)
        {
            return; // Arrêter ici pour éviter d'exécuter le reste du code
        }
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

void end_game(Game *game)
{
    if (!game)
    {
        fprintf(stderr, "Erreur : game est NULL dans end_game.\n");
        return;
    }

    // Réinitialiser les états des joueurs
    game->player1->etat = Initialisation;
    game->player2->etat = Initialisation;

    game->player1->tour = no;
    game->player2->tour = no;

    game->player1->game = NULL;
    game->player2->game = NULL;
    write_client(game->player1->sock, "La partie est terminée.\nVeuillez choisir une option :\n1. Jouer contre un adversaire en ligne\n2.Observer une partie\n3. Quitter le jeu");
    write_client(game->player2->sock, "La partie est terminée.\nVeuillez choisir une option :\n1. Jouer contre un adversaire en ligne\n2.Observer une partie\n3. Quitter le jeu");

    // Libérer la mémoire allouée au jeu

    printf("La partie est terminée et la mémoire a été libérée.\n");
    fflush(stdout);
}
