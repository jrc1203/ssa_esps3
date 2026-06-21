// =============================================================
//  admin_config.h — Persistent configuration via NVS (Preferences)
//  Stores game settings, quiz questions, and video URLs
//  so they survive reboots and don't require code re-upload.
// =============================================================
#ifndef ADMIN_CONFIG_H
#define ADMIN_CONFIG_H

#include "config.h"
#include <ArduinoJson.h>
#include <Preferences.h>

// Maximum number of quiz questions stored
#define MAX_QUIZ_QUESTIONS 20

// ─────────────────────────────────────────────
//  DATA STRUCTURES
// ─────────────────────────────────────────────

struct QuizQuestion {
  String question;
  String optionA;
  String optionB;
  String optionC;
  int correctAnswer; // 0 = A, 1 = B, 2 = C
};

struct GameConfig {
  int gameTimer;               // Total game time in seconds
  int quizTime;                // Time limit per quiz question (seconds)
  int ptsRoom;                 // Points awarded per room hit
  int ptsQuiz;                 // Points for correct quiz answer
  int ptsTimeMult;             // Time bonus multiplier (remaining_sec × this)
  String videoUrls[NUM_ROOMS]; // YouTube URLs for each room's hologram
};

// ─────────────────────────────────────────────
//  GLOBAL INSTANCES
// ─────────────────────────────────────────────
Preferences preferences;
GameConfig gameConfig;
QuizQuestion quizQuestions[MAX_QUIZ_QUESTIONS];
int quizQuestionCount = 0;

// ─────────────────────────────────────────────
//  FORWARD DECLARATIONS (functions used before defined)
// ─────────────────────────────────────────────
void loadDefaultQuestions();
void saveQuestions();
void saveConfig();

// ─────────────────────────────────────────────
//  HELPER: Extract YouTube Video ID from URL
//  Accepts full URLs or raw video IDs:
//    https://www.youtube.com/watch?v=dQw4w9WgXcQ
//    https://youtu.be/dQw4w9WgXcQ
//    dQw4w9WgXcQ  (raw ID)
// ─────────────────────────────────────────────
String extractYouTubeId(const String &url) {
  // Check for standard youtube.com/watch?v= format
  int vIdx = url.indexOf("v=");
  if (vIdx != -1) {
    String id = url.substring(vIdx + 2);
    int ampIdx = id.indexOf('&');
    if (ampIdx != -1)
      id = id.substring(0, ampIdx);
    return id;
  }
  // Check for youtu.be/ short format
  int beIdx = url.indexOf("youtu.be/");
  if (beIdx != -1) {
    String id = url.substring(beIdx + 9);
    int qIdx = id.indexOf('?');
    if (qIdx != -1)
      id = id.substring(0, qIdx);
    return id;
  }
  // Check for youtube.com/embed/ format
  int emIdx = url.indexOf("/embed/");
  if (emIdx != -1) {
    String id = url.substring(emIdx + 7);
    int qIdx = id.indexOf('?');
    if (qIdx != -1)
      id = id.substring(0, qIdx);
    return id;
  }
  // Assume it's already a raw video ID
  return url;
}

