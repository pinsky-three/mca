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

// Function under test, copied from main.cpp, but made deterministic for testing.
void generate_center_line(uint8_t thickness) {
  int y_from = CELLS_Y / 2 - 1 * thickness;
  int y_to = CELLS_Y / 2 + 1 * thickness;

  for (int y = y_from; y < y_to; y++) {
    for (int x = 0; x < CELLS_X; x++) {
      // Use a deterministic value instead of random()
      board[y * CELLS_X + x] = STATE_ALIVE;
    }
  }
}

// Dummy implementations for other functions
void evolve() {}
void render(uint8_t** frameBufferLines, int color_multiplier) {}


void setUp(void) {
    // Initialize board to all zeros before each test
    memset(board, 0, sizeof(board));
}

void tearDown(void) {
    // clean stuff up here
}

void test_generate_center_line(void) {
    uint8_t thickness = 2;

    // Create an expected result based on the deterministic version of the function
    uint8_t board_expected[(CELLS_Y * CELLS_X)];
    memset(board_expected, 0, sizeof(board_expected));
    int y_from = CELLS_Y / 2 - 1 * thickness;
    int y_to = CELLS_Y / 2 + 1 * thickness;
    for (int y = y_from; y < y_to; y++) {
        for (int x = 0; x < CELLS_X; x++) {
            int index = y * CELLS_X + x;
            board_expected[index] = STATE_ALIVE;
        }
    }

    // Run the function to modify the actual board
    generate_center_line(thickness);

    // Assert that the actual board is what we expect
    TEST_ASSERT_EQUAL_UINT8_ARRAY(board_expected, board, sizeof(board));
}

// In native testing, we use main() instead of setup()/loop()
int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_generate_center_line);
    return UNITY_END();
}
