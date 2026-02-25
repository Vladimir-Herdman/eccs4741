/* Light */
#include <SoftwareSerial.h>

// XBee sending setup
//SoftwareSerial Xbee(0, 1);
char msg[256] = {0};
bool led_state = false; //False for off, true for on

const int ldrPin = A0;   // LDR connected to analog pin A0
const int ledPin = 13;    // LED connected to digital pin A1

int getLightLevel() {
  return analogRead(ldrPin);  // Returns 0–1023
}

void serial_printf(const char* fmt, ...) {
  char to_send[64];

  va_list ap;
  va_start(ap, fmt);
  vsnprintf(to_send, sizeof(to_send), fmt, ap);
  va_end(ap);

  Serial.print(to_send);
}

void respond_to_msg();
void get_all_xbee_message() {
  //while xbee sender available, check for data being sent
  char incoming_byte = '\0';
  static size_t index = 0;
  while (Serial.available() > 0) {
    incoming_byte = Serial.read();
    if (incoming_byte == '\n') continue; //parse out newlines
    msg[index] = incoming_byte;
    ++index;
    if (incoming_byte == '>') {
      msg[index] = '\0';
      respond_to_msg();
      index = 0;
    }
    if (index == sizeof(msg)-1) break;
  }
  msg[index] = '\0';
}

//TODO: currently, we don't have 'confirmation' with base that we actually got the msg, so
  //that could be worked on, having the 'handshake agreement' between messenger and node.
  //Maybe, look into that one library seen that handles the API messaging and can handle
  //multiple back and forth messaging between nodes for error correction/detection in messaging.
void handle_command(const char* command_str) {
  if (!(command_str[0] == 'L' || command_str[0] == '*')) return;
  if (strcmp(command_str+1, "state") == 0) { //Return current state
    const char* led_state_str = (led_state ? "on" : "off");
    const int light_level = getLightLevel();
    serial_printf("<L%s:%d>\n", led_state_str, light_level);
    //Serial.flush();
  } else if (strcmp(command_str+1, "turn on led") == 0) {
    if (led_state) return;
    digitalWrite(ledPin, HIGH);
    led_state = true;
  } else if (strcmp(command_str+1, "turn off led") == 0) {
    if (!led_state) return;
    digitalWrite(ledPin, LOW);
    led_state = false;
  }
}

void respond_to_msg() {
  char* bracket_start = msg;
  while ((bracket_start = strchr(bracket_start, '<')) != NULL) {
    char* bracket_end = strchr(bracket_start, '>');
    if (!bracket_end) break;
    if (bracket_end == bracket_start+1) { //bad msg, '<>' recieved, so nothing of note
      *bracket_end = '\0';
      bracket_start = bracket_end + 1;
      continue;
    };

    *bracket_end = '\0';
    handle_command(bracket_start+1);
    bracket_start = bracket_end + 1;
  }
  //Message searched through for node responses, so set msg 'empty' to say done, in case anything else touches it
  msg[0] = '\0';
}

void setup() {
  Serial.begin(9600);
  //Xbee.begin(9600);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  get_all_xbee_message();
  delay(10); //10 worked fine, but I like more sleep zzzZZ
}
