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
void afficherPlateau(const Awale *jeu, char *buffer)
{
    snprintf(buffer, BUF_SIZE, "==============================\n");

    strcat(buffer, "Camp Joueur 2 : ");
    for (int i = TROUS - 1; i >= TROUS / 2; i--)
    {
        char trou[4];
        snprintf(trou, sizeof(trou), "%d ", jeu->trous[i]);
        strcat(buffer, trou);
    }
    strcat(buffer, "\nCamp Joueur 1 : ");
    for (int i = 0; i < TROUS / 2; i++)
    {
        char trou[4];
        snprintf(trou, sizeof(trou), "%d ", jeu->trous[i]);
        strcat(buffer, trou);
    }

    char scores[64];
    snprintf(scores, sizeof(scores), "\n==============================\nScore Joueur 1: %d | Score Joueur 2: %d\n",
             jeu->scoreJoueur1, jeu->scoreJoueur2);
    strcat(buffer, scores);
}

// Fonction pour jouer un tour et renvoyer un message demandant le choix d'un trou
void jouerTour(const Awale *jeu, char *buffer)
{
    if (jeu->tour % 2 == 0) // Joueur 1
    {
        snprintf(buffer, BUF_SIZE, "Joueur 1, choisissez un trou (0 à 5): ");
    }
    else // Joueur 2
    {
        snprintf(buffer, BUF_SIZE, "Joueur 2, choisissez un trou (6 à 11): ");
    }
}
// Fonction pour distribuer les graines
int distribuerGraines(Awale *jeu, int trou)
{
    int graines = jeu->trous[trou];
    jeu->trous[trou] = 0;
    int index = trou;

    // Distribuer les graines une par une
    while (graines > 0)
    {
        index = (index + 1) % TROUS;
        if (index != trou) // Éviter de remettre une graine dans le trou d'origine
        {
            jeu->trous[index]++;
            graines--;
        }
    }
    return index; // Retourne le dernier trou où une graine a été déposée
}

// Fonction pour capturer les graines
void capturerGraines(Awale *jeu, int dernierTrou)
{
    // Déterminer le joueur actuel
    bool joueur1 = (jeu->tour % 2 == 0);
    int *score = joueur1 ? &jeu->scoreJoueur1 : &jeu->scoreJoueur2;

    // Capture des graines dans le camp de l'adversaire
    while ((joueur1 && dernierTrou >= 6 && dernierTrou <= 11) ||
           (!joueur1 && dernierTrou >= 0 && dernierTrou <= 5))
    {
        // Vérifie si le trou contient 2 ou 3 graines pour capturer
        if (jeu->trous[dernierTrou] == 2 || jeu->trous[dernierTrou] == 3)
        {
            *score += jeu->trous[dernierTrou];
            jeu->trous[dernierTrou] = 0;
            dernierTrou--; // Aller au trou précédent pour continuer la capture

            // Conditions de sortie pour éviter de capturer dans le propre camp du joueur
            if (joueur1 && dernierTrou < 6)
            {
                break; // Arrêtez si le joueur 1 sort du camp de Joueur 2
            }
            else if (!joueur1 && dernierTrou < 0)
            {
                break; // Arrêtez si le joueur 2 sort du camp de Joueur 1
            }
        }
        else
        {
            break; // Arrête la capture si le trou ne contient pas 2 ou 3 graines
        }
    }
}

// Fonction pour distribuer les graines et capturer, en renvoyant un message décrivant l’action
int distribuerEtCapturer(Awale *jeu, int trou, char *buffer)
{
    int dernierTrou = distribuerGraines(jeu, trou);
    capturerGraines(jeu, dernierTrou);
    snprintf(buffer, BUF_SIZE, "Graines distribuées à partir du trou %d. Dernier trou atteint : %d.\n", trou, dernierTrou);
    return dernierTrou;
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
    bool terminee = (grainesJoueur1 == 0 || grainesJoueur2 == 0 || jeu->scoreJoueur1 > 24 || jeu->scoreJoueur2 > 24);
    if (terminee)
    {
        snprintf(buffer, BUF_SIZE, "Partie terminée ! Score final : Joueur 1 - %d, Joueur 2 - %d\n",
                 jeu->scoreJoueur1, jeu->scoreJoueur2);
    }
    return terminee;
}
