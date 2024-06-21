#ifndef WIFI
#define WIFI

#include <WiFi.h>
#include "FileOperation.h"
#include <ArduinoJson.h>

class Wifi
{
public:
  static unsigned int wifiMode;  // WIFI_MODE_AP, WIFI_MODE_STA, WIFI_AP_STA
  static String localPassword;
  
  // Configuration file name
  const static char* PUBLIC_WIFI_CONF_FILE;
  const static char* LOCAL_WIFI_CONF_FILE;

  // Station config
  const static char* DEFAULT_SSID;
  const static char* DEFAULT_PASSWORD;

  // Access Point config
  const static char* LOCAL_SSID;
  const static char* DEFAULT_LOCAL_PASSWORD;

  static bool getSavedWifiCredential(String& ssid, String& password) {
    // Read file config to get ssid and password
    String jsonStr = FileOperation::readFile(PUBLIC_WIFI_CONF_FILE);
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonStr);
  
    if (jsonStr == "" || error) {
      Serial.print("ConnectWifi(): deserializeJson() failed - ");
      Serial.println(error.c_str());
  
      return false;
    } 
    
    ssid = doc["name"].as<String>();
    password = doc["password"].as<String>();
    return true;
  }
 
  static bool getSavedLocalPassword(String& password) {
    password = FileOperation::readFile(LOCAL_WIFI_CONF_FILE);
    if (password != "") 
      return true;
      
    return false;
  }

  static bool initWifiStationMode(String ssid, String password, int timeoutInMilis = 10000) {
    unsigned long timeStone = millis();
  
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      if (millis() - timeStone > timeoutInMilis) 
        return false;
      
      Serial.print(".");
      delay(1000);
    }
    Serial.println();
  
    wifiMode |= WIFI_MODE_STA;
    return true;
  }

  static bool connectWifi(int timeoutInMilis = 8000) {
    unsigned long timeStone = millis();
  
    // Try reading wifi config file to get ssid and password
    String ssid, password;
    if (getSavedWifiCredential(ssid, password)) {
      Serial.print("Try connecting to wifi using saved wifi credentials");
      Serial.print("- Wifi ");
      Serial.println(ssid);
      if (initWifiStationMode(ssid, password, timeoutInMilis))
        return true;
      Serial.println("timeout!!!");
    }
    
    Serial.print("Try connecting wifi by using default credentials");
    Serial.printf("- Wifi \"%s\"" , DEFAULT_SSID);
    if (initWifiStationMode(DEFAULT_SSID, DEFAULT_PASSWORD, timeoutInMilis)) 
      return true;
      
    Serial.println("timeout!!!");
    return false;
  }

  static bool initAccessPointMode() {
    // Try getting saved password
    if (!getSavedLocalPassword(localPassword)) {
      localPassword = DEFAULT_LOCAL_PASSWORD; // Set default if error occurs
    }
    
    if (!WiFi.softAP(LOCAL_SSID, localPassword, 1, 0, 4)) return false;
  
    // Cấu hình địa chỉ IP của Access Point (tùy chọn)
    IPAddress ip(192, 168, 200, 1);
    IPAddress gateway(192, 168, 200, 1);
    IPAddress subnet(255, 255, 255, 0);
    if (!WiFi.softAPConfig(ip, gateway, subnet)) return false;
  
    wifiMode |= WIFI_MODE_AP;
    
    return true;
  }

  static IPAddress softAPIP() {
    return WiFi.softAPIP();
  }

  static IPAddress localIP() {
    return WiFi.localIP();
  }

  static bool isAccessMode() {
    return wifiMode & WIFI_MODE_AP;
  }

  static bool isStationMode() {
    return wifiMode & WIFI_MODE_STA;
  }

  static int status() {
    return WiFi.status();
  }

  static bool isConnected() {
    return WiFi.status() == WL_CONNECTED;
  }

  static void init() {
    // Load saved local password
    String savedLocalPassword = FileOperation::readFile(LOCAL_WIFI_CONF_FILE);
    
    if (savedLocalPassword == "") {
      localPassword = DEFAULT_LOCAL_PASSWORD;
    } else {
      localPassword = savedLocalPassword;
    }

    // Set wifi mode
    WiFi.mode(WIFI_MODE_APSTA);
  }
};
#endif
