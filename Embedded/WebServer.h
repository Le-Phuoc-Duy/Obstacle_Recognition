#ifndef WEBSERVER
#define WEBSERVER

#include <ArduinoJson.h>
#include "esp_camera.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include "AsyncJson.h"
#include "Wifi.h"

class WebServer
{
private:
  // WebSocket config
  uint8_t socketClientCount = 0; // Số lượng client đang kết nối
  unsigned int responseTimer = 0;
  const unsigned int requestInterval = 100; // (ms)

  // Create AsyncWebServer object on port 80
  AsyncWebServer server;
  
  // Create a WebSocket object
  AsyncWebSocket ws;

  // Singleton
  WebServer() : server(80), ws("/ws") {}
  
  static WebServer& getInstance() {
    static WebServer instance; // Chỉ khởi tạo 1 lần
    return instance;
  }
  
  //==================== private method ========================
  void handleWebSocketMessage(uint32_t clientId, void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      data[len] = 0;
      String message = (char*)data;
      //Serial.println(message);
  
      if (message == "GI") {
        // Capture and send image to client
        camera_fb_t* fb = esp_camera_fb_get();
        if (!fb) {
          ws.text(clientId, "Error: failed to capture the image!");
        } else {
          ws.binary(clientId, fb->buf, fb->len);
        }
  
        esp_camera_fb_return(fb);
      }
    }
  }

  void websocketEventHandler(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
      case WS_EVT_CONNECT:
        ++socketClientCount;
        if (socketClientCount <= 1) {
          Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        } else {
          Serial.println("Connection limit per client reached, rejecting client");
          client->close();
        }
        break;
      case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        --socketClientCount;
        break;
      case WS_EVT_DATA:
        if (millis() - responseTimer > requestInterval) {
          handleWebSocketMessage(client->id(), arg, data, len);
          responseTimer = millis();
        }
        break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
       break;
    }
  }

  void printRequestInfo(AsyncWebServerRequest *request) {
    Serial.printf("%s %s\n", request->methodToString(), request->url());
    
    // List all collected headers
    int headers = request->headers();
    for(int i=0; i < headers; i++){
      AsyncWebHeader* h = request->getHeader(i);
      Serial.printf("%s: %s\n", h->name().c_str(), h->value().c_str());
    }
    Serial.println();
  }

  void startWebServer() {
    // CORS config
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    server.onNotFound([&](AsyncWebServerRequest *request) {
      printRequestInfo(request);
      if (request->method() == HTTP_OPTIONS) {
        request->send(200);
      } else {
        request->send(404);
      }
    });
    
    server.on("/", HTTP_GET, [&](AsyncWebServerRequest *request){
      printRequestInfo(request);
      request->send(SPIFFS, "/index.html", "text/html");
    });
  
    server.on("/public-wifi-info", HTTP_GET, [&](AsyncWebServerRequest *request){
      printRequestInfo(request);
      
      AsyncResponseStream *response = request->beginResponseStream("application/json");
      JsonDocument wifiInfo;
      
      String ssid, password;
      Wifi::getSavedWifiCredential(ssid, password);
      wifiInfo["name"] = ssid;
  
      serializeJson(wifiInfo, *response);
      request->send(response);
    });
  
    server.on("/local-wifi-info", HTTP_GET, [&](AsyncWebServerRequest *request){
      printRequestInfo(request);
      
      AsyncResponseStream *response = request->beginResponseStream("application/json");
      JsonDocument wifiInfo;
      wifiInfo["name"] = Wifi::LOCAL_SSID;
  
      serializeJson(wifiInfo, *response);
      request->send(response);
    });
  
    server.addHandler(new AsyncCallbackJsonWebHandler("/change-public-wifi", [&](AsyncWebServerRequest *request, JsonVariant &json) {
      printRequestInfo(request);
      
      String jsonStr = json.as<String>();
      FileOperation::writeFile(Wifi::PUBLIC_WIFI_CONF_FILE, jsonStr.c_str());
      request->send(200);
    }));
  
    server.addHandler(new AsyncCallbackJsonWebHandler("/change-local-wifi", [&](AsyncWebServerRequest *request, JsonVariant &json) {
      printRequestInfo(request);
      
      String jsonStr = json.as<String>();
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, jsonStr);
      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        request->send(400, "text/plain", "Failed to parse the request body - json!");
        return;
      }
  
      String password = doc["password"];
  
      if (password != Wifi::localPassword) {
        request->send(400, "text/plain", "The password isn't correct!");
        return;
      }
      
      String newPassword = doc["newPassword"];
      Wifi::localPassword = newPassword;
      
      FileOperation::writeFile(Wifi::LOCAL_WIFI_CONF_FILE, newPassword.c_str());
      request->send(200);
    }));
    
    server.serveStatic("/", SPIFFS, "/");
    
    server.begin();
  }
  
  void startWebSocket() {
    ws.onEvent([this] (AsyncWebSocket *server_, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
      this->websocketEventHandler(server_, client, type, arg, data, len);
    });
    
    server.addHandler(&ws);
  }

public:
  
  static void start() {
    getInstance().startWebServer();
    getInstance().startWebSocket();
  }
};
#endif
