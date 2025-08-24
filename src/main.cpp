#include <Arduino.h>
#include <ESP_8_BIT_composite.h>
#include <WiFi.h>
#include <esp_now.h>

#include <config.hpp>
#include <general.hpp>
#include <neighborhood.hpp>

// -----------------------------------------------------------------------------
// EXPECTED MAC DEFINES (from config.hpp)
//   - Left/WEST node:  #define EAST_MAC  0xAA,0xBB,0xCC,0xDD,0xEE,0xFF
//   - Right/EAST node: #define WEST_MAC  0x11,0x22,0x33,0x44,0x55,0x66
// -----------------------------------------------------------------------------

#ifdef EAST_MAC
uint8_t eastAddress[] = {EAST_MAC};
esp_now_peer_info_t peerInfoEast;
#endif

#ifdef WEST_MAC
uint8_t westAddress[] = {WEST_MAC};
esp_now_peer_info_t peerInfoWest;
#endif

// Role label for debug (derived from which neighbor macros exist)
#if defined(WEST_MAC) && !defined(EAST_MAC)
static const char* SELF_NAME = "E";  // right-most device
#elif defined(EAST_MAC) && !defined(WEST_MAC)
static const char* SELF_NAME = "W";  // left-most device
#else
static const char* SELF_NAME = "MID";  // middle device (both defined)
#endif

Neighborhood dataSide(CELLS_Y);

ESP_8_BIT_composite video_out(true);

// EAST inbound buffer (we consume this for x >= CELLS_X)
static bool eastValid = false;
static uint8_t eastInbound[CELLS_Y];
static uint32_t eastChecksum = 0;

// Telemetry
static uint32_t rxCountE = 0;
static uint32_t txCountRtoE = 0;
static uint32_t txCountLtoW = 0;
static uint32_t lastRxE_ms = 0;
static uint32_t frameNo = 0;

// -----------------------------------------------------------------------------
// Helpers (keep your original board indexing convention y * CELLS_Y + x)
// -----------------------------------------------------------------------------
static inline int INDEX(int y, int x) { return y * CELLS_Y + x; }

static inline int wrapY(int y) {
  if (y < 0) return (y % CELLS_Y + CELLS_Y) % CELLS_Y;
  if (y >= CELLS_Y) return y % CELLS_Y;
  return y;
}

static inline uint8_t toByte(uint8_t v) {
  return (uint16_t)v * 255 / CELL_LIFETIME;
}

