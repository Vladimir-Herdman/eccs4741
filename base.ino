/* Base */
#include <SoftwareSerial.h>

struct XbeeNode {
  char flag = '\0'; //char representing which xbee this node is
  char lastmsg[20] = {0}; //just a copy of the last msg for reference if needed
  bool recieved_new_msg = false; //change if new msg from all read, so need to do something
  bool state = false; //assume all off at first, until told otherwise. So, use this to determine if fan on/off, and send msg if need to change state, not every time
  float val = 0; //if msg has a value associated with it, write it down for reference (similar to last msg)
};

//SoftwareSerial Xbee(0, 1);
char msg[256] = {0}; //256 bytes arbitrary size, just one I choose when making it
XbeeNode available_nodes[8];
size_t available_nodes_idx = 0;

//Example below changed from variadic cpp reference site: https://en.cppreference.com/w/cpp/utility/variadic.html
//Currently max 64 str len sending (arbitrary, just what I choose when I made it)
//EXAMPLE: serial_printf("<%cwait this long:%d>", 'T', 123);
//Dude this one's my favorite, so cool, just printf for the arduino serial print here
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
    //node->state = (strncmp(node_str+1, "on", 2) == 0);
    node->val = atof(fcol+1);
    strncpy(node->lastmsg, node_str, 20);
    node->recieved_new_msg = true;
  }
}

bool append_node(const char* node_str) {
  if (node_str[0] == '*') return false;
  if (available_nodes_idx == 8) return false; //max num nodes in network
  for (size_t i=0; i<available_nodes_idx; i++) {
    if (available_nodes[i].flag == node_str[0]) return false;
  }
  char* fcol = strchr(node_str, ':'); //assuming example msg from state call is '<Con:123>'
  XbeeNode node;
  node.flag = node_str[0];
  //node.state = (strncmp(node_str, "on", 2) == 0);
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
    if (bracket_end == bracket_start+1) { //bad msg, '<>' recieved, so nothing of note
      *bracket_end = '\0';
      bracket_start = bracket_end + 1;
      continue;
    };

    *bracket_end = '\0';
    if (node_list_contains(bracket_start[1])) { //already in list, so just update
      update_node(&bracket_start[1]);
    } else { //not in list, new node, so append to list and update accordingly
      append_node(&bracket_start[1]);
    }
    bracket_start = bracket_end + 1;
  }
  //Message searched through for node responses, so set msg 'empty' to say done, in case anything else touches it
  msg[0] = '\0';
}

void get_available_nodes() {
  Serial.print("<*state>\n"); //expect a <C___> response, or essentially, node returns its current state -> '<Lon>' '<Toff>' for each fan/light
  delay(200);
  get_all_xbee_message();
  save_responding_nodes();
}

//TODO: update this so that it has an 'in msg' bool, so only record into the msg
//array if we've detected a '<' bracket. That way, msg isn't filled with needless
//data from just printing in the other nodes.
void get_all_xbee_message() {
  //while xbee sender available, check for data being sent
  size_t index = 0;
  char incoming_byte = '\0';
  while (Serial.available() > 0) {
    incoming_byte = Serial.read();
    if (incoming_byte == '\n') continue; //parse out newlines
    msg[index] = incoming_byte;
    ++index;
    if (index == sizeof(msg)-1) break;
  }
  msg[index] = '\0';
}

void request_node_msg(XbeeNode* xbeenode) {
  //TODO: make like a 3 loop check, so if node doesn't respond, clear it from the list.
  serial_printf("<%crequested>", xbeenode->flag); //i.e. '<Crequested>'
  delay(200);
  log_node_msg(xbeenode);
}

//Get last message if string long, i.e. '<Tasdf>\n<Lasdf><Basdf>' gets 'Basdf'
//and store in currently requested waiting for xbeenode.
void log_node_msg(XbeeNode* xbeenode) {
  char* last_endbrack = strrchr(msg, '>');
  if (!last_endbrack) return;

  *(last_endbrack) = '\0';
  const char* last_startbrack = strrchr(msg, '<');
  if (!last_startbrack) return;

  memcpy(xbeenode->lastmsg, last_startbrack+1, 20);
  xbeenode->recieved_new_msg = true;
  serial_printf("From node:%s\n", last_startbrack+1); //copy msg into last node msg for reference
  //if (atof(last_startbrack+1) < 100) Serial.print("<turn on light>\n");
  //else Serial.print("<turn off light>\n");
}

void get_all_nodes_state() {
  Serial.print("<*state>\n");
  delay(200);
  get_all_xbee_message();
  save_responding_nodes();
}

void respond_to_node(XbeeNode* xbeenode) {
  serial_printf("node flag(%c) and val(%d)\n", xbeenode->flag, (int)xbeenode->val);
  switch (xbeenode->flag) {
    case 'T': {
      const int temp = (int)xbeenode->val;
      if (temp >= 70 /*&& xbeenode->state == false*/) { //hot and fan off
        Serial.print("<Tturn on fan>\n");
        xbeenode->state = true;
      }
      else if (temp < 70 /*&& xbeenode->state == true*/) { //cold and fan on
        Serial.print("<Tturn off fan>\n");
        xbeenode->state = false;
      }
      break;
    }

    case 'L': {
      const int lightlevel = (int)xbeenode->val;
      if (lightlevel >= 600 /*&& xbeenode->state == true*/) { //bright and light on
        Serial.print("<Lturn off led>\n");// delay(30); Serial.print("<Lturn off led>\n");
        xbeenode->state = false;
      }
      else if (lightlevel < 600 /*&& xbeenode->state == false*/) { //dark and light off
        Serial.print("<Lturn on led>\n");// delay(30); Serial.print("<Lturn on led>\n");
        xbeenode->state = true;
      }
      break;
    }

    default: {
      serial_printf("Unknown flag: %c (%d)\n", xbeenode->flag, (int)xbeenode->flag);
      break;
    }
  }
  //Update that the node has been visited, and its message dealt with
  xbeenode->recieved_new_msg = false;
}

void setup() {
  Serial.begin(9600);
  //Xbee.begin(9600);
}

void loop() {
  //Just call *state to all xbees, then get all data from reading msg
  get_all_nodes_state();

  //Go through each xbee in node list, and respond to info recieved
  for (size_t i=0; i<available_nodes_idx; i++) {
    XbeeNode* xbeenode = &available_nodes[i];
    if (!xbeenode->recieved_new_msg) continue;
    respond_to_node(xbeenode);
    xbeenode->recieved_new_msg = false;
  }

  delay(1000);
}
