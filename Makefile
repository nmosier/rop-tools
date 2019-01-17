CFLAGS=-c -g -Wall -pedantic -Wno-comment
LIBS=-lelf `llvm-config --cxxflags --libs` `llvm-config --ldflags`
OBJS=elftest.o ropelf.o util.o trie.o vec.o ropasm.o ropalg.o

elftest: $(OBJS)
	gcc $(LIBS) -o $@ $^

%.o: %.c
	gcc $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f *.o elftest
