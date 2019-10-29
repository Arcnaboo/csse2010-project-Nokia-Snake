/*
 * project.c
 *
 * Main file for the Snake Project.
 *
 * Author: Peter Sutton. Modified by Arda Akgur
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>		// For random()
#include <avr/eeprom.h>
#include <string.h>
#include "ledmatrix.h"
#include "scrolling_char_display.h"
#include "buttons.h"
#include "serialio.h"
#include "terminalio.h"
#include "score.h"
#include "timer0.h"
#include "game.h"
#include "snake.h"
#include "superFood.h"
#include "food.h"
#include "pixel_colour.h"
#include "rat.h"
#include "board.h"
#include "joystick.h"
#include "tron.h"

// Define the CPU clock speed so we can use library delay functions
#define F_CPU 8000000L

#include <util/delay.h>

// Function prototypes - these are defined below (after main()) in the order
// given here
void initialise_hardware(void);
void splash_screen(void);
void new_game(void);
void play_game(void);
void handle_game_over(void);
void handle_new_lap(void);
void print_grid(void);


// ASCII code for Escape character
#define ESCAPE_CHAR 27

// seven segment display characters
uint8_t seven_seg[10] = { 63,6,91,79,102,109,125,7,127,111};

// control character for EEPROM
uint8_t control_character = 65;
uint8_t load_control = (uint8_t) '@';
uint8_t load = 0;

// is currently we have saved leader boards
uint8_t old_player = 0;

// for storing names and scores
// after reading from ROM
char** player_names;
uint16_t* player_scores;


// Helper function
static void update_display_at_position(PosnType posn, PixelColour colour) {
	ledmatrix_update_pixel(x_position(posn), y_position(posn), colour);
}

static void check_old_player(void) {
	uint8_t byteRead = eeprom_read_byte((uint8_t*)90);
	if (byteRead == control_character) {
		old_player = 1;
	}
	byteRead = eeprom_read_byte((uint8_t*)253);
	if (byteRead == load_control) {
		load = 1;
	}
}

static void read_high_scores(void) {
	uint8_t total = eeprom_read_byte((uint8_t*)89);
	char buffer[total];
	uint8_t i;
	uint8_t j = 9;
	uint8_t index = 0;
	uint8_t* pos = (uint8_t*) 91;
	for (i = 0; i < total; i++) {
		buffer[i] = eeprom_read_byte(pos);
		pos++;
	}
	for (i = 0; i < strlen(buffer); i++) {
		if (buffer[i] != ':') {
			player_names[index][j] = buffer[i];
			j++;
		} else {
			j = 0;
			index++;
		}
		
	}
	player_scores[0] = eeprom_read_word((uint16_t*)300);
	player_scores[1] = eeprom_read_word((uint16_t*)302);
	player_scores[2] = eeprom_read_word((uint16_t*)304);
	player_scores[3] = eeprom_read_word((uint16_t*)306);
	player_scores[4] = eeprom_read_word((uint16_t*)308);
	
} 

/////////////////////////////// main //////////////////////////////////
int main(void) {
	// Setup hardware and call backs. This will turn on 
	// interrupts.
	init_joystic();
	initialise_hardware();
	check_old_player();
	if (old_player) {
		player_names = malloc(sizeof(char*) * 5);
		uint8_t i = 0;
		for (i = 0; i < 5; i++) {
			player_names[i] = malloc(sizeof(char) * 10);
		}
		player_scores = malloc(sizeof(uint8_t) * 5);
		read_high_scores();
	}
	// Show the splash screen message. Returns when display
	// is complete
	splash_screen();
	while(1) {
		new_game();
		play_game();
		handle_game_over();
	}
}

// initialise_hardware()
//
// Sets up all of the hardware devices and then turns on global interrupts
void initialise_hardware(void) {
	// Set up SPI communication with LED matrix
	ledmatrix_setup();
	
	// Set up pin change interrupts on the push-buttons
	init_button_interrupts();
	
	// Setup serial port for 19200 baud communication with no echo
	// of incoming characters
	init_serial_stdio(19200, 0);
	
	// Set up our main timer to give us an interrupt every millisecond
	init_timer0();
	
	// Turn on global interrupts
	sei();
}

void splash_screen(void) {
	// Reset display attributes and clear terminal screen then output a message
	set_display_attribute(TERM_RESET);
	clear_terminal();
	
	hide_cursor();	// We don't need to see the cursor when we're just doing output
	move_cursor(3,3);
	printf_P(PSTR("Snake"));
	
	move_cursor(3,5);
	set_display_attribute(FG_GREEN);	// Make the text green
	// Modify the following line
	printf_P(PSTR("CSSE2010/7201 Snake Project by Arda Akgur"));	
	set_display_attribute(FG_WHITE);	// Return to default colour (White)
	
	// Output the scrolling message to the LED matrix
	// and wait for a push button to be pushed.
	ledmatrix_clear();
	
	// Red message the first time through
	PixelColour colour = COLOUR_ARC;
	while(1) {
		set_scrolling_display_text("43829114", colour);
		// Scroll the message until it has scrolled off the 
		// display or a button is pushed. We pause for 130ms between each scroll.
		while(scroll_display()) {
			_delay_ms(130);
			get_volt();
			if(is_there_joystic_input() ||button_pushed() != -1 ) {
				// A button has been pushed
				return;
			}
		}
		// Message has scrolled off the display. Change colour
		// to a random colour and scroll again.
		switch(random() % 4) {
			case 0: colour = COLOUR_LIGHT_ORANGE; break;
			case 1: colour = COLOUR_RED; break;
			case 2: colour = COLOUR_YELLOW; break;
			case 3: colour = COLOUR_GREEN; break;
		}
	}
}

void new_game(void) {
	// Clear the serial terminal
	clear_terminal();
	
	// Initialise the game and display
	init_game();
		
	// Initialise the score
	init_score();
	
	// Delete any pending button pushes or serial input
	empty_button_queue();
	clear_serial_input_buffer();
}

void play_game(void) {
	// for time based events
	uint32_t last_print_time;
	uint32_t last_move_time;
	uint32_t rat_last_move_time;
	uint32_t last_len_time;
	uint32_t pause_time;
	
	// forbutton and serial in
	int8_t button;
	char serial_input, escape_sequence_char;
	uint8_t characters_into_escape_sequence = 0;
	
	// for flow of control
	uint8_t pause = 0;
	uint8_t control = 0;
	uint8_t ignore = 0;
	
	// for joystick movement
	uint16_t joystick_x;
	uint16_t joystick_y;
	
	// DDRC 255 for displaying seven segment 
	DDRC = 0xFF;
	// CC of seven segment
	PORTC &= 0xFE;
	
	// to keep track of snake length in main
	uint8_t snake_length;
	
	// Record the last time the snake moved as the current time -
	// this ensures we don't move the snake immediately.
	// Also record rest of the time variables
	last_move_time = get_clock_ticks();
	rat_last_move_time = get_clock_ticks();
	last_print_time = get_clock_ticks();
	last_len_time = get_clock_ticks();

	while(1) {
		//initiate inputs, serial, button and joystic
		serial_input = -1;
		escape_sequence_char = -1;
		button = button_pushed();
		get_volt();
		
		// first check joystick input since it should override any other
		if (is_there_joystic_input()) {
			joystick_x = get_joystick_x();
			joystick_y = get_joystick_y();
			if (joystick_y >= 600 && joystick_x < 600 && joystick_x >= 400) {
				set_snake_dirn(SNAKE_LEFT);
			}
			if (joystick_y < 400 && joystick_x < 600 && joystick_x >= 400) {
				
				set_snake_dirn(SNAKE_RIGHT);
			}
			if (joystick_x >= 600 && joystick_y < 600 && joystick_y >= 400) {
				
				set_snake_dirn(SNAKE_UP);
			}
			if (joystick_x <400 && joystick_y < 600 && joystick_y >= 400) {
				
				set_snake_dirn(SNAKE_DOWN);
			}
			// make sure to ignore other inputs until we move via joystick
			ignore = 1;
		}
		
		// no joystick or button input so check serial input
		if(button == -1) {
			if(serial_input_available()) {
				serial_input = fgetc(stdin);
				if(characters_into_escape_sequence == 0 && serial_input == ESCAPE_CHAR) {
					characters_into_escape_sequence++;
					serial_input = -1; 
				} else if(characters_into_escape_sequence == 1 && serial_input == '[') {
					characters_into_escape_sequence++;
					serial_input = -1; 
				} else if(characters_into_escape_sequence == 2) {
					escape_sequence_char = serial_input;
					serial_input = -1;  
					characters_into_escape_sequence = 0;
				} else {
					characters_into_escape_sequence = 0;
				}
			}
		}
		// if button or serial input true
		// then move snake
		if (!ignore && (button==0 || escape_sequence_char=='C')) {
			set_snake_dirn(SNAKE_RIGHT);
		} else  if (!ignore && (button==2 || escape_sequence_char == 'A')) {
			set_snake_dirn(SNAKE_UP);
		} else if (!ignore && (button==3 || escape_sequence_char=='D')) {
			set_snake_dirn(SNAKE_LEFT);
		} else if (!ignore && (button==1 || escape_sequence_char == 'B')) {
			set_snake_dirn(SNAKE_DOWN);
			
			// if P pressed Pause
		} else if (serial_input == 'p' || serial_input == 'P') {
			if (!pause) {
				pause = 1;
				pause_time = get_clock_ticks();
			} else {
				pause = 0;
				last_move_time += get_clock_ticks() - pause_time;
				rat_last_move_time +=  get_clock_ticks() - pause_time;
				set_super_food_timer(get_super_food_timer() + get_clock_ticks() - pause_time);
			}
			
			// if T pressed initiate Tron Mini Game Mode
		} else if (serial_input == 't' || serial_input =='T') {
			if (!is_tron_mode()) {
				init_tron();
				update_display_at_position(get_tron_head_position(), COLOUR_ARC);
				update_display_at_position(get_tron_tail_position(), COLOUR_RED);
				set_tron_mode(1);
			} else {
				clear_tron();
				set_tron_mode(0);
			}
			
			// if H pressed then display high scores
		} else if (serial_input == 'h' || serial_input == 'H') {
			uint8_t i;
			for (i = 0; i < 5; i++) {
				printf_P(PSTR("%d %s - %d\n"), i+1, player_names[i], player_scores[i]);
			}
			_delay_ms(1000);
		}
		
		// Check for timer related events here
		
		// check rat time, then step rat
		if (!pause && get_clock_ticks() >= rat_last_move_time + 1000) {
			PosnType current_rat_pos = get_position_of_rat();
			PosnType test_position = step_rat();
			rat_last_move_time = get_clock_ticks();
			if (is_position_valid(test_position)) {
				update_display_at_position(current_rat_pos, COLOUR_BLACK);
				update_display_at_position(test_position, COLOUR_ARC);
				set_rat_pos(test_position);
			}
		}
		
		// check last move time then step snake
		if(!pause && get_clock_ticks() >= last_move_time + (600 / get_game_speed())) {
			// 600ms (0.6 second) has passed since the last time we moved the snake,
			// so move it now
			if(!attempt_to_move_snake_forward()) {
				// Move attempt failed - game over
				break;
			}
			// if Tron mode then also step Tron
			if (is_tron_mode()) {
				if (!step_tron()) {
					clear_tron();
				}
			}
			// move succesfull add score, if move made with joystic change ignore
			last_move_time = get_clock_ticks();
			add_to_score(1);
			if(ignore) {
				ignore = 0;
			}
		}
		
		// if time is right remove superfood
		if (!pause && is_there_super_food() && get_clock_ticks() >= get_super_food_timer() + 5000) {
			update_display_at_position(get_position_of_super_food(), COLOUR_BLACK);
			reverse_super_food();
		}
		// if time is right add new superFood
		if (!pause && !is_there_super_food() && get_clock_ticks() >= get_super_food_timer() + 15000) {
			PosnType super_food_pos = add_super_food_item();
			if (is_position_valid(super_food_pos)) {
				update_display_at_position(super_food_pos, COLOUR_ORANGE);
				set_super_food_timer(get_clock_ticks());
			}
		}
		// if time is right display length on io board	
		if (get_clock_ticks() >= last_len_time + 10) {
			DDRA = 0xFF;
			snake_length = get_snake_length();
			if (snake_length < 10) {
				PORTC = seven_seg[snake_length % 10];
			} else {
				if (control) {
					PORTA |= 0x80;
					PORTC = seven_seg[snake_length / 10];
				}
				if (!control) {
					PORTA &= 0x7f;
					PORTC = seven_seg[snake_length % 10];
				}
				if (control == 1) {
					control = 0;
					} else {
					control = 1;
				}
			}
			DDRA = 0x00;
			last_len_time = get_clock_ticks();
		}
		
		// if time is right refresh terminal display
		if (get_clock_ticks() >= last_print_time + 180) {
			set_display_attribute(TERM_RESET);
			clear_terminal();
			hide_cursor();
			move_cursor(3,3);
			// print score
			printf_P(PSTR("Score: %5ld"), get_score());
			move_cursor(3,6);
			printf_P(PSTR("\n"));
			// print the terminal view Grid
			print_grid();
			printf_P(PSTR("\n"));
			// if Tron mode active print EE message
			if (is_tron_mode()) {
				printf_P(PSTR("TRON MODE\n"));
				printf_P(PSTR("-----------\n"));
				printf_P(PSTR("CLU: I WILL SHOW YOU NO MERCY, USER!\n"));
				printf_P(PSTR("TRON: DON'T WORRY USER, I FIGHT FOR THE USERS!\n"));
			}
			// if game is on pause print pause
			if (pause) {
				printf_P(PSTR("GAME PAUSE"));
			}
			// regresh timer
			last_print_time = get_clock_ticks();
		}
	}
	// If we get here the game is over. 
	// reset game speed and remove if there is superfood
	reset_game_speed();
	if (is_there_super_food()) {
		reverse_super_food();
		update_display_at_position(get_position_of_super_food(), COLOUR_BLACK);
	}
	
}

//prints grid to the terminal
void print_grid(void) {
	uint8_t x, y;
	for (y = BOARD_HEIGHT + 1; y > 0 ; y--) {
		for (x = 0; x < BOARD_WIDTH + 2; x++) {
			if (x != 0 && y != 0 && x != BOARD_WIDTH + 1 && y != BOARD_HEIGHT + 1) {
				PosnType test_position = position(x - 1, y - 1);
				if (is_snake_at(test_position)) {
					if (test_position == get_snake_head_position()) {
						//uncomment to make it colorful but decreases performance
						//set_display_attribute(FG_BLUE);
						printf_P(PSTR("H"));
					} else if (test_position == get_snake_tail_position()) {
						//set_display_attribute(FG_BLUE);
						printf_P(PSTR("T"));
					} else {
						//set_display_attribute(FG_MAGENTA);
						printf_P(PSTR("#"));
					}
				} else if (is_rat_at(test_position)) {
					//set_display_attribute(FG_GREEN);
					printf_P(PSTR("r"));
				} else if (is_food_at(test_position)) {
					//set_display_attribute(FG_WHITE);
					printf_P(PSTR("f"));
				} else if (is_super_food_at(test_position)) {
					//set_display_attribute(FG_RED);
					printf_P(PSTR("s"));
				} else {
					if (is_tron_mode()) {
						if (is_tron_at(test_position)) {
							if(test_position == get_tron_tail_position()) {
								printf_P(PSTR("T"));
								} else if (test_position == get_tron_head_position()) {
								printf_P(PSTR("H"));
								} else {
								printf_P(PSTR("@"));
							}
						} else {
							printf_P(PSTR(" "));
						}
					} else {
						printf_P(PSTR(" "));
					}
					//set_display_attribute(FG_WHITE);
				}
			} else {
				printf_P(PSTR("#"));
				if (x == BOARD_WIDTH + 1) {
					printf_P(PSTR("\n"));
				}
			}	
		}
	}
	for (x = 0; x < BOARD_WIDTH + 2; x++) {
		printf_P(PSTR("#"));
		if (x == BOARD_WIDTH + 1) {
			printf_P(PSTR("\n"));
		}
	}
}

// default leader board if there is nosaved leader board
void prepare_default_leaders(char** names, uint16_t* scores) {
	names[0] = "AAA";
	names[1] = "BBB";
	names[2] = "CCC";
	names[3] = "DDD";
	names[4] = "EEE";
	scores[0] = 500;
	scores[1] = 400;
	scores[2] = 300;
	scores[3] = 200;
	scores[4] = 50;
}

// check player score vs scores in current leader board
// if player score enough to join the list then
// return index value of the position in scores array
int8_t check_score(uint16_t* scores, uint16_t player_score) {
	uint8_t i;
	for (i = 0; i < 5; i++) {
		if (player_score > scores[i]) {
			return i;
		}
	}
	return -1;
}

// adds players score to scores list from given index position, and shifts the array
void add_score_to_list(char** names, uint16_t* scores, char name[], uint16_t score, uint8_t index) {
	uint8_t i;
	for (i = index; i < 5; i++) {
		scores[i + 1] = scores[i];
		names[i + 1] = names[i];
	}
	/*for (i = 4; i > index; i--) {
		scores[i] = scores[i - 1];
		names[i] = names[i - 1];
	}*/
	scores[index] = score;
	names[index] = name;
}

