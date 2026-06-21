// =============================================================
//  LASER TRIPWIRE SECURITY SYSTEM - Arduino Sketch
//  Components: LDR, LED, Passive Buzzer, Relay, Push Button,
//              NeoPixel LED Strip (120 LEDs on Pin 6)
//  Logic: Non-blocking (millis-based), State Machine approach
// =============================================================

#include <Adafruit_NeoPixel.h>

// ─────────────────────────────────────────────
//  PIN DEFINITIONS
// ─────────────────────────────────────────────
#define PIN_LDR       A0   // LDR connected to analog pin A0
#define PIN_LED       8    // Single LED on digital pin 8
#define PIN_BUZZER    9    // Passive buzzer on pin 9 (PWM capable)
#define PIN_RELAY     7    // Relay module IN pin on digital pin 7
#define PIN_BUTTON    2    // Push button on digital pin 2 (INPUT_PULLUP)

#define NEO_PIN       6    // NeoPixel data pin
#define NEO_COUNT     20  // Number of NeoPixel LEDs

// ─────────────────────────────────────────────
//  NEOPIXEL SETUP
// ─────────────────────────────────────────────
Adafruit_NeoPixel strip(NEO_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);

// NeoPixel blink tracking (used in TRIGGERED state)
unsigned long neoBlinkTimer    = 0;
bool          neoBlinkState    = false;  // true = ON, false = OFF
int           neoBlinksDone    = 0;
bool          neoBlinkFinished = false;

#define NEO_BLINK_INTERVAL  400   // ON/OFF speed in ms (camera-friendly)
#define NEO_BLINK_COUNT     6     // Number of blinks before going steady

// NeoPixel colors
#define NEO_COLOR_BLINK   strip.Color(255, 0,  0)   // Amber blink
#define NEO_COLOR_STEADY  strip.Color(255, 80, 0)   // Warm glow (LOCKED)
#define NEO_COLOR_OFF     strip.Color(0,   0,  0)    // OFF
#define NEO_COLOR_CALIBRATE strip.Color(255,   255,  255)
// ─────────────────────────────────────────────
//  TIMING CONSTANTS (all in milliseconds)
// ─────────────────────────────────────────────
#define CALIBRATION_DURATION    3000
#define CALIBRATION_BLINK_ON    150
#define CALIBRATION_BLINK_OFF   150
#define CALIBRATION_BEEP_DUR    300

#define BEEP_ON_DURATION        400
#define BEEP_OFF_DURATION       400
#define BEEP_COUNT              6

#define RELAY_BLINK_ON          300
#define RELAY_BLINK_OFF         300
#define RELAY_BLINK_COUNT       3

#define THRESHOLD_MARGIN        80

#define BEEP_FREQUENCY          1000
#define CALIB_BEEP_FREQUENCY    1500

#define DEBOUNCE_DELAY          50

// ─────────────────────────────────────────────
//  STATE MACHINE STATES
// ─────────────────────────────────────────────
enum SystemState {
  CALIBRATING,
  ARMED,
  TRIGGERED,
  LOCKED
};

SystemState currentState = CALIBRATING;

// ─────────────────────────────────────────────
//  CALIBRATION VARIABLES
// ─────────────────────────────────────────────
#define LDR_SAMPLE_INTERVAL     250

unsigned long calibrationStartTime = 0;
unsigned long lastLDRSampleTime    = 0;
long          ldrSum               = 0;
int           ldrReadCount         = 0;
int           ldrThreshold         = 0;

// ─────────────────────────────────────────────
//  TRIGGER ALERT SEQUENCE VARIABLES
// ─────────────────────────────────────────────
int           beepsDone            = 0;
bool          buzzerOn             = false;
unsigned long buzzerTimer          = 0;

int           relayBlinksDone      = 0;
bool          relayOn              = false;
unsigned long relayTimer           = 0;

bool          alertSequenceDone    = false;

// ─────────────────────────────────────────────
//  CALIBRATION FEEDBACK VARIABLES
// ─────────────────────────────────────────────
bool          calibFeedbackDone    = false;
bool          calibLedOn           = false;
unsigned long calibFeedbackTimer   = 0;
int           calibBlinkCount      = 0;
#define       CALIB_BLINK_TIMES    4

// ─────────────────────────────────────────────
//  BUTTON DEBOUNCE VARIABLES
// ─────────────────────────────────────────────
bool          lastButtonState      = HIGH;
unsigned long lastDebounceTime     = 0;

