/*
** tron.c
**
** Written by Arda Akgur
** pretty much modified version of
** Prof Sutton's Snake.c
**
** Details of the tron and making it move.
*/

#include <stdlib.h>
#include "position.h"
#include "snake.h"
#include "board.h"
#include "food.h"
#include "score.h"
#include "superFood.h"
#include "rat.h"
#include "tron.h"
#include "game.h"
#include "snake.h"
#include "food.h"
#include "pixel_colour.h"
#include "board.h"
#include "ledmatrix.h"
#include "timer0.h"
#include "superFood.h"
#include "rat.h"

//using same inistance variables of snake.c
#define SNAKE_POSITION_ARRAY_SIZE ((MAX_SNAKE_SIZE)+1)

static PosnType snakePositions[SNAKE_POSITION_ARRAY_SIZE];

static uint8_t snakeLength;

static int8_t snakeHeadIndex;
static int8_t snakeTailIndex;

static SnakeDirnType curSnakeDirn;
static SnakeDirnType nextSnakeDirn;

// extra variable boolean Tron mode on off
static uint8_t tron_mode = 0;

// helper method
static void update_display_at_position(PosnType posn, PixelColour colour) {
	ledmatrix_update_pixel(x_position(posn), y_position(posn), colour);
}

// similar copy of snake move function from game.c
static int8_t attempt_to_move_tron_forward(void) {
	PosnType prior_head_position = get_tron_head_position();
	int8_t move_result = advance_tron_head();
	if(move_result < 0) {
		// Snake moved out of bounds (if this is not permitted) or
		// collided it with itself. Return false because we couldn't
		// move the snake
		return 0;
	}
	PosnType new_head_position = get_tron_head_position();
	if (move_result == ATE_RAT || move_result == ATE_RAT_BUT_CANT_GROW) {
		PosnType rat_position = add_rat_item();
		if (is_position_valid(rat_position)) {
			update_display_at_position(rat_position, COLOUR_ARC);
		}
	}
	
	if (move_result == ATE_SUPER_FOOD || move_result == ATE_SUPER_FOOD_BUT_CANT_GROW) {
		if (is_there_super_food()) {
			reverse_super_food();
		}
	}
	if(move_result == ATE_FOOD || move_result == ATE_FOOD_BUT_CANT_GROW) {
		// remove food item
		int8_t foodID = food_at(new_head_position);
		remove_food(foodID);

		// Add a new food item. Might fail if a free position can't be
		// found on the board but shouldn't usually.
		PosnType new_food_posn = add_food_item();
		if(is_position_valid(new_food_posn)) {
			update_display_at_position(new_food_posn, COLOUR_LIGHT_YELLOW);
		}
		
		// We don't remove the eaten food from the display since we'll just
		// display the snake head at that position.
	}
	
	// If we didn't eat food OR if we ate food but the snake is at
	// maximum length, then we move the tail forward and remove this
	// element from the display
	if(move_result == MOVE_OK || move_result == ATE_FOOD_BUT_CANT_GROW || move_result == ATE_SUPER_FOOD_BUT_CANT_GROW) {
		PosnType prev_tail_posn = advance_tron_tail();
		update_display_at_position(prev_tail_posn, COLOUR_BLACK);
	}
	
	// We update the previous head position to become a body part and
	// update the new head position.
	update_display_at_position(prior_head_position, COLOUR_RED);
	update_display_at_position(new_head_position, COLOUR_ARC);
	return 1;
}

// removes tron from the game
void clear_tron(void) {
	uint8_t i;
	for (i = 0; i < MAX_SNAKE_SIZE; i++) {
		if (is_position_valid(snakePositions[i])) {
			update_display_at_position(snakePositions[i], COLOUR_BLACK);
		}
	}
}

// is tron mode active
uint8_t is_tron_mode(void) {
	return tron_mode;
}

// change tron mode
void set_tron_mode(uint8_t mode) {
	tron_mode = mode;
}

// initiate tron snake
void init_tron(void) {

	snakeLength = 2;
	snakeTailIndex = 0;
	snakeHeadIndex = 1;
	snakePositions[0] = position(10,6);
	snakePositions[1] = position(11,6);
	curSnakeDirn = SNAKE_RIGHT;
    nextSnakeDirn = SNAKE_RIGHT;
}