// -----------------------------------------------------------------------------
// ESP-NOW receive: accept only from EAST peer into eastInbound
// -----------------------------------------------------------------------------
void OnDataSideRecv(const uint8_t* mac, const uint8_t* incomingData, int len) {
  if ((size_t)len < sizeof(Neighborhood)) {
    Serial.println("Recv: too small packet");
    return;
  }
  memcpy(&dataSide, incomingData, sizeof(Neighborhood));

#ifdef EAST_MAC
  if (memcmp(mac, eastAddress, 6) == 0) {
    eastChecksum = 0;
    for (int y = 0; y < CELLS_Y; ++y) {
      eastInbound[y] = dataSide.data[y];
      eastChecksum += eastInbound[y];
    }
    eastValid = true;
    rxCountE++;
    lastRxE_ms = millis();
    Serial.printf("[RX from EAST] name=%s checksum=%u count=%lu\n",
                  dataSide.name, eastChecksum, (unsigned long)rxCountE);
  } else {
    Serial.println("Recv from non-EAST peer (ignored for eastInbound)");
  }
#else
  Serial.println("Recv but EAST_MAC not defined on this build");
#endif
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup() {
  video_out.begin();
  Serial.begin(115200);
  delay(100);

  WiFi.mode(WIFI_STA);
  Serial.printf("\nBoot %s | CELLS %dx%d | PIXELS %dx%d\n", SELF_NAME, CELLS_X,
                CELLS_Y, PIXELS_X, PIXELS_Y);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_register_recv_cb(OnDataSideRecv);

  // Seed RNG and ADC
  randomSeed(esp_random());
  analogReadResolution(9);

  // ***** DEBUG INITIAL PATTERN *****
  // Fill with a bold diagonal-stripe pattern for instant visual ID.
  for (int y = 0; y < CELLS_Y; ++y) {
    for (int x = 0; x < CELLS_X; ++x) {
      // Four-level diagonal "barcode"
      uint8_t stripe = ((x + y) & 3) * (CELL_LIFETIME / 3);
      board[INDEX(y, x)] = stripe;
    }
  }
  // Force a bright horizontal bar at row 0 and a dim bar at last row
  for (int x = 0; x < CELLS_X; ++x) {
    board[INDEX(0, x)] = CELL_LIFETIME;                // top edge bright
    board[INDEX(CELLS_Y - 1, x)] = CELL_LIFETIME / 6;  // bottom edge dim
  }
  // Give the leftmost & rightmost columns obvious patterns
  for (int y = 0; y < CELLS_Y; ++y) {
    board[INDEX(y, 0)] = (uint8_t)((uint32_t)y * CELL_LIFETIME /
                                   (CELLS_Y - 1));  // vertical gradient
    board[INDEX(y, CELLS_X - 1)] =
        (uint8_t)(((CELLS_Y - 1 - y) * CELL_LIFETIME) /
                  (CELLS_Y - 1));  // inverted gradient
  }

#ifdef EAST_MAC
  memset(&peerInfoEast, 0, sizeof(peerInfoEast));
  memcpy(peerInfoEast.peer_addr, eastAddress, 6);
  peerInfoEast.channel = 0;
  peerInfoEast.encrypt = false;
  if (esp_now_add_peer(&peerInfoEast) == ESP_OK) {
    Serial.print("Peer EAST added: ");
    for (int i = 0; i < 6; ++i)
      Serial.printf("%02X%s", eastAddress[i], (i < 5 ? ":" : ""));
    Serial.println();
  } else {
    Serial.println("Failed to add EAST peer");
  }
#endif

#ifdef WEST_MAC
  memset(&peerInfoWest, 0, sizeof(peerInfoWest));
  memcpy(peerInfoWest.peer_addr, westAddress, 6);
  peerInfoWest.channel = 0;
  peerInfoWest.encrypt = false;
  if (esp_now_add_peer(&peerInfoWest) == ESP_OK) {
    Serial.print("Peer WEST added: ");
    for (int i = 0; i < 6; ++i)
      Serial.printf("%02X%s", westAddress[i], (i < 5 ? ":" : ""));
    Serial.println();
  } else {
    Serial.println("Failed to add WEST peer");
  }
#endif

  Serial.println("ESP-NOW ready.");
}

// -----------------------------------------------------------------------------
// Evolution with EAST integration for right-edge lookups
// -----------------------------------------------------------------------------
void evolve() {
  memcpy(board_copy, board, sizeof(uint8_t) * CELLS_Y * CELLS_X);

  auto cell_at = [&](int yy, int xx) -> uint8_t {
    yy = wrapY(yy);

    if (xx >= CELLS_X) {
#ifdef EAST_MAC
      if (eastValid) return eastInbound[yy];  // use neighbor’s leftmost column
#endif
      xx = xx % CELLS_X;  // fallback wrap
    } else if (xx < 0) {
      xx = (xx % CELLS_X + CELLS_X) % CELLS_X;
    }
    return board[INDEX(yy, xx)];
  };

  for (int y = 0; y < CELLS_Y; ++y) {
    for (int x = 0; x < CELLS_X; ++x) {
      uint8_t current_state = board[INDEX(y, x)];

      uint8_t total_n = 0;
      for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
          if (dx == 0 && dy == 0) continue;
          if (cell_at(y + dy, x + dx) == STATE_ALIVE) total_n++;
        }
      }

      uint8_t next = current_state;
      if (current_state == STATE_DEAD) {
        if ((born_rule >> total_n) & 1) next = STATE_ALIVE;
      } else if (current_state == STATE_ALIVE) {
        if (!((survive_rule >> total_n) & 1)) next = STATE_DEAD;
      } else {
        next = current_state - 1;  // decay
      }
      board_copy[INDEX(y, x)] = next;
    }
  }

  memcpy(board, board_copy, sizeof(uint8_t) * CELLS_Y * CELLS_X);
}

// -----------------------------------------------------------------------------
// Render + Debug overlays
//  - Leftmost pixel column: our board column x=0 (TX to WEST).
//  - Rightmost pixel column: EAST inbound preview (RX from EAST).
//  - Second-to-last pixel column: our board column x=last (TX to EAST).
//  - Top-left pixel bright if eastValid.
// -----------------------------------------------------------------------------
void render(uint8_t** frameBufferLines, int color_multiplier) {
  for (int y = 0; y < PIXELS_Y; ++y) {
    for (int x = 0; x < PIXELS_X; ++x) {
      int cy = (y / CELL_SIZE_Y);
      int cx = (x / CELL_SIZE_X);
      int index = INDEX(cy, cx);
      uint8_t v = (uint16_t)board[index] * color_multiplier / CELL_LIFETIME;
      frameBufferLines[y][x] = v;
    }
  }

  // Leftmost video column = our leftmost board column (what we TX to WEST)
  for (int y = 0; y < PIXELS_Y; ++y) {
    int cy = (y / CELL_SIZE_Y);
    frameBufferLines[y][0] = toByte(board[INDEX(cy, 0)]);
  }

  // Second-to-last video column = our rightmost board column (what we TX to
  // EAST)
  for (int y = 0; y < PIXELS_Y; ++y) {
    int cy = (y / CELL_SIZE_Y);
    frameBufferLines[y][PIXELS_X - 2] = toByte(board[INDEX(cy, CELLS_X - 1)]);
  }

  // Rightmost video column = inbound from EAST (if present)
#ifdef EAST_MAC
  if (eastValid) {
    for (int y = 0; y < PIXELS_Y; ++y) {
      int cy = (y / CELL_SIZE_Y);
      frameBufferLines[y][PIXELS_X - 1] = toByte(eastInbound[cy]);
    }
  } else {
    // Dark if nothing received yet
    for (int y = 0; y < PIXELS_Y; ++y) frameBufferLines[y][PIXELS_X - 1] = 0;
  }
#else
  for (int y = 0; y < PIXELS_Y; ++y) frameBufferLines[y][PIXELS_X - 1] = 0;
#endif

  // Indicator pixel: top-left bright when we have eastValid (first RX happened)
  frameBufferLines[0][0] = eastValid ? 255 : frameBufferLines[0][0];

  video_out.waitForFrame();
}

