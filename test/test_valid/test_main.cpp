#include <stdint.h>
#include <string.h>
#include <unity.h>
#include "general.hpp"

// Mock Arduino functions
void delay(uint32_t ms) { (void)ms; }

// Define the global variables for this test application.
// general.hpp declares them as extern.
uint8_t board[(CELLS_Y * CELLS_X)];
uint8_t board_copy[(CELLS_Y * CELLS_X)];

// Provide implementations for the functions needed by the test.

// Function under test, copied from main.cpp
void evolve() {
  memcpy(board_copy, board, sizeof(uint8_t) * CELLS_Y * CELLS_X);

  for (int y = 0; y < CELLS_Y; y++) {
    for (int x = 0; x < CELLS_X; x++) {
      int current_state = board[y * CELLS_X + x];

      uint8_t total_n = 0;

      for (int dx = -1; dx < 2; dx++) {
        for (int dy = -1; dy < 2; dy++) {
          if (dx != 0 || dy != 0) {
            int neighborX = (x + dx) % CELLS_X;
            int neighborY = (y + dy) % CELLS_Y;

            int neighbor_state = board[neighborY * CELLS_X + neighborX];

            if (neighbor_state == STATE_ALIVE) {
              total_n += 1;
            }
          }
        }
      }

      if (current_state == STATE_DEAD) {
        if ((born_rule >> total_n) & 1) {
          board_copy[y * CELLS_X + x] = STATE_ALIVE;
        }
      } else if (current_state == STATE_ALIVE) {
        if (!((survive_rule >> total_n) & 1)) {
          board_copy[y * CELLS_X + x] = STATE_DEAD;
        }
      } else {
        board_copy[y * CELLS_X + x] = current_state - 1;
      }
    }
  }

  memcpy(board, board_copy, sizeof(uint8_t) * CELLS_Y * CELLS_X);

  return;
}

// Dummy implementations for other functions
void render(uint8_t** frameBufferLines, int color_multiplier) {}
void generate_center_line(uint8_t thickness) {}

void setUp(void) {
    memset(board, 0, sizeof(board));
    // Standard Conway's Game of Life rules
    born_rule = 0b00001000; // B3
    survive_rule = 0b00001100; // S23
}

void tearDown(void) {
}

void test_still_life_block(void) {
    // A 2x2 block is a stable pattern (still life)
    board[1 * CELLS_X + 1] = STATE_ALIVE;
    board[1 * CELLS_X + 2] = STATE_ALIVE;
    board[2 * CELLS_X + 1] = STATE_ALIVE;
    board[2 * CELLS_X + 2] = STATE_ALIVE;

    uint8_t board_expected[sizeof(board)];
    memcpy(board_expected, board, sizeof(board));

    evolve();

    TEST_ASSERT_EQUAL_UINT8_ARRAY(board_expected, board, sizeof(board));
}

void test_oscillator_blinker(void) {
    // A blinker is a period-2 oscillator.
    // We will test this property by evolving it twice and checking if it returns to the original state.

    // State 1: Horizontal bar
    board[1 * CELLS_X + 1] = STATE_ALIVE;
    board[1 * CELLS_X + 2] = STATE_ALIVE;
    board[1 * CELLS_X + 3] = STATE_ALIVE;

    uint8_t board_state1[sizeof(board)];
    memcpy(board_state1, board, sizeof(board));

    // Evolve to State 2
    evolve();

    // Evolve back to State 1
    evolve();

    TEST_ASSERT_EQUAL_UINT8_ARRAY(board_state1, board, sizeof(board));
}


int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_still_life_block);
    RUN_TEST(test_oscillator_blinker);
    return UNITY_END();
}
