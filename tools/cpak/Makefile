
all   : cpak cpak2

cpak  : cpak.c
	gcc -Wall -Wunused -O2 cpak.c -o cpak
	strip cpak

cpak2 : cpak2.c
	gcc -Wall -Wunused -O2 cpak2.c -o cpak2
	strip cpak2

clean :
	rm -f cpak cpak2

