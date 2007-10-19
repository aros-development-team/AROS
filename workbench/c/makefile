CFLAGS = -O2

PURE_FILES = Assign.c

PURE_PROGS = $(PURE_FILES:.c=)

$(PURE_PROGS): %: %.c
	gcc $(CFLAGS) $< -o $@ -nostartfiles
	Protect $@ +p

clean:
	Delete $(PURE_PROGS)

install:
	Copy $(PURE_PROGS) TO C:
