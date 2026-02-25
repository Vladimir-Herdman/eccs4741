/* Temperature */
#include <SoftwareSerial.h>

// XBee sending setup
//SoftwareSerial Xbee(0, 1);
char msg[128] = {0};
bool fan_state = false; //False for off, true for on

// Temperature reading setup
//pin
const int ldrPin = A0;   // LDR connected to analog pin A0
const float Vref = 5.0;  // Reference voltage (adjust if using 3.3V board)
//Calibration constants (replace with your own from testing)
const float slope = 25.0;   // °C per volt
const float offset = 5.0;   // °C offset

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
  if (!(command_str[0] == 'T' || command_str[0] == '*')) return;
  if (strcmp(command_str+1, "state") == 0) { //Return current state
    const char* fan_state_str = (fan_state ? "on" : "off");
    const float temp = getTemp();
    serial_printf("<T%s:%.2f>\n", fan_state_str, temp);
    //serial_printf("<T%s:74.00>\n", fan_state_str);
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
  pinMode(A1, OUTPUT);
}

void loop() {
  get_all_xbee_message();
  delay(10);
}
