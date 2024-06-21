#define CAMERA_MODEL_AI_THINKER
#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "camera_pins.h"
#include <ArduinoJson.h>
#include "SPIFFS.h"
#include "WebServer.h"
#include "UltrasonicSensorReader.h"
#include "FileOperation.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include "Wifi.h"
#include <TinyGPSPlus.h>
#include "LocationUploader.h"

#define LED_LEDC_CHANNEL 2 //Using different ledc channel/timer than camera
#define LED_BUILTIN 33

// Pins for ultrasonic sensor 
#define TRIG_PIN 4 // PWM trigger
#define ECHO_PIN 2 // PWM Output 0-25000US, Every 50US represent 1cm 

const String apiEndpoint = "http://192.168.43.69:8888/detect/"; 
// const String apiEndpoint = "http://192.168.1.15:8888/detect/"; 

// Timer
unsigned int recognizeTimer = 0;

UltrasonicSensorReader distanceReader(TRIG_PIN, ECHO_PIN);

// Create df player
SoftwareSerial serialDF(14,15);
DFRobotDFPlayerMini player;

// GPS Reader instance
SoftwareSerial serialGPS(12,13);
TinyGPSPlus gps;

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
  config.frame_size = FRAMESIZE_VGA; //FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
  if(psramFound()){
    config.jpeg_quality = 15; //0-63 lower number means higher quality
    config.fb_count = 1;
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

  sensor_t * s = esp_camera_sensor_get();
  // reverse the image if you need to
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
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
void playAudio(String jsonString, int dis) {
  int count = 0, name = 0, length = 0;
  DynamicJsonDocument doc(512);

  DeserializationError error = deserializeJson(doc, jsonString);

  if (error) return;

  JsonObject data = doc["data"];
  if (data.size() == 0 && dis <= 100) {
    player.play(76);
    delay(1600);
    return;
  }
  for (JsonPair kv : data) {
    if (kv.value().is<JsonObject>()) {
      JsonObject obj = kv.value().as<JsonObject>();

      for (JsonPair inner_kv : obj) {
        String key = inner_kv.key().c_str();
        int val = inner_kv.value().as<const int>(); 
        if(key == "object_count_id") count = val; 
        if(key == "delay_time") length = val;
      }
      Serial.println(count); 
      Serial.println(length);
      if(count != 0 && length !=0){
        Serial.println("phat nhac"); 
        player.play(count); 
        delay(length);  
      }
    }
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

void recognizeObstacle(int dis) {
  // Capture the image
  Serial.println("Capturing image");
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    return;
  }

  // Upload the captured image to the web server
  if (Wifi::status() == WL_CONNECTED) {
    Serial.println("Sending request to server");

    HTTPClient httpClient;
    int responseCode = postHttpRequest(fb, &apiEndpoint, &httpClient);
    
    if (responseCode > 0) {
      String jsonResponse = httpClient.getString();
      Serial.print("Image upload successful, server response: "); 
      showResult(jsonResponse);
      Serial.println("------------");
      playAudio(jsonResponse, dis);
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

void setup() {
  Serial.begin(115200);

  // Turn on the built-in LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Set up for DF player mini
  serialDF.begin(9600);
  player.begin(serialDF);
  player.volume(30); 

  // Set up for Neo-6m
  serialGPS.begin(9600);

  // Init Flash File System
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
  } else{
    Serial.println("SPIFFS mounted successfully");
  }
  
  // Setup WiFi
  Wifi::init();
  
  // Setup station mode
  if (Wifi::connectWifi()) {
    Serial.print("Wifi connected, lives on IP: ");
    Serial.println(Wifi::localIP());
  } else {
    Serial.println("Wifi connection timeout!");
  }

  // Setup access point mode
  if (!Wifi::initAccessPointMode()) {
    Serial.println("Failed to init Access Point mode!");
  } else {
    IPAddress IP = Wifi::softAPIP();
    Serial.printf("Access Point address: %s - ", Wifi::LOCAL_SSID);
    Serial.println(IP);
  }
  
  initCamera();
  //setupLedFlash(LED_GPIO_NUM);

  WebServer::start();

  // List file in SPIFFS
  FileOperation::listDir("/", 0);
  delay(1000);

  FileOperation::readFileToSerial(Wifi::PUBLIC_WIFI_CONF_FILE);
  FileOperation::readFileToSerial(Wifi::LOCAL_WIFI_CONF_FILE);
  delay(1000);
}

void loop() {
  if (!Wifi::isConnected()) {
    delay(10000);
    return;
  }

  /// GPS logic
  double tmpLat = 0;
  double tmpLng = 0;
  while (serialGPS.available()>0) {
    gps.encode(serialGPS.read());   
    tmpLat =  gps.location.lat();
    tmpLng =  gps.location.lng(); 
  }

  if (tmpLat != 0 && tmpLng != 0 && LocationUploader::isU1pdated(tmpLat, tmpLng)) {
    Serial.println("Send GPS"); 
    Serial.print("Lat: ");
    Serial.print(tmpLat);
    Serial.print(", Long: "); 
    Serial.println(tmpLng);
    LocationUploader::upload(tmpLat, tmpLng);
  } else {
    Serial.println("GPS-Location is not updated");
  }

  /// Recognization obstacle logic
  int disFromObs = distanceReader.getDistanceFromObstacle();
  Serial.print("Distance from obstacle : ");
  Serial.print(disFromObs);
  Serial.println(" (cm)");
  
  if (millis() - recognizeTimer > 1000) {
    if (disFromObs < 200) {
      Serial.println("Start recognization");
      recognizeTimer = millis();
      recognizeObstacle(disFromObs);
    }
  } 
  Serial.println("---------------");  
  delay(200);
}