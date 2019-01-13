CFLAGS=-c -g -Wall -pedantic


elftest: elftest.o ropelf.o util.o trie.o vec.o
	gcc -lelf -o $@ $^

%.o: %.c
	gcc $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f *.o elftest
