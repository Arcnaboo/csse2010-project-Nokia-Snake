/*
 * tron.h
 *
 * Author: Arda Akgur
 *
 * Functions for utilising Tron computer controlled player
 */
#ifndef TRON_H_
#define TRON_H_

#include <inttypes.h>
#include "position.h"

void init_tron(void);
PosnType get_tron_head_position(void);
PosnType get_tron_tail_position(void);
uint8_t get_tron_length(void);
int8_t advance_tron_head();
PosnType advance_tron_tail();
void set_tron_dirn(SnakeDirnType dirn);
int8_t is_tron_at(PosnType position);
int8_t step_tron(void);
uint8_t is_tron_mode(void);
void clear_tron(void);
void set_tron_mode(uint8_t mode);

#endif