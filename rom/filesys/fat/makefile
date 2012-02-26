OS=$(shell uname)
ifeq "$(OS)" "AROS"
STARTUP  = startup-aros.c
NOIXEMUL =
INCLUDES =
LIBS     = -lrom
PROG     = fat.handler
DESTDIR  = DEVS:
else
STARTUP  = startup-amigaos.c
NOIXEMUL = -noixemul
INCLUDES = -isystem .
LIBS     = -ldebug
PROG     = fat-handler
DESTDIR  = L:
endif

CFLAGS = -O2 -Wall

FILES := $(STARTUP) cache.c direntry.c disk.c fat.c file.c lock.c \
	 main.c names.c packet.c ops.c support.c notify.c charset.c \
	 timer.c

OBJS = $(FILES:.c=.o)
            
all: $(PROG)

$(OBJS): %.o: %.c
	gcc -c $(CFLAGS) $(INCLUDES) $< $(NOIXEMUL)

$(PROG): $(OBJS)
	gcc $(NOIXEMUL) -nostartfiles $^ -o $@ $(LIBS)

cache.o: cache.h debug.h fat_fs.h fat_struct.h fat_protos.h
charset.o: fat_fs.h fat_struct.h cache.h support.h charset.h
direntry.o: cache.h fat_fs.h fat_struct.h support.h fat_protos.h debug.h
disk.o: fat_fs.h fat_struct.h cache.h support.h fat_protos.h debug.h
fat.o: fat_fs.h fat_struct.h cache.h support.h fat_protos.h debug.h
file.o: fat_fs.h fat_struct.h cache.h support.h fat_protos.h debug.h
lock.o: fat_fs.h fat_struct.h cache.h support.h fat_protos.h debug.h
main.o: fat_fs.h fat_struct.h cache.h support.h fat_protos.h charset.h debug.h
names.o: fat_fs.h fat_struct.h cache.h support.h fat_protos.h debug.h
notify.o: fat_fs.h fat_struct.h cache.h support.h fat_protos.h debug.h
ops.o: fat_fs.h fat_struct.h cache.h support.h fat_protos.h debug.h
packet.o: fat_fs.h fat_struct.h cache.h support.h fat_protos.h debug.h
startup.o: debug.h
support.o: fat_fs.h fat_struct.h cache.h support.h
timer.o: fat_fs.h fat_struct.h fat_protos.h debug.h

clean:
	Delete $(OBJS)
	Delete $(PROG)

install: all
	Copy $(PROG) $(DESTDIR)

