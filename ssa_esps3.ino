// =============================================================
//  ssa_esps3.ino — House of Ancient Secrets
//  Main sketch: Game state machine, setup, loop
//  ─────────────────────────────────────────────
//  WRO 2026 | Team: House of Ancient Secrets
//  Theme: India's Spiritual Heritage
//  Board: ESP32-S3 N16R8
// =============================================================

#include "admin_config.h"
#include "config.h"
#include "hardware.h"
#include "webserver.h"

// =============================================================
//  GAME STATE MACHINE
// =============================================================
enum GameState {
  STATE_IDLE,         // Waiting for "Start Game" from web dashboard
  STATE_GAME_ACTIVE,  // Game in progress — monitoring room LDRs
  STATE_ROOM_HIT,     // A room was hit — playing blink/buzzer effect
  STATE_BELUR_RISING, // Room 5 hit — servos raising Belur Math model
  STATE_WAITING_EXIT, // Belur raised — waiting for car at IR sensor
  STATE_TABLE_FLIP,   // IR triggered — flipping reception table
  STATE_QUIZ,         // Quiz mode — showing question, waiting for answer
  STATE_QUIZ_RESULT,  // Showing quiz correct/wrong result
  STATE_GAME_OVER     // Game finished — showing final score
};

GameState gameState = STATE_IDLE;

// =============================================================
//  GAME VARIABLES
// =============================================================
bool roomHit[NUM_ROOMS] = {false}; // Track which rooms are hit
int roomsHitCount = 0;
int currentScore = 0;
int roomPoints = 0;       // Accumulated room points
int quizPointsEarned = 0; // Points from quiz
int timeBonus = 0;        // Calculated at game over

// Timer
unsigned long gameStartTime = 0;
int gameDurationSec = DEFAULT_GAME_TIMER;
int remainingSeconds = DEFAULT_GAME_TIMER;
unsigned long lastTimerUpdate = 0;

// Room hit effect
int hitRoomIndex = -1; // Which room was just hit

// Quiz
int quizQuestionIdx = -1; // Index of selected question
unsigned long quizStartTime = 0;
int quizTimeLimitSec = DEFAULT_QUIZ_TIME;
unsigned long lastQuizTimerUpdate = 0;

// Quiz result display
unsigned long quizResultTimer = 0;

// Game over display
unsigned long gameOverTimer = 0;

// OLED cycling
unsigned long oledLastCycle = 0;
int oledDisplayMode = 0; // Cycles through different info

// =============================================================
//  FORWARD DECLARATIONS
// =============================================================
String getGameStatusJson();
void onWebSocketAction(const String &action);
void startGame();
void endGame();
void resetGame();
void broadcastRoomHit(int roomIndex);
void broadcastQuizQuestion();
void broadcastQuizAnswer(bool correct, int selectedOption);
void broadcastGameOver();
void updateGameTimer();
void updateOLEDCycle();

// =============================================================
//  SETUP
// =============================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("╔══════════════════════════════════════════╗");
  Serial.println("║    HOUSE OF ANCIENT SECRETS — ESP32-S3   ║");
  Serial.println("║    WRO 2026 | India's Spiritual Heritage ║");
  Serial.println("╚══════════════════════════════════════════╝");
  Serial.println();

  // Initialize persistent config from NVS flash
  loadConfig();

  // Initialize all hardware (NeoPixels, servos, buzzer, OLED, sensors)
  initHardware();

  // Show welcome on OLED
  oledWelcome();

  // Initialize WiFi (AP or Station mode)
  initWiFi();

  // Show IP on OLED briefly
  if (USE_AP_MODE) {
    oledShowIP("AP Mode", espIPAddress.c_str());
  } else {
    oledShowIP("Station Mode", espIPAddress.c_str());
  }
  delay(3000); // Show IP for 3 seconds at boot

  // Show welcome again
  oledWelcome();

  // Initialize web server and WebSocket
  initWebServer();

  Serial.println();
  Serial.println("[READY] System initialized. Open dashboard at:");
  Serial.printf("[READY]   http://%s/\n", espIPAddress.c_str());
  Serial.printf("[READY]   http://%s/hologram  (phone)\n",
                espIPAddress.c_str());
  Serial.printf("[READY]   http://%s/admin     (config)\n",
                espIPAddress.c_str());
  Serial.println();
}

