/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: m68k-amiga ROM checksum generator
    Lang: english
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/mman.h>

static int amiga_checksum(uint8_t *mem, int size, uint32_t chkoff, int update)
{
    uint32_t oldcksum = 0, cksum = 0, prevck = 0;
    int i;

    for (i = 0; i < size; i+=4) {
	uint32_t val = (mem[i+0] << 24) + 
		       (mem[i+1] << 16) +
		       (mem[i+2] <<  8) +
		       (mem[i+3] <<  0);

    	/* Clear old checksum */
	if (update && i == chkoff) {
		oldcksum = val;
		val = 0;
	}

	cksum += val;
	if (cksum < prevck)
	    cksum++;
	prevck = cksum;
    }

    cksum = ~cksum;

    if (update && cksum != oldcksum) {
    	printf("Updating checksum from 0x%08x to 0x%08x\n", oldcksum, cksum);
	
	mem[chkoff + 0] = (cksum >> 24) & 0xff;
	mem[chkoff + 1] = (cksum >> 16) & 0xff;
	mem[chkoff + 2] = (cksum >>  8) & 0xff;
	mem[chkoff + 3] = (cksum >>  0) & 0xff;

	return 1;
   }

   return 0;
}

int main(int argc, char **argv)
{
	int err, fd, i;
	void *rom;
	uint8_t *p;
	uint32_t size = 512 * 1024;
	off_t len;

	fd = open(argv[1], O_RDWR | O_CREAT, 0666);
	if (fd < 0) {
		perror(argv[1]);
		return EXIT_FAILURE;
	}

	/* Pad with 0xff */
	for (len = lseek(fd, 0, SEEK_END); len < size; len++) {
	    unsigned char ff = 0xff;
	    write(fd, &ff, 1);
	}
        if (len > size)
            printf("Warning: ROM Size > 512KB\n");

	rom = mmap(NULL, len, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, 0);
	if (rom == MAP_FAILED) {
		perror(argv[1]);
		close(fd);
		return EXIT_FAILURE;
	}

	/* add interrupt vector offsets, needed by 68000 and 68010 */
	p = (uint8_t*)rom + len - 16;
	for (i = 0; i < 7; i++) {
		p[i * 2 + 1] = i + 0x18;
		p[i * 2 + 0] = 0;
	}

	/* set rom size */
	p = (uint8_t*)rom + len - 20;
	p[0] = len >> 24;
	p[1] = len >> 16;
	p[2] = len >>  8;
	p[3] = len >>  0;

	err = amiga_checksum(rom, len, len - 24, 0);
	err = amiga_checksum(rom, len, len - 24, 1);

	munmap(rom, len);

	close(fd);

	return EXIT_SUCCESS;
}


