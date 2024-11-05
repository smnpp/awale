#include <stdio.h>
#include <stdbool.h>

#define TROUS 12

// Structure pour représenter le plateau de jeu
typedef struct {
    int trous[TROUS];
    int scoreJoueur1;
    int scoreJoueur2;
} Awale;

// Fonction pour initialiser le plateau de jeu
void initialiserPlateau(Awale *jeu) {
    for (int i = 0; i < TROUS; i++) {
        jeu->trous[i] = 4;
    }
    jeu->scoreJoueur1 = 0;
    jeu->scoreJoueur2 = 0;
}

// Afficher le plateau de jeu
void afficherPlateau(const Awale *jeu) {
    printf("Camp Joueur 2 : ");
    for (int i = TROUS / 2; i < TROUS; i++) {
        printf("%d ", jeu->trous[i]);
    }
    printf("\nCamp Joueur 1 : ");
    for (int i = TROUS / 2 - 1; i >= 0; i--) {
        printf("%d ", jeu->trous[i]);
    }
    printf("\nScore Joueur 1: %d | Score Joueur 2: %d\n", jeu->scoreJoueur1, jeu->scoreJoueur2);
}

// Fonction pour distribuer les graines
void distribuerGraines(Awale *jeu, int trou) {
    int graines = jeu->trous[trou];
    jeu->trous[trou] = 0;
    int index = trou;

    // Distribuer les graines une par une
    while (graines > 0)
    {
        index = (index - 1) % TROUS;
        jeu->trous[index]++;
        graines--;
    }
}

// Fonction pour vérifier si le joueur peut capturer des graines
void capturerGraines(Awale *jeu, int dernierTrou, bool joueur1) {
    int *score = joueur1 ? &jeu->scoreJoueur1 : &jeu->scoreJoueur2;
    while (dernierTrou >= (joueur1 ? 0 : TROUS / 2) && dernierTrou < (joueur1 ? TROUS / 2 : TROUS)) {
        if (jeu->trous[dernierTrou] == 2 || jeu->trous[dernierTrou] == 3) {
            *score += jeu->trous[dernierTrou];
            jeu->trous[dernierTrou] = 0;
            dernierTrou--;
        } else {
            break;
        }
    }
}

// Fonction principale pour jouer un tour
void jouerTour(Awale *jeu, bool joueur1) {
    int trou;
    afficherPlateau(jeu);
    printf("Joueur %d, choisissez un trou (0 à 5): ", joueur1 ? 1 : 2);
    scanf("%d", &trou);

    // Vérification de la validité de l'entrée
    int index = joueur1 ? TROUS / 2 - 1 - trou : trou + TROUS / 2;
    if (trou < 0 || trou > 5 || jeu->trous[index] == 0) {
        printf("Entrée invalide ou trou vide. Choisissez un autre trou.\n");
        jouerTour(jeu, joueur1);
        return;
    }

    distribuerGraines(jeu, index);
    capturerGraines(jeu, index, joueur1);
}

// Fonction pour vérifier si la partie est terminée
bool partieTerminee(const Awale *jeu) {
    int grainesJoueur1 = 0, grainesJoueur2 = 0;
    for (int i = 0; i < TROUS / 2; i++) {
        grainesJoueur1 += jeu->trous[i];
        grainesJoueur2 += jeu->trous[i + TROUS / 2];
    }
    return (grainesJoueur1 == 0 || grainesJoueur2 == 0 || jeu->scoreJoueur1 > 24 || jeu->scoreJoueur2 > 24);
}

// Fonction principale
int main() {
    Awale jeu;
    initialiserPlateau(&jeu);
    bool joueur1 = true;

    while (!partieTerminee(&jeu)) {
        jouerTour(&jeu, joueur1);
        joueur1 = !joueur1;
    }

    printf("Partie terminée ! Score final : Joueur 1 - %d, Joueur 2 - %d\n", jeu.scoreJoueur1, jeu.scoreJoueur2);
    return 0;
}
