/* Hardcoded Location Node */
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

#define passed_ms(ms) curms - lastms >= (ms)
#define passed_s(s) curms - lastms >= ((s)*1000)
#define sp Serial.print

struct Location {
  double lat;
  double lng;
};

TinyGPSPlus gps;
const int RXPin = 0, TXPin = 1;
SoftwareSerial ss(RXPin, TXPin);

Location cur_loc;
unsigned long curms, lastms;

void displayInfo() {
  Serial.print(F("Location: ")); 
  if (gps.location.isValid()) {
    static int count = 0;
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else {
    Serial.print(F("INVALID Haversign: INVALID"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid()) {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid()) {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else {
    Serial.print(F("INVALID"));
  }

  Serial.println();
}

void getInfo() {
  static bool start_set = false;
  if (gps.location.isValid() && gps.location.lat() != 0) {
      cur_loc.lat = gps.location.lat();
      cur_loc.lng = gps.location.lng();
  }
}


void setup() {
  Serial.begin(9600);
  ss.begin(9600);
}

void loop() {
  curms = millis();
  bool read = false;
  while (ss.available() > 0)
    read = gps.encode(ss.read());

  if (read) {
    getInfo();
    //displayInfo();
  }

  if (passed_s(1) && cur_loc.lat != 0) {
    //sp("Current Distance to Mac: "); sp(distance_m, 6); sp("m\n");
    //sp("\tTime left: "); sp(time_left_ms); sp("ms -> "); sp(time_left_ms/1000.0, 2); sp("s\n");
    //sp("\tSpeed to be in time: "); sp(distance_m/(time_left_ms/1000.0), 2); sp("mps\n\n");
    sp("<*loc"); sp(cur_loc.lat, 10); sp(";"); sp(cur_loc.lng, 10); sp(">\n");

    lastms = curms;
  }

  if (curms > 5000 && gps.charsProcessed() < 10) {
    Serial.println(F("No GPS detected: check wiring."));
    displayInfo();
  }
  //sp("test data sent");
}
