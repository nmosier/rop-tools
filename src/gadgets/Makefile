CFLAGS=-c -g -Wall -pedantic
GADGETS_LIBS=-lelf `llvm-config --cxxflags --libs` `llvm-config --ldflags` -lstdc++
GADGETS_OBJS=gadgets.o ropelf.o ropbank.o util.o trie.o vec.o ropasm.o ropalg.o ropaddr.o
ELFINTS_LIBS=-lelf -lstdc++
ELFINTS_OBJS=elfints.o ropelf.o ropbank.o util.o elfints_alg.o

.PHONY: all
all: gadgets elfints

gadgets: $(GADGETS_OBJS)
	gcc $(GADGETS_LIBS) -o $@ $^

elfints: $(ELFINTS_OBJS)
	gcc $(ELFINTS_LIBS) -o $@ $^

%.o: %.c %.h
	gcc $(CFLAGS) -o $*.o $*.c

%.o: %.cpp %.h
	gcc $(CFLAGS) -o $*.o $*.cpp

.PHONY: clean
clean:
	rm -f *.o elftest
