INSTALL_DEST := /SYS/MOSNet

PROG   = netstat
SRCS   = if.c inet.c iso.c main.c mbuf.c mroute.c ns.c route.c \
	 unix.c
CFLAGS = -c -noixemul -I../../include
LDADD  = -noixemul -L../../libkvm -lnet -lmiami -lkvm

OBJS = $(SRCS:.c=.o)

all: $(PROG)

$(PROG): $(OBJS)
	gcc $(OBJS) -o $@ $(LDADD)
	strip --remove-section .comment $@

$(OBJS): %.o: %.c
	gcc $(CFLAGS) $< -o $@

clean:
	rm -f $(PROG) *.o

install: $(PROG)
	mkdir -p $(INSTALL_DEST)/C
	cp $(PROG) $(INSTALL_DEST)/C/

package: all
	mkdir -p $(PKG_DEST)/C
	cp $(PROG) $(PKG_DEST)/C/
