CC = gcc
CFLAGS = -O2 -Wall

LINS =
SRC = *.c

OBJS = test.o method.o timerclass.o oop.o support.o

all: oop

clean:
	rm $(OBJS) oop

veryclean: clean
	rm .dep

depend:
	touch .dep
	makedepend -f.dep $(SRC)

oop: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o oop

-include .dep
