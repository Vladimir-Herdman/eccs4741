#include <SoftwareSerial.h>

#define ms_passed(ms) cur_ms - last_ms >= (ms)

void setup() {
  Serial.begin(9600);
}

void loop() {
  //If no nodes in current network list, call <*state> until a response
  if (available_nodes[0].flag == '\0') {
    get_all_nodes_state(2000);
    return;
  }

  //After network list exists, every 3 seconds check for any updates, and respond accordingly
  cur_ms = millis(); //always place 'last_ms = cur_ms' in last if statement
  if (ms_passed(3000)) {

    for (size_t i=0; i<available_nodes_idx; i++) {
      XbeeNode* xbeenode = &available_nodes[i];
      if (!xbeenode->recieved_new_msg) continue;
      respond_to_node(xbeenode);
      xbeenode->recieved_new_msg = false;
    }

    //serial_printf("3 seconds passed\n");
    last_ms = cur_ms;
  }
}
