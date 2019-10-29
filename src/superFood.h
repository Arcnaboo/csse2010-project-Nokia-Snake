/*
** superFood.h
**
** Written by Arda Akgur
**
** Function prototypes for those functions available externally
*/

/* Guard band to ensure this definition is only included once */
#ifndef SUPERFOOD_H_
#define SUPERFOOD_H_

#include <inttypes.h>
#include "position.h"


// boolean if there is super food at the given position
uint8_t is_super_food_at(PosnType posn);

// boolean is currently super food in game
uint8_t is_there_super_food(void);

// if super food 1 make it 0, vice versa
void reverse_super_food(void);

// tries to add super food item to game and changes super_food to 1
PosnType add_super_food_item(void);

// returns the current positon of super food
PosnType get_position_of_super_food(void);


#endif
