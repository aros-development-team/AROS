
all   : real-all

include $(TOP)/config/make.cfg

real-all: $(CPAK)

$(CPAK) : cpak.c
	gcc -Wall -Wunused -O2 cpak.c -o $(CPAK)
	strip $(CPAK)

clean :
	rm -f $(CPAK)

