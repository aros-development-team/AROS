CC = gcc
srcdir		= /Drivers
common_srcdir	= /Drivers/Common

DRIVER      = cmi8738.audio
MODEFILE	= CMI8738

OBJECTS		= driver-init.o cmi8738.o accel.o misc.o interrupt.o ../common/library_card.o gatestubs.o

CPPFLAGS	= -DCPU="\"603e\"" -I${common_srcdir} -I${srcdir} -DAHI
CFLAGS		= -W -Wall -Wno-unused -mcpu=603e -c -I$(srcdir)/CMI8738 -DDRIVER="${DRIVER}" -O2
LDFLAGS		= -nostartfiles -lm

all:		$(DRIVER)

$(DRIVER):		$(OBJECTS)
	gcc $(LDFLAGS) -o $@ $^ -lm
	strip cmi8738.audio
	cp $@ DEVS:AHI
