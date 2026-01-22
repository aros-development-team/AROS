SRCS = string.c my_startup.asm startup.asm hook.asm
LIBOBJS = string.ogcc 

GCC_C_FLAGS = -ansi -Wall -IINCLUDE_H: -O2 -c
SAS_C_FLAGS = ANSI STRICT NOSTACKCHECK NOSTANDARDIO COMMENTNEST\
              DATA FARONLY NODEBUG


.c.ogcc:
  gcc $(GCC_C_FLAGS) -o $*.ogcc $*.c
  sc nolink $(SAS_C_FLAGS) $*.c

all : libansi.a my_startup.o startup.o hook.o

libansi.a : $(LIBOBJS)
  rm -f libansi.a
  ar -rv libansi.a $(LIBOBJS)

my_startup.o : my_startup.asm
  phxass -q -iINCLUDE_I: -mf my_startup.asm
  hunk2gcc my_startup.o
  rm my_startup.o
  sksh -n -c "mv obj.* my_startup.o"

startup.o : startup.asm
  phxass -q -iINCLUDE_I: -sAWSTARTUP -mf startup.asm
  hunk2gcc startup.o
  rm startup.o
  sksh -n -c "mv obj.* startup.o"

hook.o : hook.asm
  phxass -q -iINCLUDE_I: -mf hook.asm
  hunk2gcc hook.o
  rm hook.o
  sksh -n -c "mv obj.* hook.o"
