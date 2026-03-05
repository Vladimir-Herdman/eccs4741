/* Base */
#include <SoftwareSerial.h>

struct XbeeNode {
  char flag = '\0';
  char lastmsg[20] = {0};
  bool recieved_new_msg = false;
  bool state = false;
  float val = 0;
};

char msg[256] = {0};
XbeeNode available_nodes[8];
size_t available_nodes_idx = 0;
const int ledPin = 13;

bool light_on = false;

void serial_printf(const char* fmt, ...) {
  char to_send[64];

  va_list ap;
  va_start(ap, fmt);
  vsnprintf(to_send, sizeof(to_send), fmt, ap);
  va_end(ap);

  Serial.print(to_send);
}

void update_node(const char* node_str) {
  for (size_t i=0; i<available_nodes_idx; i++) {
    XbeeNode* node = &available_nodes[i];
    if (!(node->flag == node_str[0])) continue;

    char* fcol = strchr(node_str, ':');
    node->val = atof(fcol+1);
    strncpy(node->lastmsg, node_str, 20);
    node->recieved_new_msg = true;
  }
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

bool node_list_contains(const char flag) {
  for (size_t i=0; i<available_nodes_idx; i++) {
    if (available_nodes[i].flag == flag) return true;
  }
  return false;
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
    if (node_list_contains(bracket_start[1])) {
      update_node(&bracket_start[1]);
    } else {
      append_node(&bracket_start[1]);
    }
    bracket_start = bracket_end + 1;
  }
  msg[0] = '\0';
}

void get_available_nodes() {
  Serial.print("<*state>\n");
  delay(200);
  get_all_xbee_message();
  save_responding_nodes();
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

void request_node_msg(XbeeNode* xbeenode) {
  serial_printf("<%crequested>", xbeenode->flag);
  delay(200);
  log_node_msg(xbeenode);
}

void log_node_msg(XbeeNode* xbeenode) {
  char* last_endbrack = strrchr(msg, '>');
  if (!last_endbrack) return;

  *(last_endbrack) = '\0';
  const char* last_startbrack = strrchr(msg, '<');
  if (!last_startbrack) return;

  memcpy(xbeenode->lastmsg, last_startbrack+1, 20);
  xbeenode->recieved_new_msg = true;
  serial_printf("From node:%s\n", last_startbrack+1);
}

void get_all_nodes_state(const int delay_ms) {
  Serial.print("<*state>\n");
  delay(delay_ms);
  get_all_xbee_message();
  save_responding_nodes();
}

void respond_to_node(XbeeNode* xbeenode) {
  serial_printf("node flag(%c) and val(%d)\n", xbeenode->flag, (int)xbeenode->val);
  switch (xbeenode->flag) {
    case 'T': {
      const int temp = (int)xbeenode->val;
      if (temp >= 70
        Serial.print("<Tturn on fan>\n");
        xbeenode->state = true;
      }
      else if (temp < 70
        Serial.print("<Tturn off fan>\n");
        xbeenode->state = false;
      }
      break;
    }

    case 'L': {
      const int lightlevel = (int)xbeenode->val;
      if (lightlevel >= 600
        Serial.print("<Lturn off led>\n");
        xbeenode->state = false;
      }
      else if (lightlevel < 600
        Serial.print("<Lturn on led>\n");
        xbeenode->state = true;
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

void switch_light() {
  digitalWrite(ledPin, (light_on ? LOW : HIGH));
  light_on = !light_on;
}

void broadcast_cur_time() {
  while (true) {
    const unsigned long cur_time_ms = millis();
    serial_printf("<*time:%lu:>\n", cur_time_ms);
    delay(2000);
    serial_printf("<*time:%lu>\n", millis());
    delay(2000);
    for (int i=0; i<10; i++) {
      //mesure time here
      switch_light(); //do work here
      //measure time here

      delay(1000); //1000 - time it took to do all work,
      //this way always 1 second between light on/off
    }
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

void loop() {
  /* Time sync code */
  if (available_nodes[0].flag == '\0') {
    get_all_nodes_state(2000);
    if (available_nodes[0].flag != '\0') broadcast_cur_time();
    return;
  }
}
