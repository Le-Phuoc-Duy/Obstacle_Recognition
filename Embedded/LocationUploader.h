#ifndef LOCATION_UPLOADER
#define LOCATION_UPLOADER

#define UPLOAD_ENDPOINT "https://us-central1-visionaid-dut210.cloudfunctions.net/app/api/update-location/"
#define DEVICE_KEY "Device1234"

//#include "HardwareSerial.h"
#include <HTTPClient.h>

class LocationUploader
{
private:
  double lastLat = -1;
  double lastLng = -1;
  bool wasInited = false;
  
  const double ESL = 0.002;  // Nguỡng thực hiện sự cập nhật
  LocationUploader() {}
  
  static LocationUploader& getInstance() {
    static LocationUploader instance; // Chỉ khởi tạo 1 lần
    return instance;
  }
public:
  void update(double lat, double lng) {
    wasInited = true;
    lastLat = lat;
    lastLng = lng;
  }


  // check whether location is updated, if true then update the last location
  static bool isUpdated(double lat, double lng) {
    LocationUploader& instance = getInstance();

    if (!instance.wasInited) {
      instance.update(lat, lng);
      return true;
    }

    if (abs(instance.lastLat - lat) + abs(instance.lastLng - lng) > instance.ESL) {
      instance.update(lat, lng);
      return true;
    }

    return false;
  }

  static bool upload(double lat, double lng) {
    // Create the request
    HTTPClient httpClient;
    httpClient.begin(String(UPLOAD_ENDPOINT) + String(DEVICE_KEY));
    httpClient.addHeader("Content-Type", "application/json");

    String requestBody = String("{ \"latitude\": ") + String(lat,6) + ", \"longitude\": " + String(lng,6) + "}";
    Serial.print("Send GPS");
    Serial.println(requestBody);
    // PUT the request
    int responseCode = httpClient.PUT(requestBody);
    httpClient.end();
    
    if (responseCode > 0) {
      return true;
    }
    
    Serial.print("Error on LocationUploader request: ");
    Serial.println(responseCode);
    return false;
  }
};

#endif
