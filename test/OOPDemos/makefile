-include Make.Cfg
CC = gcc
CFLAGS = -O2 -Wall -I./include -D $(SYSTEM) -g

LIBS =
SRC = *.c ./sysdep/$(SYSFILE).c

OBJS = test.o method.o timerclass.o oop.o support.o hash.o ./sysdep/$(SYSFILE).o

all: oop

clean:
	-rm $(OBJS) oop

veryclean: clean
	-rm .dep .dep.bak

depend:
	touch .dep
	makedepend -f.dep -I./include $(SRC)

oop: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o oop
	
config:
	./configure

-include .dep

dist: veryclean
	cd .. ; \
	tar cvzf $(HOME)/oop.tgz oop
