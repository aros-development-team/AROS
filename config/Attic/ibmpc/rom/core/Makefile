# (C) 1997-1998 AROS - The Amia Research OS

NODES = cachecleare.o cacheclearu.o coldreboot.o disable.o enable.o \
    forbid.o getcc.o permit.o setsr.o supervisor.o andimask.o orimask.o \
    intserver.o irq.o storecpu.o restorecpu.o storefpu.o restorefpu.o \
    semaphores.o cachecontrol.o cachepredma.o cachepostdma.o switch.o \
    dispatch.o stackswap.o

$(CORE): machine.i $(NODES)
	@$(LINK) $(NODES) -o $(CORE)

machine.i: geninc
	@./geninc > machine.i

geninc: geninc.c
	gcc -I$(IPATH) -I$(INCLUDE) geninc.c -o geninc

%.o: %.S
	@$(CC) -c $<

clean:
	@rm -f $(NODES) $(CORE) geninc
