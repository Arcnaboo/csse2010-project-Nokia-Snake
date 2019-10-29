/*
** rat.h
**
** Written by Arda Akgur
**
** Function prototypes for those functions available externally
*/

/* Guard band to ensure this definition is only included once */
#ifndef RAT_H_
#define RAT_H_

#include <inttypes.h>
#include "position.h"


void set_rat_pos(PosnType posn);

// boolean if there is rat at the given position
uint8_t is_rat_at(PosnType posn);

// tries to add rat item to game
PosnType add_rat_item(void);

// returns the current positon of rat
PosnType get_position_of_rat(void);

// moves the rat
PosnType step_rat(void);


#endif
