#include <TinyGPS++.h>
#include <SoftwareSerial.h>

const int RXPin = 0, TXPin = 1;
const uint32_t GPSBaud = 9600;

TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

double latlist[2];
double lnglist[2];
unsigned long curms, lastms;

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

void displayInfo() {
  Serial.print(F("Location: ")); 
  if (gps.location.isValid()) {
    static int count = 0;
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
    double lat=gps.location.lat(), lng=gps.location.lng();
    if (latlist[0] == 0) {
      latlist[0] = lat;
      lnglist[0] = lng;
    } else {
      latlist[0] = latlist[1];
      latlist[1] = lat;
      lnglist[0] = lnglist[1];
      lnglist[1] = lng;
      const double distm = haversign(latlist[0], latlist[1], lnglist[0], lnglist[1]);
      Serial.print(F(" dist(m):")); Serial.print(distm, 2);
      Serial.print(F(" speed(m/s):")); Serial.print(distm/5.0, 4);
      Serial.print(F(" speed(mi/s):")); Serial.print(distm/1609.34/5.0, 4);
    }

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

void setup() {
  Serial.begin(9600);
  ss.begin(GPSBaud);
}

void loop() {
  curms = millis();
  bool read = false;
  // This sketch displays information every time a new sentence is correctly encoded.
  while (ss.available() > 0)
    read = gps.encode(ss.read());

  if (curms - lastms >= 5000) {
    displayInfo();
    lastms = curms;
  }

  if (curms > 5000 && gps.charsProcessed() < 10) {
    Serial.println(F("No GPS detected: check wiring."));
    while(true);
  }
}
