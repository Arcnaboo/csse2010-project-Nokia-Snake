/*
** rat.c
** Provides functionality of rat item in game
** Written by Arda Akgur
*/


#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdio.h>
#include "joystick.h"
#include "serialio.h"

// current x and y volt
static uint16_t x;
static uint16_t y;

// returns x volt
uint16_t get_joystick_x(void) {
	return x;
}

// returns y volt
uint16_t get_joystick_y(void) {
	return y;
}

// returns true if currently there is joystick input
uint8_t is_there_joystic_input(void) {
	if (x < 550 && x > 450 && y < 550 && y > 450) {
		return 0;
	}
	return 1;
}

// initiates jotstic 
void init_joystic(void) {
	// Set up ADC - AVCC reference, right adjust
	// Input selection doesn't matter yet - we'll swap this around in the while
	// loop below.
	ADMUX = (1<<REFS0);
	// Turn on the ADC (but don't start a conversion yet). Choose a clock
	// divider of 64. (The ADC clock must be somewhere
	// between 50kHz and 200kHz. We will divide our 8MHz clock by 64
	// to give us 125kHz.)
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1);
}

// gets 2 loop and reads the currents
// x and y ADC values, stores values
// in memory
void get_volt(void) {
	DDRA = 0x00;
	uint8_t counter = 2;
	uint8_t x_or_y = 0;
	uint16_t value;
	while(counter > 0) {
		// Set the ADC mux to choose ADC0 if x_or_y is 0, ADC1 if x_or_y is 1
		if(x_or_y == 0) {
			ADMUX &= ~1;
			} else {
			ADMUX |= 1;
		}
		// Start the ADC conversion
		ADCSRA |= (1<<ADSC);
		
		while(ADCSRA & (1<<ADSC)) {
			; /* Wait until conversion finished */
		}
		value = ADC; // read the value
		if(x_or_y == 0) {
			//printf("X: %4d ", value);
			x = value;
			} else {
			//printf("Y: %4d\n", value);
			y = value;
		}
		// Next time through the loop, do the other direction
		x_or_y ^= 1;
		counter--;
	}
}


