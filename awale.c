#include <stdio.h>
#include <stdbool.h>

#define TROUS 12

// Structure pour représenter le plateau de jeu
typedef struct
{
    int trous[TROUS];
    int scoreJoueur1;
    int scoreJoueur2;
    int tour; // Utilisé pour déterminer quel joueur joue
} Awale;

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

// Afficher le plateau de jeu
void afficherPlateau(const Awale *jeu)
{
    printf("==============================\n");

    // Afficher le camp du Joueur 2 de droite à gauche
    printf("Camp Joueur 2 : ");
    for (int i = TROUS - 1; i >= TROUS / 2; i--)
    {
        printf("%d ", jeu->trous[i]);
    }
    printf("\n");

    // Afficher le camp du Joueur 1 de gauche à droite
    printf("Camp Joueur 1 : ");
    for (int i = 0; i < TROUS / 2; i++)
    {
        printf("%d ", jeu->trous[i]);
    }
    printf("\n");

    printf("==============================\n");
    printf("Score Joueur 1: %d | Score Joueur 2: %d\n", jeu->scoreJoueur1, jeu->scoreJoueur2);
}

// Fonction pour distribuer les graines
void distribuerGraines(Awale *jeu, int trou)
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
}

// Fonction pour capturer les graines
void capturerGraines(Awale *jeu, int dernierTrou)
{
    // Déterminer le score du joueur en fonction du tour
    int score = (jeu->tour % 2 == 0) ? jeu->scoreJoueur1 : jeu->scoreJoueur2;

    // Vérifier les captures dans le camp de l'adversaire
    while ((jeu->tour % 2 == 0 && dernierTrou >= 6 && dernierTrou <= 11) ||
           (jeu->tour % 2 != 0 && dernierTrou >= 0 && dernierTrou <= 5))
    {
        if (jeu->trous[dernierTrou] == 2 || jeu->trous[dernierTrou] == 3)
        {
            score += jeu->trous[dernierTrou];
            jeu->trous[dernierTrou] = 0;
            dernierTrou--;
            if (jeu->tour % 2 == 0 && dernierTrou < 6)
            {
                jeu->scoreJoueur1 = score;
            }
            else if (jeu->tour % 2 != 0 && dernierTrou < 0)
            {
                jeu->scoreJoueur2 = score;
            }
        }
        else
        {
            break;
        }
    }
}

// Fonction pour jouer un tour
void jouerTour(Awale *jeu)
{
    int trou;
    afficherPlateau(jeu);

    // Demander au joueur de choisir un trou en fonction de son tour
    if (jeu->tour % 2 == 0) // Joueur 1
    {
        printf("Joueur 1, choisissez un trou (0 à 5): ");
    }
    else // Joueur 2
    {
        printf("Joueur 2, choisissez un trou (6 à 11): ");
    }
    scanf("%d", &trou);

    // Vérification de la validité de l'entrée
    int index;
    if (jeu->tour % 2 == 0) // Joueur 1
    {
        if (trou < 0 || trou > 5 || jeu->trous[trou] == 0)
        {
            printf("Entrée invalide ou trou vide. Choisissez un autre trou.\n");
            jouerTour(jeu);
            return;
        }
        index = trou; // Trous 0 à 5 pour le Joueur 1
    }
    else // Joueur 2
    {
        if (trou < 6 || trou > 11 || jeu->trous[trou] == 0)
        {
            printf("Entrée invalide ou trou vide. Choisissez un autre trou.\n");
            jouerTour(jeu);
            return;
        }
        index = trou; // Trous 6 à 11 pour le Joueur 2
    }

    distribuerGraines(jeu, index);
    capturerGraines(jeu, index); // Capture des graines

    jeu->tour++; // Incrémenter le tour après chaque tour
}

// Fonction pour vérifier si la partie est terminée
bool partieTerminee(const Awale *jeu)
{
    int grainesJoueur1 = 0, grainesJoueur2 = 0;
    for (int i = 0; i < TROUS / 2; i++)
    {
        grainesJoueur1 += jeu->trous[i];
        grainesJoueur2 += jeu->trous[i + TROUS / 2];
    }
    return (grainesJoueur1 == 0 || grainesJoueur2 == 0 || jeu->scoreJoueur1 > 24 || jeu->scoreJoueur2 > 24);
}

// Fonction principale
int main()
{
    Awale jeu;
    initialiserPlateau(&jeu);

    while (!partieTerminee(&jeu))
    {
        jouerTour(&jeu);
    }

    printf("Partie terminée ! Score final : Joueur 1 - %d, Joueur 2 - %d\n", jeu.scoreJoueur1, jeu.scoreJoueur2);
    return 0;
}