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
