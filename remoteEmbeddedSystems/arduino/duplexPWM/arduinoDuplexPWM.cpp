# include <Arduino.h>

// parameters:
#define OUTPUT_PIN 13 
#define INPUT_PIN 12
#define BUFFER_SIZE 1000
#define WRITE_OPCODE 1
#define READ_OPCODE 2

// write incoming serial messages to this pin:
int outputPin = OUTPUT_PIN; 

// read this pin and send to serial:
int inputPin = INPUT_PIN;  

// # buffer related functions and global declaration of buffer itself
// both the incoming serial messages (written to output pin),
//  and the outgoing serial messages (read from input pin),
//  are passed via this buffer to the WriteOutputBytes() function and
//  from the ReadInput function
uint8_t buffer[BUFFER_SIZE];

void ShiftBuffer(int numBytes, int numShift) {
  int i;
  for (i = numBytes; i >= 0; i--) {
    if (i+numShift < BUFFER_SIZE) {
      buffer[i+numShift] = buffer[i];
    }
  }
}

// mostly used for headers
void PrependToBuffer(int numBytesTotal, int numBytesHeader, uint8_t *header) {
  // numBytes by numShift amount to the right
  ShiftBuffer(numBytesTotal, numBytesHeader);

  int i;
  for (i = 0; i < numBytesHeader; i++) {
    buffer[i] = header[i];
  }
}

// # WriteOutput related functions:

// write a bit, then wait a numer of microseconds (pulseWidth)
void WriteOutputBit(uint8_t bit, int pulseWidth) {
  if (bit == 1) {
    digitalWrite(outputPin, HIGH);
  } else {
    digitalWrite(outputPin, LOW);
  }

  delayMicroseconds(pulseWidth);
}

// write a single byte, in char form, to the output pin
//  by cycling through its 8 bits.
// we use a shifting mask for this
void WriteOutputByte(uint8_t byte, int pulseWidth) {
  uint8_t mask = 1;

  int i;
  for (i = 0; i < int(sizeof(uint8_t)); i++) {
    uint8_t bit = byte & mask;

    WriteOutputBit(bit, pulseWidth);


    // shift the mask bit
    // TODO: check if the order of bits is correct,
    //  little or big endian?
    mask = mask << 1; 
  }
}

// inputs to the WriteOutputBytes() function:
//  numBytes: number of bytes to convert to bits and write to the output pin.
//   this number must be smaller than the size of the buffer
//  pulseWidth: in microseconds, the width of a single bit
// if we want to repeat the message, then it is recommended to this upstream
//  by sending the extended byte stream via the serial line. So repeating is 
//  something that the client needs to handle
void WriteOutputBytes(int numBytes, int pulseWidth) {
  int i;
  for (i = 0; i < numBytes; i++) {
    WriteOutputByte(buffer[i], pulseWidth);
  }
}

// # ReadInputBytes related functions:

// in case of timeOutCount==0, the function returns immediately.
//  this can be used to read constant states (ie. not time varying)
int WaitForClearInput(int numBytes, int pulseWidth, int timeOutCount) {
  // whenever we are sampling we need to do it at double the rate of the message pulses
  int halfPulseCount = 0;
  int halfPulseWidth = pulseWidth/2;
  int clearCount = 0; // also at double the sampling rate

  while(clearCount < 2*numBytes && halfPulseCount < 2*timeOutCount) {
    clearCount = digitalRead(inputPin) == HIGH ?  0 : clearCount + 1;

    delayMicroseconds(halfPulseWidth);

    halfPulseCount += 1;
  }

  int pulseCount = halfPulseCount/2;
  return pulseCount;
}

