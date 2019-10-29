/*
** superFood.c
**
** Written by Arda Akgur
*/

#include "position.h"
#include "food.h"
#include "snake.h"
#include "board.h"
#include "superFood.h"
#include "tron.h"
#include "rat.h"

// instance variables
static PosnType superFoodPosition;
static uint8_t superFood;



/* Returns 1 if there is superfood at the given position
** iff superFood is active in the game, 0 otherwise.
*/
uint8_t is_super_food_at(PosnType posn) {
    if (superFood && superFoodPosition == posn) {
        return 1;
    }
	return 0;
}

/* Returns 0 if there is no superfood in game, 1 otherwise.
*/
uint8_t is_there_super_food(void) {
    return superFood;
}

/* Flips the superFood
*/
void reverse_super_food(void) {
    if (superFood == 0) {
        superFood = 1;
    } else {
        superFood = 0;
    }
}

/* Add a super food item and return the position - or 
** INVALID_POSITION if we can't. it uses the skeleton
** of add_food_item() method
*/
PosnType add_super_food_item(void) {

	/* Generate "random" positions until we get one which
	** is not occupied by a snake or food.
	*/
	int8_t x, y, attempts;
	PosnType test_position;
	x = 0;
	y = 0;
	attempts = 0;
	do {
		// Generate a new position - this is based on a sequence rather
		// then being random
        x = (x+3+attempts)%BOARD_WIDTH;
        y = (y+5)%BOARD_HEIGHT;
		test_position = position(x,y);
        attempts++;
    } while(attempts < 100 && 
                (is_tron_at(test_position) || is_rat_at(test_position) || 
				is_snake_at(test_position) || is_food_at(test_position)));
        
    if(attempts >= 100) {
        /* We tried 100 times to generate a position
        ** but they were all occupied.
        */
        return INVALID_POSITION;
    }
	
	// If we get here, we've found an unoccupied position (test_position)
	// Add it to our list, display it, and return its ID.
	
	superFoodPosition = test_position;
	reverse_super_food();
	return test_position;
}

/* Returns the current position of the SuperFood
*/
PosnType get_position_of_super_food(void) {
	return superFoodPosition;
}