/* get_tron_head_position()
**
** Returns the position of the head of the tron. 
*/
PosnType get_tron_head_position(void) {
    return snakePositions[snakeHeadIndex];
}

/* get_tron_tail_position()
**
** Returns the position of the tail of the tron.
*/
PosnType get_tron_tail_position(void) {
	return snakePositions[snakeTailIndex];
}

/* get_tron_length()
**
** Returns the length of the tron.
*/
uint8_t get_tron_length(void) {
	return snakeLength;
}

/* advance_tron_head()
**
** Attempt to move snake head forward. Returns
** - OUT_OF_BOUNDS if snake has run into the edge and wrap-around is not permitted
**   (in the initial code, wrap-around happens by default) and this value is never returned
** - COLLISION if snake would run into itself
**	 (in the initial code, this is not checked for)
** - SNAKE_LENGTH_ERROR if snake is already too long
** - MOVE_OK if the snake head moves forward but no food was there
** - ATE_FOOD if there was food at the new head position and the snake can grow
** - ATE_FOOD_BUT_CANT_GROW if there was food at the new head position but the
**   snake can't grow.
** (Only the last three of these result in the head position being moved.)
*/
int8_t advance_tron_head(void) {
	int8_t headX;	/* head X position */
	int8_t headY;	/* head Y position */
	PosnType newHeadPosn;
	
	/* Check the snake isn't already too long */
	if(snakeLength > MAX_SNAKE_SIZE) {
		return SNAKE_LENGTH_ERROR;
	}
    
	/* Current head position */
	headX = x_position(snakePositions[snakeHeadIndex]);
	headY = y_position(snakePositions[snakeHeadIndex]);
    
    /* Work out where the new head position should be - we
    ** move 1 position in our NEXT direction of movement if we can.
	** If we're at the edge of the board, then we wrap around to
	** the other edge.
    */
    switch (nextSnakeDirn) {
        case SNAKE_UP:
			if(headY == BOARD_HEIGHT - 1) {
				// Head is already at the top of the board - wrap around
				headY = 0;
			} else {
	            headY += 1;
			}
            break;
        case SNAKE_RIGHT:
			if(headX == BOARD_WIDTH - 1) {
				// Snake head is already at the right hand edge of the board
				// - wrap around to left hand side
				headX = 0;
			} else {
				headX += 1;
			}
            break;
		case SNAKE_LEFT:
            if (headX == 0) {
                headX = BOARD_WIDTH - 1;
            } else {
                headX -= 1;
            }
            break;
        case SNAKE_DOWN:
            if (headY == 0) {
                headY = BOARD_HEIGHT -1;
            } else {
                headY -= 1;
            }
        
    }

	newHeadPosn = position(headX, headY);

	/* Update the current direction */
	curSnakeDirn = nextSnakeDirn;

	/* ADD CODE HERE to check whether the new head position
	** is already occupied by the snake (other than the tail), and if so, return
	** COLLISION. Do not continue. See snake.h for a function which can help you.
	** (If the new head position is the same as the current tail position then
	** this move is permitted - the tail position won't be updated until after the
	** head is advanced but we don't consider this a collision.
	*/
    if (is_tron_at(newHeadPosn) && newHeadPosn != snakePositions[snakeTailIndex]) {
        return COLLISION;
    }
	
	if (is_snake_at(newHeadPosn)) {
		clear_tron();
		set_tron_mode(0);
		add_to_score(100);
		return COLLISION;
	}

    /*
    ** If we get here, the move should be possible.
    ** Advance head by 1. First work out the index
	** of the new head position in the array of snake positions.
	** and whether this has wrapped around in our array of positions
	** or not. Update the length.
    */
	snakeHeadIndex++;
	if(snakeHeadIndex == SNAKE_POSITION_ARRAY_SIZE) {
		/* Array has wrapped around */
		snakeHeadIndex = 0;
	}
	/* Store the head position */
	snakePositions[snakeHeadIndex] = newHeadPosn;
	/* Update the snake's length */
	snakeLength++;
	
	/* Check whether we ate food or not and return the appropriate
	** value.
	*/
	if(is_food_at(newHeadPosn)) {
		if(snakeLength <= MAX_SNAKE_SIZE) {
			return ATE_FOOD;
		} else {
			return ATE_FOOD_BUT_CANT_GROW;
		}
	} else if (is_super_food_at(newHeadPosn)) {
		if (snakeLength <= MAX_SNAKE_SIZE) {
			return ATE_SUPER_FOOD;
		} else {
			return ATE_SUPER_FOOD_BUT_CANT_GROW;
		}	
	} else if (is_rat_at(newHeadPosn)) {
		if (snakeLength <= MAX_SNAKE_SIZE) {
			return ATE_RAT;
			} else {
			return ATE_RAT_BUT_CANT_GROW;
		}
	} else {
		return MOVE_OK;
	}
}

