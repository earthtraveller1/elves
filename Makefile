.PHONY all: test.o

main: main.c
	gcc -o main main.c

test.o: main
	./main
