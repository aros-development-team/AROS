
CC		= i386-linux-aros-gcc
CC_NATIVE	= gcc
CFLAGS		= -g -W -Wall -Wno-unused
CFLAGS_NATIVE	= -g -W -Wall -Wno-unused $(shell pkg-config --cflags --libs dbus-glib-1)

TARGETS		= dbus-dcop-bridge dbus-dcop dbus-ping-send dbus-ping-listen dbus-dcop.aros dbus-ping.aros

.PHONY:		all clean

all:		$(TARGETS)

clean:
	$(RM) $(TARGETS)

dbus-dcop-bridge:	dbus-dcop-bridge.c
	$(CC_NATIVE) $(CFLAGS_NATIVE) -o $@ $^

dbus-dcop:		dbus-dcop.c
	$(CC_NATIVE) $(CFLAGS_NATIVE) -o $@ $^

dbus-ping-send:		dbus-ping-send.c
	$(CC_NATIVE) $(CFLAGS_NATIVE) -DDBUS_API_SUBJECT_TO_CHANGE -o $@ $^

dbus-ping-listen:	dbus-ping-listen.c
	$(CC_NATIVE) $(CFLAGS_NATIVE) -DDBUS_API_SUBJECT_TO_CHANGE -o $@ $^

dbus-dcop.aros:		dbus-dcop.c dbus-amiga.c
	$(CC) $(CFLAGS) -o $@ $^

dbus-ping.aros:		dbus-ping.c dbus-amiga.c
	$(CC) $(CFLAGS) -o $@ $^
