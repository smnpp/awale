#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "awale.h"

#define TROUS 12
#define BUF_SIZE 512 // Taille du buffer pour les chaînes de retour

// Fonction pour initialiser le plateau de jeu
void initialiserPlateau(Awale *jeu)
{
    for (int i = 0; i < TROUS; i++)
    {
        jeu->trous[i] = 4; // Initialiser chaque trou avec 4 graines
    }
    jeu->scoreJoueur1 = 0;
    jeu->scoreJoueur2 = 0;
    jeu->tour = 0; // Tour commence à 0
}

// Afficher le plateau de jeu sous forme de chaîne
void afficherPlateau(const Awale *jeu)
{
    printf("==============================\n");
    printf("Camp Joueur 2 : ");
    for (int i = TROUS - 1; i >= TROUS / 2; i--)
    {
        printf("%d ", jeu->trous[i]);
    }
    printf("\n");

    printf("Camp Joueur 1 : ");
    for (int i = 0; i < TROUS / 2; i++)
    {
        printf("%d ", jeu->trous[i]);
    }
    printf("\n");

    printf("==============================\n");
    printf("Score Joueur 1: %d | Score Joueur 2: %d\n",
           jeu->scoreJoueur1, jeu->scoreJoueur2);
    printf("==============================\n\n");

    fflush(stdout); // S'assurer que tout est affiché immédiatement
}

// Fonction pour jouer un tour et renvoyer un message demandant le choix d'un trou
void jouerTour(Awale *jeu, char *buffer)
{
    if (jeu->tour % 2 == 0)
    {
        snprintf(buffer, BUF_SIZE, "Joueur 1, choisissez un trou (0 à 5): ");
    }
    else
    {
        snprintf(buffer, BUF_SIZE, "Joueur 2, choisissez un trou (6 à 11): ");
    }

    jeu->tour++; // Passer au joueur suivant après chaque tour
}

// Fonction pour distribuer les graines
int distribuerGraines(Awale *jeu, int trou)
{
    int graines = jeu->trous[trou];
    jeu->trous[trou] = 0; // Le trou d'origine est vidé
    int index = trou;

    while (graines > 0)
    {
        index = (index + 1) % TROUS;
        if (index != trou) // Éviter de remettre une graine dans le trou d'origine
        {
            jeu->trous[index]++;
            graines--;
        }
    }
    return index; // Retourner le dernier trou où une graine a été déposée
}

// Fonction pour capturer les graines
void capturerGraines(Awale *jeu, int dernierTrou)
{
    int *score = (jeu->tour % 2 == 0) ? &jeu->scoreJoueur1 : &jeu->scoreJoueur2;

    int campDebut, campFin;

    if (jeu->firstPlayer == 1)
    {
        campDebut = (jeu->tour % 2 == 0) ? 6 : 0;
        campFin = (jeu->tour % 2 == 0) ? 11 : 5;
    }
    else
    {
        campDebut = (jeu->tour % 2 == 0) ? 0 : 6;
        campFin = (jeu->tour % 2 == 0) ? 5 : 11;
    }

    // Parcourir les trous du camp adverse pour capturer les graines
    while (dernierTrou >= campDebut && dernierTrou <= campFin)
    {
        if (jeu->trous[dernierTrou] == 2 || jeu->trous[dernierTrou] == 3)
        {
            *score += jeu->trous[dernierTrou];
            jeu->trous[dernierTrou] = 0;
            dernierTrou--;
        }
        else
        {
            break;
        }
    }
}

// Fonction pour distribuer les graines et capturer, en renvoyant un message décrivant l’action
int distribuerEtCapturer(Awale *jeu, int trou, char *buffer)
{
    if (jeu->trous[trou] == 0) // Vérifier si le trou est vide
    {
        snprintf(buffer, BUF_SIZE, "Erreur : Le trou %d est vide. Choisissez un autre trou.\n", trou);
        return -1;
    }

    int dernierTrou = distribuerGraines(jeu, trou); // Distribuer les graines
    capturerGraines(jeu, dernierTrou);              // Capturer les graines si applicable

    snprintf(buffer, BUF_SIZE, "Graines distribuées à partir du trou %d. Dernier trou atteint : %d.\n", trou, dernierTrou);

    if (partieTerminee(jeu, buffer)) // Vérifier si la partie est terminée
    {
        jeu->tour = -1; // Indiquer que la partie est terminée
    }
    else
    {
        jeu->tour++; // Passer au joueur suivant
    }

    return dernierTrou; // Retourner le dernier trou pour vérification
}

// Fonction pour vérifier si la partie est terminée et renvoyer un message final
bool partieTerminee(const Awale *jeu, char *buffer)
{
    int grainesJoueur1 = 0, grainesJoueur2 = 0;

    for (int i = 0; i < TROUS / 2; i++)
    {
        grainesJoueur1 += jeu->trous[i];
        grainesJoueur2 += jeu->trous[i + TROUS / 2];
    }

    if (grainesJoueur1 == 0 || grainesJoueur2 == 0 || jeu->scoreJoueur1 > 24 || jeu->scoreJoueur2 > 24)
    {
        snprintf(buffer, BUF_SIZE, "Partie terminée ! Score final : Joueur 1 - %d, Joueur 2 - %d\n",
                 jeu->scoreJoueur1, jeu->scoreJoueur2);
        return true;
    }

    return false;
}
