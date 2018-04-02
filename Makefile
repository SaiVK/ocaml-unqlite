
all: build test

build:
	jbuilder build

test:
	jbuilder runtest

clean:
	rm -rf .build
	rm -f *.native
	jbuilder clean

.PHONY: build all test
