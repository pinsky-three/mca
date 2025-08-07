# Meta Cellular Automata (meta-ca)

This project implements a cellular automata simulation running on an ESP32 using the Arduino framework. The simulation generates composite video output through the [`ESP_8_BIT_composite`](src/main.cpp) library while allowing interactive control via potentiometers.

## Features

- **Composite Video Output:** Uses 8-bit composite output for video.
- **Cellular Automata Simulation:** Evolves a board based on customizable born and survival rules.
- **Interactive Controls:** 
  - **Color Multiplier:** Adjusts color intensity.
  - **Born Rule (Pin 35):** Changes how new cells are born.
  - **Survival Rule (Pin 32):** Alters cell survival conditions.
  - **Center Line Generation (Pin 33):** Adds a force to generate a center line.
- **WiFi Connectivity:** Attempts to connect to WiFi for status reporting.

## Hardware Requirements

- ESP32Doit-devkit-v1 board.
- Potentiometers connected to ADC pins 34, 35, 32, and 33.
- Compatible composite video output hardware.

## Software Requirements

- [PlatformIO](https://platformio.org/) installed.
- Arduino framework for ESP32 (configured in [platformio.ini](platformio.ini)).

## Project Structure

- **src/**: Contains the main application code ([`main.cpp`](src/main.cpp)).
- **include/**: For shared header files.
- **lib/**: Private libraries for the project.
- **test/**: Automated tests and additional resources.

## Getting Started

1. **Clone the Repository:**
    ```sh
    git clone <repository-url>
    cd meta-ca
    ```

2. **Build the Project:**
    ```sh
    pio run
    ```

3. **Upload the Firmware:**
    ```sh
    pio run --target upload
    ```

4. **(Optional) Debug:**
    Use the debug configurations available in [`.vscode/launch.json`](.vscode/launch.json).

## Controls

- **Potentiometer on Pin 34:** Adjusts the color multiplier.
- **Potentiometer on Pin 35:** Sets the born rule.
- **Potentiometer on Pin 32:** Sets the survival rule.
- **Potentiometer on Pin 33:** Controls the strength of center line generation.

## Notes

- The simulation applies a rule set inspired by Conway's Game of Life with custom modifications.
- Each generated frame is based on the current state of the cellular board.
- WiFi connectivity is used mainly for printing connection status during setup.

## License

[Insert your license information here.]

## Acknowledgements

- Thanks to the contributors of [PlatformIO](https://platformio.org/) and the authors of the [`ESP_8_BIT_composite`](src/main.cpp) library.