package blocks

import (
  "../logger/"
  "fmt"
  "net"
  "log"
  "strings"
  "strconv"
)

// see ExampleUDP.go for protocol details (like the data structure)

type ExampleUDPInput struct {
  InputBlockData
  questionBytes []byte 

  converter func([4]byte)float64 // for the carried data

  conn net.Conn
}

func (b *ExampleUDPInput) Update() {
  nwrite, errWrite := b.conn.Write(b.question)
  if nwrite <= 0 || errWrite != nil {
    log.Fatal("failed to writeto, ", errWrite)
  }

  answerBytes := make([]byte, 1460) // max length of a udp packet
  nread, err := b.conn.Read(answerBytes)

  if err != nil || nread <= 0 {
    logger.WriteEvent("warning: problem getting udp packet")
  }

  // now parse the records
  answer := exampleUDPBytesToPacket(answerBytes)

  // Get the number of records and populate b.out
  nRecords := int(answer.Header2.Uint1)
  if len(b.out) != nRecords {
    b.out = make([]float64, nRecords)
  }

  // TODO: status warning
  for i := 0; i < nRecords; i++ {
    b.out[i] = b.converter(answer.Reconrds[i].Data)
  }

  b.in = b.out
  return
}

func ExampleUDPInputConstructor(words []string) Block {
  ipaddr := words[0]
  converterType := words[1]
  recordWords := words[2:]

  question := ExampleUDPPacket{
    Header1: ExampleUDPHeader1{
      Byte1: 0xab,
    },
    Header2: ExampleUDPHeader2{
      Uint2: 1, 
    },
  }

  // store all the sdo's
  nRecords := len(recordWords)
  if nRecords == 0 {
    log.Fatal("must specify at least one record")
  } else if nRecords > 10 { // TODO: not hardcoded quantity
    log.Fatal("too many records specified")
  }

  question.Header1.Uint1 = uint16(nRecords)
  for i, w := range recordWords {
    id, errId := strconv.ParseUint(w, 0, 16)
    logger.WriteError("ExampleUDPInputConstructor()", errId)

    question.Records[i] = ExampleUDPRecord{
      Uint2: uint16(id),
    }
  }

  // convert the msg into a []byte
  questionBytes := exampleUDPPacketToBytes(question)

  assertExampleUDPConnection(ipaddr)

  b := &ExampleUDPInput{
    questionBytes: questionBytes,
    conn: exampleUDPConnection, 
    converter: ExampleUDPBytesToFloat[converterType],
  }

  return b
}

var ExampleUDPInputConstructorOk = AddConstructor("ExampleUDPInput", ExampleUDPInputConstructor)
