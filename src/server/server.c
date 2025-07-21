#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../common/game_logic.h"
#include "../common/common.h"

#define MAX_CLIENT 2

int main(){
  int listen_sock, client_sock[MAX_CLIENT];
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);

  PlayerBoard player_boards[MAX_CLIENT];

  // Create socket
  listen_sock = socket(AF_INET, SOCK_STREAM, 0);
  if(listen_sock == 0){
    perror("Socket creation failed.\n");
    exit(EXIT_FAILURE);
  }

  // Allow reuse of addrs
  int optval = 1;
  if(setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0){
    perorr("setsockopt failed.\n");
    close(listen_sock);
    exit(EXIT_FAILURE);
  }

  // Prepare sockaddr_in structure
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces
  server_addr.sin_port = htons(PORT);     // Host to network short
  
  if(bind(listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
    perror("Bind failed");
    close(listen_sock);
    exit(EXIT_FAILURE);
  }

  if (listen(listen_sock, MAX_CLIENTS) < 0) {
    perror("Listen failed");
    close(listen_sock);
    exit(EXIT_FAILURE);
  }

  printf("Server listening on port %d", PORT);

  // Accept connections from 2 clients
  for(int i = 0; i < MAX_CLIENT; i++){
    printf("Waiting for client %d ...", i);
    client_sock[i] = accept(listen_sock, (struct sockaddr_in*)&client_addr, &client_len);
    if(client_socks[i] < 0){
      perror("Accept failed");
      close(listen_sock);
      exit(EXIT_FAILURE);
    }
    printf("Player %d connected from %s:%d\n", i+1, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    init_board(&player_boards[i]);

    GameMessage msg;

    msg.type = MSG_TYPE_TEST;
    sprintf(msg.message, "Welcome Player %d", i + 1);
    send(client_socks[i], &msg, sizeof(msg), 0);
  }

  print("Both players connected.\n");

  }

}
