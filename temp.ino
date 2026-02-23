/* Temperature */
#include <SoftwareSerial.h>

// XBee sending setup
SoftwareSerial XBee(0, 1);
char incoming_byte = {0};
char msg[100] = {0};
size_t index = 0;

// Temperature reading setup
//pin
const int ldrPin = A0;   // LDR connected to analog pin A0
const float Vref = 5.0;  // Reference voltage (adjust if using 3.3V board)
//Calibration constants (replace with your own from testing)
const float slope = 25.0;   // °C per volt
const float offset = 5.0;   // °C offset

void setup() {
  Serial.begin(9600);
  XBee.begin(9600);
  pinMode(A1, OUTPUT);
}

float getTemp();

void loop() {
  // while xbee sender available, check for data being sent
  while (XBee.available() > 15) {
    incoming_byte = XBee.read();

    // if there's a '<', we're being sent a base station message (as
    // opposed to normal printing)
    if (incoming_byte == '<') {

      // get through the end of the message, then break out of the loop that
      // keeps checking for message data
      while (incoming_byte != '>') {
        incoming_byte = XBee.read();
        msg[index] = incoming_byte;
        ++index;
      }
      msg[index-1] = '\0';
      break;
    }
  }
  // if we have a message from the station, print hit
  if (strlen(msg) > 1 && msg[0] == 'T') {
    //Serial.print("\nmsg:"); Serial.print(msg);
    //Serial.print("\n\nMsg from base:");
    //Serial.print(msg);
    if (strcmp(msg, "Tfan off") == 0) {
      //Serial.print("\nkilling fan");
      analogWrite(A1, 0); //0 to 255 analog write
    } else if (strcmp(msg, "Tfan on ") == 0) {
      //Serial.print("\nliving fan");
      analogWrite(A1, 255);
    }
  }

  //check temperature
  const float tempread = getTemp();
  Serial.print("\n<T");
  Serial.print(tempread, 2);
  Serial.print(">");

  //reset global vars
  index = 0;
  msg[0] = '\0';
  delay(1000);
}

float getTemp() {
  int rawValue = analogRead(ldrPin);
  float voltage = (rawValue / 1023.0) * Vref;
  float temperature = (voltage * slope) + offset;
  return temperature;
}
