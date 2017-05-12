/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
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

    	/* Clear existing checksum */
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
	int err, fd, i, retval = EXIT_FAILURE;
	void *rom;
	uint8_t *p;
	uint32_t size = 0;
	off_t origlen, len;

	fd = open(argv[1], O_RDWR | O_CREAT, 0666);
	if (fd < 0) {
            perror(argv[1]);
            return retval;
	}

        origlen = lseek(fd, 0, SEEK_END);

        /* Make sure we have a valid ROM ID */
        rom = mmap(NULL, origlen, PROT_READ | PROT_WRITE,
                MAP_SHARED, fd, 0);
	if (rom == MAP_FAILED)
        {
            perror(argv[1]);
            close(fd);
            return retval;
        }
        p = (uint8_t*)rom;
        if (p[0] == 0x11 && (p[1] >= 0x11 || p[1] <= 0x14))
        {
            if (p[1] == 0x11)
            {
                size = 256 * 1024;
                if (origlen > size)
                {
                    printf("Warning: ROM ID is for a 256K ROM, but the image is larger -> promoting to 512KB ROM.\n");
                    p[1] = 0x14;
                }
            }
            if (p[1] == 0x14)
                size = 512 * 1024;
        }
        if (size == 0)
            printf("Error: Invalid ROM ID (%02x%02x).\n", p[0], p[1]);
        munmap(rom, origlen);
        if (size == 0)
        {
            close(fd);
            return retval;
        }

        len = origlen;

        if (len > size)
        {
            printf("Error: ROM Size > %uKB (+%uKB).\n", size/1024, (len - size)/1024);
            return retval;
        }

	/* Pad with 0xff */
	for (; len < size; len++) {
	    unsigned char ff = 0xff;
	    write(fd, &ff, 1);
	}

	rom = mmap(NULL, len, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, 0);

	if (rom != MAP_FAILED)
        {
            p = (uint8_t*)rom + len - 20;
            if ((origlen <= (size - 24)) ||
                ((p[0] == (len >> 24) & 0xFF) &&
                 (p[1] == (len >> 16) & 0xFF) &&
                 (p[2] == (len >>  8) & 0xFF) &&
                 (p[3] == (len >> 0) & 0xFF)))
            {
                /* Make sure the rom size is set*/
                p[0] = len >> 24;
                p[1] = len >> 16;
                p[2] = len >>  8;
                p[3] = len >>  0;

                /* Add the interrupt vector offsets,
                 * needed by 68000 and 68010 */
                p = (uint8_t*)rom + len - 16;
                for (i = 0; i < 7; i++) {
                        p[i * 2 + 1] = i + 0x18;
                        p[i * 2 + 0] = 0;
                }

                err = amiga_checksum(rom, len, len - 24, 0);
                err = amiga_checksum(rom, len, len - 24, 1);

                retval = EXIT_SUCCESS;
            }
            else
            {
                printf("Error: Rom Data Size exceeds available space.\n");
            }

            munmap(rom, len);
        }
        else
            perror(argv[1]);

	close(fd);

	return retval;
}


