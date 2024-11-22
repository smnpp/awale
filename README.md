# Utilisation

## Démarrer le Serveur

1. Lancez le serveur :

    ```bash
    ./bin/server_exec
    ```

2. Le serveur écoute sur le port **5788**. Il est maintenant prêt à accepter les
   connexions.

## Lancer un Client

1. Lancez un client en spécifiant l'adresse du serveur et votre pseudo :
    ```bash
    ./bin/client_exec <adresse_du_serveur> <pseudo>
    ```
    Exemple :
    ```bash
    ./bin/client_exec 127.0.0.1 Joueur1
    ```

## Commandes Disponibles

### Commandes Générales

-   `/help` : Affiche la liste des commandes.
-   `/list` : Affiche la liste des joueurs connectés.
-   `/msg <pseudo> <message>` : Envoie un message privé à un joueur.
-   `/all <message>` : Envoie un message à tous les joueurs.
-   `/logout` : Se déconnecte du serveur.

### Commandes de Jeu

-   `/play <pseudo>` : Demande à jouer avec un joueur.
-   `/accept` : Accepte une demande de partie.
-   `/decline` : Refuse une demande de partie.
-   `/quit` : Quitte une partie en cours.
-   `/matchmaking` : Rejoint la file d'attente pour jouer avec un adversaire
    aléatoire.
-   `/games` : Affiche les parties en cours.
-   `/observe <id>` : Observe une partie en cours.

### Gestion des Amis et Biographies

-   `/addfriend <pseudo>` : Ajoute un joueur à votre liste d'amis.
-   `/removefriend <pseudo>` : Retire un joueur de votre liste d'amis.
-   `/friends` : Affiche votre liste d'amis.
-   `/writebio <texte>` : Définit votre biographie.
-   `/readbio <pseudo>` : Consulte la biographie d'un joueur.

# Structure des Fichiers

-   **`Client/`** : Contient les fichiers sources pour le client (`client.c`,
    `client.h`).
-   **`Serveur/`** : Contient les fichiers sources pour le serveur (`server.c`,
    `server.h`).
-   **`awale.c` et `awale.h`** : Gestion des règles et du plateau de jeu.
-   **`game.c` et `game.h`** : Gestion des parties et des joueurs.
-   **`3rdparty/cJSON/`** : Bibliothèque externe pour gérer le JSON.
-   **`Makefile`** : Script pour compiler le projet.

# Contributeurs et Notes

-   **Auteur principal** : Simon Perret
-   **Remerciements** : Merci à tous ceux qui ont participé aux tests et aux
    améliorations.

# Notes supplémentaires

-   Les fichiers JSON des parties et des amis sont générés automatiquement dans
    le dossier `data/`. Assurez-vous que ce dossier existe et dispose des
    permissions nécessaires.
-   En cas de problème, consultez les journaux ou vérifiez la configuration
    réseau.
