
all: build test examples

build:
	jbuilder build

test:
	jbuilder runtest

examples:
	jbuilder build @examples

clean:
	rm -rf .build
	rm -f *.native
	jbuilder clean

.PHONY: build all test examples