// =============================================================
//  MAIN LOOP
// =============================================================
void loop() {
  // WebSocket housekeeping
  cleanupWebSocket();

  // Update non-blocking effects (always running)
  bool blinkDone = updateBlinkEffect();
  updateRainbowEffect();
  bool belurDone = updateBelurServo();
  bool flipDone = updateTableFlip();

  // Game timer
  updateGameTimer();

  // State machine
  switch (gameState) {
  case STATE_IDLE:
    handleIdle();
    break;

  case STATE_GAME_ACTIVE:
    handleGameActive();
    break;

  case STATE_ROOM_HIT:
    handleRoomHit(blinkDone);
    break;

  case STATE_BELUR_RISING:
    handleBelurRising(belurDone);
    break;

  case STATE_WAITING_EXIT:
    handleWaitingExit();
    break;

  case STATE_TABLE_FLIP:
    handleTableFlip_State(flipDone);
    break;

  case STATE_QUIZ:
    handleQuiz();
    break;

  case STATE_QUIZ_RESULT:
    handleQuizResult_State();
    break;

  case STATE_GAME_OVER:
    handleGameOverState();
    break;
  }

  // OLED cycling (non-blocking)
  updateOLEDCycle();
}

// =============================================================
//  STATE HANDLERS
// =============================================================

// ── IDLE: Waiting for game start ──
void handleIdle() {
  // Nothing to do — waiting for WebSocket "start" action
  // OLED shows welcome message (handled by cycling)
}

// ── GAME ACTIVE: Monitor room LDRs ──
void handleGameActive() {
  int hitRoom = checkRoomLDRs(roomHit);

  if (hitRoom >= 0) {
    // A room was just hit!
    roomHit[hitRoom] = true;
    roomsHitCount++;
    hitRoomIndex = hitRoom;

    // Award points
    roomPoints += gameConfig.ptsRoom;
    currentScore = roomPoints + quizPointsEarned;

    Serial.printf("[GAME] Room %d (%s) HIT! Score: %d\n", hitRoom + 1,
                  ROOM_NAMES[hitRoom], currentScore);

    // Start blink + buzzer effect
    startRoomHitEffect();

    // Broadcast to web clients
    broadcastRoomHit(hitRoom);

    // Show on OLED
    oledRoomHit(hitRoom, gameConfig.ptsRoom);
    oledSetTemp(String(ROOM_NAMES[hitRoom]), ROOM_NAME_DISPLAY_MS);

    gameState = STATE_ROOM_HIT;
  }
}

// ── ROOM HIT: Playing blink/buzzer effect, wait for completion ──
void handleRoomHit(bool blinkDone) {
  if (blinkDone) {
    Serial.println("[GAME] Room hit effect done.");

    // If it was Room 5 (Belur Math), start servo sequence
    if (hitRoomIndex == 4) {
      Serial.println("[GAME] → Belur Math sequence starting...");
      startBelurServo();
      broadcastEvent("belur_rising");
      oledShowCentered("BELUR MATH", 1, "RISING!", 2);
      gameState = STATE_BELUR_RISING;
    } else {
      // Return to active game
      neoOrangeGlow(); // Restore orange glow
      gameState = STATE_GAME_ACTIVE;
    }
  }
}

// ── BELUR RISING: Servos raising the model ──
void handleBelurRising(bool belurDone) {
  if (belurDone) {
    Serial.println("[GAME] Belur Math fully raised. Waiting for car at IR...");
    oledShowCentered("WAITING", 1, "for car...", 1);
    gameState = STATE_WAITING_EXIT;
  }
}

// ── WAITING EXIT: Car must pass IR sensor ──
void handleWaitingExit() {
  if (checkIRSensor()) {
    Serial.println("[GAME] Car detected at IR sensor! Flipping table...");
    startTableFlip();
    broadcastEvent("car_detected");
    oledShowCentered("TABLE", 1, "FLIPPING!", 2);
    gameState = STATE_TABLE_FLIP;
  }
}

// ── TABLE FLIP: Reception → Quiz Room ──
void handleTableFlip_State(bool flipDone) {
  if (flipDone) {
    Serial.println("[GAME] Table flipped to Quiz Room!");

    // Enter quiz mode
    enterQuizMode();
    gameState = STATE_QUIZ;
  }
}

