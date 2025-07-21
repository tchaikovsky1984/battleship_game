#Compiler Config =============================

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Isrc/common
LDFLAGS_CLIENT = -lncurses
LDFLAGS_SERVER =

#Directories =================================

BIN_DIR = bin
SRC_DIR = src
COMMON_DIR = $(SRC_DIR)/common
SERVER_DIR = $(SRC_DIR)/server
CLIENT_DIR = $(SRC_DIR)/client

#Source ======================================

GAME_LOGIC_SRC = $(COMMON_DIR)/game_logic.c
SERVER_SRC = $(SERVER_DIR)/server.c
CLIENT_SRC = $(CLIENT_DIR)/client.c

#Binaries/Executables ========================

GAME_LOGIC_BIN = $(BIN_DIR)/game_logic.o
SERVER_BIN = $(BIN_DIR)/server.o
CLIENT_BIN = $(BIN_DIR)/client.o

SERVER_EX = $(BIN_DIR)/server
CLIENT_EX = $(BIN_DIR)/client

.PHONY: all clean run-server run-client

#Rules =======================================

#Default target: run server & client
all: $(SERVER_EX) $(CLIENT_EX)

$(SERVER_EX): $(SERVER_BIN) $(GAME_LOGIC_BIN)
	$(CC) $(SERVER_BIN) $(GAME_LOGIC_BIN) -o $@ $(LDFLAGS_SERVER)

$(CLIENT_EX): $(CLIENT_BIN) $(GAME_LOGIC_BIN)
	$(CC) $(CLIENT_BIN) $(GAME_LOGIC_BIN) -o $@ $(LDFLAGS_CLIENT)

$(SERVER_BIN): $(SERVER_SRC) $(COMMON_DIR)/common.h $(COMMON_DIR)/game_logic.h
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(CLIENT_BIN): $(CLIENT_SRC) $(COMMON_DIR)/common.h $(COMMON_DIR)/game_logic.h
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(GAME_LOGIC_BIN): $(GAME_LOGIC_SRC) $(COMMON_DIR)/common.h $(COMMON_DIR)/game_logic.h 
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean the compiled files
clean: 
	rm -f $(BIN_DIR)/*.o $(BIN_DIR)/server $(BIN_DIR)/client

# Run the server
run-server: $(SERVER_EX)
	$(SERVER_EX)

# Run the client
run-client: $(CLIENT_EX)
	$(CLIENT_EX)
