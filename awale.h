#ifndef AWALE_H
#define AWALE_H

#include <stdio.h>
#include <stdbool.h>

#define TROUS 12

// Structure pour représenter le plateau de jeu
typedef struct
{
    int trous[TROUS];
    int scoreJoueur1;
    int scoreJoueur2;
    int firstPlayer;
    int tour; // Utilisé pour déterminer quel joueur joue
} Awale;

// Prototypes des fonctions
void initialiserPlateau(Awale *jeu);
void afficherPlateau(const Awale *jeu);
int distribuerEtCapturer(Awale *jeu, int trou, char *buffer);
void capturerGraines(Awale *jeu, int dernierTrou);
int distribuerGraines(Awale *jeu, int trou);
bool partieTerminee(const Awale *jeu, char *buffer);

#endif // AWALE_H