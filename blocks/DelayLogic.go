package blocks

type DelayLogic struct {
	BlockData
}

func (b *DelayLogic) Update() {
	b.out = b.in
}

func DelayLogicConstructor(name string, words []string) Block {
	b := &DelayLogic{}
	return b
}

var DelayLogicConstructorOk = AddConstructor("DelayLogic", DelayLogicConstructor)
