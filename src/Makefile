CFLAGS=-c -g -Wall -pedantic -Wno-comment -Werror
LIBS=-lelf `llvm-config --cxxflags --libs` `llvm-config --ldflags`
OBJS=elftest.o ropelf.o util.o trie.o vec.o ropasm.o ropalg.o

elftest: $(OBJS)
	gcc $(LIBS) -o $@ $^

%.o: %.c %.h
	gcc $(CFLAGS) -o $*.o $*.c

.PHONY: clean
clean:
	rm -f *.o elftest