// ── QUIZ: Show question, wait for LDR answer ──
void handleQuiz() {
  // Check quiz timer
  unsigned long elapsed = (millis() - quizStartTime) / 1000;
  int remaining = quizTimeLimitSec - (int)elapsed;

  // Broadcast quiz timer updates every second
  if (millis() - lastQuizTimerUpdate >= 1000) {
    lastQuizTimerUpdate = millis();
    JsonDocument doc;
    doc["event"] = "quiz_timer";
    doc["remaining"] = max(0, remaining);
    String output;
    serializeJson(doc, output);
    broadcastWS(output);
  }

  if (remaining <= 0) {
    // Time's up — count as wrong
    Serial.println("[QUIZ] Time expired!");
    processQuizAnswer(-1); // -1 = no answer (wrong)
    return;
  }

  // Check quiz LDRs
  int answer = checkQuizLDRs();
  if (answer >= 0) {
    Serial.printf("[QUIZ] Option %c selected!\n", 'A' + answer);
    processQuizAnswer(answer);
  }
}

// ── QUIZ RESULT: Showing correct/wrong briefly ──
void handleQuizResult_State() {
  if (millis() - quizResultTimer >= QUIZ_RESULT_DISPLAY_MS) {
    // Move to game over
    stopRainbowEffect();
    endGame();
  }
}

// ── GAME OVER: Final score display ──
void handleGameOverState() {
  // Just wait — game stays in this state until reset from web
}

// =============================================================
//  GAME CONTROL FUNCTIONS
// =============================================================

void startGame() {
  if (gameState != STATE_IDLE) {
    Serial.println("[GAME] Cannot start — game already in progress.");
    return;
  }

  Serial.println("[GAME] ===== GAME STARTING =====");

  // Reset game state
  for (int i = 0; i < NUM_ROOMS; i++)
    roomHit[i] = false;
  roomsHitCount = 0;
  currentScore = 0;
  roomPoints = 0;
  quizPointsEarned = 0;
  timeBonus = 0;
  hitRoomIndex = -1;
  quizQuestionIdx = -1;

  // Load timer from config
  gameDurationSec = gameConfig.gameTimer;
  remainingSeconds = gameDurationSec;
  gameStartTime = millis();
  lastTimerUpdate = millis();

  // Turn on NeoPixels (orangish-yellow default glow)
  neoOrangeGlow();

  // Reset servos
  resetServos();

  // Short start beep
  buzzerTone(1200, 100);

  // Broadcast game start
  {
    JsonDocument doc;
    doc["event"] = "game_start";
    doc["timer"] = gameDurationSec;
    String output;
    serializeJson(doc, output);
    broadcastWS(output);
  }

  gameState = STATE_GAME_ACTIVE;
  Serial.printf("[GAME] Timer: %d seconds. GO!\n", gameDurationSec);
}

void endGame() {
  Serial.println("[GAME] ===== GAME OVER =====");

  // Calculate time bonus
  timeBonus = remainingSeconds * gameConfig.ptsTimeMult;
  currentScore = roomPoints + quizPointsEarned + timeBonus;

  Serial.printf("[GAME] Room points  : %d\n", roomPoints);
  Serial.printf("[GAME] Quiz points  : %d\n", quizPointsEarned);
  Serial.printf("[GAME] Time bonus   : %d (%d sec × %d)\n", timeBonus,
                remainingSeconds, gameConfig.ptsTimeMult);
  Serial.printf("[GAME] FINAL SCORE  : %d\n", currentScore);

  // Show on OLED
  oledGameOver(currentScore);

  // NeoPixel game over effect
  neoAllOff();
  // Quick victory flash
  for (int i = 0; i < 3; i++) {
    neoSetAll(COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B);
    delay(100);
    neoSetAll(COLOR_ORANGE_R, COLOR_ORANGE_G, COLOR_ORANGE_B);
    delay(100);
  }
  neoAllOff();

  // Victory buzzer
  buzzerVictory();

  // Broadcast
  broadcastGameOver();

  gameOverTimer = millis();
  gameState = STATE_GAME_OVER;
}

void resetGame() {
  Serial.println("[GAME] ===== SYSTEM RESET =====");

  // Stop all effects
  neoEffectActive = false;
  neoRainbowActive = false;
  buzzerEffectActive = false;
  neoAllOff();
  buzzerOff();

  // Reset servos to start positions
  resetServos();

  // Reset game variables
  for (int i = 0; i < NUM_ROOMS; i++)
    roomHit[i] = false;
  roomsHitCount = 0;
  currentScore = 0;
  roomPoints = 0;
  quizPointsEarned = 0;
  timeBonus = 0;
  remainingSeconds = gameConfig.gameTimer;
  hitRoomIndex = -1;
  quizQuestionIdx = -1;

  // Show welcome
  oledWelcome();

  // Broadcast reset
  broadcastEvent("game_reset");

  gameState = STATE_IDLE;
  Serial.println("[GAME] Ready for new game.");
}

