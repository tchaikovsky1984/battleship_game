#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ncurses.h> // Include ncurses library

#include "../common/common.h"
#include "../common/game_logic.h" 

void draw_board(WINDOW *win, const PlayerBoard *board, int offset_y, int offset_x, int show_ships, int cursor_x, int cursor_y){
  wclear(win);
  box(win, 0, 0);

  mvwprintw(win, offset_y, offset_x + 2, "A B C D E F G H I J");

  for(int r = 0; r < BOARD_ROWS; r++){
    mvwprintw(win, offset_y + r + 1, offset_x, "%d ", r); // Print row numbers
    for(int c = 0; c < BOARD_COLS; c++){
      char cell_char = '?'; // Default for fog of war
      int color_pair = 0;

      if(show_ships){ // Display own board fully
        switch (board->grid[r][c]) {
          case WATER: cell_char = '~'; color_pair = 1; break; // Blue for water
          case SHIP:  cell_char = '#'; color_pair = 2; break; // Grey for ship
          case HIT:   cell_char = 'X'; color_pair = 3; break; // Red for hit
          case MISS:  cell_char = 'O'; color_pair = 4; break; // White for miss
        }
      } 
      else{ // Opponent's board (fog of war)
        switch (board->grid[r][c]) {
          case WATER:
          case SHIP:  cell_char = '~'; color_pair = 1; break; // Water or undiscovered ship
          case HIT:   cell_char = 'X'; color_pair = 3; break; // Red for hit
          case MISS:  cell_char = 'O'; color_pair = 4; break; // White for miss
        }
      }

      wattron(win, COLOR_PAIR(color_pair));
      mvwaddch(win, offset_y + r + 1, offset_x + c * 2 + 2, cell_char); // +2 for row num and space
      wattroff(win, COLOR_PAIR(color_pair));

      // Draw cursor
      if (r == cursor_y && c == cursor_x) {
        wattron(win, A_REVERSE); // Reverse colors for cursor
        mvwaddch(win, offset_y + r + 1, offset_x + c * 2 + 2, cell_char);
        wattroff(win, A_REVERSE);
      }
    }
  }
  wrefresh(win);
}

// Function to display messages
void display_message(WINDOW *win, const char *msg) {
  wclear(win);
  box(win, 0, 0);
  mvwprintw(win, 1, 1, "%s", msg);
  wrefresh(win);
}

