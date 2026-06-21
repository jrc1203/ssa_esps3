// =============================================================
//  hardware.h — Hardware abstraction layer
//  Controls: NeoPixel strips, Servos, Buzzer, OLED, Sensors
//  All functions are NON-BLOCKING (millis-based)
// =============================================================
#ifndef HARDWARE_H
#define HARDWARE_H

#include "config.h"
#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <Wire.h>

// ── LEDC Buzzer compatibility ──
// ESP32 Arduino Core 2.x uses channel-based API
// ESP32 Arduino Core 3.x uses pin-based API
#if ESP_ARDUINO_VERSION_MAJOR >= 3
// Core 3.x — ledcAttach(pin, freq, resolution)
#define BUZZER_LEDC_INIT() // No-op, ledcAttach does it
#define BUZZER_TONE_START(freq)                                                \
  do {                                                                         \
    ledcAttach(PIN_BUZZER, (freq), 8);                                         \
    ledcWrite(PIN_BUZZER, 128);                                                \
  } while (0)
#define BUZZER_TONE_STOP()                                                     \
  do {                                                                         \
    ledcWrite(PIN_BUZZER, 0);                                                  \
  } while (0)
#define BUZZER_DETACH()                                                        \
  do {                                                                         \
    ledcDetach(PIN_BUZZER);                                                    \
  } while (0)
#define BUZZER_DUTY(d) ledcWrite(PIN_BUZZER, (d))
#else
// Core 2.x — ledcSetup(channel, freq, resolution) + ledcAttachPin(pin, channel)
#define BUZZER_LEDC_CHANNEL 0
#define BUZZER_LEDC_INIT()                                                     \
  do {                                                                         \
    ledcSetup(BUZZER_LEDC_CHANNEL, 1000, 8);                                   \
    ledcAttachPin(PIN_BUZZER, BUZZER_LEDC_CHANNEL);                            \
  } while (0)
#define BUZZER_TONE_START(freq)                                                \
  do {                                                                         \
    ledcWriteTone(BUZZER_LEDC_CHANNEL, (freq));                                \
  } while (0)
#define BUZZER_TONE_STOP()                                                     \
  do {                                                                         \
    ledcWriteTone(BUZZER_LEDC_CHANNEL, 0);                                     \
  } while (0)
#define BUZZER_DETACH()                                                        \
  do {                                                                         \
    ledcDetachPin(PIN_BUZZER);                                                 \
  } while (0)
#define BUZZER_DUTY(d) ledcWrite(BUZZER_LEDC_CHANNEL, (d))
#endif

