# Manuel d'utilisation

Bienvenue dans le manuel utilisateur de l'application **Awale Multiplayer
Game**. Ce programme permet de jouer au célèbre jeu Awalé en mode multijoueur
avec des fonctionnalités d'observation, de gestion d'amis, et de
personnalisation.

## Fonctionnalités principales

### Fonctionnalités du serveur

-   **Gestion des connexions clients** : accepte les connexions multiples.
-   **Gestion des parties** : crée, suit et termine les parties entre joueurs.
-   **Support des observateurs** : permet aux clients d'observer des parties en
    cours.
-   **Journalisation** : sauvegarde des informations liées aux joueurs et
    parties.
-   **Gestion des inactivités** : déconnecte les clients inactifs.

### Fonctionnalités du client

-   **Rejoindre le serveur** : connexion au serveur pour interagir avec d'autres
    joueurs.
-   **Lister les joueurs connectés** : voir quels joueurs sont disponibles pour
    jouer.
-   **Démarrer une partie** : lancer un défi à un autre joueur.
-   **Observer des parties** : suivre les parties en cours.
-   **Messages privés et globaux** : communiquer avec les autres joueurs.
-   **Gestion d'amis** : ajouter, supprimer et voir la liste des amis.
-   **Biographies** : définir et consulter des biographies des joueurs.
-   **Mode privé/public** : rendre les parties accessibles ou limitées aux amis.

---

## Guide de démarrage

### Installation

1. Clonez ce dépôt :
    ```bash
    git clone <url_du_depot>
    ```
2. Compilez le programme avec `gcc` :
    ```bash
    gcc -o server server.c awale.c game.c client.c -pthread
    gcc -o client client.c -pthread
    ```
3. Préparez les fichiers nécessaires dans un répertoire `data/` :
    - `clients.csv` : liste des joueurs enregistrés.
    - `friends.json` : structure JSON pour les amis et biographies.
    - `games.json` : historique des parties sauvegardées.

---

### Démarrage du serveur

1. Lancez le serveur avec :
    ```bash
    ./server
    ```
2. Le serveur attendra les connexions des clients.

---

### Démarrage d'un client

1. Lancez le client avec :
    ```bash
    ./client
    ```
2. Saisissez votre nom d'utilisateur pour vous connecter ou créer un compte.
3. Utilisez les commandes ci-dessous pour interagir.

---

## Commandes disponibles

### Commandes générales

-   **Afficher les joueurs connectés** :
    ```text
    /list
    ```
-   **Afficher les parties en cours** :
    ```text
    /games
    ```
-   **Aide** :
    ```text
    /help
    ```
-   **Se déconnecter** :
    ```text
    /logout
    ```

### Gestion des parties

-   **Lancer une partie avec un joueur** :
    ```text
    /play <nom_du_joueur>
    ```
-   **Rejoindre la file d'attente** :
    ```text
    /matchmaking
    ```
-   **Quitter une partie** :
    ```text
    /quit
    ```

### Mode d'observation

-   **Observer une partie en cours** :
    ```text
    /observe <id_partie>
    ```
-   **Changer le statut de la partie en privée** :
    ```text
    /private
    ```
-   **Changer le statut de la partie en publique** :
    ```text
    /public
    ```

### Gestion des amis

-   **Ajouter un ami** :
    ```text
    /addfriend <nom>
    ```
-   **Supprimer un ami** :
    ```text
    /removefriend <nom>
    ```
-   **Voir votre liste d'amis** :
    ```text
    /friends
    ```

### Gestion des messages

-   **Envoyer un message privé** :
    ```text
    /msg <nom_du_destinataire> <message>
    ```
-   **Envoyer un message global** :
    ```text
    /all <message>
    ```

### Biographies

-   **Écrire votre biographie** :
    ```text
    /writebio <votre_biographie>
    ```
-   **Lire la biographie d'un joueur** :
    ```text
    /readbio <nom_du_joueur>
    ```

---

## Historique des parties

-   **Afficher l'historique des parties** :
    ```text
    /history
    ```
-   **Regarder une partie spécifique** :
    ```text
    /watch <id_partie>
    ```

---

## Structure technique

### Fichiers principaux

