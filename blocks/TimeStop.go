package blocks

import "time"
import "log"

type TimeStop struct {
	BlockData
	seconds float64
	start   time.Time
}

func (b *TimeStop) Update() {
	d := time.Since(b.start)

	t := d.Seconds()
	if t < b.seconds {
		b.out = []float64{t / b.seconds}
	} else {
		b.out = []float64{-t / b.seconds}
	}
}

func TimeStopConstructor(words []string) Block {
	d, err := time.ParseDuration(words[0])
	if err != nil {
		log.Fatal("in TimeStopConstructor, \"", words[0], "\", ", err)
	}
	seconds := d.Seconds()
	start := time.Now()

	b := &TimeStop{seconds: seconds, start: start}
	return b
}

var TimeStopConstructorOk = AddConstructor("TimeStop", TimeStopConstructor)