// =============================================================
//  NEOPIXEL — 5 strips, all acting as one unified display
// =============================================================
// NOTE: ESP32-S3 has 4 RMT TX channels. With 5 strips, the
// library may use bitbang for the 5th strip. We update strips
// sequentially so this works fine in practice.
Adafruit_NeoPixel strips[NUM_STRIPS] = {
    Adafruit_NeoPixel(NEO_LEDS_PER_STRIP, PIN_NEO1, NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(NEO_LEDS_PER_STRIP, PIN_NEO2, NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(NEO_LEDS_PER_STRIP, PIN_NEO3, NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(NEO_LEDS_PER_STRIP, PIN_NEO4, NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(NEO_LEDS_PER_STRIP, PIN_NEO5, NEO_GRB + NEO_KHZ800)};

// =============================================================
//  SERVO MOTORS
// =============================================================
Servo servo1; // Belur Math actuator #1
Servo servo2; // Belur Math actuator #2
Servo servo3; // Reception table flip

// =============================================================
//  OLED DISPLAY
// =============================================================
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// =============================================================
//  EFFECT STATE TRACKING (non-blocking)
// =============================================================

// ── NeoPixel blink effect ──
bool neoEffectActive = false;
unsigned long neoEffectTimer = 0;
bool neoBlinkOn = false;
int neoBlinksDone = 0;
uint32_t neoBlinkColor = 0; // Color to blink
uint32_t neoRestColor = 0;  // Color after blink finishes
int neoTargetBlinks = BLINK_COUNT;

// ── Rainbow/flash effect for quiz mode ──
bool neoRainbowActive = false;
unsigned long neoRainbowTimer = 0;
int neoRainbowStep = 0;

// ── Buzzer effect (synced with NeoPixel blink) ──
bool buzzerEffectActive = false;
unsigned long buzzerEffectTimer = 0;
bool buzzerOn = false;
int buzzerBeepsDone = 0;
int buzzerFrequency = BUZZER_FREQ_HIT;
int buzzerTargetBeeps = BLINK_COUNT;

// ── Servo sweep ──
bool servo12Active = false; // Belur Math servos
int servo12CurrentAngle = BELUR_SERVO_START;
int servo12TargetAngle = BELUR_SERVO_END;
unsigned long servo12Timer = 0;

bool servo3Active = false; // Reception flip servo
int servo3CurrentAngle = RECEPTION_ANGLE;
int servo3TargetAngle = QUIZ_TABLE_ANGLE;
unsigned long servo3Timer = 0;

// ── OLED cycling ──
int oledCycleIndex = 0;
unsigned long oledCycleTimer = 0;
String oledTempMessage = "";
unsigned long oledTempExpiry = 0;
bool oledTempActive = false;

// ── LDR debounce ──
unsigned long lastRoomHitTime[NUM_ROOMS] = {0};
unsigned long lastQuizHitTime[NUM_QUIZ_OPTIONS] = {0};
unsigned long lastIRTriggerTime = 0;

// =============================================================
//  HELPER: Set all NeoPixel strips to a single color
// =============================================================
void neoSetAll(uint8_t r, uint8_t g, uint8_t b) {
  for (int s = 0; s < NUM_STRIPS; s++) {
    for (int i = 0; i < NEO_LEDS_PER_STRIP; i++) {
      strips[s].setPixelColor(i, strips[s].Color(r, g, b));
    }
    strips[s].show();
  }
}

// Set all strips to a 32-bit color
void neoSetAllColor(uint32_t color) {
  for (int s = 0; s < NUM_STRIPS; s++) {
    for (int i = 0; i < NEO_LEDS_PER_STRIP; i++) {
      strips[s].setPixelColor(i, color);
    }
    strips[s].show();
  }
}

// Turn all strips off
void neoAllOff() { neoSetAll(COLOR_OFF_R, COLOR_OFF_G, COLOR_OFF_B); }

// Set all strips to orangish-yellow (game default)
void neoOrangeGlow() {
  neoSetAll(COLOR_ORANGE_R, COLOR_ORANGE_G, COLOR_ORANGE_B);
}

// =============================================================
//  HELPER: Wheel function for rainbow effects
// =============================================================
uint32_t neoWheel(byte pos) {
  pos = 255 - pos;
  if (pos < 85) {
    return strips[0].Color(255 - pos * 3, 0, pos * 3);
  } else if (pos < 170) {
    pos -= 85;
    return strips[0].Color(0, pos * 3, 255 - pos * 3);
  } else {
    pos -= 170;
    return strips[0].Color(pos * 3, 255 - pos * 3, 0);
  }
}

// =============================================================
//  INIT ALL HARDWARE
// =============================================================
void initHardware() {
  // ── NeoPixel strips ──
  for (int s = 0; s < NUM_STRIPS; s++) {
    strips[s].begin();
    strips[s].setBrightness(120);
    strips[s].show(); // Start with all off
  }
  neoAllOff();

  // ── Room LDR pins (digital input with pull-up) ──
  for (int i = 0; i < NUM_ROOMS; i++) {
    pinMode(ROOM_LDR_PINS[i], INPUT_PULLUP);
  }

  // ── Quiz LDR pins ──
  for (int i = 0; i < NUM_QUIZ_OPTIONS; i++) {
    pinMode(QUIZ_LDR_PINS[i], INPUT_PULLUP);
  }

  // ── IR sensor ──
  pinMode(PIN_IR_SENSOR, INPUT_PULLUP);

  // ── Buzzer (LEDC) ──
  BUZZER_LEDC_INIT();

  // ── Servos ──
  servo1.attach(PIN_SERVO1);
  servo2.attach(PIN_SERVO2);
  servo3.attach(PIN_SERVO3);
  servo1.write(BELUR_SERVO_START);
  servo2.write(BELUR_SERVO_START);
  servo3.write(RECEPTION_ANGLE);

  // ── OLED ──
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[OLED] ERROR: SSD1306 init failed!");
  } else {
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    oled.display();
    Serial.println("[OLED] Initialized OK.");
  }

  Serial.println("[HARDWARE] All hardware initialized.");
}

// =============================================================
//  NEOPIXEL BLINK EFFECT — Start
//  Blinks all strips between blinkColor and OFF, then settles
//  on restColor. Synced with buzzer.
// =============================================================
void startBlinkEffect(uint8_t blinkR, uint8_t blinkG, uint8_t blinkB,
                      uint8_t restR, uint8_t restG, uint8_t restB,
                      int numBlinks, int buzzerFreq) {
  neoEffectActive = true;
  neoEffectTimer = millis();
  neoBlinkOn = true;
  neoBlinksDone = 0;
  neoTargetBlinks = numBlinks;
  neoBlinkColor = strips[0].Color(blinkR, blinkG, blinkB);
  neoRestColor = strips[0].Color(restR, restG, restB);

  // Start first blink immediately
  neoSetAllColor(neoBlinkColor);

  // Start synced buzzer
  buzzerEffectActive = true;
  buzzerEffectTimer = millis();
  buzzerOn = true;
  buzzerBeepsDone = 0;
  buzzerFrequency = buzzerFreq;
  buzzerTargetBeeps = numBlinks;
  BUZZER_TONE_START(buzzerFreq);
}

// Convenience: standard room hit effect (red blink → orange)
void startRoomHitEffect() {
  startBlinkEffect(COLOR_RED_R, COLOR_RED_G, COLOR_RED_B, // blink color: red
                   COLOR_ORANGE_R, COLOR_ORANGE_G,
                   COLOR_ORANGE_B, // rest color: orange
                   BLINK_COUNT, BUZZER_FREQ_HIT);
}

// =============================================================
//  NEOPIXEL BLINK EFFECT — Update (call every loop iteration)
//  Returns true when the blink sequence is DONE.
// =============================================================
bool updateBlinkEffect() {
  if (!neoEffectActive)
    return false;

  unsigned long now = millis();
  if (now - neoEffectTimer >= BLINK_INTERVAL_MS) {
    neoEffectTimer = now;
    neoBlinkOn = !neoBlinkOn;

    if (neoBlinkOn) {
      neoSetAllColor(neoBlinkColor);
      // Buzzer ON
      if (buzzerEffectActive && buzzerBeepsDone < buzzerTargetBeeps) {
        BUZZER_TONE_START(buzzerFrequency);
      }
    } else {
      neoSetAll(COLOR_OFF_R, COLOR_OFF_G, COLOR_OFF_B);
      // Buzzer OFF
      BUZZER_TONE_STOP();
      neoBlinksDone++;

      if (neoBlinksDone >= neoTargetBlinks) {
        // Blink sequence complete — settle to rest color
        neoSetAllColor(neoRestColor);
        neoEffectActive = false;

        // Stop buzzer
        buzzerEffectActive = false;
        buzzerBeepsDone = buzzerTargetBeeps;
        BUZZER_TONE_STOP();
        BUZZER_DETACH();
        return true; // Signal: effect done
      }
    }
  }
  return false;
}

// =============================================================
//  NEOPIXEL RAINBOW EFFECT — For quiz mode
// =============================================================
void startRainbowEffect() {
  neoRainbowActive = true;
  neoRainbowTimer = millis();
  neoRainbowStep = 0;
}

void stopRainbowEffect() { neoRainbowActive = false; }

void updateRainbowEffect() {
  if (!neoRainbowActive)
    return;

  unsigned long now = millis();
  if (now - neoRainbowTimer >= 50) { // Update every 50ms
    neoRainbowTimer = now;
    neoRainbowStep = (neoRainbowStep + 3) % 256;

    for (int s = 0; s < NUM_STRIPS; s++) {
      for (int i = 0; i < NEO_LEDS_PER_STRIP; i++) {
        int pixelHue = (neoRainbowStep + (i * 256 / NEO_LEDS_PER_STRIP)) % 256;
        strips[s].setPixelColor(i, neoWheel(pixelHue));
      }
      strips[s].show();
    }
  }
}

// Quick flash for quiz correct/wrong
void flashColor(uint8_t r, uint8_t g, uint8_t b, int count) {
  // This is a simple blocking flash — only used for brief quiz result feedback
  for (int i = 0; i < count; i++) {
    neoSetAll(r, g, b);
    delay(150);
    neoAllOff();
    delay(150);
  }
}

// =============================================================
//  BUZZER — Simple tone helpers
// =============================================================
void buzzerTone(int freq, int durationMs) {
  BUZZER_TONE_START(freq);
  delay(durationMs);
  BUZZER_TONE_STOP();
}

void buzzerOff() { BUZZER_TONE_STOP(); }

// Short happy jingle for correct answer
void buzzerCorrect() {
  buzzerTone(BUZZER_FREQ_CORRECT, 100);
  delay(50);
  buzzerTone(BUZZER_FREQ_CORRECT + 500, 200);
}

// Sad tone for wrong answer
void buzzerWrong() { buzzerTone(BUZZER_FREQ_WRONG, 300); }

// Victory jingle for game over
void buzzerVictory() {
  int notes[] = {523, 659, 784, 1047}; // C5, E5, G5, C6
  for (int i = 0; i < 4; i++) {
    buzzerTone(notes[i], 150);
    delay(50);
  }
}

// =============================================================
//  SERVO SWEEP — Start Belur Math actuators (non-blocking)
// =============================================================
void startBelurServo() {
  servo12Active = true;
  servo12CurrentAngle = BELUR_SERVO_START;
  servo12TargetAngle = BELUR_SERVO_END;
  servo12Timer = millis();
  servo1.write(servo12CurrentAngle);
  servo2.write(servo12CurrentAngle);
  Serial.printf("[SERVO] Belur Math rising: %d° → %d°\n", BELUR_SERVO_START,
                BELUR_SERVO_END);
}

// Returns true when sweep is complete
bool updateBelurServo() {
  if (!servo12Active)
    return false;

  unsigned long now = millis();
  if (now - servo12Timer >= SERVO_STEP_DELAY_MS) {
    servo12Timer = now;

    if (servo12CurrentAngle < servo12TargetAngle) {
      servo12CurrentAngle++;
      servo1.write(servo12CurrentAngle);
      servo2.write(servo12CurrentAngle);
    } else {
      servo12Active = false;
      Serial.println("[SERVO] Belur Math fully raised.");
      return true;
    }
  }
  return false;
}

// =============================================================
//  SERVO SWEEP — Start Reception table flip (non-blocking)
// =============================================================
void startTableFlip() {
  servo3Active = true;
  servo3CurrentAngle = RECEPTION_ANGLE;
  servo3TargetAngle = QUIZ_TABLE_ANGLE;
  servo3Timer = millis();
  servo3.write(servo3CurrentAngle);
  Serial.printf("[SERVO] Table flipping: %d° → %d°\n", RECEPTION_ANGLE,
                QUIZ_TABLE_ANGLE);
}

// Returns true when flip is complete
bool updateTableFlip() {
  if (!servo3Active)
    return false;

  unsigned long now = millis();
  if (now - servo3Timer >= SERVO_STEP_DELAY_MS) {
    servo3Timer = now;

    if (servo3CurrentAngle < servo3TargetAngle) {
      servo3CurrentAngle++;
      servo3.write(servo3CurrentAngle);
    } else {
      servo3Active = false;
      Serial.println("[SERVO] Table flip complete — Quiz Room!");
      return true;
    }
  }
  return false;
}

// =============================================================
//  RESET SERVOS to starting positions
// =============================================================
void resetServos() {
  servo1.write(BELUR_SERVO_START);
  servo2.write(BELUR_SERVO_START);
  servo3.write(RECEPTION_ANGLE);
  servo12Active = false;
  servo3Active = false;
  servo12CurrentAngle = BELUR_SERVO_START;
  servo3CurrentAngle = RECEPTION_ANGLE;
  Serial.println("[SERVO] All servos reset.");
}

// =============================================================
//  SENSOR READING FUNCTIONS
// =============================================================

// Check if a room LDR was hit (active LOW, with debounce)
// Returns room index (0–4) or -1 if none hit
int checkRoomLDRs(const bool roomHit[NUM_ROOMS]) {
  unsigned long now = millis();
  for (int i = 0; i < NUM_ROOMS; i++) {
    if (roomHit[i])
      continue; // Already hit this game, skip
    if (digitalRead(ROOM_LDR_PINS[i]) == LOW) {
      if (now - lastRoomHitTime[i] >= LDR_DEBOUNCE_MS) {
        lastRoomHitTime[i] = now;
        return i;
      }
    }
  }
  return -1;
}

// Check if a quiz LDR was hit (active LOW, with debounce)
// Returns option index (0=A, 1=B, 2=C) or -1 if none
int checkQuizLDRs() {
  unsigned long now = millis();
  for (int i = 0; i < NUM_QUIZ_OPTIONS; i++) {
    if (digitalRead(QUIZ_LDR_PINS[i]) == LOW) {
      if (now - lastQuizHitTime[i] >= LDR_DEBOUNCE_MS) {
        lastQuizHitTime[i] = now;
        return i;
      }
    }
  }
  return -1;
}

// Check IR sensor (active LOW, with debounce)
bool checkIRSensor() {
  unsigned long now = millis();
  if (digitalRead(PIN_IR_SENSOR) == LOW) {
    if (now - lastIRTriggerTime >= IR_DEBOUNCE_MS) {
      lastIRTriggerTime = now;
      return true;
    }
  }
  return false;
}

// =============================================================
//  OLED DISPLAY FUNCTIONS
// =============================================================

// Show centered text with specified size
void oledShowCentered(const char *line1, int size1, const char *line2 = nullptr,
                      int size2 = 1, const char *line3 = nullptr,
                      int size3 = 1) {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);

  // Calculate vertical positioning
  int totalHeight = size1 * 8;
  if (line2)
    totalHeight += size2 * 8 + 4;
  if (line3)
    totalHeight += size3 * 8 + 4;
  int startY = (OLED_HEIGHT - totalHeight) / 2;
  if (startY < 0)
    startY = 0;

  // Line 1
  oled.setTextSize(size1);
  int16_t x1, y1;
  uint16_t w1, h1;
  oled.getTextBounds(line1, 0, 0, &x1, &y1, &w1, &h1);
  oled.setCursor((OLED_WIDTH - w1) / 2, startY);
  oled.print(line1);

  // Line 2
  if (line2) {
    int y2 = startY + size1 * 8 + 4;
    oled.setTextSize(size2);
    oled.getTextBounds(line2, 0, 0, &x1, &y1, &w1, &h1);
    oled.setCursor((OLED_WIDTH - w1) / 2, y2);
    oled.print(line2);

    // Line 3
    if (line3) {
      int y3 = y2 + size2 * 8 + 4;
      oled.setTextSize(size3);
      oled.getTextBounds(line3, 0, 0, &x1, &y1, &w1, &h1);
      oled.setCursor((OLED_WIDTH - w1) / 2, y3);
      oled.print(line3);
    }
  }

  oled.display();
}

// Show welcome screen
void oledWelcome() {
  oledShowCentered("HOUSE OF", 1, "ANCIENT", 2, "SECRETS", 1);
}

// Show game score and time
void oledGameStatus(int score, int remainingSec, int roomsHit) {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);

  // Score (large)
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print("SCORE:");
  oled.setTextSize(2);
  oled.setCursor(50, 0);
  oled.print(score);

  // Time
  int mins = remainingSec / 60;
  int secs = remainingSec % 60;
  oled.setTextSize(1);
  oled.setCursor(0, 24);
  oled.printf("Time: %d:%02d", mins, secs);

  // Rooms hit
  oled.setCursor(0, 36);
  oled.printf("Rooms: %d / %d", roomsHit, NUM_ROOMS);

  // Room status icons
  oled.setCursor(0, 50);
  oled.print("[ ");
  // Will be called with actual room status from game state
  oled.print("]");

  oled.display();
}

// Show room hit notification
void oledRoomHit(int roomIndex, int pointsAdded) {
  char roomBuf[26];
  snprintf(roomBuf, sizeof(roomBuf), "%s", ROOM_NAMES[roomIndex]);

  char ptsBuf[16];
  snprintf(ptsBuf, sizeof(ptsBuf), "+%d pts!", pointsAdded);

  oledShowCentered("ROOM HIT!", 1, roomBuf, 1, ptsBuf, 2);
}

// Show quiz question (condensed for 128x64)
void oledQuizQuestion(const char *question, const char *optA, const char *optB,
                      const char *optC) {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);

  // Question (truncated to fit ~2 lines)
  oled.setCursor(0, 0);
  // Print up to 42 chars (2 lines × 21 chars)
  char qBuf[43];
  strncpy(qBuf, question, 42);
  qBuf[42] = '\0';
  oled.print(qBuf);

  // Separator
  oled.setCursor(0, 20);
  oled.print("--------------------");

  // Options
  char aBuf[20], bBuf[20], cBuf[20];
  snprintf(aBuf, 20, "A) %s", optA);
  snprintf(bBuf, 20, "B) %s", optB);
  snprintf(cBuf, 20, "C) %s", optC);

  oled.setCursor(0, 28);
  oled.print(aBuf);
  oled.setCursor(0, 40);
  oled.print(bBuf);
  oled.setCursor(0, 52);
  oled.print(cBuf);

  oled.display();
}

// Show quiz result
void oledQuizResult(bool correct, int pointsAdded) {
  if (correct) {
    oledShowCentered("CORRECT!", 2, (String("+") + String(pointsAdded)).c_str(),
                     2);
  } else {
    oledShowCentered("WRONG!", 2, "0 pts", 1);
  }
}

// Show game over
void oledGameOver(int finalScore) {
  char scoreBuf[16];
  snprintf(scoreBuf, sizeof(scoreBuf), "%d", finalScore);
  oledShowCentered("GAME OVER", 2, scoreBuf, 2, "points");
}

// Show IP address on OLED
void oledShowIP(const char *mode, const char *ip) {
  oledShowCentered(mode, 1, ip, 1, "Open in browser");
}

// Set a temporary OLED message that auto-expires
void oledSetTemp(const String &msg, unsigned long durationMs) {
  oledTempMessage = msg;
  oledTempExpiry = millis() + durationMs;
  oledTempActive = true;
}

#endif // HARDWARE_H
