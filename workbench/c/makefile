TOP=..
RM=rm -rf
CFLAGS=$(COMMON_CFLAGS) -fomit-frame-pointer -Dmain=submain -I $(TOP)/include -I $(TOP)/amiga/include

EXES = helloworld newshell type echo cd avail list librarytest run execute \
     path shell dir wait

all: $(EXES)

clean:
	$(RM) $(EXES) core *.o

wait: wait.c
	$(CC) $(CFLAGS) $^ -c -o $@

newshell: newshell.c
	$(CC) $(CFLAGS) $^ -c -o $@

helloworld: helloworld.c
	$(CC) $(CFLAGS) $^ -c -o $@

type: type.c
	$(CC) $(CFLAGS) $^ -c -o $@

echo: echo.c
	$(CC) $(CFLAGS) $^ -c -o $@

cd: cd.c
	$(CC) $(CFLAGS) $^ -c -o $@

avail: avail.c
	$(CC) $(CFLAGS) $^ -c -o $@

list: list.c
	$(CC) $(CFLAGS) $^ -c -o $@

dir: dir.c strlen.c qsort.c strcpy.c
	$(CC) $(CFLAGS) dir.c -c -o $@

librarytest: librarytest.c
	$(CC) $(CFLAGS) -I ../libs $^ -c -o $@

run: run.c
	$(CC) $(CFLAGS) $^ -c -o $@

execute: execute.c
	$(CC) $(CFLAGS) $^ -c -o $@

path: path.c
	$(CC) $(CFLAGS) $^ -c -o $@

shell: shell.c
	$(CC) $(CFLAGS) $^ -c -o $@

