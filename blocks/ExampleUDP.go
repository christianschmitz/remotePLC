package blocks

// code used by both ExampleUDPInput.go and ExampleUDPOutput.go

import (
  "../logger/"
  "bytes"
  "encoding/binary"
  "fmt"
  "log"
  "net"
)

// UDP protocol over port 1800 
type ExampleUDPHeader1 struct {
  Byte1 byte
  Byte2 byte
  Uint1 uint16
}

type ExampleUDPHeader2 struct {
  Uint1 uint16 // the number of records, TODO: rename everywhere to NRecords
  Uint2 uint16 
}

type ExampleUDPRecord struct {
  Uint1 uint8 // example: counter
  Uint2 uint16 // example: address on target machine
  Uint3 uint8 // example: status byte
  Data [4]byte // example: a float32
}

type ExampleUDPPacket struct {
  Header1 ExampleUDPHeader1
  Header2 ExampleUDPHeader2
  Records [10]ExampleUDPRecord // the max number of records
}

func exampleUDPPacketToBytes(p ExampleUDPPacket) []byte {
  b := new(bytes.Buffer)
  err := binary.Write(b, binary.LittleEndian, p)
  logger.WriteError("exampleUDPPacketToBytes()", err)

  // example: n records is stored somewhere in the second header
  nRecords := int(p.Header2.Uint1)
  nBytes := 4+4+nRecords*8

  return b.Bytes()[0:nBytes]
}

func exampleUDPBytesToPacket(b []byte) ExampleUDPPacket {
  var p ExampleUDPPacket
  buffer := bytes.NewBuffer(b)
  binary.Read(buffer, binary.LittleEndian, &p)

  // TODO: handle errors here
  return p
} 

var exampleUDPConnection net.Conn // shared by ExampleUDPInput and ExampleUDPOutput

// a common port for sending and receiving
func assertExampleUDPConnection(ipaddr string) {
  port := "666"

  if exampleUDPConnection == nil {
    // resolve the addresses
    raddr, errRaddr := net.ResolveUDPAddr("udp", ipaddr+":"+port)
    if errRaddr != nil { 
      log.Fatal(errRaddr) 
    }

    laddr, errLaddr := net.ResolveUDPAddr("udp", ":"+port)
    if errLaddr != nil {
      log.Fatal(errLaddr)
    }

    var errDial error
    exampleUDPConnection, errDial = net.DialUDP("udp", laddr, raddr)
    if errDial != nil {
      log.Fatal(errDial)
    }
  }
}

// handle the carried 4 byte data
var ExampleUDPBytesToFloat = make(map[string]func([4]byte)float64)

var ExampleUDPFloatToBytes = make(map[string]func(float64)[4]byte)

func AddExampleUDPBytesToFloat(key string, fn func([4]byte)float64) bool {
  ExampleUDPBytesToFloat[key] = fn
  return true
}

func AddExampleUDPFloatToBytes(key string, fn func(float64)[4]byte) bool {
  ExampleUDPFloatToBytes[key] = fn
  return true
}

func ExampleUDPType1BytesToFloat(b [4]byte) float64 {
  // example: the last 2 byte are read in little endian fashion into an int16, the first two are ignored
  // finally convert literally to float64

  buffer := bytes.NewBuffer(b[2:])
  var tmp int16
  binary.Read(buffer, binary.LittleEndian, &tmp)

  x := float64(tmp)

  return x
}

var ExampleUDPType1BytesToFloatOk = AddExampleUDPBytesToFloat("Type1", ExampleUDPType1BytesToFloat)

// TODO: write inverse function for Type1: func(float64)[4]byte

func ExampleUDPType2BytesToFloat(b [4]byte) float64 {
  // Take first two bytes instead of last two bytes
  buffer := bytes.NewBuffer(b[0:2])
  var tmp uint16
  binary.Read(buffer, binary.LittleEndian, &tmp)

  x := float64(tmp)
  return x
}

var ExampleUDPType2BytesToFloatOk = AddExampleUDPBytesToFloat("Type2", ExampleUDPType2BytesToFloat)

func ExampleUDPType2FloatToBytes(x float64) (b [4]byte) {
  // convert to uint16
  tmp := uint16(x)
  buffer := new(bytes.Buffer)
  err := binary.Write(buffer, binary.LittleEndian, tmp)
  logger.WriteError("ExampleUDPType2FloatToBytes()", err)

  b[0] = buffer.Bytes()[0]
  b[1] = buffer.Bytes()[1]
  fmt.Println(b, x)
  return
}

var ExampleUDPType2FloatToBytesOk = AddExampleUDPFloatToBytes("Type2", ExampleUDPType2FloatToBytes)