-   **`server.c`** : gère les connexions, la logique des parties et les
    commandes des clients.
-   **`client.c`** : interface pour l'utilisateur final.
-   **`game.c`** : contient la logique de jeu et l'état des parties.
-   **`awale.c`** : implémente les règles spécifiques du jeu Awalé.

### Dépendances

-   **cJSON** : pour la gestion des fichiers JSON.
-   **Sockets** : pour la communication réseau.
-   **Pthreads** : pour gérer la concurrence côté serveur.

---

## Problèmes connus et limitations

1. Le fichier `games.json` doit être géré manuellement pour éviter les
   corruptions.
2. Le système de ping pour l'inactivité peut avoir des faux positifs dans
   certains cas.

---

## Contact

Pour toute question ou amélioration, veuillez contacter l'auteur à :
**simon.perret@insa-lyon.fr** ou bien **jassir.habba@insa-lyon.fr**

Awale Multiplayer Game Introduction

This project implements a multiplayer version of the Awale game. It consists of
a server program that manages games and multiple client programs for players to
interact with the game. The game allows players to join matches, play moves, and
observe ongoing games. Project Structure

The project is organized into the following files and directories: Directories:

    Client/: Contains the client-side code (client.c, client.h).
    Serveur/: Contains the server-side code (server.c, server.h).
    3rdparty/: Contains the cJSON library for JSON handling.
    bin/: The build directory where compiled executables are generated.

Key Files:

    awale.c and awale.h: Contains the core logic for the Awale game (game board, moves, and rules).
    game.c and game.h: Implements game management (creating games, tracking moves, and game states).
    Makefile: Automates the build process for the client and server programs.

Features

    Multiplayer: Supports multiple players and observers.
    Commands: Includes various commands for players, such as /play, /observe, /msg, and /quit.
    Persistence: Saves game history and player information using JSON (via cJSON library).
    Real-time updates: Players and observers receive real-time updates of the game board.

Prerequisites

    A C compiler (e.g., GCC).
    make utility for build automation.
    POSIX-compliant environment for compilation (Linux or macOS).
    Optional: Windows environment requires MinGW for compatibility.

Compilation

Run the following commands to compile the server and client programs:

    Clone the repository and navigate to the project directory.
    Compile the project using the provided Makefile:

    make

    The compiled executables will be available in the bin/ directory:
        bin/client_exec: The client executable.
        bin/server_exec: The server executable.

Running the Programs Start the Server

    Navigate to the bin/ directory.
    Start the server:

    ./server_exec

Start a Client

    Navigate to the bin/ directory.
    Connect a client to the server by specifying the server address and username:

    ./client_exec [server_address] [username]

    Replace [server_address] with the server's IP or hostname and [username] with a unique player name.

Commands (Client-side)

    /list: Show all connected players.
    /play <username>: Request a match with another player.
    /observe <game_id>: Observe an ongoing game.
    /msg <username> <message>: Send a private message to a player.
    /all <message>: Send a message to all connected players.
    /quit: Quit the current game.
    /logout: Disconnect from the server.
    /help: Display the list of available commands.

Cleaning Up

To clean the build files and executables, run:

make clean

Troubleshooting

    Connection issues: Ensure the server is running and reachable from the client machine.
    Compilation errors: Verify that GCC and make are properly installed.
    Windows users: Use MinGW or a similar environment to compile and run the project.

Future Improvements

    Enhance security by adding authentication mechanisms.
    Improve error handling and user feedback.
    Add a graphical user interface (GUI) for a more user-friendly experience.

License

This project is developed for educational purposes and may be adapted or
extended with proper attribution.

Voici une version améliorée et bien structurée de votre `README.md` :

````markdown
# Jeu Awalé Multijoueur

## Introduction

Ce projet implémente une version multijoueur du jeu traditionnel africain
**Awalé**. L'application est divisée en deux parties : un **serveur** pour gérer
les parties et les connexions, et un **client** pour permettre aux joueurs de se
connecter, jouer, et interagir avec le serveur.

Le jeu suit les règles officielles et permet une expérience immersive avec des
fonctionnalités comme l'observation des parties, l'historique des jeux, et un
système de liste d'amis.

---

## Fonctionnalités

