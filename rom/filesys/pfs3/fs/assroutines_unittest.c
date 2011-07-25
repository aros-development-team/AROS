;/*
phxass I=Watti:NDK_3.9/Include/include_i/ assroutines.asm
sc resopt parms=r assroutines_unittest.c
sc resopt link assroutines_unittest.o assroutines.o to assroutines_unittest
gcc -noixemul -O2 -o assroutines.o -c assroutines.c
gcc -noixemul -O2 -o assroutines_unittest.o -c assroutines_unittest.c
gcc -noixemul assroutines_unittest.o assroutines.o -o assroutines_unittest_c
quit
*/

#include <exec/types.h>
#include "ass_protos.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
	struct 
	{
		ULONG a;
		UWORD b;
		ULONG r;
	} div[] =
	{
		{0,1,0x00000000},
		{1,1,0x00000001},
		{1,2,0x00010000},
		{1,3,0x00010000},
		{1,4,0x00010000},
		{1,1000,0x00010000},
		{1,6000,0x00010000},
		{10000,666,0x000a000f},
		{80000,666,0x01de0015},
		{80000,1,0x00003880},
	};
	const div_numof = sizeof(div) / sizeof(div[0]);
	int i;
	UBYTE tmp[256], *p;
	UBYTE t2[64];

	printf("\ndivide:\n");

	for (i = 0; i< div_numof; i++)
	{
		printf("%d / %d = 0x%08x\n", div[i].a, div[i].b, divide(div[i].a, div[i].b));
		if (divide(div[i].a, div[i].b) != div[i].r)
		{
			printf("failed!\n");
			return 5;
		}
	}

	printf("\nctodstr:\n");
	ctodstr("", tmp);
	if (tmp[0] != 0 || memcmp(tmp+1, "", 1)) { printf("failed \"\"!\n"); return 5; }
	ctodstr("foobar", tmp);
	if (tmp[0] != 6 || memcmp(tmp+1, "foobar", 6)) { printf("failed \"foobar\"!\n"); return 5; }

	printf("\nstcu_d:\n");
	stcu_d(tmp, 0);
	if (strcmp(tmp, "0")) { printf("failed 0\n"); return 5; }
	stcu_d(tmp, 123456);
	if (strcmp(tmp, "123456")) { printf("failed 123456\n"); return 5; }
	stcu_d(tmp, 10000000);
	if (strcmp(tmp, "10000000")) { printf("failed 10000000\n"); return 5; }

	printf("\nstpcpy:\n");
	p = stpcpy(tmp, "");
	if (p != tmp || strcmp(tmp, "")) { printf("failed \"\"\n"); return 5; }
	p = stpcpy(tmp, "foobar");
	if (p != tmp + 6 || strcmp(tmp, "foobar")) { printf("failed \"foobar\"\n"); return 5; }

	printf("\nintltoupper:\n");

	ctodstr("", tmp);
	intltoupper(tmp);
	if (tmp[0] != 0) { printf("failed \"\"\n"); return 5; }
	ctodstr("fooBaröäåßð!@£", tmp);
	intltoupper(tmp);
	if (tmp[0] != 14 || memcmp(tmp + 1, "FOOBARÖÄÅßÐ!@£", 14)) { printf("failed \"fooBaröäåßð!@£\"\n"); return 5; }

	printf("\nintlcmp:\n");
	ctodstr("foo", tmp);
	intltoupper(tmp);
	if (intlcmp(tmp, tmp) != 1) { printf("failed \"foo\" selfcompare\n"); return 5; }
	ctodstr("foo", tmp);
	intltoupper(tmp);
	ctodstr("foo", t2);
	if (intlcmp(tmp, t2) != 1) { printf("failed \"FOO\" vs \"foo\"\n"); return 5; }
	ctodstr("fooBaröäåßð!@£", tmp);
	intltoupper(tmp);
	if (intlcmp(tmp, tmp) != 1) { printf("failed \"fooBaröäåßð!@£\" selfcompare\n"); return 5; }
	ctodstr("fooBar", t2);
	intltoupper(t2);
	if (intlcmp(tmp, t2) != 0) { printf("failed \"fooBaröäåßð!@£\" vs \"fooBar\"\n"); return 5; }
	ctodstr("fooBaröäåßð!@£xxx", t2);
	intltoupper(t2);
	if (intlcmp(tmp, t2) != 0) { printf("failed \"fooBaröäåßð!@£\" vs \"fooBaröäåßð!@£xxx\"\n"); return 5; }
	ctodstr("fooBaxöäåßð!@£", t2);
	if (intlcmp(tmp, t2) != 0) { printf("failed \"fooBaröäåßð!@£\" vs \"fooBaxöäåßð!@£\"\n"); return 5; }
	ctodstr("fooBar", t2);
	if (intlcmp(tmp, t2) != 0) { printf("failed \"fooBaröäåßð!@£\" vs \"fooBar\"\n"); return 5; }
	ctodstr("fooBaröäåßð!@£xxx", t2);
	if (intlcmp(tmp, t2) != 0) { printf("failed \"fooBaröäåßð!@£\" vs \"fooBaröäåßð!@£xxx\"\n"); return 5; }
	ctodstr("fooBaxöäåßð!@£", t2);
	if (intlcmp(tmp, t2) != 0) { printf("failed \"fooBaröäåßð!@£\" vs \"fooBaxöäåßð!@£\"\n"); return 5; }

	printf("\nintlcdcmp:\n");
	ctodstr("", tmp);
	intltoupper(tmp);
	if (intlcdcmp("", tmp) != 1) { printf("failed \"\"\n"); return 5; }
	ctodstr("A", tmp);
	if (intlcdcmp("A", tmp) != 1) { printf("failed \"A\" vs \"A\"\n"); return 5; }
	ctodstr("a", tmp);
	if (intlcdcmp("A", tmp) != 1) { printf("failed \"A\" vs \"a\"\n"); return 5; }
	ctodstr("abcDEF", tmp);
	if (intlcdcmp("ABCDEF", tmp) != 1) { printf("failed \"ABCDEF\" vs \"abcDEF\"\n"); return 5; }
	ctodstr("fooBaröäåßð!@£", tmp);
	if (intlcdcmp("FOOBARÖÄÅßÐ!@£", tmp) != 1) { printf("failed \"FOOBARÖÄÅßÐ!@£\" vs \"fooBaröäåßð!@£\"n"); return 5; }
	ctodstr("fooBaröäåßð!@£", tmp);
	if (intlcdcmp("FOOBARÖÄÅßÐ!@£", tmp) != 1) { printf("failed \"FOOBARÖÄÅßÐ!@£\" vs \"fooBaröäåßð!@£\"\n"); return 5; }
	ctodstr("fooBaröäåßð!@£", tmp);
	if (intlcdcmp("FOOBAXÖÄÅßÐ!@£", tmp) != 0) { printf("failed \"FOOBAXÖÄÅßÐ!@£\" vs \"fooBaröäåßð!@£\"\n"); return 5; }
	ctodstr("fooBaröäåßð!@£", tmp);
	if (intlcdcmp("FOOBAR", tmp) != 0) { printf("failed \"FOOBAR\" vs \"fooBaröäåßð!@£\"\n"); return 5; }
	ctodstr("fooBaröäåßð!@£", tmp);
	if (intlcdcmp("FOOBARÖÄÅßÐ!@£XXX", tmp) != 0) { printf("failed \"FOOBARÖÄÅßÐ!@£XXX\" vs \"fooBaröäåßð!@£\"\n"); return 5; }

	return 0;
}


void EntryPoint(void)
{
}