// =============================================================
//  HELPER: Set all NeoPixel LEDs to a single color
// =============================================================
void neoSetAll(uint32_t color) {
  for (int i = 0; i < NEO_COUNT; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

// =============================================================
//  SETUP
// =============================================================
void setup() {
  Serial.begin(9600);

  pinMode(PIN_LED,    OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_RELAY,  OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  digitalWrite(PIN_LED,   LOW);
  digitalWrite(PIN_RELAY, LOW);
  noTone(PIN_BUZZER);

  // NeoPixel init – start fully OFF
  strip.begin();
  strip.setBrightness(100);
  neoSetAll(NEO_COLOR_OFF);

  calibrationStartTime = millis();
  Serial.println("=== LASER TRIPWIRE SYSTEM STARTED ===");
  Serial.println("[CALIBRATING] Reading ambient LDR for 5 seconds...");
}

// =============================================================
//  MAIN LOOP
// =============================================================
void loop() {
  handleButton();

  switch (currentState) {
    case CALIBRATING: handleCalibration(); break;
    case ARMED:       handleArmed();       break;
    case TRIGGERED:   handleTriggered();   break;
    case LOCKED:      handleLocked();      break;
  }
}

// =============================================================
//  BUTTON HANDLER (non-blocking debounce)
// =============================================================
void handleButton() {
  bool reading = digitalRead(PIN_BUTTON);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading == LOW) {
      resetSystem();
    }
  }

  lastButtonState = reading;
}

// =============================================================
//  STATE: CALIBRATING
// =============================================================
void handleCalibration() {
  unsigned long now     = millis();
  unsigned long elapsed = now - calibrationStartTime;

  if (!calibFeedbackDone) {
    if (elapsed < CALIBRATION_DURATION) {
      if (now - lastLDRSampleTime >= LDR_SAMPLE_INTERVAL) {
        lastLDRSampleTime = now;
        int ldrVal = analogRead(PIN_LDR);
        ldrSum += ldrVal;
        ldrReadCount++;
        Serial.print("  [Sample "); Serial.print(ldrReadCount);
        Serial.print("] LDR = ");   Serial.println(ldrVal);
      }
    }
    else if (ldrReadCount > 0 && !calibFeedbackDone) {
      int avgLDR   = ldrSum / ldrReadCount;
      ldrThreshold = avgLDR + THRESHOLD_MARGIN;

      Serial.println();
      Serial.println("[CALIBRATION DONE]");
      Serial.print("  Samples collected : "); Serial.println(ldrReadCount);
      Serial.print("  Average LDR value : "); Serial.println(avgLDR);
      Serial.print("  Threshold set to  : "); Serial.println(ldrThreshold);
      Serial.println("[FEEDBACK] Blinking LED and beeping...");

      calibFeedbackTimer = millis();
      calibBlinkCount    = 0;
      calibLedOn         = true;
      digitalWrite(PIN_LED, HIGH);
      neoSetAll(NEO_COLOR_CALIBRATE);
      tone(PIN_BUZZER, CALIB_BEEP_FREQUENCY, CALIBRATION_BEEP_DUR);
      calibFeedbackDone  = true;
    }
  }
  else {
    unsigned long feedbackElapsed = millis() - calibFeedbackTimer;
    unsigned long blinkPeriod     = CALIBRATION_BLINK_ON + CALIBRATION_BLINK_OFF;
    int cycle = feedbackElapsed / blinkPeriod;

    if (cycle < CALIB_BLINK_TIMES) {
      unsigned long posInCycle = feedbackElapsed % blinkPeriod;
      digitalWrite(PIN_LED, posInCycle < CALIBRATION_BLINK_ON ? HIGH : LOW);
    }
    else {
      digitalWrite(PIN_LED, LOW);
      noTone(PIN_BUZZER);
      Serial.println("[ARMED] System is now monitoring for laser break...");
      currentState = ARMED;
    }
  }
}

// =============================================================
//  STATE: ARMED
//  NeoPixel stays OFF. Triggers on LDR spike.
// =============================================================
void handleArmed() {
  int ldrVal = analogRead(PIN_LDR);

  if (ldrVal > ldrThreshold) {
    Serial.println("[TRIGGER DETECTED] LDR spike detected!");
    Serial.print("  LDR value   : "); Serial.println(ldrVal);
    Serial.print("  Threshold   : "); Serial.println(ldrThreshold);

    // Reset alert counters
    beepsDone         = 0;
    relayBlinksDone   = 0;
    buzzerOn          = false;
    relayOn           = false;
    alertSequenceDone = false;
    buzzerTimer       = millis();
    relayTimer        = millis();

    // Reset NeoPixel blink state
    neoBlinksDone    = 0;
    neoBlinkState    = false;
    neoBlinkFinished = false;
    neoBlinkTimer    = millis();

    currentState = TRIGGERED;
  }
}

// =============================================================
//  STATE: TRIGGERED
//  Beeps + relay blinks + NeoPixel blinks simultaneously,
//  then latches LED + Relay + NeoPixel ON → LOCKED
// =============================================================
void handleTriggered() {
  unsigned long now = millis();

  // ── BUZZER: 3 beeps ──
  if (beepsDone < BEEP_COUNT) {
    if (!buzzerOn) {
      if (now - buzzerTimer >= BEEP_OFF_DURATION) {
        tone(PIN_BUZZER, BEEP_FREQUENCY);
        buzzerOn    = true;
        buzzerTimer = now;
      }
    } else {
      if (now - buzzerTimer >= BEEP_ON_DURATION) {
        noTone(PIN_BUZZER);
        buzzerOn    = false;
        buzzerTimer = now;
        beepsDone++;
        Serial.print("[BEEP] "); Serial.print(beepsDone);
        Serial.print("/"); Serial.println(BEEP_COUNT);
      }
    }
  }

  // ── RELAY: 3 blinks ──
  if (relayBlinksDone < RELAY_BLINK_COUNT) {
    if (!relayOn) {
      if (now - relayTimer >= RELAY_BLINK_OFF) {
        digitalWrite(PIN_RELAY, HIGH);
        relayOn    = true;
        relayTimer = now;
      }
    } else {
      if (now - relayTimer >= RELAY_BLINK_ON) {
        digitalWrite(PIN_RELAY, LOW);
        relayOn         = false;
        relayTimer      = now;
        relayBlinksDone++;
        Serial.print("[RELAY BLINK] "); Serial.print(relayBlinksDone);
        Serial.print("/"); Serial.println(RELAY_BLINK_COUNT);
      }
    }
  }

  // ── NEOPIXEL: 3 blinks (amber) ──
  if (!neoBlinkFinished) {
    if (now - neoBlinkTimer >= NEO_BLINK_INTERVAL) {
      neoBlinkTimer = now;
      neoBlinkState = !neoBlinkState;

      neoSetAll(neoBlinkState ? NEO_COLOR_BLINK : NEO_COLOR_OFF);

      // Count completed ON pulses
      if (neoBlinkState) {
        neoBlinksDone++;
        Serial.print("[NEO BLINK] "); Serial.print(neoBlinksDone);
        Serial.print("/"); Serial.println(NEO_BLINK_COUNT);
        if (neoBlinksDone >= NEO_BLINK_COUNT) {
          neoBlinkFinished = true;
        }
      }
    }
  }

  // ── All three sequences done → LOCK ──
  if (beepsDone >= BEEP_COUNT &&
      relayBlinksDone >= RELAY_BLINK_COUNT &&
      neoBlinkFinished &&
      !alertSequenceDone) {

    alertSequenceDone = true;

    digitalWrite(PIN_RELAY, HIGH);
    digitalWrite(PIN_LED,   HIGH);
    noTone(PIN_BUZZER);

    // NeoPixel latches to steady warm glow
    neoSetAll(NEO_COLOR_STEADY);

    Serial.println("[LOCKED] Alert sequence complete. Relay + LED + NeoPixel latched ON.");
    Serial.println("[LOCKED] Press reset button to re-arm system.");
    currentState = LOCKED;
  }
}

// =============================================================
//  STATE: LOCKED
//  Idle. NeoPixel stays on steady warm glow.
// =============================================================
void handleLocked() {
  // Nothing to do. Button handler calls resetSystem() on press.
}

// =============================================================
//  RESET SYSTEM
// =============================================================
void resetSystem() {
  Serial.println();
  Serial.println("[RESET] Button pressed. Restarting calibration...");

  digitalWrite(PIN_LED,   LOW);
  digitalWrite(PIN_RELAY, LOW);
  noTone(PIN_BUZZER);

  // NeoPixel fully OFF on reset
  neoSetAll(NEO_COLOR_OFF);

  // Clear calibration data
  ldrSum             = 0;
  ldrReadCount       = 0;
  ldrThreshold       = 0;
  lastLDRSampleTime  = 0;
  calibFeedbackDone  = false;
  calibBlinkCount    = 0;

  // Clear alert state
  beepsDone          = 0;
  relayBlinksDone    = 0;
  buzzerOn           = false;
  relayOn            = false;
  alertSequenceDone  = false;

  // Clear NeoPixel blink state
  neoBlinksDone      = 0;
  neoBlinkState      = false;
  neoBlinkFinished   = false;

  calibrationStartTime = millis();
  currentState         = CALIBRATING;

  Serial.println("[CALIBRATING] Reading ambient LDR for 5 seconds...");
}