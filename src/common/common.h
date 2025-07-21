#ifndef COMMON_H
#define COMMON_H

#define PORT 8080
#define MAX_MSG_LEN 1024
#define BOARD_COLS 10
#define BOARD_ROWS 10
#define NUM_SHIPS 5

typedef enum {
  WATER,
  SHIP,
  HIT,
  MISS 
} CellState;

typedef enum {
  CARRIER,        // Size 5 
  BATTLESHIP,     // Size 4
  CRUISER,        // Size 3
  SUBMARINE,      // Size 3
  DESTROYER       // Size 2
} ShipType;

typedef enum {
  HORIZONTAL,
  VERTICAL
} Orientation;

typedef enum {
  GAME_PHASE_PLACEMENT,
  GAME_PHASE_SHOOTING,
  GAME_PHASE_GAMEOVER 
} GamePhase;

typedef struct{
  ShipType type;
  int size;
  int row;
  int col;
  Orientation orientation;
  int hits;
  int is_placed;
} Ship;

typedef struct{
  CellState grid[BOARD_ROWS][BOARD_COLS];
  Ship ships[NUM_SHIPS];
  int ships_remaining;
} PlayerBoard;

typedef struct{
  int type;
  int row;
  int col;
  char message[MAX_MSG_LEN];
} GameMessage;

// Message types (example)
#define MSG_TYPE_TEST 0
#define MSG_TYPE_PLACEMENT_REQ 1
#define MSG_TYPE_PLACEMENT_RES 2
#define MSG_TYPE_SHOT_REQ 3
#define MSG_TYPE_SHOT_RES 4
#define MSG_TYPE_TURN_IND 5 // Turn indication
#define MSG_TYPE_GAME_OVER 6
#define MSG_TYPE_PLACE_SHIP_PROMPT 7 

#endif // !COMMON_H
