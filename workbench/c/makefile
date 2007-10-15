CFLAGS = -O2

FILES = Assign.c

PROGS = $(FILES:.c=)

$(PROGS): %: %.c
	gcc $(CFLAGS) $< -o $@

clean:
	Delete $(PROGS)

install:
	Copy $(PROGS) TO C:
