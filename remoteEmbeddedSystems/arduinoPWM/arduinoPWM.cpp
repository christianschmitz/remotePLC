#include <Arduino.h>

#include "arduinoPWMPacket.h"
#include "pwmRead.h"
#include "pwmWrite.h"
#include "serialReadWrite.h"

// parameters:
#define OUTPUT_PIN 7
#define SERIAL_BITRATE 9600

arduinoPWMPacket handleMessage(arduinoPWMPacket question) {
  // after every message a reply needs to be sent upstream.
  // this is to assure that everything is synchronous
  arduinoPWMPacket answer;

  switch(question.header1.opCode) {
    case ARDUINO_PWM_OPCODE_WRITE: {
      answer = pwmWrite(question);
    } break;
    case ARDUINO_PWM_OPCODE_READ: {
      answer = pwmRead::pwmRead(question);
    } break;
    default:
      answer.header1.errorCode = ARDUINO_PWM_ERROR_OPCODE_NOT_RECOGNIZED;
      break;
  }

  return answer;
}


void setup() {
  // use the slowest baudrate (9600 bps) for robustness
  serialSetup(SERIAL_BITRATE);

  pwmWriteSetup(OUTPUT_PIN);

  pwmRead::pwmReadSetupUnoPin2();
}

void loop() {
  if (serialReadWriteIsReady()) {
    arduinoPWMPacket question = serialReadMessage();

    arduinoPWMPacket answer = handleMessage(question);
    
    // only return a message in case of the READ OPCODE
    if (question.header1.opCode == ARDUINO_PWM_OPCODE_READ) {
      serialWriteMessage(answer);
    }

    delay(100); // delay between messages, as there might be interference
  }
}

// program entry point
int main(void) {
  init();
  setup();
  for (;;) {
    loop();
  }
}
