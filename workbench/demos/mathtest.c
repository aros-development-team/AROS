#include <proto/exec.h>
#include <proto/aros.h>
#include <proto/dos.h>
#include <proto/mathffp.h>
#include <proto/mathtrans.h>
#include <proto/mathieeesingtrans.h>

#include <stdio.h>

#include <exec/types.h>

struct Library * MathBase;
struct Library * MathTransBase;
struct Library * MathIeeeSingTransBase;

int main(int argc, char ** argv)
{
    LONG FFPOne, FFPTwo, FFPOnehalf, FFPMinusOne, FFPNull;
    LONG SPOne, SPTwo;
    LONG res;

    FFPOne	= 0x80000041UL;
    FFPTwo	= 0x80000042UL;
    FFPMinusOne = 0x800000C1UL;
    FFPOnehalf	= 0x80000040UL;
    FFPNull	= 0x00000000UL;

    SPOne	= 0x3f800000UL;
    SPTwo	= 0x40000000UL;

#define CHECK(func,args,cres) \
    res = func args; \
    if (res != cres) \
	printf ("FAIL: " #func " " #args " in line %d (got=0x%08lx expected=%08lx)\n", __LINE__, res, cres); \
    else \
	printf ("OK  : " #func " " #args "\n");

    if (!(MathBase = OpenLibrary("mathffp.library", 0L)))
    {
	printf ("Couldn't open mathffp.library\n");
	return (0);
    }

    printf("Very basic mathffp functionality test...\n");

    /* this should set the zero-bit*/
    CHECK(SPAbs,(0),FFPNull);

    CHECK(SPFlt,(0),FFPNull);
    CHECK(SPFlt,(1),FFPOne);
    CHECK(SPFlt,(2),FFPTwo);
    CHECK(SPFlt,(-1),FFPMinusOne);
    CHECK(SPAdd,(FFPOne, FFPOne),FFPTwo);
    CHECK(SPDiv,(FFPTwo, FFPOne),FFPOnehalf);
    CHECK(SPMul,(FFPOne, FFPTwo),FFPTwo);

    CloseLibrary(MathBase);

    if (!(MathTransBase = OpenLibrary("mathtrans.library", 0L)))
    {
	fprintf (stderr, "Couldn't open mathtrans.library\n");
	return (0);
    }

    printf("Very basic mathtrans functionality test...\n");

    CHECK (SPLog,   (FFPTwo),     0xb1721840UL);
    CHECK (SPLog10, (FFPTwo),     0x9a209b3fUL);
    CHECK (SPSin,   (FFPOne),     0xd76aa440UL);
    CHECK (SPCos,   (FFPOne),     0x8a514040UL);
    CHECK (SPTan,   (FFPOne),     0xc7592341UL);
    CHECK (SPSinh,  (FFPOne),     0x966cfe41UL);
    CHECK (SPCosh,  (FFPOne),     0xc583aa41UL);
    CHECK (SPTanh,  (FFPOne),     0xc2f7d640UL);
    CHECK (SPExp,   (FFPTwo),     0xec732543UL);
    CHECK (SPAsin,  (FFPOnehalf), 0x860a9240UL);
    CHECK (SPAcos,  (FFPOnehalf), 0x860a9241UL);

    CloseLibrary(MathTransBase);

    if (!(MathIeeeSingTransBase = OpenLibrary("mathieeesingtrans.library", 0L)))
    {
	printf ("Couldn't open mathieeesingtrans.library\n");
	return (0);
    }

    printf("Very basic mathieeesingtrans functionality test...\n");


    CHECK (IEEESPLog,   (SPTwo),      0x3f317218UL);
    CHECK (IEEESPLog10, (SPTwo),      0x3e9a209aUL);
    CHECK (IEEESPSin,   (SPOne),      0x3f576aa4UL);
    CHECK (IEEESPCos,   (SPOne),      0x3f0a5140UL);
/*
    CHECK (IEEESPTan,   (SPOne),      0xUL);
*/
    CHECK (IEEESPSinh,  (SPOne),      0x40681e7bUL);
    CHECK (IEEESPCosh,  (SPOne),      0x4070c7d1UL);
/*
    CHECK (IEEESPTanh,  (SPOne),     0xc2f7d640UL);
    CHECK (IEEESPExp,   (SPTwo),     0xec732543UL);
    CHECK (IEEESPAsin,  (SPOnehalf), 0x860a9240UL);
    CHECK (IEEESPAcos,  (SPOnehalf), 0x860a9241UL);
*/
    CloseLibrary(MathIeeeSingTransBase);

    return (0);
}
