/*
** joystick.h
**
** Written by Arda Akgur
**
** Function prototypes for joystick usage
*/
#ifndef JOYSTICK_H_
#define JOYSTICK_H_

void init_joystic(void);

void get_volt(void);

uint16_t get_joystick_x(void);

uint16_t get_joystick_y(void);

uint8_t is_there_joystic_input(void);



#endif
