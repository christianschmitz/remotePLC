package blocks

type ForkLine struct {
	BlockData
	b0 string
	b1 []string
}

func (b *ForkLine) Update() {
	b.in = Blocks[b.b0].Get()

	for _, v := range b.b1 {
		Blocks[v].Put(b.in)
	}

	b.out = b.in
}

func ForkLineConstructor(words []string) Block {
	b0 := words[0]
	b1 := words[1:]

	b := &ForkLine{b0: b0, b1: b1}
	return b
}

var ForkLineConstructorOk = AddConstructor("ForkLine", ForkLineConstructor)