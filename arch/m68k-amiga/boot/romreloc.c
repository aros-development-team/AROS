/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id: romcheck.c

    Desc: m68k-amiga RAM loadable relocatable ROM image generator
    Lang: english
 */

#include <stdio.h>
#include <stdlib.h>

#define MAXSIZE (2 * 524288)

static void putlong(unsigned char *p, unsigned int v)
{
    p[0] = v >> 24;
    p[1] = v >> 16;
    p[2] = v >> 8;
    p[3] = v;
}
static unsigned int getlong(unsigned char *p)
{
    return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

int main(int argc,char *argv[])
{
	FILE *src1, *src2, *dst;
	unsigned char *b1, *b2, *b3, temp[256];
	int i, j, size;
	unsigned char empty[16] = { 0 };
	
	b1 = calloc(MAXSIZE, 1);
	b2 = calloc(MAXSIZE, 1);
	b3 = calloc(MAXSIZE, 1);
	src1 = fopen("aros-amiga-m68k-ram.20.bin", "rb");
	src2 = fopen("aros-amiga-m68k-ram.c0.bin", "rb");
	if (!src1 || !src2)
		return 0;
	size = fread(b1, 1, MAXSIZE, src1);
	if (fread(b2, 1, MAXSIZE, src2) != size)
		return 0;
	if (size <= 0)
		return 0;
	fclose(src1);
	fclose(src2);
	j = 0;
	for (i = 0; i < size; i += 2) {
		unsigned int v1 = getlong(b1 + i);
		unsigned int v2 = getlong(b2 + i);
		if (v1 == v2)
			continue;
		if ((v1 & 0xffff) != (v2 & 0xffff))
			continue;
		v1 &= 0x1fffff;
		putlong(b1 + i, v1);
		putlong(b3 + j, i);
		j += 4;
		i += 2;
	}
	dst = fopen("aros-amiga-m68k-rel.bin", "wb");
	fwrite("AROS_ROM", 8, 1, src1);
	fwrite(empty, 8, 1, src1);
	putlong(temp + 0, size);
	putlong(temp + 4, j / 4);
	fwrite(temp, 4, 2, dst);
	fwrite(empty, 8, 1, src1);
	fwrite(b1, 1, size, dst);
	fwrite("ROM__END", 8, 1, src1);
	fwrite(empty, 8, 1, src1);
	fwrite(b3, 1, j, dst);
	fwrite("REL__END", 8, 1, src1);
	fwrite(empty, 8, 1, src1);
	fclose(dst);
}

