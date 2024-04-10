#define CAMERA_MODEL_AI_THINKER
#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "camera_pins.h"
#include <ArduinoJson.h>
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include "AsyncJson.h"
#include "UltrasonicSensorReader.h"
#include "FileOperation.h"

#define LED_LEDC_CHANNEL 2 //Using different ledc channel/timer than camera
#define LED_BUILTIN 33

// Pins for ultrasonic sensor
#define TRIG_PIN 12 // PWM trigger
#define ECHO_PIN 13 // PWM Output 0-25000US, Every 50US represent 1cm

// Configuration file name
#define PUBLIC_WIFI_CONF_FILE "/public-wifi-config.txt"
#define LOCAL_WIFI_CONF_FILE "/local-wifi-config.txt"

unsigned int wifiMode = WIFI_MODE_STA;  // WIFI_MODE_AP, WIFI_MODE_STA

// Station config
#define DEFAULT_SSID "5 AE Siêu Nhân"
#define DEFAULT_PASSWORD "0364651600"

const String apiEndpoint = "http://192.168.43.209:8000/detect/";

// Access Point config
const char* localSsid = "MyESP32AP";
const char* localPassword = "12345678";

// WebSocket config
uint8_t socketClientCount = 0; // Số lượng client đang kết nối
unsigned int responseTimer = 0;
const unsigned int requestInterval = 100; // (ms)

// Timer
unsigned int recognizeTimer = 0;


UltrasonicSensorReader distanceReader(TRIG_PIN, ECHO_PIN);

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create a WebSocket object
AsyncWebSocket ws("/ws");


void initCamera() {
  // Stores the camera configuration parameters
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG; //YUV422,GRAYSCALE,RGB565,JPEG

  // Select lower framesize if the camera doesn't support PSRAM
  config.frame_size = FRAMESIZE_QVGA; //FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
  if(psramFound()){
    config.jpeg_quality = 15; //0-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    config.jpeg_quality = 20;
    config.fb_count = 1;
  }

  // Initialize the Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}

bool getWifiCredential(String& ssid, String& password) {
  // Read file config to get ssid and password
  String jsonStr = FileOperation::readFile(PUBLIC_WIFI_CONF_FILE);
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonStr);

  if (jsonStr == "" || error) {
    Serial.print("ConnectWifi(): deserializeJson() failed - ");
    Serial.println(error.c_str());

    ssid = DEFAULT_SSID;
    password = DEFAULT_PASSWORD;
    return false;
  } 
  
  ssid = doc["name"].as<String>();
  password = doc["password"].as<String>();
  return true;
}

bool connectWifi(int timeoutInMilis = 10000) {
  unsigned long timeStone = millis();

  // Try reading wifi config file to get ssid and password
  String ssid, password;
  if (getWifiCredential(ssid, password)) {
    Serial.print("Connecting to WiFi");
  } else {
    Serial.print("Try connecting wifi by using default credentials");
  }
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - timeStone > timeoutInMilis) 
      return false;
    
    Serial.print(".");
    delay(1000);
  }
  Serial.println();

  wifiMode = WIFI_MODE_STA;
  return true;
}

bool initAccessPointMode() {
  // Khởi tạo ESP32 trong chế độ AP
  if (!WiFi.softAP(localSsid, localPassword)) return false;

  // Cấu hình địa chỉ IP của Access Point (tùy chọn)
  IPAddress ip(192, 168, 1, 1);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  if (!WiFi.softAPConfig(ip, gateway, subnet)) return false;

  wifiMode = WIFI_MODE_AP;
  
  return true;
}

void setupLedFlash(int pin) {
    ledcSetup(LED_LEDC_CHANNEL, 5000, 8);
    ledcAttachPin(pin, LED_LEDC_CHANNEL);
}

void showResult(String json){
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, json);
  JsonObject data = doc["data"];

  for (JsonPair kv : data) {
    const char* obstacle = kv.key().c_str();
    unsigned quantity = kv.value().as<int>();

    Serial.print("Obstacle: ");
    Serial.print(obstacle);
    Serial.print(", quantity: ");
    Serial.println(quantity);
  }
}

