#include <stdio.h>
#include <string.h>
#include "game_logic.h"

void init_board(PlayerBoard *board){
  if(board == NULL)
    return;
  for(int r = 0; r < BOARD_ROWS; r++){
    for(int c = 0; c < BOARD_COLS; c++)
      board->grid[r][c] = WATER;
  }

  board->ships_remaining = NUM_SHIPS; // Initially all ships are unsunk
  // Initialize ship details (sizes, types)
  board->ships[0] = (Ship){CARRIER, 5, -1, -1, HORIZONTAL, 0, 0}; // -1 for not placed
  board->ships[1] = (Ship){BATTLESHIP, 4, -1, -1, HORIZONTAL, 0, 0};
  board->ships[2] = (Ship){CRUISER, 3, -1, -1, HORIZONTAL, 0, 0};
  board->ships[3] = (Ship){SUBMARINE, 3, -1, -1, HORIZONTAL, 0, 0};
  board->ships[4] = (Ship){DESTROYER, 2, -1, -1, HORIZONTAL, 0, 0};

  return;
}

int get_ship_size(ShipType type){
  switch (type) {
    case CARRIER: return 5;
    case BATTLESHIP: return 4;
    case CRUISER: return 3; 
    case SUBMARINE: return 3;
    case DESTROYER: return 2;
    default: return 0; // never happening 
  }
}

int can_place_ship(const PlayerBoard *board, const Ship *ship){
  if(board == NULL || ship == NULL)
    return 0; 
  if(ship->orientation == HORIZONTAL){
    if(ship->col < 0 || ship->col + ship->size > BOARD_COLS || ship->rows < 0 || ship->rows >= BOARD_ROWS)
      return 0;
  }
  else{
    if(ship->col < 0 || ship->col >= BOARD_COLS || ship->rows < 0 || ship->rows + ship->size > BOARD_ROWS)
      return 0;
  }
  for(int i = 0; i < ship->size; i++){
    int r = ship->row + (ship->orientation == HORIZONTAL ? i : 0);
    int c = ship->col + (ship->orientation == VERTICAL ? i : 0);
    if(board->grid[r][c] == SHIP)
      return 0;
  }
  return 1; 
}

void place_ship(PlayerBoard *board, const Ship *ship_to_place) {
    if (board == NULL || ship_to_place == NULL) return;

    // FIND the specific unplaced ship in the board->ships array that matches ship_to_place->type
    // This is the core logic that was missing.
    Ship *target_ship = NULL;
    for (int i = 0; i < NUM_SHIPS; i++) {
        if (board->ships[i].type == ship_to_place->type && !board->ships[i].is_placed) {
            target_ship = &board->ships[i];
            break; // Found an unplaced ship of this type
        }
    }

    if (target_ship == NULL) {
        // Error: No unplaced ship of this type found, or all already placed
        fprintf(stderr, "Error: No unplaced ship of type %d available.\n", ship_to_place->type);
        return;
    }

    // Now update the found ship's details
    target_ship->row = ship_to_place->row;
    target_ship->col = ship_to_place->col; 
    target_ship->orientation = ship_to_place->orientation;
    target_ship->is_placed = true; // Mark it as placed

    // Apply to the grid
    for (int i = 0; i < ship_to_place->size; i++) {
        int r = ship_to_place->row + (ship_to_place->orientation == VERTICAL ? i : 0);
        int c = ship_to_place->col + (ship_to_place->orientation == HORIZONTAL ? i : 0);
        board->grid[r][c] = SHIP;
    }
}

