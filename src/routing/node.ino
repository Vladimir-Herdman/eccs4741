/* Node */
#include <SoftwareSerial.h>

#define last_node_in_chain() (self.flag-'a'+1) == self.ncount
#define next_node_in_chain() (self.flag+1)

struct {
  char sh[16];
  char sl[16];
  char flag = '!';
  int ncount = 0;
} self;

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
  if (!(command_str[0] == '*' || command_str[0] == self.flag)) return;
  if (strncmp(command_str+1, "state", 5) == 0) { 
    //TODO - this section, recurse till last node in chain
    //Determine if last node, if not, then send state down to last.
    //Once you receive the propogate, send it back in the chain.
    //  If you're cluster head ('a'), then send back to base station ('0' flag)
    if (last_node_in_chain()) handle_command(" propogate");
    else serial_printf("<%cstate>\n", next_node_in_chain());

    // const char* led_state_str = (led_state ? "on" : "off");
    // const int light_level = getLightLevel();
    // serial_printf("<%c:%d>\n", self.flag, light_level);
  }
  else if (strncmp(command_str+1, "propogate", 9) == 0) { //send back up to previous node, then base if cluster head

  }
  else if (strncmp(command_str+1, "turn", 4) == 0) { //send back up to previous node, then base if cluster head
    char* turnlight = command_str+1+4;
    if (strncmp(turnlight, "on", 2) == 0) {
      if (led_state) return;
      digitalWrite(ledPin, HIGH);
      led_state = true;
    } else { //turn off LED
      if (!led_state) return;
      digitalWrite(ledPin, LOW);
      led_state = false;
    }
  }
  //set self.flag and self.ncount (nodes in network count) based off base
  //setting msg (the FOLLOWING 'else if')
  else if (strncmp(command_str+1, "set", 3) == 0) {
    char fullsid[16] = {0};
    strlcat(fullsid, self.sh, sizeof(fullsid));
    strlcat(fullsid, self.sl, sizeof(fullsid));
    char* sid = strstr(command_str, fullsid);
    if (!sid) {Serial.print("  ERROR: sid not found in command_str\n"); return;}
    self.flag = sid[6+8];
    //self.flag = *(strstr(command_str+4, self.sl)+8);

    char* semicol = strchr(command_str, ';');
    int simicol_count = 0;
    while (semicol != NULL) {
      ++simicol_count;
      curmsg = strchr(command_str, ';');
    }
    self.ncount = simicol_count; //Use ncount to determine if we need to propogate a msg or send back to base

    serial_printf("I'm set to %c\n, and there's %d seonsornode in network", self.flag, self.ncount); //COMMENT
  }
  else if (strncmp(command_str+1, "istate", 6) == 0) {
    serial_printf("<0%s%s>\n", self.sh, self.sl);
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

//Make sure to turn on xbee mode for actual response, so USB doesn't get sh sl values
void getSHSL() { 
  delay(1000);

  //Enter command mode
  Serial.print("+++");
  delay(1000);

  char response[16];
  read_serial(response); //clear 'OK' response after first connecting
  
  Serial.println("ATSH");
  delay(100);
  read_serial(self.sh);
  
  Serial.println("ATSL");
  delay(100);
  read_serial(self.sl);
  
  //Exit command mode
  Serial.println("ATCN");
}

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  getSHSL();
  if (self.sh[0] == '\0' || self.sl[0] == '\0') Serial.println("SH and SL were not gathered, might be error, reconnect and try again.");
}

void loop() {
  get_all_xbee_message();
}

