cow_trie.o: cow_trie.c
	g++ -c -g -O1 $<

tester.o: tester.c
	g++ -c -g -O1 $<

boost.o: boost.cpp
	g++ -c -g $<

test: boost.o cow_trie.o tester.o
	g++ -o $@ -g $?

clean:
	rm -f cow_trie.o tester.o boost.o test
