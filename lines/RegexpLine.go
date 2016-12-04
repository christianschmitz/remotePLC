package lines

import (
	"../blocks/"
)

func RegexpLineConstructor(words []string, b map[string]blocks.Block) Line {
	b0, _ := getRegexpBlocks(b, words[0])
	b1, _ := getRegexpBlocks(b, words[1])

	// truncate to minimum of either
	if len(b0) < len(b1) {
		b1 = b1[:len(b0)]
	} else if len(b0) > len(b1) {
		b0 = b0[:len(b1)]
	}

	l := &LineData{
		b0:        b0,
		b1:        b1,
		DebugName: getDebugName("RegexpLine", words),
	}

	return l
}

var RegexpLineConstructorOk = AddConstructor("RegexpLine", RegexpLineConstructor)
