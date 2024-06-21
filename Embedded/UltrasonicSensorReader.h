#ifndef ULTRASONIC_SENSOR_READER
#define ULTRASONIC_SENSOR_READER

class UltrasonicSensorReader
{
  private:
    int _trigPin;
    int _echoPin;
    const float SOUND_VELOCITY = 0.034;
  public:
    UltrasonicSensorReader(int trigPin, int echoPin) {
      _trigPin = trigPin;
      _echoPin = echoPin;

      // Set pin mode for ultrasonic sensor
      pinMode(trigPin, OUTPUT);
      pinMode(echoPin, INPUT);
    }
    
    int getDistanceFromObstacle() {
      //Clears the trigPin
      digitalWrite(_trigPin, LOW); 
      delayMicroseconds (2);
      // Sets the trigPin on HIGH state for 10 micro seconds
      digitalWrite(_trigPin, HIGH);
      delayMicroseconds (10);
      digitalWrite(_trigPin, LOW);
    
      // Reads the echoPin, returns the sound wave travel time in microseconds
      unsigned long duration= pulseIn(_echoPin, HIGH); // us
      // Calculating the distance
      float distance = duration * SOUND_VELOCITY / 2; 
    
      return distance;
    }
};
#endif