### Client

-   Connexion au serveur avec un pseudo.
-   Démarrage et participation à une partie.
-   Envoi de commandes pour interagir avec d'autres joueurs.
-   Système de messagerie privée et publique.
-   Gestion d'une liste d'amis (ajout, suppression, consultation).
-   Écriture et lecture de biographies.
-   Affichage de l'historique des parties jouées.

### Serveur

-   Gestion des connexions multiples (jusqu'à 100 clients).
-   Organisation des parties avec un suivi des scores.
-   Observation des parties en cours.
-   Archivage des parties terminées dans un fichier JSON.
-   Commandes pour administrer le jeu, y compris le matchmaking.

---

## Installation

### Prérequis

-   **Système** : Linux, macOS ou Windows.
-   **Compilateur** : `gcc` (version 4.8 ou plus récente).
-   **Bibliothèques** : `cJSON` (fournie dans le dossier `3rdparty`).

### Compilation

1. Clonez le projet :
    ```bash
    git clone <url_du_projet>
    cd <nom_du_dossier>
    ```
````

2. Compilez le projet :
    ```bash
    make
    ```
    Les exécutables pour le client et le serveur seront créés dans le dossier
    `bin/`.

---

## Utilisation

### Démarrer le Serveur

1. Lancez le serveur :

    ```bash
    ./bin/server_exec
    ```

2. Le serveur écoute sur le port **5788**. Il est maintenant prêt à accepter les
   connexions.

### Lancer un Client

1. Lancez un client en spécifiant l'adresse du serveur et votre pseudo :
    ```bash
    ./bin/client_exec <adresse_du_serveur> <pseudo>
    ```
    Exemple :
    ```bash
    ./bin/client_exec 127.0.0.1 Joueur1
    ```

### Commandes Disponibles

#### Commandes Générales

-   `/help` : Affiche la liste des commandes.
-   `/list` : Affiche la liste des joueurs connectés.
-   `/msg <pseudo> <message>` : Envoie un message privé à un joueur.
-   `/all <message>` : Envoie un message à tous les joueurs.
-   `/logout` : Se déconnecte du serveur.

#### Commandes de Jeu

-   `/play <pseudo>` : Demande à jouer avec un joueur.
-   `/accept` : Accepte une demande de partie.
-   `/decline` : Refuse une demande de partie.
-   `/quit` : Quitte une partie en cours.
-   `/matchmaking` : Rejoint la file d'attente pour jouer avec un adversaire
    aléatoire.
-   `/games` : Affiche les parties en cours.
-   `/observe <id>` : Observe une partie en cours.

#### Gestion des Amis et Biographies

-   `/addfriend <pseudo>` : Ajoute un joueur à votre liste d'amis.
-   `/removefriend <pseudo>` : Retire un joueur de votre liste d'amis.
-   `/friends` : Affiche votre liste d'amis.
-   `/writebio <texte>` : Définit votre biographie.
-   `/readbio <pseudo>` : Consulte la biographie d'un joueur.

---

## Structure des Fichiers

-   **`Client/`** : Contient les fichiers sources pour le client (`client.c`,
    `client.h`).
-   **`Serveur/`** : Contient les fichiers sources pour le serveur (`server.c`,
    `server.h`).
-   **`awale.c` et `awale.h`** : Gestion des règles et du plateau de jeu.
-   **`game.c` et `game.h`** : Gestion des parties et des joueurs.
-   **`3rdparty/cJSON/`** : Bibliothèque externe pour gérer le JSON.
-   **`Makefile`** : Script pour compiler le projet.

---

## Contributeurs et Notes

-   **Auteur principal** : Simon Perret
-   **Remerciements** : Merci à tous ceux qui ont participé aux tests et aux
    améliorations.

---

## Notes supplémentaires

-   Les fichiers JSON des parties et des amis sont générés automatiquement dans
    le dossier `data/`. Assurez-vous que ce dossier existe et dispose des
    permissions nécessaires.
-   En cas de problème, consultez les journaux ou vérifiez la configuration
    réseau.

```

Cette version suit une structure claire et professionnelle. Dites-moi si vous souhaitez personnaliser ou ajouter des sections spécifiques !
```
