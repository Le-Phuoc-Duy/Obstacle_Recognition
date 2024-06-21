#ifndef FILE_OPERATION
#define FILE_OPERATION

#include "SPIFFS.h"

class FileOperation
{
public:
  static void listDir(const char * dirname, uint8_t levels){
    Serial.println("--------------------------");
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = SPIFFS.open(dirname);
    if(!root){
        Serial.println("\t- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("\t- not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
      if(file.isDirectory()){
          Serial.print("  DIR : ");
          Serial.println(file.name());
          if(levels){
              listDir(file.path(), levels - 1);
          }
      } else {
          Serial.print("  FILE: ");
          Serial.print(file.name());
          Serial.print("\tSIZE: ");
          Serial.println(file.size());
      }
      file = root.openNextFile();
    }
  }

  static void readFileToSerial(const char * path){
    Serial.println("--------------------------");
    Serial.printf("Reading file: %s\r\n", path);

    File file = SPIFFS.open(path);
    if(!file || file.isDirectory()){
        Serial.println("\t failed to open file for reading!");
        return;
    }

    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
    Serial.println();
  }

  static String readFile(const char *path) {
    // Đọc nội dung từ tệp và trả về dưới dạng String
    File file = SPIFFS.open(path, "r");
    if (!file) {
        Serial.printf("Failed to open file \"%s\" for reading", path);
        return "";
    }

    String content = file.readString();
    file.close();
    return content;
  }

  static void writeFile(const char * path, const char * message){
      Serial.println("--------------------------");
      Serial.printf("Writing file: %s\r\n", path);
  
      File file = SPIFFS.open(path, FILE_WRITE);
      if(!file){
          Serial.println("\t- failed to open file for writing");
          return;
      }
      if(file.print(message)){
          Serial.println("\t- file written");
      } else {
          Serial.println("\t- write failed");
      }
      file.close();
  }

  void deleteFile(const char * path){
    Serial.println("--------------------------");
    Serial.printf("Deleting file: %s\r\n", path);
    if(SPIFFS.remove(path)){
        Serial.println("\t- file deleted");
    } else {
        Serial.println("\t- delete failed");
    }
  }
};
#endif
