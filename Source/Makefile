c_t_2.o: c_t_2.c
	${CC} -c -g -std=c99 -O0 $?

tester.o: tester.c
	${CC} -c -g -std=c99 -O0 $?

boost.o: boost.cpp
	g++ -c -std=c99 -g $<

test: boost.o c_t_2.o tester.o
	g++ -o $@ -g $?

clean:
	rm -f c_t_2.o tester.o boost.o test
