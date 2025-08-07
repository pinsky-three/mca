
#include <Arduino.h>

struct Neighborhood {
  char name[32];
  uint8_t* data;
  size_t data_size;

  // Constructor: allocate board with a dynamic size.
  Neighborhood(size_t size) : data_size(size) { data = new uint8_t[data_size]; }

  // Destructor: free the allocated memory.
  ~Neighborhood() { delete[] data; }

  // Disable copy semantics.
  Neighborhood(const Neighborhood&) = delete;
  Neighborhood& operator=(const Neighborhood&) = delete;
};
