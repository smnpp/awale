# Jeu Awalé Multijoueur

## Introduction

Ce projet implémente une version multijoueur du jeu traditionnel africain
**Awalé**. Il repose sur une architecture client-serveur où le serveur gère les
parties et les interactions, et les clients permettent aux utilisateurs de
seconnecter et de jouer.

## Installation

### Prérequis

-   **Système** : Linux, macOS ou Windows.
-   **Compilateur** : `gcc`
-   **Bibliothèques** :
    -   `cJSON` (fournie en tant que sous-module dans le dossier `3rdparty`).

### Étapes d'installation

1. **Cloner le dépôt**

    ```bash
    git clone https://github.com/smnpp/awale.git
    cd awale
    ```

2. **Importer les sous-modules** Si le projet utilise des sous-modules (comme
   `cJSON`), initialisez-les et mettez-les à jour :

    ```bash
    git submodule init
    git submodule update
    ```

3. **Compiler avec le Makefile** Utilisez le fichier `Makefile` pour compiler le
   projet. Cela créera les exécutables dans le dossier `bin/` :

    ```bash
    make
    ```

    Les fichiers suivants seront créés :

    - `bin/client_exec` : exécutable du client.
    - `bin/server_exec` : exécutable du serveur.

    En cas de besoin, vous pouvez nettoyer les fichiers générés avec :

    ```bash
    make clean
    ```

## Exécution

### Lancer le Serveur

1. Démarrez le serveur pour qu'il écoute sur le port 5788 :

    ```bash
    ./bin/server_exec
    ```

2. Le serveur est maintenant prêt à accepter les connexions des clients.

### Lancer un Client

1. Exécutez le client en spécifiant l'adresse IP du serveur et votre pseudo :

    ```bash
    ./bin/client_exec <adresse_du_serveur> <pseudo>
    ```

    Exemple :

    ```bash
    ./bin/client_exec 127.0.0.1 adalovelace
    ```

## Commandes Disponibles

### Commandes Générales

-   `/help` : Affiche la liste des commandes disponibles.
-   `/list` : Affiche la liste des joueurs connectés.
-   `/msg <pseudo> <message>` : Envoie un message privé à un joueur.
-   `/all <message>` : Envoie un message à tous les joueurs connectés.
-   `/logout` : Déconnecte le client du serveur.

### Commandes de Jeu

-   `/play <pseudo>` : Demande à jouer une partie avec un joueur.
-   `/accept` : Accepte une demande de partie.
-   `/decline` : Refuse une demande de partie.
-   `/quit` : Quitte une partie en cours.
-   `/matchmaking` : Rejoint une partie contre un adversaire aléatoire.
-   `/games` : Affiche la liste des parties en cours.
-   `/observe <id>` : Observe une partie en cours.

### Gestion des Amis et Biographies

-   `/addfriend <pseudo>` : Ajoute un joueur à votre liste d'amis.
-   `/removefriend <pseudo>` : Retire un joueur de votre liste d'amis.
-   `/friends` : Affiche votre liste d'amis.
-   `/writebio <texte>` : Définit votre biographie.
-   `/readbio <pseudo>` : Consulte la biographie d'un joueur.

## Structure Générale du Projet

-   **`Client/`** : Contient les fichiers sources pour le client :

    -   `client.c` : Implémentation du client.
    -   `client.h` : Déclarations et constantes spécifiques au client.

-   **`Serveur/`** : Contient les fichiers sources pour le serveur :

    -   `server.c` : Implémentation du serveur.
    -   `server.h` : Déclarations et constantes spécifiques au serveur.

-   **`awale.c` et `awale.h`** : Contiennent la logique du jeu (plateau, règles,
    etc.).
-   **`game.c` et `game.h`** : Gestion des parties, des joueurs, et de
    l'interaction client-serveur.
-   **`3rdparty/cJSON/`** : Bibliothèque externe pour manipuler les fichiers
    JSON (ex : sauvegarde des parties, liste d'amis).
-   **`Makefile`** : Automatisation de la compilation.
-   **`bin/`** : Contient les fichiers exécutables générés (`server_exec` et
    `client_exec`).

## Licence

Ce projet est sous licence **MIT**. Vous êtes libre de l'utiliser, le modifier
et le redistribuer, à condition de mentionner l'auteur original.

## Notes

-   En cas de problème, vérifiez les journaux ou assurez-vous que les
    dépendances sont correctement configurées.
-   Pour toute question ou amélioration, veuillez contacter l'un des auteurs :
    **simon.perret@insa-lyon.fr** || **jassir.habba@insa-lyon.fr**
