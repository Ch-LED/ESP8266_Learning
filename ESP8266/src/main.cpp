#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <esp8266_peri.h>

const char* ssid     = "荣耀Magic6";
const char* password = "p0o9i8u7";

WebSocketsClient webSocket;

unsigned long startTime = 0;

bool shouldReboot = false;
unsigned long rebootTime = 0;

void mockSensorResponse() {
    float mockTemperature = 25.0 + random(-500, 500) / 100.0; // Random temp between 20.0 and 30.0
    float mockHumidity = 50.0 + random(-2000, 2000) / 100.0;   // Random humidity between 30.0 and 70.0

    DynamicJsonDocument doc(256);
    doc["type"] = "mock_sensor_response";
    doc["content"]["temperature"] = mockTemperature;
    doc["content"]["humidity"] = mockHumidity;
    doc["timestamp"] = (millis() - startTime) / 1000.0;

    String response;
    serializeJson(doc, response);
    webSocket.sendTXT(response);

    Serial.println("[CMD] Mock sensor data sent.");
}

void safeReboot() {
    Serial.println("[CMD] Preparing to reboot...");
    delay(100);
    webSocket.disconnect();
    delay(100);
    Serial.println("[CMD] Rebooting now!");
    ESP.restart();
}

void rebootResponse() {
    shouldReboot = true;
    rebootTime = millis() + 1000;

    DynamicJsonDocument doc(128);
    doc["type"] = "command_response";
    doc["content"]["status"] = "ok";
    doc["content"]["message"] = "Reboot scheduled.";
    doc["timestamp"] = (millis() - startTime) / 1000.0;
    String response;
    serializeJson(doc, response);
    webSocket.sendTXT(response);

    Serial.println("[CMD] Reboot scheduled from command.");
}

void ledResponse(const char* state) {
    if (strcmp(state, "on") == 0) {
        digitalWrite(LED_BUILTIN, LOW);
    } else {
        digitalWrite(LED_BUILTIN, HIGH);
    }

    DynamicJsonDocument doc(128);
    doc["type"] = "command_response";
    doc["content"]["status"] = "ok";
    doc["content"]["message"] = String("LED turned ") + state + ".";
    doc["timestamp"] = (millis() - startTime) / 1000.0;
    String response;
    serializeJson(doc, response);
    webSocket.sendTXT(response);

    Serial.printf("[CMD] LED turned %s from command.\n", state);
}

String getDeviceInfo() {
    String info;
    info.reserve(512); 

    uint32_t freeHeap = ESP.getFreeHeap();

    uint32_t chipId = ESP.getChipId();
    uint32_t flashChipId = ESP.getFlashChipId();
    uint32_t flashChipSize = ESP.getFlashChipSize();
    uint32_t flashChipSpeed = ESP.getFlashChipSpeed();

    uint32_t sketchSize = ESP.getSketchSize();
    uint32_t freeSketchSpace = ESP.getFreeSketchSpace();

    unsigned long uptimeMs = millis();
    unsigned long seconds = uptimeMs / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    uptimeMs %= 1000;
    seconds %= 60;
    minutes %= 60;
    hours %= 24;

    info = "{";
    info += "\"command\": \"info_response\",";
    info += "\"device_id\": \"" + String(chipId, HEX) + "\",";
    info += "\"heap_free\": " + String(freeHeap) + ",";
    info += "\"sdk_version\": \"" + String(ESP.getSdkVersion()) + "\",";
    info += "\"cpu_freq_mhz\": " + String(ESP.getCpuFreqMHz()) + ",";
    info += "\"flash_id\": \"0x" + String(flashChipId, HEX) + "\",";
    info += "\"flash_size\": " + String(flashChipSize) + ",";
    info += "\"flash_speed\": " + String(flashChipSpeed / 1000000) + ",";
    info += "\"sketch_size\": " + String(sketchSize) + ",";
    info += "\"free_sketch_space\": " + String(freeSketchSpace) + ",";
    info += "\"wifi_ssid\": \"" + WiFi.SSID() + "\",";
    info += "\"ip_address\": \"" + WiFi.localIP().toString() + "\",";
    info += "\"mac_address\": \"" + WiFi.macAddress() + "\",";
    info += "\"uptime\": \"" + String(days) + "d " + String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s\"";
    info += "}";

    return info;
}

void infoResponse() {
    String info = getDeviceInfo();

    DynamicJsonDocument doc(128);
    doc["type"] = "info_response";
    doc["content"] = info;
    doc["timestamp"] = (millis() - startTime) / 1000.0;

    String response;
    serializeJson(doc, response);
    webSocket.sendTXT(response);
    Serial.println("[CMD] Info sent.");
}

void sendPong(const char* id, float ping_timestamp = 0) {
  DynamicJsonDocument doc(256);

  doc["type"] = "pong";
  doc["id"] = id;
  doc["ping_timestamp"] = ping_timestamp;
  doc["client_timestamp"] = (millis() - startTime) / 1000.0;

  String response;
  serializeJson(doc, response);
  webSocket.sendTXT(response);
}

void handleTimeSyncRequest(const char* id, float server_send_time) {
  float client_receive_time = (millis() - startTime) / 1000.0;
  float client_send_time = (millis() - startTime) / 1000.0;

  DynamicJsonDocument doc(256);

  doc["type"] = "time_sync_response";
  doc["id"] = id;
  doc["server_send_time"] = server_send_time;
  doc["client_send_time"] = client_send_time;
  doc["client_receive_time"] = client_receive_time;

  String response;
  serializeJson(doc, response);
  webSocket.sendTXT(response);

  Serial.printf("Sent time sync response: %s\n", response.c_str());
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED:
      Serial.printf("[WSc] Connected to url: %s\n", payload);
      startTime = millis();
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] Received text: %s\n", payload);

      String msg = String((const char*)payload);

      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, msg);

      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }

      const char* msg_type = doc["type"];

      if (strcmp(msg_type, "ping") == 0) {
        const char* id = doc["id"];
        float ping_timestamp = doc["ping_timestamp"];
        sendPong(id, ping_timestamp);
      }
      else if (strcmp(msg_type, "time_sync_request") == 0) {
        const char* id = doc["id"];
        float server_send_time = doc["server_send_time"];
        handleTimeSyncRequest(id, server_send_time);
      }
      else if (strcmp(msg_type, "command") == 0) {
        const char* command = doc["content"];
        Serial.printf("Command: %s\n", command);

        if (strcmp(command, "info") == 0) {
          infoResponse();
        }
        else if (strcmp(command, "reboot") == 0) {
          rebootResponse();
        }
        else if (strcmp(command, "set led") == 0) {
          JsonArray state = doc["data"]["arguments"];
          ledResponse(state[0] == "on" ? "on" : "off");
        }
        else if (strcmp(command, "get mock_sensor") == 0) {
          mockSensorResponse();
        }
        else {
          DynamicJsonDocument respDoc(256);
          respDoc["type"] = "command_response";
          respDoc["content"]["status"] = "error";
          respDoc["content"]["message"] = "Unknown command.";
          respDoc["timestamp"] = (millis() - startTime) / 1000.0;
          String response;
          serializeJson(respDoc, response);
          webSocket.sendTXT(response);
          Serial.println("[CMD] Unknown command received.");
        }
      }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  webSocket.begin("192.168.9.59", 8765, "/");
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();
  if (shouldReboot && millis() >= rebootTime) {
      safeReboot();
  }
}