// =============================================================
//  QUIZ MODE
// =============================================================

void enterQuizMode() {
  Serial.println("[QUIZ] Entering quiz mode!");

  // Pick a random question
  if (quizQuestionCount > 0) {
    quizQuestionIdx = random(0, quizQuestionCount);
  } else {
    Serial.println("[QUIZ] ERROR: No quiz questions available!");
    endGame();
    return;
  }

  quizTimeLimitSec = gameConfig.quizTime;
  quizStartTime = millis();
  lastQuizTimerUpdate = millis();

  // Start rainbow effect on NeoPixels
  startRainbowEffect();

  // Show question on OLED (condensed)
  QuizQuestion &q = quizQuestions[quizQuestionIdx];
  oledQuizQuestion(q.question.c_str(), q.optionA.c_str(), q.optionB.c_str(),
                   q.optionC.c_str());

  // Broadcast quiz start + question
  broadcastEvent("quiz_start");
  broadcastQuizQuestion();
}

void processQuizAnswer(int selectedOption) {
  QuizQuestion &q = quizQuestions[quizQuestionIdx];
  bool correct = (selectedOption == q.correctAnswer);

  if (correct) {
    quizPointsEarned = gameConfig.ptsQuiz;
    currentScore = roomPoints + quizPointsEarned;
    Serial.printf("[QUIZ] CORRECT! +%d points\n", quizPointsEarned);
    oledQuizResult(true, quizPointsEarned);

    // Green flash + happy buzzer
    stopRainbowEffect();
    flashColor(COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B, 3);
    buzzerCorrect();
  } else {
    quizPointsEarned = 0;
    Serial.println("[QUIZ] WRONG! 0 points");
    oledQuizResult(false, 0);

    // Red flash + sad buzzer
    stopRainbowEffect();
    flashColor(COLOR_RED_R, COLOR_RED_G, COLOR_RED_B, 2);
    buzzerWrong();
  }

  // Broadcast result
  broadcastQuizAnswer(correct, selectedOption);

  quizResultTimer = millis();
  gameState = STATE_QUIZ_RESULT;
}

// =============================================================
//  GAME TIMER
// =============================================================
void updateGameTimer() {
  if (gameState == STATE_IDLE || gameState == STATE_GAME_OVER)
    return;

  unsigned long now = millis();
  if (now - lastTimerUpdate >= 1000) {
    lastTimerUpdate = now;

    unsigned long elapsed = (now - gameStartTime) / 1000;
    remainingSeconds = gameDurationSec - (int)elapsed;
    if (remainingSeconds < 0)
      remainingSeconds = 0;

    // Broadcast timer update every 5 seconds (reduce WS traffic)
    static int timerBroadcastCounter = 0;
    timerBroadcastCounter++;
    if (timerBroadcastCounter >= 5) {
      timerBroadcastCounter = 0;
      JsonDocument doc;
      doc["event"] = "timer_update";
      doc["remaining"] = remainingSeconds;
      String output;
      serializeJson(doc, output);
      broadcastWS(output);
    }

    // Time's up — end game
    if (remainingSeconds <= 0 && gameState != STATE_QUIZ_RESULT &&
        gameState != STATE_GAME_OVER) {
      Serial.println("[GAME] TIME'S UP!");
      stopRainbowEffect();
      endGame();
    }
  }
}

// =============================================================
//  OLED CYCLING DISPLAY
// =============================================================
void updateOLEDCycle() {
  // Don't cycle if a temp message is showing
  if (oledTempActive) {
    if (millis() > oledTempExpiry) {
      oledTempActive = false;
    } else {
      return; // Temp message still active
    }
  }

  // Only cycle in GAME_ACTIVE state
  if (gameState != STATE_GAME_ACTIVE)
    return;

  unsigned long now = millis();
  if (now - oledLastCycle >= OLED_CYCLE_INTERVAL_MS) {
    oledLastCycle = now;
    oledDisplayMode = (oledDisplayMode + 1) % 3;

    switch (oledDisplayMode) {
    case 0: // Score
      oledGameStatus(currentScore, remainingSeconds, roomsHitCount);
      break;
    case 1: { // Room status
      oled.clearDisplay();
      oled.setTextColor(SSD1306_WHITE);
      oled.setTextSize(1);
      oled.setCursor(0, 0);
      oled.println("ROOM STATUS:");
      oled.println();
      for (int i = 0; i < NUM_ROOMS; i++) {
        oled.printf("%d.%s %s\n", i + 1, roomHit[i] ? "[X]" : "[ ]",
                    i == 0   ? "Dakshines."
                    : i == 1 ? "Vivekananda"
                    : i == 2 ? "Ramakrishna"
                    : i == 3 ? "Sharada Devi"
                             : "Belur Math");
      }
      oled.display();
      break;
    }
    case 2: { // Timer (large)
      int mins = remainingSeconds / 60;
      int secs = remainingSeconds % 60;
      char buf[8];
      snprintf(buf, sizeof(buf), "%d:%02d", mins, secs);
      oledShowCentered("TIME LEFT", 1, buf, 2);
      break;
    }
    }
  }
}

