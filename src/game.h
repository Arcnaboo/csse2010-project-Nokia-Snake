/*
 * game.h
 *
 * Written by Peter Sutton
 */ 

#ifndef GAME_H_
#define GAME_H_

#include <inttypes.h>

uint32_t get_super_food_timer(void);
void set_super_food_timer(uint32_t time);
float get_game_speed(void);
void reset_game_speed(void);

// Initialise game. This initialises the board with snake and food items
// and initialises the display.
void init_game(void);

// Attempt to move snake forward. If food is eaten it removes it, grows
// the snake if possible and replaces the food item with a new one.
// Display is updated as required. Returns true if successful, 
// false otherwise (move off board if not permitted, or snake collides
// with self). (Moves off board and collisions permitted in initially 
// supplied code.)
int8_t attempt_to_move_snake_forward(void);

#endif /* GAME_H_ */