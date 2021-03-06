package blocks

import (
	"../parser/"
	//"fmt"
)

// for more info see the ArduinoPWM.go file
type ArduinoPWMOutput struct {
	OutputBlockData
	address  string
	question ArduinoPWMPacket
}

func (b *ArduinoPWMOutput) InputIsUndefined() bool {
	isUndefined := false

	for _, v := range b.in {
		if v == UNDEFINED {
			isUndefined = true
		}
	}

	return isUndefined
}

func (b *ArduinoPWMOutput) Update() {
	if b.InputIsUndefined() {
		return
	}

	numBytes := len(b.in)
	b.question.Header1.NumBytes = uint8(numBytes)

	for i, v := range b.in {
		if v < 256.0 {
			b.question.Bytes[i] = byte(uint8(v))
		} else {
			b.question.Bytes[i] = 0
		}
	}

	//fmt.Println("sending message")
	_, err := SendReceiveArduinoPWM(b.address, b.question) // ORIG
	//SendArduinoPWM(b.address, b.question)

	if err == nil { // ORIG
		b.out = b.in
	} // ORIG
}

func ArduinoPWMOutputConstructor(name string, words []string) Block {
	var address string
	var bitRate int
	var pulseWidth int
	var clearCount int
	var numRepeat int

	positional := parser.PositionalArgs(&address, &bitRate, &pulseWidth, &clearCount, &numRepeat)
	optional := parser.OptionalArgs()

	parser.ParseArgs(words, positional, optional)

	// function implemented in ./Serial.go
	configId := 0
	MakeSerialPort(address, bitRate, configId)

	b := &ArduinoPWMOutput{
		address: address,
		question: ArduinoPWMPacket{
			Header1: ArduinoPWMHeader1{
				OpCode:     ARDUINOPWM_WRITEOP,
				PulseWidth: uint16(pulseWidth),
			},
			Header2: ArduinoPWMHeader2{
				ClearCount: uint8(clearCount),
				NumRepeat:  uint8(numRepeat),
			},
		},
	}

	return b
}

var ArduinoPWMOutputOk = AddConstructor("ArduinoPWMOutput", ArduinoPWMOutputConstructor)