/* 
** advance_tron_tail() 
**
** Attempt to move the snake's tail forward by 1. This
** should always succeed provided it followed a successful
** advance of the head. (We don't check this.) We decrement
** the snake length (to match the increment when the head
** was advanced.)
** No positions need to be updated, we just let the last tail
** position "drop" off the end.
** We return the previous tail position.
*/
PosnType advance_tron_tail(void) {
	// Get the current tail position
	PosnType prev_tail_position = snakePositions[snakeTailIndex];
	
	/* Update the tail index */
	snakeTailIndex++;
	if(snakeTailIndex == SNAKE_POSITION_ARRAY_SIZE) {
		/* Array has wrapped around */
		snakeTailIndex = 0;
	}
	snakeLength--;
	
	return prev_tail_position;
}

/* set_tron_dirn
**      Attempt to set the next snake direction.
**      (Should be ignored if try and reverse snake from its current
**      direction, but otherwise the direction will be accepted.)
**      It's OK to continue in same direction or turn 90 degrees.
*/
void set_tron_dirn(SnakeDirnType dirn) {
	/* YOUR CODE HERE - MODIFY THIS FUNCTION */
	/* You must write code to check that the proposed direction (dirn)
	** is not opposite to the current direction (stored in curSnakeDirn). 
	**
	** Initially, we assume the move is OK and just set the 
	** next direction.
	*/
    if ((curSnakeDirn == SNAKE_UP && dirn != SNAKE_DOWN) ||
        (curSnakeDirn == SNAKE_DOWN && dirn != SNAKE_UP) ||
        (curSnakeDirn == SNAKE_LEFT && dirn != SNAKE_RIGHT) ||
        (curSnakeDirn == SNAKE_RIGHT && dirn != SNAKE_LEFT)) {
            nextSnakeDirn = dirn;
        }
}

/* is_tron_at
**		Check all snake positions and see if any part of the 
**		snake is at the given position
*/
int8_t is_tron_at(PosnType position) {
	int8_t index;

	/* Start at tail and work forward to the head.
	*/
	index = snakeTailIndex;
	while(index != snakeHeadIndex) {
		if(position == snakePositions[index]) {
			return 1;
		}
		index++;
		if(index > MAX_SNAKE_SIZE) {
			index = 0;
		}
	}
	/* Now check head position, since it is not checked above. */
	if(position == snakePositions[snakeHeadIndex]) {
		return 1;
	}
	/* Snake does not occupy the given position */
	return 0;
}

//attempts to step tron to a new position		
int8_t step_tron(void) {
	PosnType foodPos = get_position_of_food(0);
	uint8_t fx = x_position(foodPos);
	uint8_t fy = y_position(foodPos);
	uint8_t x = x_position(snakePositions[snakeHeadIndex]);
	uint8_t y = y_position(snakePositions[snakeHeadIndex]);
	
	int8_t dx = x - fx;
	int8_t dy = y - fy;
	if ((abs(dx) < abs(dy)) ^ 1) {
		if (dx) {
			set_tron_dirn(SNAKE_RIGHT);
		} else {
			set_tron_dirn(SNAKE_LEFT);
		}
	} else {
		if (dy) {
			set_tron_dirn(SNAKE_UP);
		} else {
			set_tron_dirn(SNAKE_DOWN);
		}
	}
	int8_t move_result = attempt_to_move_tron_forward();
	return move_result;
}


