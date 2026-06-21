// =============================================================
//  webserver.h — Async Web Server + WebSocket
//  Handles all HTTP routes, WebSocket events, and API endpoints
// =============================================================
#ifndef SSA_WEBSERVER_H
#define SSA_WEBSERVER_H

#include <WiFi.h>

// AsyncTCP must be included BEFORE ESPAsyncWebServer for proper resolution
#if ESP_ARDUINO_VERSION_MAJOR >= 3
#include <AsyncTCP.h>
#else
// Core 2.x may use the me-no-dev AsyncTCP; try to include it
#include <AsyncTCP.h>
#endif

#include "admin_config.h"
#include "config.h"
#include "html_admin.h"
#include "html_dashboard.h"
#include "html_hologram.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

// ── Server & WebSocket instances ──
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ── Forward declarations (implemented in main .ino) ──
extern String getGameStatusJson();
extern void onWebSocketAction(const String &action);

// ── Track connected clients ──
int wsClientCount = 0;

// =============================================================
//  WebSocket Event Handler
// =============================================================
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {

  switch (type) {
  case WS_EVT_CONNECT:
    wsClientCount++;
    Serial.printf("[WS] Client #%u connected (total: %d)\n", client->id(),
                  wsClientCount);
    // Send current game state to newly connected client
    {
      String status = getGameStatusJson();
      client->text(status);
    }
    break;

  case WS_EVT_DISCONNECT:
    wsClientCount--;
    Serial.printf("[WS] Client #%u disconnected (total: %d)\n", client->id(),
                  wsClientCount);
    break;

  case WS_EVT_DATA: {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len &&
        info->opcode == WS_TEXT) {
      // Parse incoming JSON message
      data[len] = 0; // Null-terminate
      String msg = (char *)data;
      Serial.printf("[WS] Received: %s\n", msg.c_str());

      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, msg);
      if (!err) {
        String action = doc["action"].as<String>();

        // Handle special requests that return data
        if (action == "get_status") {
          client->text(getGameStatusJson());
        } else if (action == "get_videos") {
          // Send video URLs to hologram player
          JsonDocument vDoc;
          vDoc["event"] = "videos";
          JsonArray urls = vDoc["urls"].to<JsonArray>();
          for (int i = 0; i < NUM_ROOMS; i++) {
            urls.add(gameConfig.videoUrls[i]);
          }
          String output;
          serializeJson(vDoc, output);
          client->text(output);
        } else {
          // Forward game control actions to main sketch
          onWebSocketAction(action);
        }
      }
    }
    break;
  }

  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

// =============================================================
//  BROADCAST to all WebSocket clients
// =============================================================
void broadcastWS(const String &message) { ws.textAll(message); }

// Build and broadcast a JSON event
void broadcastEvent(const char *eventName) {
  JsonDocument doc;
  doc["event"] = eventName;
  String output;
  serializeJson(doc, output);
  broadcastWS(output);
}

// =============================================================
//  SETUP WEB SERVER & ROUTES
// =============================================================
void initWebServer() {

  // ── WebSocket ──
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // ── Page Routes ──

  // Dashboard (main page)
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", PAGE_DASHBOARD);
  });

  // Hologram player
  server.on("/hologram", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", PAGE_HOLOGRAM);
  });

  // Admin portal
  server.on("/admin", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", PAGE_ADMIN);
  });

  // ── API Routes ──

  // =============================================================
  //  API Routes
  // =============================================================

  // GET /api/config — Return current configuration as JSON
  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = getConfigJson();
    request->send(200, "application/json", json);
  });

  // POST /api/config — Save configuration
  server.on(
      "/api/config", HTTP_POST,
      [](AsyncWebServerRequest *request) {
        // This handler only fires if there was NO body, or after body is
        // processed. The actual body processing is done in the onRequestBody
        // handler below.
        if (!request->hasParam("body", true, true)) {
          request->send(400, "application/json",
                        "{\"status\":\"error\",\"msg\":\"No body\"}");
        }
      },
      NULL,
      // Body handler
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len,
         size_t index, size_t total) {
        if (index == 0) {
          request->_tempObject = new String();
        }
        String *bodyStr = (String *)request->_tempObject;
        for (size_t i = 0; i < len; i++) {
          *bodyStr += (char)data[i];
        }

        if (index + len == total) {
          // Body fully received
          bool success = parseConfigJson(*bodyStr);
          if (success) {
            request->send(200, "application/json", "{\"status\":\"ok\"}");
            Serial.println("[API] Config saved via admin portal.");
          } else {
            request->send(400, "application/json",
                          "{\"status\":\"error\",\"msg\":\"Invalid JSON\"}");
          }
          delete bodyStr;
          request->_tempObject = NULL;
        }
      });

  // GET /api/status — Return current game state
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = getGameStatusJson();
    request->send(200, "application/json", json);
  });

  // ── Start Server ──
  server.begin();
  Serial.println("[WEB] Async web server started on port 80.");
}

// =============================================================
//  WIFI INITIALIZATION
// =============================================================
String espIPAddress = "";

void initWiFi() {
  if (USE_AP_MODE) {
    // ── Access Point Mode ──
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    espIPAddress = WiFi.softAPIP().toString();

    Serial.println("[WIFI] Access Point mode started.");
    Serial.printf("[WIFI] SSID    : %s\n", AP_SSID);
    Serial.printf("[WIFI] Password: %s\n", AP_PASS);
    Serial.printf("[WIFI] IP      : %s\n", espIPAddress.c_str());
  } else {
    // ── Station Mode ──
    WiFi.mode(WIFI_STA);
    WiFi.begin(STA_SSID, STA_PASS);

    Serial.printf("[WIFI] Connecting to '%s'", STA_SSID);
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 30) {
      delay(500);
      Serial.print(".");
      timeout++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      espIPAddress = WiFi.localIP().toString();
      Serial.println(" Connected!");
      Serial.printf("[WIFI] SSID : %s\n", STA_SSID);
      Serial.printf("[WIFI] IP   : %s\n", espIPAddress.c_str());
    } else {
      Serial.println(" FAILED!");
      Serial.println("[WIFI] Falling back to AP mode...");
      WiFi.mode(WIFI_AP);
      WiFi.softAP(AP_SSID, AP_PASS);
      espIPAddress = WiFi.softAPIP().toString();
      Serial.printf("[WIFI] AP IP : %s\n", espIPAddress.c_str());
    }
  }
}

// Periodic WebSocket cleanup (call in loop every ~1 second)
unsigned long lastWsCleanup = 0;
void cleanupWebSocket() {
  unsigned long now = millis();
  if (now - lastWsCleanup >= 1000) {
    lastWsCleanup = now;
    ws.cleanupClients();
  }
}

#endif // SSA_WEBSERVER_H
