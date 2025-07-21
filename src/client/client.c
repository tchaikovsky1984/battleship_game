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

}
