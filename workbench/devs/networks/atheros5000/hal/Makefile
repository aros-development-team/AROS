MAKEFILE = Makefile
CFLAGS = -O2 -Wall -I. -I..
CC = gcc $(CFLAGS)

all: hal.o

include Makefile.inc

.c.o :
	$(CC) -c $< -o $@

hal.o: ${HAL_OBJS}
	ld -r ${HAL_OBJS} -o $@

$(HAL_OBJS) opt_ah.h: $(MAKEFILE) Makefile.inc
$(HAL_OBJS): opt_ah.h

clean:
	Delete hal.o $(HAL_OBJS) opt_ah.h
