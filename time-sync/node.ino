/* Node */
#include <SoftwareSerial.h>

char msg[256] = {0};
bool led_state = false;

const int ldrPin = A0;   
const int ledPin = 13;    

unsigned long msg_delay[2] = {0};
int msg_delay_idx = 0;

int getLightLevel() {
  return analogRead(ldrPin);  
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
  
void handle_command(const char* command_str) {
  if (!(command_str[0] == 'L' || command_str[0] == '*')) return;
  if (strcmp(command_str+1, "state") == 0) { 
    const char* led_state_str = (led_state ? "on" : "off");
    const int light_level = getLightLevel();
    serial_printf("<L%s:%d>\n", led_state_str, light_level);
    
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
  get_all_xbee_message();
  if (msg_delay_idx == 2) {
    serial_printf("<L>\n");
    unsigned long msg_delay_diff = msg_delay[1] - msg_delay[0];
    delay(4000 - msg_delay_diff);
    for (int i=0; i<10; i++) {
      turn_on_led();
      delay(1000);
    }
    msg_delay_idx = 0;
  }
}