// Post image fb to apiEndpoint via form-data with key "image"
int postHttpRequest(camera_fb_t* fb, const String* endpoint, HTTPClient* httpClient) {
  // Create the request
  httpClient->begin(*endpoint);
  httpClient->addHeader("Content-Type", "multipart/form-data; boundary=--------------------------697881354623426881025817");
  
  String requestBody = "";
  requestBody += "----------------------------697881354623426881025817\r\n";
  requestBody += "Content-Disposition: form-data; name=\"image\"; filename=\"image.jpg\"\r\n";
  requestBody += "Content-Type: image/jpeg\r\n\r\n";
  requestBody += String(reinterpret_cast<const char*>(fb->buf), fb->len);
  requestBody += "\r\n----------------------------697881354623426881025817--\r\n";

  // Send the POST request with the manually constructed request body
  int responseCode = httpClient->POST(requestBody);
  return responseCode;
}

void recognizeObstacle() {
  // Capture the image
  Serial.println("Capturing image");
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    return;
  }

  // Upload the captured image to the web server
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Sending request to server");

    HTTPClient httpClient;
    int responseCode = postHttpRequest(fb, &apiEndpoint, &httpClient);
    
    if (responseCode > 0) {
      String jsonResponse = httpClient.getString();
      Serial.print("Image upload successful, server response: ");
      Serial.println(jsonResponse);
      showResult(jsonResponse);
    } else {
      Serial.print("Image upload failed, error code: ");
      Serial.println(responseCode);
      Serial.print("Details: ");
      Serial.println(httpClient.errorToString(responseCode));
    }

    httpClient.end();
  } else {
    Serial.println("WiFi not connected, image upload skipped");
  }
  esp_camera_fb_return(fb);
}

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
  server.onNotFound([](AsyncWebServerRequest *request) {
    printRequestInfo(request);
    if (request->method() == HTTP_OPTIONS) {
      request->send(200);
    } else {
      request->send(404);
    }
  });
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    printRequestInfo(request);
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.on("/wifi-info", HTTP_GET, [](AsyncWebServerRequest *request){
    printRequestInfo(request);
    
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    JsonDocument wifiInfo;
    
    String ssid, password;
    getWifiCredential(ssid, password);
    wifiInfo["name"] = ssid;

    serializeJson(wifiInfo, *response);
    request->send(response);
  });

  server.addHandler(new AsyncCallbackJsonWebHandler("/change-public-wifi", [](AsyncWebServerRequest *request, JsonVariant &json) {
    printRequestInfo(request);
    
    String jsonStr = json.as<String>();
    FileOperation::writeFile(PUBLIC_WIFI_CONF_FILE, jsonStr.c_str());
    request->send(200);
  }));
  
  server.serveStatic("/", SPIFFS, "/");
  
  server.begin();
}
void startWebSocket() {
  ws.onEvent(websocketEventHandler);
  server.addHandler(&ws);
}

void setup() {
  Serial.begin(115200);

  // Turn on the built-in LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Init Flash File System
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
  } else{
    Serial.println("SPIFFS mounted successfully");
  }

  // Setup WiFi
  if (connectWifi()) {
    Serial.print("Wifi connected, lives on IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wifi connection timeout!");
    Serial.println("Switch to Access Point mode!");

    // Setup access point
    if (!initAccessPointMode()) {
      Serial.println("Failed to init Access Point mode!");
    } else {
      IPAddress IP = WiFi.softAPIP();
      Serial.print("Access Point IP address: ");
      Serial.println(IP);
    }
  }
  
  initCamera();
  setupLedFlash(LED_GPIO_NUM);

  startWebServer();
  startWebSocket();

  // List file in SPIFFS
  FileOperation::listDir("/", 0);
  delay(2000);
  Serial.println("Public-wifi-config.txt : ");
  Serial.println(FileOperation::readFile("/public-wifi-config.txt"));
  delay(2000);
}

void loop() {
  if (wifiMode & WIFI_MODE_AP) {
    delay(10000);
    return;
  }

  int disFromObs = distanceReader.getDistanceFromObstacle();
  Serial.print("Distance from obstacle : ");
  Serial.print(disFromObs);
  Serial.println(" (cm)");
  
  if (millis() - recognizeTimer > 1000) {
    if (disFromObs < 100) {
      Serial.println("Start recognization");
      recognizeObstacle();
      recognizeTimer = millis();
    }
  }
  
  delay(200);
}