/* Light */
#include <SoftwareSerial.h>

#define ms_passed(ms) cur_ms - last_ms >= (ms)

// XBee sending setup
//SoftwareASerial Xbee(0, 1);
char msg[256] = {0};
bool led_state = false; //False for off, true for on

const int ldrPin = A0;   // LDR connected to analog pin A0
const int ledPin = 13;    // LED connected to digital pin A1

unsigned long cur_ms;
unsigned long last_ms;

unsigned long msg_delay[2] = {0};
int msg_delay_idx = 0;

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
  char incoming_byte = '\0';
  static size_t index = 0;
  while (Serial.available() > 0) {
    incoming_byte = Serial.read();
    if (incoming_byte == '\n') continue; 
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

void turn_on_led() {
  static bool lightstate = false;
  digitalWrite(ledPin, (lightstate ? LOW : HIGH));
  lightstate = !lightstate;
}

//TODO: currently, we don't have 'confirmation' with base that we actually got the msg, so
  //that could be worked on, having the 'handshake agreement' between messenger and node.
  //Maybe, look into that one library seen that handles the API messaging and can handle
  //multiple back and forth messaging between nodes for error correction/detection in messaging.
void handle_command(const char* command_str) {
  if (!(command_str[0] == 'L' || command_str[0] == '*')) return;
  if (strncmp(command_str+1, "state", 5) == 0) { 
    const char* led_state_str = (led_state ? "on" : "off");
    const int light_level = getLightLevel();
    const char* hashkey = strchr(command_str, ':')+1;
    serial_printf("<L%s:%d;%s>\n", led_state_str, light_level, hashkey);
    
  } else if (strcmp(command_str+1, "turn on led") == 0) {
    if (led_state) return;
    digitalWrite(ledPin, HIGH);
    led_state = true;
  } else if (strcmp(command_str+1, "turn off led") == 0) {
    if (!led_state) return;
    digitalWrite(ledPin, LOW);
    led_state = false;
  } else if (strncmp(command_str+1, "time", 4) == 0) {
    
    msg_delay[msg_delay_idx] = millis();
    ++msg_delay_idx;
  }
}

void respond_to_msg() {
  char* bracket_start = msg;
  while ((bracket_start = strchr(bracket_start, '<')) != NULL) {
    char* bracket_end = strchr(bracket_start, '>');
    if (!bracket_end) break;
    if (bracket_end == bracket_start+1) { 
      *bracket_end = '\0';
      bracket_start = bracket_end + 1;
      continue;
    };

    *bracket_end = '\0';
    handle_command(bracket_start+1);
    bracket_start = bracket_end + 1;
  }
  msg[0] = '\0';
}

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  cur_ms = millis();
  get_all_xbee_message();
  //delay(10);
}
