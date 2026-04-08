/* Base */
#include <SoftwareSerial.h>

#define ms_passed(ms) cur_ms - last_ms >= (ms)
#define read_serial(dst) \
count = 0; \
while(Serial.available()) { \
  (dst)[count] = (char)Serial.read(); \
  ++count; \
}

struct XbeeNode {
  char flag = '\0';
  bool recieved_new_msg = false;
  float val = 0;
  char sh[16] = {0};
  char sl[16] = {0};
};

//Just reference data for this individual xbee/arduino, whenever needed throughout the project
struct {
  char sh[16];
  char sl[16];
  char dflag;
  char last_received_msg[64];
  bool new_msg = false;
  int ncount = 0;
} self;

char msg[256] = {0};
int count; //Global count value, because we love global access to variables anywhere anytime.
XbeeNode available_nodes[8];
size_t available_nodes_idx = 0;

unsigned long cur_ms;
unsigned long last_ms;

void serial_printf(const char* fmt, ...) {
  char to_send[64];

  va_list ap;
  va_start(ap, fmt);
  vsnprintf(to_send, sizeof(to_send), fmt, ap);
  va_end(ap);

  Serial.print(to_send);
}

void update_initial_node(const char* node_str) {
  for (size_t i=0; i<available_nodes_idx; i++) {
    XbeeNode* node = &available_nodes[i];
    if (!(node->flag == node_str[0])) continue;

    char* fcol = strchr(node_str, ':');
    node->val = atof(fcol+1);
    node->recieved_new_msg = true;
  }
}

void update_node(const char* node_str) {
  for (size_t i=0; i<available_nodes_idx; i++) {
    XbeeNode* xbn = &available_nodes[i];
    if (!(xbn->flag == node_str[0])) continue;

    strlcpy(self.last_received_msg, node_str, sizeof(self.last_received_msg));
    self.new_msg = true;
    // char* fcol = strchr(node_str, ':');
    // xbn->val = atof(fcol+1);
    // xbn->recieved_new_msg = true;
  }
}

bool append_initial_node(const char* node_str) {
  if (node_str[0] == '*') return false;
  if (available_nodes_idx == 8) return false;
  for (size_t i=0; i<available_nodes_idx; i++) { //if already in list, return and do nothing
    const XbeeNode* xbn = &available_nodes[i];
    if (strncmp(xbn->sh, node_str+1, 6) == 0 && strncmp(xbn->sl, node_str+1+6, 8) == 0) return false;
  }
  XbeeNode node;
  static char iflag = 'a';
  node.flag = iflag++;
  node.recieved_new_msg = true;
  //serial_printf("  node_str:%s\n", node_str);
  strncpy(node.sh, node_str+1, 6);
  strcpy(node.sl, node_str+1+6);
  //serial_printf("  node_str sl:%s\n", node_str+1+6);
  //serial_printf("  %s\n", node.sl);
  available_nodes[available_nodes_idx] = node;
  ++available_nodes_idx;
  return true;
}

bool append_node(const char* node_str) {
  if (node_str[0] == '*') return false;
  if (available_nodes_idx == 8) return false;
  for (size_t i=0; i<available_nodes_idx; i++) {
    if (available_nodes[i].flag == node_str[0]) return false;
  }
  char* fcol = strchr(node_str, ':');
  XbeeNode node;
  node.flag = node_str[0];
  node.val = atof(fcol+1);
  node.recieved_new_msg = true;
  available_nodes[available_nodes_idx] = node;
  ++available_nodes_idx;
  return true;
}

bool node_list_contains_str(const char* sh) {
  for (size_t i=0; i<available_nodes_idx; i++) {
    const XbeeNode* xbn = &available_nodes[i];
    if (strncmp(xbn->sh, sh, 6) == 0 && strncmp(xbn->sl, sh+6, 8) == 0) return true;
  }
  return false;
}

bool node_list_contains_flag(const char flag) {
  for (size_t i=0; i<available_nodes_idx; i++) {
    if (available_nodes[i].flag == flag) return true;
  }
  return false;
}

void save_initial_responding_nodes() {
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
    if (node_list_contains_str(bracket_start[2])) {
      //I don't think we should be updating at all in initial node list generation, just appending
      //update_node(&bracket_start[1]);
      ;;;
    } else {
      append_initial_node(&bracket_start[1]);
    }
    bracket_start = bracket_end + 1;
  }
  msg[0] = '\0';
}

void save_state_msg() {
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
    if (bracket_start[1] == '0') { //for base station
      self.new_msg = true;
      strlcpy(self.last_received_msg, bracket_start+2, sizeof(self.last_received_msg));
    }
  }
}