// removes extra white space
void strip_name(char* name) {
	uint8_t i;
	for (i = 0; i < strlen(name); i++) {
		if (name[i] == '\n') {
			name[i] = '\0';
		}
	}
}

// stored bytes on rom
uint8_t gather_byte_size(char** names) {
	uint8_t result = 0;
	uint8_t i;
	for (i = 0 ; i < 5; i++) {
		result += (uint8_t) strlen(names[i]);
	}
	return result;
}

// handle end game
void handle_game_over() {
	clear_terminal();
	normal_display_mode();
	reverse_video();
	
	// check if first time player
	if (!old_player) {
		// allocate memory for names and scores
		char** names = malloc(sizeof(char*) * 5);
		uint16_t* scores = malloc(sizeof(uint16_t) * 5);
		
		// prepare default leader board
		prepare_default_leaders(names, scores);
		
		// check player score
		uint16_t player_score = (uint16_t) get_score();
		int8_t index = check_score(scores, player_score);
		if (index!= -1) {
			char name[20];
			move_cursor(10,14);
			// Print a message to the terminal.
			show_cursor();
			printf_P(PSTR("GAME OVER\n"));
			printf_P(PSTR("You Scored %d\n"), get_score());
			printf_P(PSTR("Please enter your name: \n"));
			fgets(name, 20, stdin);
			printf_P(PSTR("Thank you for playing %s\n"), name);
			strip_name(name);
			add_score_to_list(names, scores, name, player_score, index);
			printf_P(PSTR("Highscores: \n"));
			uint8_t i;
			uint8_t j;
			uint8_t* pos = (uint8_t*)90;
			for (i = 0; i < 5; i++) {
				printf_P(PSTR("%d. %s : %d \n"), i+1, names[i], scores[i]);
			}
			eeprom_write_byte (pos, control_character);
			uint8_t totalByte = gather_byte_size(names);

			eeprom_write_byte (pos - 1, totalByte);
			for (i = 0; i < 5; i++) {
				for (j = 0; j < strlen(names[i]); j++) {
					eeprom_write_byte ((uint8_t*) ++pos, (uint8_t)names[i][j]);
				}
				eeprom_write_byte (++pos, (uint8_t)':');
			}
			eeprom_write_word((uint16_t*)300, scores[0]);
			eeprom_write_word((uint16_t*)302, scores[1]);
			eeprom_write_word((uint16_t*)304, scores[2]);
			eeprom_write_word((uint16_t*)306, scores[3]);
			eeprom_write_word((uint16_t*)308, scores[4]);
			
		} 
	} else {
		uint16_t player_score = get_score();
		int8_t index = check_score(player_scores, player_score);
		char name[20];
		move_cursor(10,14);
		// Print a message to the terminal.
		show_cursor();
		// print game over
		printf_P(PSTR("GAME OVER\n"));
		printf_P(PSTR("You Scored %d\n"), get_score());
		printf_P(PSTR("Please enter your name: \n"));
		
		// getplayer name
		fgets(name, 20, stdin);
		printf_P(PSTR("Thank you for playing %s\n"), name);
		strip_name(name);
		// add player to list
		add_score_to_list(player_names, player_scores, name, player_score, index);
		printf_P(PSTR("Highscores: \n"));
		uint8_t i;
		uint8_t j;
		uint8_t* pos = (uint8_t*)90;
		for (i = 0; i < 5; i++) {
			printf_P(PSTR("%d. %s : %d \n"), i+1, player_names[i], player_scores[i]);
		}
		eeprom_write_byte (pos, control_character);
		uint8_t totalByte = gather_byte_size(player_names);

		eeprom_write_byte (pos - 1, totalByte);
		for (i = 0; i < 5; i++) {
			for (j = 0; j < strlen(player_names[i]); j++) {
				eeprom_write_byte ((uint8_t*) ++pos, (uint8_t)player_names[i][j]);
			}
			eeprom_write_byte (++pos, (uint8_t)':');
		}
		eeprom_write_word((uint16_t*)300, player_scores[0]);
		eeprom_write_word((uint16_t*)302, player_scores[1]);
		eeprom_write_word((uint16_t*)304, player_scores[2]);
		eeprom_write_word((uint16_t*)306, player_scores[3]);
		eeprom_write_word((uint16_t*)308, player_scores[4]);
	
	}

	move_cursor(10,25);
	printf_P(PSTR("Press a button to start again"));
	if (is_tron_mode()) {
		clear_tron();
		set_tron_mode(0);
	}
	while(is_there_joystic_input() || button_pushed() == -1) {
		get_volt(); // wait until a button has been pushed
	}
	
}

