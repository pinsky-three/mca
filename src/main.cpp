#include <Arduino.h>
#include <ESP_8_BIT_composite.h>
#include <WiFi.h>
#include <esp_now.h>

#include <config.hpp>
#include <general.hpp>
#include <neighborhood.hpp>

#if NORTH_MAC
uint8_t northAddress[] = {NORTH_MAC};
#endif

#if SOUTH_MAC
uint8_t southAddress[] = {SOUTH_MAC};
#endif

#ifdef EAST_MAC
uint8_t eastAddress[] = {EAST_MAC};
Neighborhood east(CELLS_Y);
esp_now_peer_info_t peerInfoEast;
#endif

#if WEST_MAC
uint8_t westAddress[] = {WEST_MAC};
Neighborhood west(CELLS_X);
esp_now_peer_info_t peerInfoWest;
#endif

Neighborhood dataSide(CELLS_Y);

ESP_8_BIT_composite video_out(true);

void OnDataSideRecv(const uint8_t* mac, const uint8_t* incomingData, int len) {
  memcpy(&dataSide, incomingData, sizeof(dataSide));

  Serial.print("Received data from: ");
  Serial.print(dataSide.name);
  Serial.print(" with MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(mac[i], HEX);
    if (i < 5) {
      Serial.print(":");
    }
  }
  Serial.println();
}

void setup() {
  video_out.begin();

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  Serial.println("Initializing ESP-NOW...");

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // for (int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++) {
  //   Serial.print(".");
  //   delay(1000);
  // }

  analogReadResolution(9);

  for (int y = 0; y < CELLS_Y; y++) {
    for (int x = 0; x < CELLS_X; x++) {
      board[y * CELLS_Y + x] = random(0, CELL_LIFETIME);
    }
  }

#ifdef NORTH_MAC
  memcpy(peerInfoNorth.peer_addr, northAddress, 6);
  peerInfoNorth.channel = 0;
  peerInfoNorth.encrypt = false;
#endif

#ifdef SOUTH_MAC
  memcpy(peerInfoSouth.peer_addr, southAddress, 6);
  peerInfoSouth.channel = 0;
  peerInfoSouth.encrypt = false;
#endif

#ifdef EAST_MAC
  memcpy(peerInfoEast.peer_addr, eastAddress, 6);
  peerInfoEast.channel = 0;
  peerInfoEast.encrypt = false;

  if (esp_now_add_peer(&peerInfoEast) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
#endif

#ifdef WEST_MAC
  memcpy(peerInfoWest.peer_addr, westAddress, 6);
  peerInfoWest.channel = 0;
  peerInfoWest.encrypt = false;

  if (esp_now_add_peer(&peerInfoWest) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.print("WEST MAC Peer Added: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(westAddress[i], HEX);
    if (i < 5) {
      Serial.print(":");
    }
  }
  Serial.println();
#endif

  if (esp_now_register_recv_cb(OnDataSideRecv) != ESP_OK) {
    Serial.println("Failed to register receive callback");
    return;
  }

  Serial.println("ESP-NOW initialized successfully");
}

void loop() {
  uint8_t** frame_buffer = video_out.getFrameBufferLines();

  int center_line_force = map(analogRead(input_pot_4_pin), 0, 511, 0, 15);

  render(frame_buffer, 127);

  evolve();

  if (center_line_force > 0) {
    generate_center_line(center_line_force);
  }

#if EAST_MAC
  strcpy(dataSide.name, "east");
  dataSide.data[CELLS_Y] = board[CELLS_X - 1];

  esp_err_t result =
      esp_now_send(eastAddress, (uint8_t*)&dataSide, sizeof(dataSide));

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  } else {
    Serial.println("Error sending the data");
  }
#endif

  delay(1000);
}

void render(uint8_t** frameBufferLines, int color_multiplier) {
  for (int y = 0; y < PIXELS_Y; y++) {
    for (int x = 0; x < PIXELS_X; x++) {
      int index = (y / CELL_SIZE_Y) * CELLS_Y + (x / CELL_SIZE_X);

      frameBufferLines[y][x] = board[index] * color_multiplier / CELL_LIFETIME;
    }
  }

  video_out.waitForFrame();
}

void generate_center_line(uint8_t thickness) {
  int y_from = CELLS_Y / 2 - 1 * thickness;
  int y_to = CELLS_Y / 2 + 1 * thickness;

  for (int y = y_from; y < y_to; y++) {
    for (int x = 0; x < CELLS_X; x++) {
      board[y * CELLS_Y + x] = random(0, CELL_LIFETIME);
    }
  }
}

void evolve() {
  memcpy(board_copy, board, sizeof(uint8_t) * CELLS_Y * CELLS_X);

  for (int y = 0; y < CELLS_Y; y++) {
    for (int x = 0; x < CELLS_X; x++) {
      int current_state = board[y * CELLS_Y + x];

      uint8_t total_n = 0;

      for (int dx = -1; dx < 2; dx++) {
        for (int dy = -1; dy < 2; dy++) {
          if (dx != 0 || dy != 0) {
            int neighborX = (x + dx) % CELLS_X;
            int neighborY = (y + dy) % CELLS_Y;

            int neighbor_state = board[neighborY * CELLS_Y + neighborX];

            if (neighbor_state == STATE_ALIVE) {
              total_n += 1;
            }
          }
        }
      }

      if (current_state == STATE_DEAD) {
        if ((born_rule >> total_n) & 1) {
          board_copy[y * CELLS_Y + x] = STATE_ALIVE;
        }
      } else if (current_state == STATE_ALIVE) {
        if (!((survive_rule >> total_n) & 1)) {
          board_copy[y * CELLS_Y + x] = STATE_DEAD;
        }
      } else {
        board_copy[y * CELLS_Y + x] = current_state - 1;
      }
    }
  }

  memcpy(board, board_copy, sizeof(uint8_t) * CELLS_Y * CELLS_X);

  return;
}