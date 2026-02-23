/* Base */
#include <SoftwareSerial.h>

struct XbeeNode {
  char flag;
  char lastmsg[20];
  bool recieved_new_msg = false;
  bool state = false; //assume all off at first, until told otherwise. So, use this to determine if fan on/off, and send msg if need to change state, not every time
};

SoftwareSerial XBee(0, 1);
char incoming_byte = {0};
char msg[100] = {0};
XbeeNode available_nodes[8] = {0};
size_t available_nodes_idx = 0;

//Example below changed from variadic cpp reference site: https://en.cppreference.com/w/cpp/utility/variadic.html
//Currently max 64 str len sending (arbitrary, just what I choose when I made it)
//EXAMPLE: serial_printf("<%cwait this long:%d>", 'T', 123);
void serial_printf(const char* fmt, ...) {
  char to_send[64];

  va_list ap;
  va_start(ap, fmt);
  vsnprintf(to_send, sizeof(to_send), fmt, ap);
  va_end(ap);

  Serial.print(to_send);
}

bool append_node(const char c) {
  for (size_t i=0; i<8; i++) {
    if (available_nodes[i].flag == c) return false;
  }
  if (available_nodes_idx == 8) return false;
  available_nodes[available_nodes_idx].flag = c;
  ++available_nodes_idx;
  return true;
}

void save_responding_nodes() {
  char* bracket_start;
  while ((bracket_start = strchr(msg, '<')) != NULL) {
    if (bracket_start[1] != '>') append_node(bracket_start[1]);
  }
}

void get_available_nodes() {
  Serial.print("<*state>"); //expect a <C___> response, or essentially, node returns its current state -> '<Lon>' '<Toff>' for each fan/light
  delay(2000);
  get_all_xbee_message();
  save_responding_nodes();
}

void get_all_xbee_message() {
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

void request_node_msg(XbeeNode* xbeenode) {
  //TODO: make like a 3 loop check, so if node doesn't respond, clear it from the list.
  serial_printf("<%crequested>", xbeenode->flag); //i.e. '<Crequested>'
  delay(100);
  log_node_msg(xbeenode);
}

//Get last message if string long, i.e. '<Tasdf>\n<Lasdf><Basdf>' gets 'Basdf'
//and store in currently requested waiting for xbeenode.
void log_node_msg(const XbeeNode* xbeenode) {
  char* last_endbrack = strrchr(msg, '>');
  if (!last_endbrack) return;

  *(last_endbrack) = '\0';
  const char* last_startbrack = strrchr(msg, '<');
  if (!last_startbrack) return;

  memcpy(xbeenode->lastmsg, last_startbrack+1, strlen(last_startbrack+1));
  xbeenode->recieved_new_msg = true;
  serial_printf("From node:%s\n", last_startbrack+1); //copy msg into last node msg for reference
  //if (atof(last_startbrack+1) < 100) Serial.print("<turn on light>\n");
  //else Serial.print("<turn off light>\n");
}

void respond_to_node(const XbeeNode* xbeenode) {
  switch (xbeenode->flag) {
    case 'T':
      const int temp = atoi(&xbeenode->lastmsg[1]);
      if (temp >= 70 && xbeenode->state == false) { //hot and fan off
        Serial.print("<Tturn on fan>\n");
        xbeenode->state = true;
      }
      else if (temp < 70 && xbeenode->state == true) { //cold and fan on
        Serial.print("<Tturn off fan>\n");
        xbeenode->state = false;
      }
      break;

    case 'L':
      const int ligtlevel = atoi(&xbeenode->lastmsg[1]);
      if (light >= 200 && xbeenode->state == true) { //bright and light on
        Serial.print("<Tturn on fan>\n");
        xbeenode->state = true;
      }
      else if (light < 200 && xbeenode->state == false) { //dark and light off
        Serial.print("<Tturn off fan>\n");
        xbeenode->state = false;
      }
      break;
  }
}

/*void handle_message_from_node() {
const char node_flag = msg[0];
char msgval[6]; //set msg one ahead so it's just the textual value from that node
memcpy(msgval, &msg[1], 6);
const float nodeval = (float)atof(msgval); //assumes nodes only sends number values to base

const float lightThreshhold = 200; //switch this number when we know how dark it needs to be
//bool isDark = false;

switch (node_flag) {

  case 'T':
    Serial.print("\ntemp val:"); Serial.print(msg);
    if (nodeval >= 70) Serial.print("\n<Tfan on >");
    else if (nodeval < 70) Serial.print("\n<Tfan off>");
    break;

  case 'L':
    Serial.print("\nlight val:"); Serial.print(msg);
    //if(nodeval <= lightThreshhold){
      //isDark = true;
    //}
    if(nodeval <= lightThreshhold){
      Serial.print("\n<Llight on >");
    }
    else if (nodeval > lightThreshhold) {
      //isDark = false;
      Serial.print("\n<Llight off>");
    }

    break;
}
}*/

void setup() {
  Serial.begin(9600);
  XBee.begin(9600);
}

void loop() {
  if (available_nodes[0] == nullptr) get_available_nodes();
  if (available_nodes[0] == nullptr) return;
  //get_all_xbee_message();

  //Go through each xbee in node list, and request/get data
  for (size_t i=0; i<available_nodes_idx; i++) {
    request_node_msg(available_nodes[i]);
  }

  //Go through each xbee in node list, and respond to info recieved
  for (size_t i=0; i<available_nodes_idx; i++) {
    XbeeNode* xbeenode = available_nodes[i];
    if (!xbeenode->recieved_new_msg) continue;
    respong_to_node(xbeenode);
  }

  if (strlen(msg) > 0) {
    handle_message_from_node();
    msg[0] = '\0';
  }

  delay(1000);
}