// the first bit of the first byte will always be 0x1
int ReadFirstInputByte(int pulseWidth, int byteI) {
  int halfPulseCount = 0;
  int halfPulseWidth = pulseWidth/2;

  while (digitalRead(inputPin) == LOW) {
    delayMicroseconds(halfPulseWidth);
    halfPulseCount += 1;
  }

  delayMicroseconds(halfPulseWidth);
  halfPulseCount += 1;

  uint8_t byte = 0x1;
  uint8_t mask = 0x1;

  // if both samples are high the pulse is clearly high
  // if there is a mixed state then we only look at the first half pulse
  // if both are false then the pulse is clearly low
  // this in fact means that we only need to look at the first half
  bool firstHalfHigh = true; 

  int i;
  for (i = 1; i < int(sizeof(uint8_t)); i++) {
    firstHalfHigh = digitalRead(inputPin) == HIGH;
    delayMicroseconds(halfPulseWidth);
    halfPulseCount += 1;

    delayMicroseconds(halfPulseWidth);
    halfPulseCount += 1;

    mask = mask << 1;

    // add a bit to the byte if the pulse is high
    if (firstHalfHigh) {
      byte = byte | mask;
    }
  }

  // finally put the byte into the buffer
  buffer[byteI] = byte;

  int pulseCount = halfPulseCount/2;
  return pulseCount;
}

int ReadInputByte(int pulseWidth, int byteI) {
  int halfPulseCount = 0;
  int halfPulseWidth = pulseWidth/2;

  uint8_t byte = 0x0;
  uint8_t mask = 0x0;

  // if both samples are high the pulse is clearly high
  // if there is a mixed state then we only look at the first half pulse
  // if both are false then the pulse is clearly low
  // this in fact means that we only need to look at the first half
  bool firstHalfHigh = true; 

  int i;
  for (i = 1; i < int(sizeof(uint8_t)); i++) {
    firstHalfHigh = digitalRead(inputPin) == HIGH;
    delayMicroseconds(halfPulseWidth);
    halfPulseCount += 1;

    delayMicroseconds(halfPulseWidth);
    halfPulseCount += 1;

    mask = mask << 1;

    // add a bit to the byte if the pulse is high
    if (firstHalfHigh) {
      byte = byte | mask;
    }
  }

  // finally put the byte into the buffer
  buffer[byteI] = byte;

  int pulseCount = halfPulseCount/2;
  return pulseCount;
}

// inputs to the ReadInputBytes() function:
//  numBytes: fill the global buffer with this number of bytes,
//   only then is the reading considered a success.
//   the incoming message is numBytes*pulseWidth long
//  pulseWidth: in microseconds, the width of a single bit
//  timeOutCount: multiply pulseWidth by this number to get the timeOut time. 
//   stop trying to read input after this time.
//   this requires counting the pulses and comparing to this number
// outputs of the ReadInputBytes() function:
//  buffer[]: parsed bytes are saved into the global buffer
//  bool return value: true for succes, false for failure
//   in case of failure the buffer can contain an incomplete message, 
//   the downstream function should then ignore this
bool ReadInputBytes(int numBytes, int pulseWidth, int timeOutCount) {
  bool isSuccess = false;
  int pulseCount = 0;

  // using Nyquist theory we know we need to sample each pulse at 
  //  least twice in order to get all the information
  
  // the inputPin must be in a low state for the length of a message-2.
  //  -2 because the message must be bounded by at least 2 high states
  //  for practical reasons we just use the length of the message (so we don't need handling of numBytes=0, 1 or 2)
  // waiting this long assures that the next high state we read is from the start of a message,
  //  not somewhere halfway
  pulseCount = WaitForClearInput(numBytes, pulseWidth, timeOutCount);

  int byteI = 0;

  // ReadFirstInputByte() waits for the high state, and only then starts sampling
  pulseCount += ReadFirstInputByte(pulseWidth, byteI);
  byteI += 1;

  while (pulseCount <= timeOutCount) {

    pulseCount += ReadInputByte(pulseWidth, byteI);
    byteI += 1;

    if (byteI >= numBytes) {
      isSuccess = true;
      break;
    }
  }

  return isSuccess;
}

// # Serial message related functions:

// # a message has the following structure
// header (6 bytes):
//  - message type (1 byte) : WRITE_OPCODE for a write instruction
//                              READ_OPCODE for a read instruction
//  - numBytes, between 0 and 255 (1 byte)
//  - pulseWidth int (2bytes/16 bits)
//  - timeOutCount int16 (only for the read instruction, otherwise ignored)
// body:
//  - remaining bytes are written/read from/to the pins

// TODO: are there arduino library functions that do this better?
int twoBytesToInt(uint8_t i0, uint8_t i1) {
  int i = int(i0) + int(i1)*256;
  return i;
}

uint8_t intToFirstByte(int i) {
  // i = i0 + i1*256
  uint8_t i0 = i%256;
  return i0;
}

uint8_t intToSecondByte(int i) {
  // i = i0 + i1*256
  uint8_t i1 = i/256;
  return i1;
}

void SetHeader(int opCode, int numBytes, int pulseWidth, int timeOutCount, uint8_t header[6]) {
  header[0] = uint8_t(opCode);

  header[1] = uint8_t(numBytes);

  header[2] = intToFirstByte(pulseWidth);
  header[3] = intToSecondByte(pulseWidth);

  header[4] = intToFirstByte(timeOutCount);
  header[5] = intToSecondByte(timeOutCount);
}

void GetHeader(uint8_t header[6], int *opCode, int *numBytes, int *pulseWidth, int *timeOutCount) {
  *opCode = int(header[0]);
  *numBytes = int(header[1]);
  *pulseWidth = twoBytesToInt(header[2], header[3]);
  *timeOutCount = twoBytesToInt(header[4], header[5]);
}

// HandleMessage() inputs:
//  - the timeOutCount is ignored in the case of the write instruction opCode (WRITE_OPCODE)
// do nothing in case the opCode isn't recognized
void HandleMessage(int opCode, int numBytes, int pulseWidth, int timeOutCount) {
  switch(opCode) {
    case WRITE_OPCODE: {
      WriteOutputBytes(numBytes, pulseWidth);
    } break;
    case READ_OPCODE: {
      bool ok = ReadInputBytes(numBytes, pulseWidth, timeOutCount);
      if (ok) {
        // send the reply message via the serial line
        // the reply contains the same header as the received message
        uint8_t header[6];

        SetHeader(opCode, numBytes, pulseWidth, timeOutCount, header);

        PrependToBuffer(numBytes, 6, header);

        Serial.write(buffer, 6 + numBytes);
      }
    } break;
    default:
      // do nothing in case the opCode isn't recognized
      break;
  }
}

void ParseSerialMessages() {
  bool serialIsClear = true;
  int prevOpCode = 0;

  while (Serial.available() > 0) {
    // read the header 
    Serial.readBytes((char*)buffer, 6);

    int opCode, numBytes, pulseWidth, timeOutCount;
    GetHeader(buffer, &opCode, &numBytes, &pulseWidth, &timeOutCount);

    // read the body
    Serial.readBytes((char*)buffer, numBytes);

    // decide what to do with message
    //  to make sure the read instructions and write instructions are treated equally
    //  (an overeager client could fill the buffer with its onesided request)
    bool isFair = serialIsClear || (prevOpCode != opCode);
    if (isFair) {
      HandleMessage(opCode, numBytes, pulseWidth, timeOutCount);

      prevOpCode = opCode;
    } else {
      // ignore the current message, and read the next message
      continue;
    }

    // after the first message of the serial buffer has been parsed
    //  the remaining messages need to be checked for fairness
    serialIsClear = false;
  }
}

// no delay in this loop
void loop() {
  ParseSerialMessages();
}

// the slowest baudrate (9600 bps) for robustness
void setup() {
  Serial.begin(9600, SERIAL_8N2);
  pinMode(outputPin, OUTPUT);
  pinMode(inputPin , INPUT);
}

// program entry point
int main(void) {
  init();
  setup();
  for (;;) {
    loop();
  }
}