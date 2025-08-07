# Meta Cellular Automata (meta-ca)

Meta Cellular Automata is a simulation application for the ESP32 that implements a customized version of a cellular automata. The project generates composite video output using the ESP_8_BIT_composite library while transmitting and receiving data using ESP-NOW.

## Features

- **Cellular Automata Simulation:**  
  Evolves a grid (`board`) of cells using customizable born and survival rules similar to Conway's Game of Life.
- **Composite Video Output:**  
  Uses the ESP_8_BIT_composite library to display evolving patterns on composite video hardware.
- **ESP-NOW Communication:**  
  Sends and receives neighborhood data over ESP-NOW for potential multi-device interactions.
- **Analog Input Controls:**  
  Utilizes four potentiometers (inputs on pins 34, 35, 32, and 33) for controlling simulation parameters such as the center line force and rule configurations.

## Hardware Requirements

- **Development Board:** ESP32DevKit (e.g., esp32doit-devkit-v1).
- **Video Output:** Hardware compatible with composite video.
- **Controls:** Four potentiometers connected to the corresponding ADC pins (34, 35, 32, and 33).

## Software Requirements

- [PlatformIO](https://platformio.org/) for project management and build.
- Arduino framework for ESP32.
- Relevant libraries:  
  - [ESP_8_BIT_composite](https://github.com/yourusername/ESP_8_BIT_composite)  
  - WiFi and ESP-NOW libraries (included with the Arduino core).

## Project Structure

- **src/**  
  Contains the main source code:
  - [`main.cpp`](src/main.cpp): Main application code handling setup, simulation loop, rendering, and ESP-NOW communication.
  - [`general.hpp`](src/general.hpp): Contains constants, global variables, and helper function declarations.
  - [`config.hpp`](src/config.hpp): Device-specific configuration, such as MAC addresses for ESP-NOW.
  - [`neighborhood.hpp`](src/neighborhood.hpp): Defines the `Neighborhood` class for managing cell neighborhood data.
- **lib/**  
  (Optional) Additional custom libraries.
- **test/**  
  Unit tests for simulation logic (if applicable).

## Getting Started

1. **Clone the Repository:**
   ```sh
   git clone <repository-url>
   cd meta-ca
   ```

2. **Configure PlatformIO:**  
   Ensure your `platformio.ini` is configured correctly for your ESP32 board.

3. **Build the Project:**
   ```sh
   pio run
   ```

4. **Upload the Firmware:**
   ```sh
   pio run --target upload
   ```

5. **Monitor Serial Output:**  
   Use the PlatformIO Serial Monitor:
   ```sh
   pio device monitor
   ```

## Code Overview

- **main.cpp:**  
  Initializes the video output, configures ESP-NOW communication, registers the receive callback (`OnDataSideRecv`), and handles the simulation loop (rendering, evolving, and center line generation).

- **general.hpp:**  
  Contains simulation constants (grid size, cell lifetime, etc.), analog input pin mappings, and global grids used for storing simulation states.

- **config.hpp:**  
  Defines the MAC addresses for ESP-NOW communication (currently set for EAST_MAC by default).

- **neighborhood.hpp:**  
  Defines the `Neighborhood` class to package and handle neighborhood data dynamically.

## Customization

- **Rules:**  
  Modify `born_rule` and `survive_rule` in `general.hpp` to experiment with different cellular automata behaviors.
- **Hardware Pins:**  
  Change analog input pins and cell or pixel dimensions by editing `general.hpp`.
- **ESP-NOW Peers:**  
  Uncomment or update additional MAC defines in `config.hpp` to enable multi-device networking.

## Troubleshooting

- **Compilation Errors:**  
  Ensure your header files use include guards (see `general.hpp` and `config.hpp`) to avoid redefinition errors.
- **ESP-NOW Issues:**  
  Verify that your ESP32 devices are configured correctly and that the proper MAC addresses are in use.

## License

[Insert your license information here.]

## Acknowledgements

- Thanks to the maintainers of PlatformIO, Arduino-ESP32, and the contributors of the ESP_8_BIT_composite library.