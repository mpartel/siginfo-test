
all: test

test: test.c
	gcc -o test -Wall -Wno-div-by-zero -Wno-unused-value -O0 -g test.c -static -lm

clean:
	rm -f test
