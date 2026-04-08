/* Temp */
#include <SoftwareSerial.h>

#define ms_passed(ms) cur_ms - last_ms >= (ms)

// XBee sending setup
//SoftwareSerial Xbee(0, 1);
char msg[256] = {0};
bool fan_state = false; //False for off, true for on

const int ldrPin = A0;
const float Vref = 5.0;
const float slope = 25.0;
const float offset = 5.0;

unsigned long cur_ms;
unsigned long last_ms;

unsigned long msg_delay[2] = {0};
int msg_delay_idx = 0;

float getTemp() {
  int rawValue = analogRead(ldrPin);
  float voltage = ((float)rawValue / 1023.0f) * Vref;
  float temperature = (voltage * slope) + offset;
  return temperature;
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
  digitalWrite(ldrPin, (lightstate ? LOW : HIGH));
  lightstate = !lightstate;
}

//TODO: currently, we don't have 'confirmation' with base that we actually got the msg, so
//that could be worked on, having the 'handshake agreement' between messenger and node.
//Maybe, look into that one library seen that handles the API messaging and can handle
//multiple back and forth messaging between nodes for error correction/detection in messaging.
void handle_command(const char* command_str) {
  if (!(command_str[0] == 'T' || command_str[0] == '*')) return; //<Cstate> <Cturn on light>
  if (strncmp(command_str+1, "state", 5) == 0) { //Return current state
    const char* fan_state_str = (fan_state ? "on" : "off");
    const char* hashkey=strchr(command_str, ':')+1;
    float temp = getTemp();
    serial_printf("<T%s:", fan_state_str);Serial.print(temp);serial_printf(";%s>\n", hashkey);
    //serial_printf("<T%s:", fan_state_str); Serial.print(temp); SerialSerial.print(">\n");
  } else if (strcmp(command_str+1, "turn on fan") == 0) {
    analogWrite(A1, 255);
    fan_state = true;
  } else if (strcmp(command_str+1, "turn off fan") == 0) {
    analogWrite(A1, 0);
    fan_state = false;
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
  pinMode(A1, OUTPUT);
}

void loop() {
  cur_ms = millis();
  get_all_xbee_message();
  //delay(10);
}
