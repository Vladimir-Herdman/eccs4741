#include <TinyGPS++.h>
#include <SoftwareSerial.h>

#define passed_ms(ms) curms - lastms >= (ms)
#define passed_s(s) curms - lastms >= ((s)*1000)

const int RXPin = 0, TXPin = 1;
const uint32_t GPSBaud = 9600;

TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

struct Location {
  double lat;
  double lng;
}

const Location mac = {40.766888, -83.827956};
const Location jlk = {40.767861, -83.827911};
Location cur_loc;

unsigned long curms, lastms;
unsigned long readcount;

//will do distance in meters
double haversign(const double lat1, const double lat2, const double lng1, const double lng2) {
  const int R = 6371000;
  const double phi1 = (lat1*PI)/180;
  const double phi2 = (lat2*PI)/180;
  const double dphi = ((lat2-lat1)*PI)/180;
  const double dlambda = ((lng2-lng1)*PI)/180;
  const double a = pow(sin(dphi/2), 2) + cos(phi1)*cos(phi2)*pow(sin(dlambda/2), 2);
  const double c = 2*atan2(sqrt(a), sqrt(1-a));
  return R * c;
}

double haversign(const Location loc1, const Location loc2) {
  const int R = 6371000;
  const double phi1 = (loc1.lat*PI)/180;
  const double phi2 = (loc2.lat*PI)/180;
  const double dphi = ((loc2.lat-loc1.lat)*PI)/180;
  const double dlambda = ((loc2.lng-loc1.lng)*PI)/180;
  const double a = pow(sin(dphi/2), 2) + cos(phi1)*cos(phi2)*pow(sin(dlambda/2), 2);
  const double c = 2*atan2(sqrt(a), sqrt(1-a));
  return R * c;
}

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
  if (gps.location.isValid() && gps.location.lat() != 0) {
      cur_loc.lat = gps.location.lat();
      cur_loc.lng = gps.location.lng();
  }
}

void setup() {
  Serial.begin(9600);
  ss.begin(GPSBaud);
}

void loop() {
  curms = millis();
  bool read = false;
  while (ss.available() > 0)
    read = gps.encode(ss.read());

  if (read) {
    ++readcount;
    getInfo();
    //displayInfo();
    Serial.print("currently read "); Serial.print((unsigned int)readcount); Serial.print(" times\n");
  }

  if (passed_s(10)) {
    const double distance_m = haversign(cur_loc, mac);
    readcount = 0;
    lastms = curms;
  }

  if (curms > 5000 && gps.charsProcessed() < 10) {
    Serial.println(F("No GPS detected: check wiring."));
  }
}
