
#include <Arduino.h>

#include <general.hpp>

struct neighborhood {
  char a[32];
  uint8_t* board;
  size_t board_size;

  // Constructor: allocate board with a dynamic size.
  neighborhood(size_t size) : board_size(size) {
    board = new uint8_t[board_size];
  }

  // Destructor: free the allocated memory.
  ~neighborhood() { delete[] board; }

  // Disable copy semantics.
  neighborhood(const neighborhood&) = delete;
  neighborhood& operator=(const neighborhood&) = delete;
};

typedef neighborhood neighborhood_t;

neighborhood* create_neighborhood(size_t size) {
  neighborhood_t* n = new neighborhood_t(size);
  return n;
}