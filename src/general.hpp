#ifndef GENERAL_HPP
#define GENERAL_HPP

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <stdint.h>
#include <string.h>
#endif

const uint8_t input_pot_1_pin = 34;
const uint8_t input_pot_2_pin = 35;
const uint8_t input_pot_3_pin = 32;
const uint8_t input_pot_4_pin = 33;

const int CELL_SIZE_X = 2;
const int CELL_SIZE_Y = 2;

const int PIXELS_X = 256;
const int PIXELS_Y = 240;

const int CELL_LIFETIME = 7;

const int CELLS_X = PIXELS_X / CELL_SIZE_X;
const int CELLS_Y = PIXELS_Y / CELL_SIZE_Y;

const uint8_t STATE_DEAD = 0;
const uint8_t STATE_ALIVE = CELL_LIFETIME - 1;

uint16_t born_rule = 0b000001000;     // {3}
uint16_t survive_rule = 0b000001100;  // {3,2}

extern uint8_t board[(CELLS_Y * CELLS_X)];
extern uint8_t board_copy[(CELLS_Y * CELLS_X)];

void evolve();
void render(uint8_t** frameBufferLines, int color_multiplier);
void generate_center_line(uint8_t thickness);

#endif  // GENERAL_HPP