#include <proto/exec.h>
#include <proto/aros.h>
#include <proto/dos.h>
#include <proto/mathffp.h>
#include <proto/mathtrans.h>

#include <stdio.h>

#include <exec/types.h>

struct Library * MathBase;
struct Library * MathTransBase;

int main(int argc, char ** argv)
{
    LONG FFPOne, FFPTwo, FFPOnehalf, FFPMinusOne, FFPNull;
    LONG res;

    FFPOne	= 0x80000041UL;
    FFPTwo	= 0x80000042UL;
    FFPMinusOne = 0x800000C1UL;
    FFPOnehalf	= 0x80000040UL;
    FFPNull	= 0x00000000UL;

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

    printf("Basic mathffp functionality test...\n");

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

    return (0);
}
