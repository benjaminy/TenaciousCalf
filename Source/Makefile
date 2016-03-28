c_t_2.o: c_t_2.c
	${CC} -c -g -O0 $?

tester.o: tester.c
	${CC} -c -g -O0 $?

boost.o: boost.cpp
	g++ -c -g $<

test: boost.o c_t_2.o tester.o
	g++ -o $@ -g $?

clean:
	rm -f c_t_2.o tester.o boost.o test
