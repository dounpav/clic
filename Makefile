
CFLAGS=-Wall

all: stack.o clic.o
	gcc clic.o stack.o -o clic $(CFLAGS)

clic.o: clic.c
	gcc -c clic.c $(CFLAGS)

stack.o: stack.c stack.h
	gcc -c stack.c $(CFLAGS)

clean: clic.o stack.o
	rm clic.o stack.o

