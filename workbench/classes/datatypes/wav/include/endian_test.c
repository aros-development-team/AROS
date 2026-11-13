/*
	For testing the speed of endian.h endian conversion routines

	PPC (in order of performance. fastest versions first, then slower ones):
	1. read_leXX/write_leXX (asm) - uses the PPC's endian converting load/store instructions
	2. endianXX routines (asm) - much slower than the above but still a *lot* faster than the plain C stuff
	3. plain C versions (endianXX/read_leXX/write_leXX)
*/

#ifdef __amigaos4__
#define __USE_INLINE__ 1
#endif

#include <exec/types.h>

#ifndef __amigaos4__
typedef BYTE BYTE;
typedef UBYTE UBYTE;
typedef WORD WORD;
typedef UWORD UWORD;
typedef LONG LONG;
typedef ULONG ULONG;
typedef signed long long int int64;
typedef unsigned long long int uint64;
#endif

#include <proto/exec.h>
#include <proto/lowlevel.h>

#include <stdio.h>

#include "endian.h"

struct Library * LowLevelBase;

struct LowLevelIFace * ILowLevel;

struct EClockVal time = {0};

int main (void) {
	int retv = 20;
	ULONG a = 0x12345678;
	ULONG b = 0xEFCDAB89;
	uint64 c = 0x1122334455667788;
	LONG i;
	ULONG secs;

	LowLevelBase = OpenLibrary("lowlevel.library", 0);
	if (!LowLevelBase) goto error;
	#ifdef __amigaos4__
	ILowLevel = (struct LowLevelIFace *)GetInterface(LowLevelBase, "main", 1, NULL);
	if (!ILowLevel) goto error;
	#endif

	retv = 0;

	printf("%lx, %lx, %llx\n", a, b, c);

	a = endian32(a);
	b = endian32(b);
	c = endian64(c);

	printf("%lx, %lx, %llx\n", a, b, c);

	write_le32(&a, a);
	write_le32(&b, b);
	//write_le64(&c, c);

	printf("%lx, %lx, %llx\n", a, b, c);
	printf("%x, %x\n", (UWORD)endian16(a), (UWORD)endian16(b));

	#define NUM_LOOPS (256*1000*1000L)

	printf("Iterating loop %ld times.\n", NUM_LOOPS);
	ElapsedTime(&time);
	for (i=0; i<NUM_LOOPS; i++) {
		a = read_le32(&a);
		b = read_le32(&b);
		c = read_le64(&c);
	}
	secs = ElapsedTime(&time);
	printf ("Took %f seconds.\n", secs / 65536.0);

	printf("Iterating loop %ld times.\n", NUM_LOOPS);
	ElapsedTime(&time);
	for (i=0; i<NUM_LOOPS; i++) {
		write_le32(&a, a);
		write_le32(&b, b);
		write_le64(&c, c);
	}
	secs = ElapsedTime(&time);
	printf ("Took %f seconds.\n", secs / 65536.0);

	printf("Iterating loop %ld times.\n", NUM_LOOPS);
	ElapsedTime(&time);
	for (i=0; i<NUM_LOOPS; i++) {
		a = endian32(a);
		b = endian32(b);
		c = endian64(c);
	}
	secs = ElapsedTime(&time);
	printf ("Took %f seconds.\n", secs / 65536.0);

error:
	#ifdef __amigaos4__
	DropInterface((struct Interface *)ILowLevel);
	#endif
	CloseLibrary(LowLevelBase);
	return retv;
}
