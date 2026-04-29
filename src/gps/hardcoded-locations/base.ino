/* Coords from Friday walk around area

Main Locations
  Chapel (East Door): 40.767339N / -83.828133W
  Chapel (SW Door): 40.767159N / -83.828060W
  Chapel (North Door): 40.767258N / -83.828499W
  Library (Front Door): 40.767051N / -83.827962W
  Library (Side): 40.767397N / -83.827964
  Fire Hydrant (near parking lot): 40.767674N / -83.827773W
  JLK (East): 40.767683N / -83.827765W
  JLK (Mid): 40.767722N / -83.827924W
  JLK (West): 40.767475 / -83.828182W

Interesting Spots
  Flame of Unity (Center statue): 40.767313N / -83.828159W
  Picnic Table: 40.767425N / -83.828208W
  Bike Rack (put my bike here on test day): 40.767222N / -83.828047W
  Bike Repair (near library): 40.767107N / -83.828040W
  Potted Tree: 40.767339N / -83.827923W
  McPheron Office: 40.767627N / -83.828064W
  ‘69 Kucklick Bench: 40.767734N / -83.828744W
  Treenis (baby branch on KLJ to center path): 40.767521N / -83.827978W

Midpoints
  -/- JLK(W)/Chapel(N): 40.767461N / -83.828659W
  -/- JLK(E)/Library(N): 40.767530N / -83.827863
  -/- All Buildings: 40.767269N / -83.828158W
*/


/* Hardcoded Location Base */
#include <SoftwareSerial.h>

#define sp Serial.print
#define MYPI 3.14159265358979323846

struct Location {
  double lat;
  double lng;
};

struct NamedLocation {
  const char* name;
  const Location loc;
  double distance_m;
};

NamedLocation hardcoded_locations[] = {
  //main locations
  {"Chapel", {40.767339, -83.828133}, 0},
  {"Chapel", {40.767159, -83.828060}, 0},
  {"Chapel", {40.767258, -83.828499}, 0},
  {"Library", {40.767051, -83.827962}, 0},
  {"Library", {40.767397, -83.827964}, 0},
  {"Fire Hydrant", {40.767674, -83.827773}, 0},
  {"JLK", {40.767683, -83.827765}, 0},
  {"JLK", {40.767722, -83.827924}, 0},
  {"JLK", {40.767475, -83.828182}, 0},

  //midpoints
  {"JLK|Chapel", {40.767461, -83.828659}, 0},
  {"JLK|Library", {40.767530, -83.827863}, 0},
  {"Center", {40.767269, -83.828158}, 0},

  //locations of interest
  {"Flame of Unity (Center statue)", {40.767313, -83.828159}, 0},
  {"Picnic Table", {40.767425, -83.828208}, 0},
  {"Bike Rack (put my bike here on test day)", {40.767222, -83.828047}, 0},
  {"Bike Repair (near library)", {40.767107, -83.828040}, 0},
  {"Potted Tree", {40.767339, -83.827923}, 0},
  {"McPheron Office", {40.767627, -83.828064}, 0},
  {"'69 Kucklick Bench", {40.767734, -83.828744}, 0},
  {"Treenis (baby branch on KLJ to center path)", {40.767521, -83.827978}, 0},
};
const int hardcoded_size = sizeof(hardcoded_locations)/sizeof(hardcoded_locations[0]);
int hardcoded_interest_index = 0;

char msg[256];

//will do distance in meters
double haversign(const double lat1, const double lat2, const double lng1, const double lng2) {
  const double R = 6371009.0;
  const double phi1 = (lat1*MYPI)/180;
  const double phi2 = (lat2*MYPI)/180;
  const double dphi = ((lat2-lat1)*MYPI)/180;
  const double dlambda = ((lng2-lng1)*MYPI)/180;
  const double a = pow(sin(dphi/2), 2) + cos(phi1)*cos(phi2)*pow(sin(dlambda/2), 2);
  const double c = 2*atan2(sqrt(a), sqrt(1-a));
  return R * c;
}

double haversign(const Location loc1, const Location loc2) {
  const double R = 6371009;
  const double phi1 = (loc1.lat*MYPI)/180;
  const double phi2 = (loc2.lat*MYPI)/180;
  const double dphi = ((loc2.lat-loc1.lat)*MYPI)/180;
  const double dlambda = ((loc2.lng-loc1.lng)*MYPI)/180;
  const double a = pow(sin(dphi/2), 2) + cos(phi1)*cos(phi2)*pow(sin(dlambda/2), 2);
  const double c = 2*atan2(sqrt(a), sqrt(1-a));
  return R * c;
}

void display_location(char* locstr) {
  const Location curloc = {
    .lat = atof(strchr(locstr, 'c')+1),
    .lng = atof(strchr(locstr, ';')+1)
  };
  sp("lat:"); sp(curloc.lat, 6); sp("\n");
  sp("lng:"); sp(curloc.lng, 6); sp("\n");

  for (int i=0; i<hardcoded_size; i++)
    hardcoded_locations[i].distance_m = haversign(curloc, hardcoded_locations[i].loc);

  double closest_distance = 1000; //TODO - set to maximum double value
  NamedLocation *mainarea=NULL, *interestarea=NULL, *curlistloc;
  for (int i=0; i<hardcoded_interest_index; i++) {
    curlistloc = &hardcoded_locations[i];
    if (curlistloc->distance_m >= closest_distance) continue;

    closest_distance = curlistloc->distance_m;
    mainarea = curlistloc;
  }
  for (int i=hardcoded_interest_index; i<hardcoded_size; i++) {
    curlistloc = &hardcoded_locations[i];
    if (curlistloc->distance_m > 5) continue;

    interestarea = curlistloc;
  }

  for (int i=0; i<hardcoded_size; i++)
    hardcoded_locations[i].distance_m = 0;

  if (mainarea == NULL) {
    sp("NO MAIN YET\n");
    return;
  }
  sp("\tMain Area (closest): "); sp(mainarea->name); sp(".\n");
  sp("\tArea of Interest (within 5 m): "); 
  if (interestarea)
    sp(interestarea->name);
  else
    sp("NONE WITHIN 5 METERS");
  sp(".\n");
}

void respond() {
  char* bracket_start = msg;
  while (bracket_start[1] == '<') bracket_start += 1;

  while ((bracket_start = strchr(bracket_start, '<')) != NULL) {
    char* bracket_end = strchr(bracket_start, '>');
    if (!bracket_end) break;
    if (bracket_end == bracket_start+1) {
      *bracket_end = '\0';
      bracket_start = bracket_end + 1;
      continue;
    };

    *bracket_end = '\0';
    if (strncmp(bracket_start+2, "loc", 3) == 0) {
      display_location(bracket_start+2);
    }
  }
  msg[0] = '\0';
}

void get_all_xbee_message() {
  size_t index = 0;
  char incoming_byte = '\0';
  while (Serial.available() > 0) {
    incoming_byte = Serial.read();
    if (incoming_byte == '\n') continue;
    msg[index] = incoming_byte;
    ++index;
    if (index == sizeof(msg)-1) break;
  }
  msg[index] = '\0';
}

void setup() {
  Serial.begin(9600);
  for (int i=0; i<hardcoded_size; i++)
    if (hardcoded_locations[i].name == "Flame of Unity (Center statue)") {
      hardcoded_interest_index = i;
      break;
    }
    sp("hardcoded_size:"); sp(hardcoded_size); sp("\n");
    sp("hardcoded_interest_size:"); sp(hardcoded_interest_index); sp("\n");
}

void loop() {
  get_all_xbee_message();
  respond();
  delay(500);
}
