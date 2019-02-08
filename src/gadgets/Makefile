CFLAGS=-c -g -Wall -pedantic
LIBS=-lelf `llvm-config --cxxflags --libs` `llvm-config --ldflags`
OBJS=gadgets.o ropelf.o ropbank.o util.o trie.o vec.o ropasm.o ropalg.o

gadgets: $(OBJS)
	gcc $(LIBS) -o $@ $^

%.o: %.c %.h
	gcc $(CFLAGS) -o $*.o $*.c

.PHONY: clean
clean:
	rm -f *.o elftest
