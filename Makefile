.PHONY: all
all: bin/gadgets bin/ropc

bin/gadgets: bin
	cd src/gadgets && make && cp gadgets ../../bin/gadgets

bin/ropc: bin
	cd src/compiler && make && cp ropc ../../bin/ropc

bin:
	mkdir $@

.PHONY: clean
clean:
	rm -f bin/*
	pushd src/compiler && make clean && popd
	pushd src/gadgets && make clean && popd
