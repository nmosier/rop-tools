CFLAGS=-c -g -Wall -pedantic -Werror
LIBS=-lelf `llvm-config --cxxflags --libs` `llvm-config --ldflags`
OBJS=gadgdump.o ropelf.o ropbank.o util.o trie.o vec.o ropasm.o ropalg.o

gadgdump: $(OBJS)
	gcc $(LIBS) -o $@ $^

%.o: %.c %.h
	gcc $(CFLAGS) -o $*.o $*.c

.PHONY: clean
clean:
	rm -f *.o elftest
