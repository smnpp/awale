# Répertoire de build
BUILD_DIR = bin
CLIENT_BUILD_DIR = $(BUILD_DIR)/Client
SERVER_BUILD_DIR = $(BUILD_DIR)/Serveur

# Noms des exécutables
CLIENT_EXEC = $(BUILD_DIR)/client_exec
SERVER_EXEC = $(BUILD_DIR)/server_exec

# Compilateur et options de compilation
CC = gcc
CFLAGS = -Wall -g

# Fichiers sources et objets pour le client et le serveur
CLIENT_SRC = Client/client.c awale.c game.c
SERVER_SRC = Serveur/server.c awale.c game.c
CLIENT_OBJ = $(CLIENT_BUILD_DIR)/client.o $(BUILD_DIR)/awale.o $(BUILD_DIR)/game.o 
SERVER_OBJ = $(SERVER_BUILD_DIR)/server.o $(BUILD_DIR)/awale.o $(BUILD_DIR)/game.o 

# Cible par défaut pour compiler le client et le serveur
all: $(BUILD_DIR) $(CLIENT_BUILD_DIR) $(SERVER_BUILD_DIR) $(CLIENT_EXEC) $(SERVER_EXEC)

# Créer le répertoire de build
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Créer le répertoire de build pour le client
$(CLIENT_BUILD_DIR):
	mkdir -p $(CLIENT_BUILD_DIR)

# Créer le répertoire de build pour le serveur
$(SERVER_BUILD_DIR):
	mkdir -p $(SERVER_BUILD_DIR)

# Compilation du client
$(CLIENT_EXEC): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_OBJ)

# Compilation du serveur
$(SERVER_EXEC): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $(SERVER_OBJ)

# Règle pour compiler client.o dans le répertoire build/Client
$(CLIENT_BUILD_DIR)/client.o: Client/client.c Client/client.h awale.h game.h | $(CLIENT_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Règle pour compiler server.o dans le répertoire build/Serveur
$(SERVER_BUILD_DIR)/server.o: Serveur/server.c Serveur/server.h awale.h game.h | $(SERVER_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Règle pour compiler awale.o dans le répertoire build
$(BUILD_DIR)/awale.o: awale.c awale.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Règle pour compiler game.o dans le répertoire build
$(BUILD_DIR)/game.o: game.c game.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage des fichiers objets et exécutables
clean:
	rm -rf $(BUILD_DIR)