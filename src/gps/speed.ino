#include <TinyGPS++.h>
#include <SoftwareSerial.h>

#define passed_ms(ms) curms - lastms >= (ms)
#define passed_s(s) curms - lastms >= ((s)*1000)

const int RXPin = 0, TXPin = 1;
const uint32_t GPSBaud = 9600;

TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

struct LocationReading {
  double lat;
  double lng;
  unsigned long long time_ms;
};

LocationReading location_reading[2];
unsigned long long readcount;
unsigned long curms, lastms;

void serial_printf(const char* fmt, ...) {
  char to_send[256];

  va_list ap;
  va_start(ap, fmt);
  vsnprintf(to_send, sizeof(to_send), fmt, ap);
  va_end(ap);

  Serial.print(to_send);
}

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
    if (readcount >= 10 && location_reading[0].lat == 0 && gps.time.isValid()) { //10 uninformed value, just change up later.
      location_reading[0].lat = gps.location.lat();
      location_reading[0].lng = gps.location.lng();
      location_reading[0].time_ms = gps.time.hour()*60*60*1000
                                    + gps.time.minute()*60*1000
                                    + gps.time.second()*1000
                                    + gps.time.centisecond()*10;
    } else if (readcount >= 10 && location_reading[0].lat != 0 && gps.time.isValid()) { //will update second each time after the first has been found
      location_reading[1].lat = gps.location.lat();
      location_reading[1].lng = gps.location.lng();
      location_reading[1].time_ms = gps.time.hour()*60*60*1000
                                    + gps.time.minute()*60*1000
                                    + gps.time.second()*1000
                                    + gps.time.centisecond()*10;
    }
    // if (latlist[0] == 0) {
    //   latlist[0] = lat;
    //   lnglist[0] = lng;
    // } else {
    //   latlist[0] = latlist[1];
    //   latlist[1] = lat;
    //   lnglist[0] = lnglist[1];
    //   lnglist[1] = lng;
    //   const double distm = haversign(latlist[0], latlist[1], lnglist[0], lnglist[1]);
    //   Serial.print(F(" dist(m):")); Serial.print(distm, 2);
    //   Serial.print(F(" speed(m/s):")); Serial.print(distm/5.0, 4);
    //   Serial.print(F(" speed(mi/s):")); Serial.print(distm/1609.34/5.0, 4);
    // }

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
  if (gps.location.isValid()) {
    if (location_reading[0].lat == 0 && gps.time.isValid() && gps.location.isValid()) { //10 uninformed value, just change up later.
      location_reading[0].lat = gps.location.lat();
      location_reading[0].lng = gps.location.lng();
      location_reading[0].time_ms = gps.time.hour()*60*60*1000
                                    + gps.time.minute()*60*1000
                                    + gps.time.second()*1000
                                    + gps.time.centisecond()*10;
    } else if (location_reading[0].lat != 0 && gps.time.isValid() && gps.location.isValid()) { //will update second each time after the first has been found
      location_reading[1].lat = gps.location.lat();
      location_reading[1].lng = gps.location.lng();
      location_reading[1].time_ms = gps.time.hour()*60*60*1000
                                    + gps.time.minute()*60*1000
                                    + gps.time.second()*1000
                                    + gps.time.centisecond()*10;
    }
  }
  else {
    ;;
  }
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

  if (read) {
    ++readcount;
    getInfo();
    displayInfo();
    Serial.print("currently read "); Serial.print((unsigned int)readcount); Serial.print(" times\n");
  }

  //every ## seconds, get the first reading and the last reading, calculate a distance, and a time to see how far and fast you've traveled.
  //  30 s is about 180
  if (passed_s(10)) {
    const LocationReading* lr = location_reading;
    const double distance_m = haversign(lr[0].lat, lr[1].lat, lr[0].lng, lr[1].lng)*100;
    const double time_diff_ms = lr[1].time_ms - lr[0].time_ms;
    const double speed_mps = distance_m/(time_diff_ms/1000.0);
    //serial_printf("\n10 seconds passed:\n\tdistance(m):%f\n\ttime(ms):%f\n\tspeed(m/s):%f\n",
        //distance_m,
        //time_diff_ms,
        //speed_mps
    //);
    Serial.print("\n 10 sec:\n\tdistance(m): "); Serial.print(distance_m); Serial.print("\n\ttime diff: "); Serial.print(time_diff_ms); Serial.print("\n\tSpeed(m/s): "); Serial.print(speed_mps);
    //serial_printf("\n\tloc params: 1(%f %f) 2(%f %f)\n", lr[0].lat, lr[0].lng, lr[1].lat, lr[1].lng);
    Serial.print("\ttime params: 1("); Serial.print((unsigned int)lr[0].time_ms); Serial.print(") 2("); Serial.print((unsigned int)lr[1].time_ms); Serial.print(")\n");
    //serial_printf("\ttime params: 1(%llu) 2(%llu)\n", lr[0].time_ms, lr[1].time_ms);

    memset(location_reading, 0, sizeof(location_reading));
    // lr[0].lat = 0;
    // lr[1].lat = 0;
    readcount = 0;
    lastms = curms;
  }

  if (curms > 5000 && gps.charsProcessed() < 10) {
    Serial.println(F("No GPS detected: check wiring."));
    while(true);
  }
}
