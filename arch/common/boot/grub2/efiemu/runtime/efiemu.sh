gcc -c -m32 -DELF32 -o efiemu32.o ./efiemu.c -Wall -Werror -nostdlib -O2 -I. -I../../include
gcc -c -m64 -DELF64 -o efiemu64_c.o ./efiemu.c -Wall -Werror -mcmodel=large -O2 -I. -I../../include
gcc -c -m64 -DELF64 -o efiemu64_s.o ./efiemu.S -Wall -Werror -mcmodel=large -O2 -I. -I../../include
ld -o efiemu64.o -r efiemu64_s.o efiemu64_c.o -nostdlib
