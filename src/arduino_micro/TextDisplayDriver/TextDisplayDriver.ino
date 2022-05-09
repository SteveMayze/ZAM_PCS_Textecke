/*
  Arctix Text Display Driver

  A ZAM PCS Project
*/

#include "Keyboard.h"

// #define TEST_MODE


// Define some modes of opperation. Two at the moment, 
// but perhaps there could be more.
#define MODE_IDLE 0
#define MODE_TEXT_ENTRY 1

#define LED 13
#define SYNC 2


byte operation_mode = MODE_IDLE;

void press_key(uint8_t key){
    Keyboard.press(key);
    delay(500);
    Keyboard.release(key);  
}

void prepare_display_for_text_entry(){
//  #ifdef TEST_MODE
//    Keyboard.println("ESC 1 - Enter text entry mode");
//  #else
    press_key((uint8_t)KEY_ESC);
    delay(100);
    press_key('1');
//  #endif
}

void exit_text_entry_mode() {
//  #ifdef TEST_MODE
//    Keyboard.println("RETURN - exit text entry mode");
//  #else
    press_key((uint8_t)KEY_RETURN);
//  #endif
  
}
void clear_display_buffer() {
//  #ifdef TEST_MODE
//    Keyboard.println("ALT CTRL DELETE - Clear the buffer");
//  #else
    // 
    // press and hold CTRL
    Keyboard.press(KEY_LEFT_CTRL);
    Keyboard.press(KEY_LEFT_ALT);
    Keyboard.press(KEY_DELETE);
    delay(100);
    // release all the keys at the same instant
    Keyboard.releaseAll();
// #endif
  
}



void setup() {

  pinMode(LED, OUTPUT);
  pinMode(SYNC, INPUT_PULLUP);
  digitalWrite(LED, LOW);
  
  // open the serial port:
  Serial1.begin(9600);
  operation_mode = MODE_IDLE;
  delay(500);
  Keyboard.press(KEY_ESC);
  delay(500);
  Keyboard.releaseAll();
 
}

void loop() {
  // Check the SYNCH button. This puts the Text Display Driver into
  // the same mode as the Text Display Unit.
  if ( digitalRead(SYNC) == LOW ){
    operation_mode = MODE_IDLE;
    digitalWrite(LED, LOW);
  }
  // Check for any characters from the host
  while (Serial1.available () > 0)
  {
      uint8_t rxByte = Serial1.read();

      // Enter was pressed. Depending on the current mode, either
      // change into text entry mode or go back into idle mode.
      if( rxByte == '\r' ) {
        if(operation_mode == MODE_IDLE) {
          prepare_display_for_text_entry();
          clear_display_buffer();
          operation_mode = MODE_TEXT_ENTRY;
          digitalWrite(LED, HIGH);
        } else {
          // Complete the message
          exit_text_entry_mode();
          operation_mode = MODE_IDLE;
          digitalWrite(LED, LOW);
        }
      } else {
        if( operation_mode == MODE_TEXT_ENTRY){
          Keyboard.write(rxByte);
        }
      }
      delay(10);
  }
}