// -----------------------------------------------------------------------------
// Optional center-line stimulator (unchanged)
// -----------------------------------------------------------------------------
void generate_center_line(uint8_t thickness) {
  int y_from = CELLS_Y / 2 - 1 * thickness;
  int y_to = CELLS_Y / 2 + 1 * thickness;

  for (int y = y_from; y < y_to; y++) {
    if (y < 0 || y >= CELLS_Y) continue;
    for (int x = 0; x < CELLS_X; x++) {
      board[INDEX(y, x)] = random(0, CELL_LIFETIME);
    }
  }
}

// -----------------------------------------------------------------------------
// Main loop
// -----------------------------------------------------------------------------
void loop() {
  uint8_t** frame_buffer = video_out.getFrameBufferLines();
  frameNo++;

  // Strong per-role animated column so the neighbor sees obvious motion:
#if defined(WEST_MAC) && !defined(EAST_MAC)
  // Right/EAST device: animate its LEFTMOST column (this goes WEST)
  for (int y = 0; y < CELLS_Y; ++y) {
    uint8_t g =
        (uint8_t)((((y + (frameNo % CELLS_Y)) % CELLS_Y) * CELL_LIFETIME) /
                  (CELLS_Y - 1));
    board[INDEX(y, 0)] = g;
  }
#elif defined(EAST_MAC) && !defined(WEST_MAC)
  // Left/WEST device: animate its RIGHTMOST column (this goes EAST)
  for (int y = 0; y < CELLS_Y; ++y) {
    uint8_t g =
        (uint8_t)((((y + (frameNo % CELLS_Y)) % CELLS_Y) * CELL_LIFETIME) /
                  (CELLS_Y - 1));
    board[INDEX(y, CELLS_X - 1)] = g;
  }
#endif

  int center_line_force = map(analogRead(input_pot_4_pin), 0, 511, 0, 15);

  render(frame_buffer, 255);
  evolve();

  if (center_line_force > 0) {
    generate_center_line(center_line_force);
  }

  // ---- SEND: our LEFTMOST column to WEST neighbor ---------------------------
#ifdef WEST_MAC
  {
    Neighborhood pkt(CELLS_Y);
    strcpy(pkt.name, "L->W");
    uint32_t sum = 0;
    for (int y = 0; y < CELLS_Y; ++y) {
      pkt.data[y] = board[INDEX(y, 0)];
      sum += pkt.data[y];
    }
    esp_err_t res = esp_now_send(westAddress, (uint8_t*)&pkt, sizeof(pkt));
    if (res == ESP_OK) {
      txCountLtoW++;
      Serial.printf("[TX L->W] checksum=%u count=%lu\n", sum,
                    (unsigned long)txCountLtoW);
    } else {
      Serial.println("[TX L->W] FAILED");
    }
  }
#endif

  // ---- SEND: our RIGHTMOST column to EAST neighbor --------------------------
#ifdef EAST_MAC
  {
    Neighborhood pkt(CELLS_Y);
    strcpy(pkt.name, "R->E");
    uint32_t sum = 0;
    for (int y = 0; y < CELLS_Y; ++y) {
      pkt.data[y] = board[INDEX(y, CELLS_X - 1)];
      sum += pkt.data[y];
    }
    esp_err_t res = esp_now_send(eastAddress, (uint8_t*)&pkt, sizeof(pkt));
    if (res == ESP_OK) {
      txCountRtoE++;
      Serial.printf("[TX R->E] checksum=%u count=%lu\n", sum,
                    (unsigned long)txCountRtoE);
    } else {
      Serial.println("[TX R->E] FAILED");
    }
  }
#endif

  // Status every ~1s
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint > 1000) {
    lastPrint = millis();
#ifdef EAST_MAC
    Serial.printf("Status %s | eastValid=%d rxE=%lu (last %ums ago)\n",
                  SELF_NAME, eastValid ? 1 : 0, (unsigned long)rxCountE,
                  (unsigned int)(millis() - lastRxE_ms));
#else
    Serial.printf("Status %s\n", SELF_NAME);
#endif
  }

  delay(30);  // ~33 FPS
}
