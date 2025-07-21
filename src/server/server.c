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

  GamePhase current_game_phase = GAME_PHASE_PLACEMENT; // Players place ships initially
  int current_player_turn = 0; // Player 0 starts placement/shooting
  int players_ready_for_shooting = 0; // Count players who finished placement

  // Create socket
  listen_sock = socket(AF_INET, SOCK_STREAM, 0);
  if(listen_sock == 0){
    perror("Socket creation failed.\n");
    exit(EXIT_FAILURE);
  }

  // Allow reuse of addrs
  int optval = 1;
  if(setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0){
    perror("setsockopt failed.\n");
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

  if (listen(listen_sock, MAX_CLIENT) < 0) {
    perror("Listen failed");
    close(listen_sock);
    exit(EXIT_FAILURE);
  }

  printf("Server listening on port %d", PORT);

  // Accept connections from 2 clients
  for(int i = 0; i < MAX_CLIENT; i++){
    printf("Waiting for client %d ...", i);
    client_sock[i] = accept(listen_sock, (struct sockaddr*)&client_addr, &client_len);
    if(client_sock[i] < 0){
      perror("Accept failed");
      close(listen_sock);
      exit(EXIT_FAILURE);
    }
    printf("Player %d connected from %s:%d\n", i+1, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    init_board(&player_boards[i]);

    GameMessage msg;

    msg.type = MSG_TYPE_TEST;
    sprintf(msg.message, "Welcome Player %d", i + 1);
    send(client_sock[i], &msg, sizeof(msg), 0);
  }

  printf("Both players connected.\n");

  GameMessage recieved_msg;
  ssize_t bytes_recieved;

  while(current_game_phase != GAME_PHASE_GAMEOVER){
    int current_socket = client_sock[current_player_turn];
    int opponent_socket = client_sock[(current_player_turn == 1)? 0 : 1];
    PlayerBoard *current_player_board = &player_boards[current_player_turn];
    PlayerBoard *opponent_player_board = &player_boards[(current_player_turn == 1)? 0 : 1];

    // Placement Phase
    if(current_game_phase == GAME_PHASE_PLACEMENT){
      int all_ships_placed_for_current_player = 1;
      for(int i = 0 ; i < NUM_SHIPS; i++){
        if(!current_player_board->ships[i].is_placed){
          all_ships_placed_for_current_player = 0;
          break;
        }
      }
      if(all_ships_placed_for_current_player){
        players_ready_for_shooting++;
        printf("Player %d finished placing ships. Total Ready : %d", current_player_turn + 1, players_ready_for_shooting);

        if(players_ready_for_shooting == MAX_CLIENT){
          current_game_phase = GAME_PHASE_SHOOTING;
          printf("All players placed ships. Transitioning to shooting phase.\n");
          current_player_turn = 0;

          GameMessage game_start_msg;
          game_start_msg.type = MSG_TYPE_TEST;
          sprintf(game_start_msg.message, "All ships placed! Game starting!");
          send(client_sock[0], &game_start_msg, sizeof(game_start_msg), 0);
          send(client_sock[1], &game_start_msg, sizeof(game_start_msg), 0);

          continue;
        }
        else{
          current_player_turn = (current_player_turn == 1)? 0 : 1;
          printf("Switching to Player %d for placement.\n", current_player_turn + 1);
          continue;
        }
      }
      
      // Placing Prompts

      // Find the first ship to be placed
      ShipType ship_to_place_type = -1;
      int ship_size_to_place = 0;
      for(int i = 0 ; i < NUM_SHIPS; i++){
        if(!current_player_board->ships[i].is_placed){
          ship_to_place_type = current_player_board->ships[i].type;
          ship_size_to_place = current_player_board->ships[i].size;
          break; // Found the next ship to place
        }
      }

      if (ship_to_place_type != -1) {
        GameMessage placement_prompt_msg;
        placement_prompt_msg.type = MSG_TYPE_PLACE_SHIP_PROMPT;
        placement_prompt_msg.ship_type = ship_to_place_type;
        sprintf(placement_prompt_msg.message, "Place your %s (size %d).", (ship_to_place_type == CARRIER) ? "Carrier" :(ship_to_place_type == BATTLESHIP) ? "Battleship" : (ship_to_place_type == CRUISER) ? "Cruiser" : (ship_to_place_type == SUBMARINE) ? "Submarine" : "Destroyer", ship_size_to_place);
        send(current_socket, &placement_prompt_msg, sizeof(placement_prompt_msg), 0);
        printf("Sent placement prompt for %s (Player %d).\n", placement_prompt_msg.message, current_player_turn + 1);

        // Wait for response
        bytes_recieved = recv(current_socket, &recieved_msg, sizeof(recieved_msg), 0);
        if (bytes_recieved <= 0) {
          printf("Player %d disconnected during placement or error.\n", current_player_turn + 1);
          current_game_phase = GAME_PHASE_GAMEOVER; // End game if a player disconnects
          continue;
        }

        if (recieved_msg.type == MSG_TYPE_PLACEMENT_REQ) {
          Ship new_ship_placement = {
            .type = recieved_msg.ship_type,
            .row = recieved_msg.row,
            .col = recieved_msg.col,
            .orientation = recieved_msg.orientation,
            .size = get_ship_size(recieved_msg.ship_type), // Get size based on type
            .hits = 0, // Freshly placed
            .is_placed = 0 // Will be set to 1 by place_ship if successful
          };
          GameMessage placement_response_msg;
          placement_response_msg.type = MSG_TYPE_PLACEMENT_RES;
          placement_response_msg.row = recieved_msg.row;
          placement_response_msg.col = recieved_msg.col;
          placement_response_msg.ship_type = recieved_msg.ship_type;
          placement_response_msg.orientation = recieved_msg.orientation;


          // Validate placement
          if (can_place_ship(current_player_board, &new_ship_placement)) {
            // Place ship on server's internal board
            place_ship(current_player_board, &new_ship_placement); // This function will find and update the correct ship instance
            placement_response_msg.success = 1; // Success
            sprintf(placement_response_msg.message, "Ship placed successfully!");
            printf("Player %d placed %s at (%d,%d) %s.\n", current_player_turn + 1, (recieved_msg.ship_type == CARRIER) ? "Carrier": (recieved_msg.ship_type == BATTLESHIP) ? "Battleship" : (recieved_msg.ship_type == CRUISER) ? "Cruiser" : (recieved_msg.ship_type == SUBMARINE) ? "Submarine" : "Destroyer", recieved_msg.row, recieved_msg.col, (recieved_msg.orientation == HORIZONTAL) ? "Horizontal" : "Vertical");
          }
          else{
            placement_response_msg.success = 0; // Failure
            sprintf(placement_response_msg.message, "Invalid placement. Try again.");
            printf("Player %d tried invalid placement for %s.\n", current_player_turn + 1, (recieved_msg.ship_type == CARRIER) ? "Carrier" : (recieved_msg.ship_type == BATTLESHIP) ? "Battleship" : (recieved_msg.ship_type == CRUISER) ? "Cruiser" : (recieved_msg.ship_type == SUBMARINE) ? "Submarine" : "Destroyer");
          }
          send(current_socket, &placement_response_msg, sizeof(placement_response_msg), 0);

          // If placement failed, stay on the same player and prompt for the same ship again.
          // If successful, the loop will naturally move to the next unplaced ship for this player,
          // or switch players/phase as handled above.
        } 
        else {
          printf("Player %d sent unexpected message type %d during placement.\n", current_player_turn + 1, recieved_msg.type); // Could send an error message back, or simply ignore and re-prompt.
        }
      }
    }
    else if(current_game_phase == GAME_PHASE_SHOOTING){
      GameMessage turn_msg;
      turn_msg.type = MSG_TYPE_TURN_IND;
      sprintf(turn_msg.message, "It's your turn.");
      send(current_socket, &turn_msg, sizeof(turn_msg), 0);
      printf("Sent turn message to Player %d\n", current_player_turn + 1);
      // Wait for current player's action (e.g., a shot)
      bytes_recieved = recv(current_socket, &recieved_msg, sizeof(recieved_msg), 0);
      if (bytes_recieved <= 0) {
        printf("Player %d disconnected or error.\n", current_player_turn + 1);
        current_game_phase = GAME_PHASE_GAMEOVER; // End game if a player disconnects
        continue;
      }
      printf("Player %d sent message (Type: %d, Data: %s)\n", current_player_turn + 1, recieved_msg.type, recieved_msg.message);
      if(recieved_msg.type == MSG_TYPE_SHOT_REQ){
        int target_player_idx = (current_player_turn == 0) ? 1 : 0; // Other player
        int is_hit_flag, is_sunk_flag;
        int result_of_shot = take_shot(&player_boards[target_player_idx], recieved_msg.row, recieved_msg.col, &is_hit_flag, &is_sunk_flag);

        GameMessage shot_result_msg_to_shooter;
        shot_result_msg_to_shooter.type = MSG_TYPE_SHOT_RES;
        shot_result_msg_to_shooter.row = recieved_msg.row;
        shot_result_msg_to_shooter.col = recieved_msg.col;

        GameMessage shot_result_msg_to_target; // To inform the target player
        shot_result_msg_to_target.type = MSG_TYPE_SHOT_RES;
        shot_result_msg_to_target.row = recieved_msg.row;
        shot_result_msg_to_target.col = recieved_msg.col;

        if (is_hit_flag) {
          if (is_sunk_flag) {
            sprintf(shot_result_msg_to_shooter.message, "HIT and SUNK! Your shot at (%d,%d) sunk a ship.", recieved_msg.row, recieved_msg.col);
            sprintf(shot_result_msg_to_target.message, "Your ship at (%d,%d) was HIT and SUNK!", recieved_msg.row, recieved_msg.col);
            printf("Player %d hit and sunk a ship on Player %d's board at (%d,%d)\n", current_player_turn + 1, target_player_idx + 1, recieved_msg.row, recieved_msg.col);
          } 
          else {
            sprintf(shot_result_msg_to_shooter.message, "HIT! Your shot at (%d,%d) hit a ship.", recieved_msg.row, recieved_msg.col);
            sprintf(shot_result_msg_to_target.message, "Your ship at (%d,%d) was HIT!", recieved_msg.row, recieved_msg.col);
            printf("Player %d hit Player %d's board at (%d,%d)\n", current_player_turn + 1, target_player_idx + 1, recieved_msg.row, recieved_msg.col);
          }
        }
        else{
          sprintf(shot_result_msg_to_shooter.message, "MISS. Your shot at (%d,%d) hit water.", recieved_msg.row, recieved_msg.col);
          sprintf(shot_result_msg_to_target.message, "Opponent MISSED your board at (%d,%d).", recieved_msg.row, recieved_msg.col);
          printf("Player %d missed Player %d's board at (%d,%d)\n", current_player_turn + 1, target_player_idx + 1, recieved_msg.row, recieved_msg.col);
        }
        send(current_socket, &shot_result_msg_to_shooter, sizeof(shot_result_msg_to_shooter), 0);
        send(opponent_socket, &shot_result_msg_to_target, sizeof(shot_result_msg_to_target), 0);
        if(check_game_over(&player_boards[target_player_idx])){
          GameMessage game_over_msg;
          game_over_msg.type = MSG_TYPE_GAME_OVER;
          sprintf(game_over_msg.message, "GAME OVER! Player %d wins!", current_player_turn + 1);
          send(current_socket, &game_over_msg, sizeof(game_over_msg), 0); // Winner
          sprintf(game_over_msg.message, "GAME OVER! Player %d loses! Player %d wins!", target_player_idx + 1, current_player_turn + 1);
          send(opponent_socket, &game_over_msg, sizeof(game_over_msg), 0); // Loser
          printf("Game Over! Player %d wins.\n", current_player_turn + 1);
          current_game_phase = GAME_PHASE_GAMEOVER; // Set phase to exit loop
        }
        else{
          current_player_turn = (current_player_turn == 0) ? 1 : 0; // swtich turns
        }
      }
      else{
        printf("Player %d sent unexpected message type %d during shooting phase.\n", current_player_turn + 1, recieved_msg.type);
        // Could send a message back or something.
      }
    }
  }
  close(client_sock[0]);
  close(client_sock[1]);
  close(listen_sock);

  printf("Server Shutting Down.\n");
  return 0;
}
