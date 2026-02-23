/* Light */
#include <SoftwareSerial.h>

// XBee sending setup
SoftwareSerial XBee(0, 1);
char incoming_byte = {0};
char msg[100] = {0};

const int ldrPin = A0;   // LDR connected to analog pin A0
const int ledPin = 13;    // LED connected to digital pin A1

void setup() {
  Serial.begin(9600);
  XBee.begin(9600);
  pinMode(ledPin, OUTPUT);
}

int getLightLevel();
void get_xbee_message();
void handle_msg_from_base();

void loop() {
  get_xbee_message();

  // if we have a message from the station, print hit
  if (strlen(msg) > 1) {
    handle_msg_from_base();
    msg[0] = '\0';
  }

  int lightLevel = getLightLevel();
  Serial.print('<'); Serial.print(lightLevel); Serial.print('>'); Serial.print("\n");

  delay(1000);
}

int getLightLevel() {
  return analogRead(ldrPin);  // Returns 0–1023
}

void get_xbee_message() {
  //while xbee sender available, check for data being sent
  size_t index = 0;
  while (XBee.available() > 0) {
    incoming_byte = XBee.read();
    if (incoming_byte == '\n') continue;
    msg[index] = incoming_byte;
    ++index;
    if (index == 99) break;
  }
  msg[index] = '\0';
}
void handle_msg_from_base() {
  //get last message
  char* last_endbrack = strrchr(msg, '>');
  if (!last_endbrack) return;

  *(last_endbrack) = '\0';
  const char* last_startbrack = strrchr(msg, '<');
  if (!last_startbrack) return;

  Serial.print("From base:"); Serial.print(last_startbrack+1); Serial.print("\n");
  if (strcmp(last_startbrack+1, "turn on light")) digitalWrite(ledPin, LOW);
  else if (strcmp(last_startbrack+1, "turn off light")) digitalWrite(ledPin, HIGH);
}
