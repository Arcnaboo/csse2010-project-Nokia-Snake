/*
** rat.c
** Provides functionality of rat item in game
** Written by Arda Akgur
*/

#include <stdlib.h>
#include "position.h"
#include "food.h"
#include "snake.h"
#include "board.h"
#include "superFood.h"
#include "rat.h"
#include "timer0.h"
#include "tron.h"

// current position of Rat
static PosnType ratPosition;



// sets new position for rat
void set_rat_pos(PosnType posn) {
	ratPosition = posn;
}

// checks if rat is at given pos
uint8_t is_rat_at(PosnType posn) {
    if (ratPosition == posn) {
        return 1;
    }
    return 0;
}

// returns the pos of rat
PosnType get_position_of_rat(void) {
    return ratPosition;
}

/* Add a rat item and return the position - or 
** INVALID_POSITION if we can't. it uses the skeleton
** of add_food_item() method
*/
PosnType add_rat_item(void) {

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
                (is_tron_at(test_position) || is_super_food_at(test_position) || is_snake_at(test_position) || is_food_at(test_position)));
        
    if(attempts >= 100) {
        /* We tried 100 times to generate a position
        ** but they were all occupied.
        */
        return INVALID_POSITION;
    }
	
	// If we get here, we've found an unoccupied position (test_position)
	// Add it to our list, display it, and return its ID.
	
	ratPosition = test_position;
	return test_position;
}

//attempts to step the rat randomly
PosnType step_rat(void) {
    uint8_t x = x_position(ratPosition);
    uint8_t y = y_position(ratPosition);
    PosnType test_position = ratPosition;
    int8_t attempts = 0;
    do {
        switch (random() % 4) {
            case 0:
                test_position = position(x, y + 1);
                break;
            case 1:
                test_position = position(x + 1, y);
                break;
            case 2:
                test_position = position(x, y - 1);
                break;
            case 3:
                test_position = position(x - 1, y);
                break;   
        }
        attempts++;
		
    } while (attempts < 100 &&
                (is_food_at(test_position) || is_snake_at(test_position) || is_super_food_at(test_position)));
    
    if (attempts >= 100) {
        return INVALID_POSITION;
    }
    return test_position;
}