// ─────────────────────────────────────────────
//  LOAD CONFIG FROM NVS
// ─────────────────────────────────────────────
void loadConfig() {
  preferences.begin("hoas", true); // read-only

  gameConfig.gameTimer = preferences.getInt("gameTimer", DEFAULT_GAME_TIMER);
  gameConfig.quizTime = preferences.getInt("quizTime", DEFAULT_QUIZ_TIME);
  gameConfig.ptsRoom = preferences.getInt("ptsRoom", DEFAULT_PTS_ROOM);
  gameConfig.ptsQuiz = preferences.getInt("ptsQuiz", DEFAULT_PTS_QUIZ);
  gameConfig.ptsTimeMult =
      preferences.getInt("ptsTimeMult", DEFAULT_PTS_TIME_MULT);

  // Load video URLs
  for (int i = 0; i < NUM_ROOMS; i++) {
    String key = "video" + String(i);
    gameConfig.videoUrls[i] = preferences.getString(key.c_str(), "");
  }

  // Load quiz questions from JSON string
  String qJson = preferences.getString("questions", "");
  preferences.end();

  if (qJson.length() > 0) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, qJson);
    if (!err) {
      JsonArray arr = doc.as<JsonArray>();
      quizQuestionCount = 0;
      for (JsonObject obj : arr) {
        if (quizQuestionCount >= MAX_QUIZ_QUESTIONS)
          break;
        quizQuestions[quizQuestionCount].question = obj["q"].as<String>();
        quizQuestions[quizQuestionCount].optionA = obj["a"].as<String>();
        quizQuestions[quizQuestionCount].optionB = obj["b"].as<String>();
        quizQuestions[quizQuestionCount].optionC = obj["c"].as<String>();
        quizQuestions[quizQuestionCount].correctAnswer = obj["ans"].as<int>();
        quizQuestionCount++;
      }
    }
  }

  // If no questions saved, load defaults
  if (quizQuestionCount == 0) {
    loadDefaultQuestions();
  }

  Serial.println("[CONFIG] Loaded from NVS:");
  Serial.printf("  Game Timer : %d sec\n", gameConfig.gameTimer);
  Serial.printf("  Quiz Time  : %d sec\n", gameConfig.quizTime);
  Serial.printf("  Pts/Room   : %d\n", gameConfig.ptsRoom);
  Serial.printf("  Pts/Quiz   : %d\n", gameConfig.ptsQuiz);
  Serial.printf("  Time Mult  : %d\n", gameConfig.ptsTimeMult);
  Serial.printf("  Questions  : %d\n", quizQuestionCount);
}

// ─────────────────────────────────────────────
//  SAVE CONFIG TO NVS
// ─────────────────────────────────────────────
void saveConfig() {
  preferences.begin("hoas", false); // read-write

  preferences.putInt("gameTimer", gameConfig.gameTimer);
  preferences.putInt("quizTime", gameConfig.quizTime);
  preferences.putInt("ptsRoom", gameConfig.ptsRoom);
  preferences.putInt("ptsQuiz", gameConfig.ptsQuiz);
  preferences.putInt("ptsTimeMult", gameConfig.ptsTimeMult);

  // Save video URLs
  for (int i = 0; i < NUM_ROOMS; i++) {
    String key = "video" + String(i);
    preferences.putString(key.c_str(), gameConfig.videoUrls[i]);
  }

  preferences.end();
  Serial.println("[CONFIG] Game settings saved to NVS.");
}

// ─────────────────────────────────────────────
//  SAVE QUIZ QUESTIONS TO NVS
// ─────────────────────────────────────────────
void saveQuestions() {
  // Serialize questions array to JSON string
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();

  for (int i = 0; i < quizQuestionCount; i++) {
    JsonObject obj = arr.add<JsonObject>();
    obj["q"] = quizQuestions[i].question;
    obj["a"] = quizQuestions[i].optionA;
    obj["b"] = quizQuestions[i].optionB;
    obj["c"] = quizQuestions[i].optionC;
    obj["ans"] = quizQuestions[i].correctAnswer;
  }

  String output;
  serializeJson(doc, output);

  preferences.begin("hoas", false);
  preferences.putString("questions", output);
  preferences.end();

  Serial.printf("[CONFIG] Saved %d quiz questions to NVS.\n",
                quizQuestionCount);
}

