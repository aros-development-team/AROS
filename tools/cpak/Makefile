
all   : real-all

include $(TOP)/config/make.cfg

real-all: $(CPAK) $(CPAK2)

$(CPAK) : cpak.c
	gcc -Wall -Wunused -O2 cpak.c -o $(CPAK)
	strip $(CPAK)

$(CPAK2) : cpak2.c
	gcc -Wall -Wunused -O2 cpak2.c -o $(CPAK2)
	strip $(CPAK2)

clean :
	rm -f $(CPAK) $(CPAK2)