int main(int argc, char *argv[]){
  int client_sock;
  struct sockaddr_in server_addr;
  char *server_ip;

  PlayerBoard my_board;        // My ships
  PlayerBoard opponent_board; // My view of opponent's board (fog of war)

    // ncurses variables
  WINDOW *my_board_win, *opponent_board_win, *message_win;
  int my_cursor_y = 0, my_cursor_x = 0; // For placement cursor
  int op_cursor_y = 0, op_cursor_x = 0; // For shooting cursor

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
    return EXIT_FAILURE;
  }
  server_ip = argv[1];

  // --- Ncurses Initialization ---
  initscr();             // Start ncurses mode
  cbreak();              // Line buffering disabled, pass character immediately
  noecho();              // Don't echo input characters
  keypad(stdscr, TRUE);  // Enable special keys (like arrow keys)
  curs_set(0);           // Hide the cursor

  if (has_colors()) {
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_CYAN);   // Water
    init_pair(2, COLOR_WHITE, COLOR_BLACK); // Ship
    init_pair(3, COLOR_RED, COLOR_BLACK);   // Hit
    init_pair(4, COLOR_WHITE, COLOR_BLACK); // Miss
    init_pair(5, COLOR_YELLOW, COLOR_BLACK); // Cursor/Highlight
  }

  my_board_win = newwin(BOARD_ROWS + 3, BOARD_COLS * 2 + 4, 0, 0);
  opponent_board_win = newwin(BOARD_ROWS + 3, BOARD_COLS * 2 + 4, 0, BOARD_COLS * 2 + 5);
  message_win = newwin(5, (BOARD_COLS * 2 + 4) * 2 + 1, BOARD_ROWS + 4, 0);

  display_message(message_win, "Connecting to server...");
  refresh();

  // creating socket
  client_sock = socket(AF_INET, SOCK_STREAM, 0);
  if(client_sock < 0){
    perror("Socker creation failed");
    endwin();
    exit(EXIT_FAILURE);
  }

  // prepare socketaddr_in structure
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
    fprintf(stderr, "Invalid address/ Address not supported\n");
    close(client_sock);
    endwin();
    exit(EXIT_FAILURE);
  }

  // connect to the server
  if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Connection failed");
    close(client_sock);
    endwin();
    exit(EXIT_FAILURE);
  }

  display_message(message_win, "Connected to server. Waiting for game to start...");

  // Initialize both boards locally
  init_board(&my_board);
  init_board(&opponent_board);

  GamePhase current_client_phase = GAME_PHASE_PLACEMENT;
  GameMessage received_msg;
  GameMessage send_msg;
  int ch; // For keyboard input

    // Placement phase specific variables
  ShipType current_ship_to_place_type = -1; // -1 indicates not yet prompted for a ship
  Orientation current_placement_orientation = HORIZONTAL;
  Ship temp_ship_for_placement; // Temporary ship object to hold current placement attempt

  while(current_client_phase != GAME_PHASE_GAME_OVER){
    if(current_client_phase == GAME_PHASE_PLACEMENT){
      draw_board(my_board_win, &my_board, 0, 0, true, my_cursor_y, my_cursor_x);
      draw_board(opponent_board_win, &opponent_board, 0, 0, false, -1, -1)
    }
    else{
      // GAME PHASE shooting
      draw_board(my_board_win, &my_board, 0, 0, true, -1, -1); // No cursor on own board during shooting
      draw_board(opponent_board_win, &opponent_board, 0, 0, false, op_cursor_y, op_cursor_x);
    }
    doupdate();

    nodelay(stdscr, TRUE);

    if(current_client_phase == GAME_PHASE_PLACEMENT){
      ch = getch();
      if(ch != ERR){
        switch(ch){
          case 'w':
          case 'W': if(my_cursor_y > 0) my_cursor_y--; break;
          case 'a':
          case 'A': if(my_cursor_x > 0) my_cursor_x--; break;
          case 's':
          case 'S': if(my_cursor_y < BOARD_ROWS - 1) my_cursor_y++; break;
          case 'd':
          case 'D': if(my_cursor_x < BOARD_COLS - 1) my_cursor_x++; break;
          case 'r':
          case 'R': current_placement_orientation = (current_placement_orientation == HORIZONTAL) ? VERTICAL : HORIZONTAL;
                    display_message(message_win, (current_placement_orientation == HORIZONTAL) ? "Orientation: HORIZONTAL (Press R to rotate)" : "Orientation: VERTICAL (Press R to rotate)"); break;
          case 10:
                    if(current_ship_to_place_type != -1){
                      temp_ship_for_placement = (Ship){
                        .type = current_ship_to_place_type,
                        .size = get_ship_size(current_ship_to_place_type),
                        .row = my_cursor_y,
                        .col = my_cursor_x,
                        .orientation = current_placement_orientation,
                        .hits = 0,
                        .is_placed = 0
                      };
                      if(can_place_ship(&my_board, &temp_ship_for_placement)){
                        send_msg.type = MSG_TYPE_PLACEMENT_REQ;
                        send_msg.row = my_cursor_y;
                        send_msg.col = my_cursor_x;
                        send_msg.ship_type = current_ship_to_place_type;
                        send_msg.orientation = current_placement_orientation;
                        sprintf(send_msg.message_data, "Placement attempt for %s at (%d,%d) %s.", (current_ship_to_place_type == CARRIER) ? "Carrier" : (current_ship_to_place_type == BATTLESHIP) ? "Battleship" : (current_ship_to_place_type == CRUISER) ? "Cruiser" : (current_ship_to_place_type == SUBMARINE) ? "Submarine" : "Destroyer", my_cursor_y, my_cursor_x, (current_placement_orientation == HORIZONTAL) ? "Horizontal" : "Vertical");
                        send(client_sock, &send_msg, sizeof(send_msg), 0);
                        display_message(message_win, "Sending placement request to server...");
                        nodelay(stdscr, FALSE); // Make getch() blocking again while waiting for server response
                       }
                      else{
                        display_message(message_win, "Invalid placement (local check). Overlaps or out of bounds. Try again.");
                      }
                    }
                    else{
                      display_message(message_win, "Server hasn't prompted for a ship yet. Waiting... ");
                    }
                    break;
        }
      }
    }
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000;

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(client_sock, &read_fds);

    int select_result = select(client_sock + 1, &read_fds, NULL, NULL, &tv);

    if(select_result > 0 && FD_ISSET(client_sock, &read_fds)){
      bytes_received = recv(client_sock, &received_msg, sizeof(received_msg), 0);
      if (bytes_received <= 0) {
        display_message(message_win, "Server disconnected or error.");
        current_client_phase = GAME_PHASE_GAME_OVER;
        continue;
      }

      switch (received_msg.type) {
        case MSG_TYPE_TEST:
                            display_message(message_win, received_msg.message_data);
                            break;

        case MSG_TYPE_PLACE_SHIP_PROMPT:
                            current_ship_to_place_type = received_msg.ship_type;
                            display_message(message_win, received_msg.message_data);
                            // Reset cursor for new placement
                            my_cursor_y = 0;
                            my_cursor_x = 0;
                            current_placement_orientation = HORIZONTAL; // Default orientation for new placement
                            nodelay(stdscr, TRUE); // Re-enable non-blocking input for placement
                            break;

        case MSG_TYPE_PLACEMENT_RES:
                          nodelay(stdscr, TRUE); // Ensure input is non-blocking again after server response
                          if (received_msg.success) {
                            // Update local board
                            temp_ship_for_placement.row = received_msg.row;
                            temp_ship_for_placement.col = received_msg.col;
                            temp_ship_for_placement.orientation = received_msg.orientation;
                            temp_ship_for_placement.type = received_msg.ship_type;
                            temp_ship_for_placement.size = get_ship_size(received_msg.ship_type);
                            place_ship(&my_board, &temp_ship_for_placement);

                            display_message(message_win, received_msg.message_data);

                            // Check if all ships are placed locally
                            bool all_ships_placed_locally = true;
                            for (int i = 0; i < NUM_SHIPS; i++) {
                              if (!my_board.ships[i].is_placed) {
                                all_ships_placed_locally = false;
                                break;
                              }
                            }
                            if (all_ships_placed_locally) {
                              current_client_phase = GAME_PHASE_SHOOTING;
                              display_message(message_win, "All your ships are placed! Waiting for opponent...");
                              // Reset op_cursor for shooting
                              op_cursor_y = 0;
                              op_cursor_x = 0;
                            }
                          } 
                          else {
                            display_message(message_win, received_msg.message_data);
                            // Stay in placement phase for the same ship type.
                          }
                          current_ship_to_place_type = -1; // Reset so we don't accidentally reuse old prompt
                          break;

        case MSG_TYPE_TURN_IND:
                          current_client_phase = GAME_PHASE_SHOOTING; // Confirm shooting phase
                          display_message(message_win, "YOUR TURN! Use WASD to aim, ENTER to fire."); // Updated message
                          nodelay(stdscr, FALSE); // Make getch() blocking again for turn
                          // --- Input loop for shooting ---
                          bool shot_fired = false;
                          while (!shot_fired) {
                            draw_board(my_board_win, &my_board, 0, 0, true, -1, -1);
                            draw_board(opponent_board_win, &opponent_board, 0, 0, false, op_cursor_y, op_cursor_x);
                            doupdate();

                            ch = getch();
                            switch (ch) {
                              case 'w': // WASD controls for shooting
                              case 'W': if (op_cursor_y > 0) op_cursor_y--; break;
                              case 's':
                              case 'S': if (op_cursor_y < BOARD_ROWS - 1) op_cursor_y++; break;
                              case 'a':
                              case 'A': if (op_cursor_x > 0) op_cursor_x--; break;
                              case 'd':
                              case 'D': if (op_cursor_x < BOARD_COLS - 1) op_cursor_x++; break;
                              case 10: // Enter key
                                        send_msg.type = MSG_TYPE_SHOT_REQ;
                                        send_msg.row = op_cursor_y;
                                        send_msg.col = op_cursor_x;
                                        sprintf(send_msg.message_data, "Shot at %c%d", 'A'+op_cursor_x, op_cursor_y);
                                        send(client_sock, &send_msg, sizeof(send_msg), 0);
                                        shot_fired = true;
                                        break;
                            }
                          }
                          nodelay(stdscr, TRUE); // After shot, make getch() non-blocking again
                          break; // End of MSG_TYPE_TURN_IND block

        case MSG_TYPE_SHOT_RES:
                        if (strcmp(received_msg.message_data, "HIT!") == 0 || strcmp(received_msg.message_data, "HIT and SUNK!") == 0 || strstr(received_msg.message_data, "was HIT") != NULL) {
                          if (strstr(received_msg.message_data, "Your ship at") != NULL) {
                            my_board.grid[received_msg.row][received_msg.col] = HIT;
                          } 
                          else {
                            opponent_board.grid[received_msg.row][received_msg.col] = HIT;
                          }
                         } 
                        else {
                          if (strstr(received_msg.message_data, "Opponent MISSED your board at") != NULL) {
                            my_board.grid[received_msg.row][received_msg.col] = MISS;
                          } 
                          else {
                            opponent_board.grid[received_msg.row][received_msg.col] = MISS;
                          }
                        }
                        display_message(message_win, received_msg.message_data);
                        nodelay(stdscr, TRUE);
                        break;

        case MSG_TYPE_GAME_OVER:
                        display_message(message_win, received_msg.message_data);
                        current_client_phase = GAME_PHASE_GAME_OVER;
                        nodelay(stdscr, FALSE);
                        getch();
                        break;

        default:
                        display_message(message_win, received_msg.message_data);
                        nodelay(stdscr, TRUE);
                        break;
      }
    } 
    else if (select_result == -1) {
      perror("select error");
      display_message(message_win, "Network error. Disconnecting.");
      current_client_phase = GAME_PHASE_GAME_OVER;
    }
  } // end while (current_client_phase != GAME_PHASE_GAME_OVER)
    end_game_loop:;
    // --- Cleanup ---
    close(client_sock);
    cleanup_ncurses();
    return 0;
}
