// =============================================================
//  config.h — House of Ancient Secrets
//  Pin definitions, WiFi config, timing constants, room names
// =============================================================
#ifndef CONFIG_H
#define CONFIG_H

// =============================================================
//  WIFI MODE SELECTION FLAG
//  ─────────────────────────────────────────────────────────────
//  Set USE_AP_MODE to true  → ESP creates its own hotspot
//  Set USE_AP_MODE to false → ESP joins an existing WiFi network
// =============================================================
#define USE_AP_MODE false

// Station Mode WiFi Credentials (used when USE_AP_MODE = false)
#define STA_SSID "Joy"
#define STA_PASS "123456789"

// Access Point Credentials (used when USE_AP_MODE = true)
#define AP_SSID "HouseOfAncientSecrets"
#define AP_PASS "123456789"

// =============================================================
//  GPIO PIN DEFINITIONS
// =============================================================

// ── Room LDR Sensors ──
// Digital Input, Active LOW — output goes LOW when laser hits
#define PIN_LDR_ROOM1 4  // Room 1 — Dakshineswar Kali Temple
#define PIN_LDR_ROOM2 5  // Room 2 — Swami Vivekananda
#define PIN_LDR_ROOM3 6  // Room 3 — Ramakrishna Paramahamsa
#define PIN_LDR_ROOM4 7  // Room 4 — Maa Sharada Devi
#define PIN_LDR_ROOM5 15 // Room 5 — Belur Math

// ── Quiz LDR Sensors ──
// Digital Input, Active LOW — output goes LOW when laser hits
#define PIN_LDR_QUIZ_A 16 // Quiz Option A
#define PIN_LDR_QUIZ_B 17 // Quiz Option B
#define PIN_LDR_QUIZ_C 18 // Quiz Option C

// ── IR Sensor ──
// Digital Input, Active LOW — output goes LOW when object detected
#define PIN_IR_SENSOR 12 // Between Room 5 & Room 6

// ── NeoPixel LED Strips ──
// 5 separate data lines, all strips act as ONE unified lighting system
// (split across 5 pins only for power distribution reasons)
#define PIN_NEO1 8  // Strip segment 1  (~20 LEDs)
#define PIN_NEO2 3  // Strip segment 2  (~20 LEDs)
#define PIN_NEO3 9  // Strip segment 3  (~20 LEDs)
#define PIN_NEO4 10 // Strip segment 4  (~20 LEDs)
#define PIN_NEO5 11 // Strip segment 5  (~20 LEDs)
#define NEO_LEDS_PER_STRIP 20

// ── Buzzer ──
#define PIN_BUZZER 13 // Passive buzzer (PWM via LEDC)

// ── Servo Motors ──
#define PIN_SERVO1 37 // Belur Math linear actuator #1
#define PIN_SERVO2 36 // Belur Math linear actuator #2
#define PIN_SERVO3 35 // Reception table flip servo

// ── OLED Display (I2C) ──
#define PIN_OLED_SDA 21
#define PIN_OLED_SCL 20
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDR 0x3C // Typical SSD1306 I2C address

// =============================================================
//  COUNTS
// =============================================================
#define NUM_STRIPS 5
#define NUM_ROOMS 5
#define NUM_QUIZ_OPTIONS 3

// =============================================================
//  DEFAULT GAME SETTINGS (overridden by admin portal config)
// =============================================================
#define DEFAULT_GAME_TIMER 300  // 5 minutes (seconds)
#define DEFAULT_QUIZ_TIME 30    // 30 seconds per question
#define DEFAULT_PTS_ROOM 100    // Points per room hit
#define DEFAULT_PTS_QUIZ 200    // Points for correct quiz answer
#define DEFAULT_PTS_TIME_MULT 2 // Time bonus = remaining_sec × this

// =============================================================
//  EFFECT TIMING CONSTANTS (milliseconds)
// =============================================================
#define BLINK_INTERVAL_MS 300    // NeoPixel on/off toggle speed
#define BLINK_COUNT 5            // Number of red blinks on room hit
#define BUZZER_FREQ_HIT 1000     // Hz — room hit buzzer tone
#define BUZZER_FREQ_CORRECT 1500 // Hz — quiz correct tone
#define BUZZER_FREQ_WRONG 400    // Hz — quiz wrong tone

#define ROOM_NAME_DISPLAY_MS 3000   // Show room name on OLED (ms)
#define PTS_DISPLAY_MS 2000         // Show "+100 pts" on OLED (ms)
#define OLED_CYCLE_INTERVAL_MS 2500 // OLED info cycle interval (ms)
#define QUIZ_RESULT_DISPLAY_MS 3000 // Show quiz result (ms)
#define GAME_OVER_DISPLAY_MS 5000   // Show game over screen (ms)

// Servo sweep configuration
#define SERVO_STEP_DELAY_MS 30 // ms between 1° steps (controls speed)
#define BELUR_SERVO_START 10   // Belur Math actuator start angle
#define BELUR_SERVO_END 170    // Belur Math actuator end angle
#define RECEPTION_ANGLE 0      // Servo 3 — Reception position
#define QUIZ_TABLE_ANGLE 180   // Servo 3 — Quiz room position

// LDR / IR debounce
#define LDR_DEBOUNCE_MS 200 // Ignore re-triggers within this window
#define IR_DEBOUNCE_MS 500  // IR sensor debounce

// =============================================================
//  NEOPIXEL COLORS (GRB format via Adafruit_NeoPixel)
// =============================================================
// These are defined as macros using strip.Color() at runtime
// because strip.Color() is not constexpr
#define COLOR_OFF_R 0
#define COLOR_OFF_G 0
#define COLOR_OFF_B 0

#define COLOR_ORANGE_R 255
#define COLOR_ORANGE_G 140
#define COLOR_ORANGE_B 0

#define COLOR_RED_R 255
#define COLOR_RED_G 0
#define COLOR_RED_B 0

#define COLOR_GREEN_R 0
#define COLOR_GREEN_G 255
#define COLOR_GREEN_B 0

// =============================================================
//  ROOM NAMES (indexed 0–4)
// =============================================================
const char *const ROOM_NAMES[] = {
    "Dakshineswar Kali Temple", "Swami Vivekananda", "Ramakrishna Paramahamsa",
    "Maa Sharada Devi", "Belur Math"};

// Room LDR pin array (indexed 0–4, matching ROOM_NAMES)
const int ROOM_LDR_PINS[] = {PIN_LDR_ROOM1, PIN_LDR_ROOM2, PIN_LDR_ROOM3,
                             PIN_LDR_ROOM4, PIN_LDR_ROOM5};

// Quiz LDR pin array (indexed 0–2 for options A, B, C)
const int QUIZ_LDR_PINS[] = {PIN_LDR_QUIZ_A, PIN_LDR_QUIZ_B, PIN_LDR_QUIZ_C};

// NeoPixel data pin array
const int NEO_PINS[] = {PIN_NEO1, PIN_NEO2, PIN_NEO3, PIN_NEO4, PIN_NEO5};

#endif // CONFIG_H
