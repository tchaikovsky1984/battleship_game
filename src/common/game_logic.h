#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "common.h" // Include common definitions

// Function declarations for game logic
void init_board(PlayerBoard *board);

int can_place_ship(const PlayerBoard *board, const Ship *ship);

void place_ship(PlayerBoard *board, const Ship *ship);

int take_shot(PlayerBoard *board, int row, int col, int *is_hit, int *is_sunk);

int check_game_over(const PlayerBoard *board);

int get_ship_size(ShipType type); // Helper to get ship size based on type

#endif // GAME_LOGIC_H