// =============================================================
//  WEBSOCKET BROADCAST HELPERS
// =============================================================

void broadcastRoomHit(int roomIndex) {
  JsonDocument doc;
  doc["event"] = "room_hit";
  doc["room"] = roomIndex;
  doc["name"] = ROOM_NAMES[roomIndex];
  doc["pts"] = gameConfig.ptsRoom;
  doc["total"] = currentScore;
  doc["roomsHit"] = roomsHitCount;
  String output;
  serializeJson(doc, output);
  broadcastWS(output);
}

void broadcastQuizQuestion() {
  if (quizQuestionIdx < 0 || quizQuestionIdx >= quizQuestionCount)
    return;
  QuizQuestion &q = quizQuestions[quizQuestionIdx];

  JsonDocument doc;
  doc["event"] = "quiz_question";
  doc["question"] = q.question;
  doc["optA"] = q.optionA;
  doc["optB"] = q.optionB;
  doc["optC"] = q.optionC;
  doc["quizTime"] = quizTimeLimitSec;
  String output;
  serializeJson(doc, output);
  broadcastWS(output);
}

void broadcastQuizAnswer(bool correct, int selectedOption) {
  JsonDocument doc;
  doc["event"] = "quiz_answer";
  doc["correct"] = correct;
  doc["selected"] = selectedOption;
  if (quizQuestionIdx >= 0) {
    doc["correctIdx"] = quizQuestions[quizQuestionIdx].correctAnswer;
  }
  doc["quizPts"] = quizPointsEarned;
  doc["total"] = currentScore;
  String output;
  serializeJson(doc, output);
  broadcastWS(output);
}

void broadcastGameOver() {
  JsonDocument doc;
  doc["event"] = "game_over";
  doc["score"] = currentScore;
  doc["roomPts"] = roomPoints;
  doc["quizPts"] = quizPointsEarned;
  doc["timeBonus"] = timeBonus;
  doc["remaining"] = remainingSeconds;
  String output;
  serializeJson(doc, output);
  broadcastWS(output);
}

// =============================================================
//  GET GAME STATUS JSON (for new WS connections and API)
// =============================================================
String getGameStatusJson() {
  JsonDocument doc;
  doc["event"] = "status";

  // Map game state to string
  const char *stateStr = "idle";
  switch (gameState) {
  case STATE_IDLE:
    stateStr = "idle";
    break;
  case STATE_GAME_ACTIVE:
    stateStr = "active";
    break;
  case STATE_ROOM_HIT:
    stateStr = "hit_effect";
    break;
  case STATE_BELUR_RISING:
    stateStr = "belur";
    break;
  case STATE_WAITING_EXIT:
    stateStr = "wait_exit";
    break;
  case STATE_TABLE_FLIP:
    stateStr = "table_flip";
    break;
  case STATE_QUIZ:
    stateStr = "quiz";
    break;
  case STATE_QUIZ_RESULT:
    stateStr = "quiz";
    break;
  case STATE_GAME_OVER:
    stateStr = "over";
    break;
  }
  doc["state"] = stateStr;
  doc["score"] = currentScore;
  doc["remaining"] = remainingSeconds;

  JsonArray roomsArr = doc["roomsHit"].to<JsonArray>();
  for (int i = 0; i < NUM_ROOMS; i++) {
    roomsArr.add(roomHit[i]);
  }

  String output;
  serializeJson(doc, output);
  return output;
}

// =============================================================
//  WEBSOCKET ACTION HANDLER (called from webserver.h)
// =============================================================
void onWebSocketAction(const String &action) {
  Serial.printf("[WS] Action received: %s\n", action.c_str());

  if (action == "start") {
    startGame();
  } else if (action == "end") {
    if (gameState != STATE_IDLE && gameState != STATE_GAME_OVER) {
      stopRainbowEffect();
      endGame();
    }
  } else if (action == "reset") {
    resetGame();
  } else {
    Serial.printf("[WS] Unknown action: %s\n", action.c_str());
  }
}