void save_responding_nodes() {
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
    if (node_list_contains_flag(bracket_start[1])) {
      update_node(&bracket_start[1]);
    } /*else { //I just don't think it should append it if it wasn't in the inital state call flag list
      append_node(&bracket_start[1]);
    }*/
    bracket_start = bracket_end + 1;
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

/*setDHDL(const char* dh, const char* dl) {
  for (int i=0; i<available_nodes_idx; i++) {
    XbeeNode* xbn = &available_nodes[i];
    if (!(strncmp(xbn->sh, dh, 6) == 0 && strncmp(xbn->sl, dl, 8) == 0)) continue;
    self.dflag = xbn->flag;
  }

  Serial.print("+++");
  delay(1000);

  Serial.print("ATDH");
  Serial.println(dh);
  delay(100);

  Serial.print("ATDL");
  Serial.println(dl);
  delay(100);

  Serial.println("ATCN");
  delay(100);
}*/


void send_initial_node_flags() {
  if (available_nodes_idx == 0) return;
  char imsg[128] = {0};
  strlcat(imsg, "<*set", sizeof(imsg));
  for (size_t i=0; i<available_nodes_idx; i++) {
    XbeeNode* xbn = &available_nodes[i];
    //if (!xbn->recieved_new_msg) continue;
    strncat(imsg, xbn->sh, 6);
    strcat(imsg, xbn->sl);
    char flagflag[2] = {0}; flagflag[0] = xbn->flag;
    strlcat(imsg, flagflag, sizeof(imsg));
    strlcat(imsg, ";", sizeof(imsg));
    //xbn->recieved_new_msg = false;
  }
  strlcat(imsg, ">", sizeof(imsg));
  self.ncount = available_nodes_idx;
  Serial.println(imsg);
  //setDHDL()
}

void get_all_initial_nodes_state(const int delay_ms) {
  Serial.print("<*istate>\n");
  delay(delay_ms);
  get_all_xbee_message();
  save_initial_responding_nodes();
  send_initial_node_flags();
}

void get_A_node_state(const int delay_ms) {
  Serial.print("<astate>\n");
  delay(delay_ms);
  get_all_xbee_message();
  //save_responding_nodes();
  save_state_msg();
}

void get_all_nodes_state(const int delay_ms) {
  Serial.print("<*state>\n");
  delay(delay_ms);
  get_all_xbee_message();
  save_responding_nodes();
}

void respond_to_nodes() {
  char tosend[64] = {0};
  char* curmsg = self.last_received_msg; //past '<%c', so just data and no '>' at end: 'a123;b321;c441;'
  int lightlevel = atof(curmsg+1);
  snprintf(tosend, sizeof(tosend), "<aturna%s;", (lightlevel > 600 ? "off" : "on"));
  for (int i=1; i<self.ncount; i++) {
    char nodeflag = 'a'+i;
    curmsg = strchr(curmsg, nodeflag);
    lightlevel = atof(curmsg+1);
    strlcat(tosend, nodeflag, sizeof(tosend));
    strlcat(tosend, (lightlevel > 600 ? "off" : "on"), sizeof(tosend));
    strlcat(tosend, ";", sizeof(tosend));
  }
  strlcat(tosend, ">", sizeof(tosend));
  Serial.println(tosend); //<aturnaon;boff;con;>
}

void respond_to_node(XbeeNode* xbeenode) {
  //serial_printf("node flag(%c) and val(%d)\n", xbeenode->flag, (int)xbeenode->val);
  switch (xbeenode->flag) {
    case 'T': {
      const int temp = (int)xbeenode->val;
      if (temp >= 70) {
        Serial.print("<Tturn on fan>\n");
      }
      else if (temp < 70) {
        Serial.print("<Tturn off fan>\n");
      }
      break;
    }

    case 'L': {
      const int lightlevel = (int)xbeenode->val;
      if (lightlevel >= 600) {
        Serial.print("<Lturn off led>\n");
      }
      else if (lightlevel < 600) {
        Serial.print("<Lturn on led>\n");
      }
      break;
    }

    default: {
      serial_printf("Unknown flag: %c (%d)\n", xbeenode->flag, (int)xbeenode->flag);
      break;
    }
  }
  xbeenode->recieved_new_msg = false;
}

/*void sendToNode(const char*  dh, const char* dl, const char* message) {
  setDHDL(dh, dl);
  Serial.println(message);
}*/

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
  getSHSL();
  if (self.sh[0] == '\0' || self.sl[0] == '\0') Serial.println("SH and SL were not gathered, might be error, reconnect and try again.");
}

void loop() {
  /* Routing Code */
  if (available_nodes[0].flag == '\0') {
    get_all_initial_nodes_state(3000);
    return;
  }

  //After network list exists, every 3 seconds check for any updates, and respond accordingly
  cur_ms = millis(); //always place 'last_ms = cur_ms' in last if statement
  if (ms_passed(5000)) {
    get_A_node_state(5000);

    if (self.new_msg) {
      respond_to_nodes();
    }
    //serial_printf("3 seconds passed\n");
    last_ms = cur_ms;
  }
}