// ─────────────────────────────────────────────
//  DEFAULT QUIZ QUESTIONS (3 demo questions)
// ─────────────────────────────────────────────
void loadDefaultQuestions() {
  quizQuestionCount = 3;

  quizQuestions[0].question = "Where is Dakshineswar Kali Temple located?";
  quizQuestions[0].optionA = "Mumbai";
  quizQuestions[0].optionB = "Kolkata";
  quizQuestions[0].optionC = "Chennai";
  quizQuestions[0].correctAnswer = 1; // B

  quizQuestions[1].question =
      "Swami Vivekananda's famous speech was at which event?";
  quizQuestions[1].optionA = "Olympics 1896";
  quizQuestions[1].optionB = "Parliament of Religions 1893";
  quizQuestions[1].optionC = "UN Assembly 1900";
  quizQuestions[1].correctAnswer = 1; // B

  quizQuestions[2].question =
      "Who was the spiritual guru of Swami Vivekananda?";
  quizQuestions[2].optionA = "Maa Sharada Devi";
  quizQuestions[2].optionB = "Ramakrishna Paramahamsa";
  quizQuestions[2].optionC = "Swami Brahmananda";
  quizQuestions[2].correctAnswer = 1; // B

  saveQuestions();
  Serial.println("[CONFIG] Default quiz questions loaded and saved.");
}

// ─────────────────────────────────────────────
//  BUILD CONFIG JSON (for API response)
// ─────────────────────────────────────────────
String getConfigJson() {
  JsonDocument doc;
  doc["gameTimer"] = gameConfig.gameTimer;
  doc["quizTime"] = gameConfig.quizTime;
  doc["ptsRoom"] = gameConfig.ptsRoom;
  doc["ptsQuiz"] = gameConfig.ptsQuiz;
  doc["ptsTimeMult"] = gameConfig.ptsTimeMult;

  JsonArray videos = doc["videos"].to<JsonArray>();
  for (int i = 0; i < NUM_ROOMS; i++) {
    videos.add(gameConfig.videoUrls[i]);
  }

  JsonArray questions = doc["questions"].to<JsonArray>();
  for (int i = 0; i < quizQuestionCount; i++) {
    JsonObject q = questions.add<JsonObject>();
    q["q"] = quizQuestions[i].question;
    q["a"] = quizQuestions[i].optionA;
    q["b"] = quizQuestions[i].optionB;
    q["c"] = quizQuestions[i].optionC;
    q["ans"] = quizQuestions[i].correctAnswer;
  }

  String output;
  serializeJson(doc, output);
  return output;
}

// ─────────────────────────────────────────────
//  PARSE CONFIG JSON (from admin POST)
// ─────────────────────────────────────────────
bool parseConfigJson(const String &json) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    Serial.printf("[CONFIG] JSON parse error: %s\n", err.c_str());
    return false;
  }

  // Update game settings
  if (doc.containsKey("gameTimer"))
    gameConfig.gameTimer = doc["gameTimer"].as<int>();
  if (doc.containsKey("quizTime"))
    gameConfig.quizTime = doc["quizTime"].as<int>();
  if (doc.containsKey("ptsRoom"))
    gameConfig.ptsRoom = doc["ptsRoom"].as<int>();
  if (doc.containsKey("ptsQuiz"))
    gameConfig.ptsQuiz = doc["ptsQuiz"].as<int>();
  if (doc.containsKey("ptsTimeMult"))
    gameConfig.ptsTimeMult = doc["ptsTimeMult"].as<int>();

  // Update video URLs
  if (doc.containsKey("videos")) {
    JsonArray videos = doc["videos"].as<JsonArray>();
    for (int i = 0; i < NUM_ROOMS && i < (int)videos.size(); i++) {
      gameConfig.videoUrls[i] = videos[i].as<String>();
    }
  }

  // Update quiz questions
  if (doc.containsKey("questions")) {
    JsonArray questions = doc["questions"].as<JsonArray>();
    quizQuestionCount = 0;
    for (JsonObject obj : questions) {
      if (quizQuestionCount >= MAX_QUIZ_QUESTIONS)
        break;
      quizQuestions[quizQuestionCount].question = obj["q"].as<String>();
      quizQuestions[quizQuestionCount].optionA = obj["a"].as<String>();
      quizQuestions[quizQuestionCount].optionB = obj["b"].as<String>();
      quizQuestions[quizQuestionCount].optionC = obj["c"].as<String>();
      quizQuestions[quizQuestionCount].correctAnswer = obj["ans"].as<int>();
      quizQuestionCount++;
    }
    saveQuestions();
  }

  saveConfig();
  return true;
}

#endif // ADMIN_CONFIG_H
