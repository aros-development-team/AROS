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
    LONG FFPOne, FFPTwo, FFPOnehalf;
    LONG res;

    if (!(MathBase = OpenLibrary("mathffp.library", 0L)))
    {
	printf ("Couldn't open mathffp.library\n");
	return (0);
    }

    printf("Basic mathffp functionality test...\n");

    /* this should set the zero-bit*/
    if ( 0 != SPAbs(0))
	printf("Error with the SPAbs-function!\n");
    else
	printf("SPAbs-function seems ok!\n");

    FFPOne = SPFlt(1);
    FFPTwo = SPAdd(FFPOne, FFPOne);
    FFPOnehalf = SPDiv(FFPTwo, FFPOne);
	/* 0.5 = 1/2 ;the call to SPDiv is correct even if it seems wrong!*/

    if ( 0x80000041 != FFPOne)
	printf("Error with the SPlt-function!\n");
    else
	printf("SPFlt-function seems ok!\n");

    if ( 0x80000042 != FFPTwo)
    {
	printf("Error with the SPAdd-function!\n Exiting!\n");
	return -1;
    }
    else
	printf("SPAdd-function seems ok!\n");

    if ( 0x80000040 != FFPOnehalf)
    {
	printf("Error with the SPDiv-function!\n Exiting!\n");
	return -1;
    }
    else
	printf("SPDiv-function seems ok!\n");


    CloseLibrary(MathBase);

    if (!(MathTransBase = OpenLibrary("mathtrans.library", 0L)))
    {
	fprintf (stderr, "Couldn't open mathtrans.library\n");
	return (0);
    }

    printf("Very basic mathtrans functionality test...\n");

#define CHECK(func,args,cres) \
    res = func args; \
    if (res != cres) \
	printf ("Error with the " #func "-function (got=0x%08lx expected=" #cres ")\n", res); \
    else \
	printf (#func "-function ok!\n");

    CHECK (SPLog, (FFPTwo), 0xb1721840);
    CHECK (SPLog10, (FFPTwo), 0x9a209b3f);
    CHECK (SPSin, (FFPOne), 0xd76aa440);
    CHECK (SPCos, (FFPOne), 0x8a514040);
    CHECK (SPTan, (FFPOne), 0xc7592341);
    CHECK (SPSinh, (FFPOne), 0x966cfe41);
    CHECK (SPCosh, (FFPOne), 0xc583aa41);
    CHECK (SPTanh, (FFPOne), 0xc2f7d640);
    CHECK (SPExp, (FFPTwo), 0xec732543);
    CHECK (SPAsin, (FFPOnehalf), 0x860a9240);
    CHECK (SPAcos, (FFPOnehalf), 0x860a9241);

    CloseLibrary(MathTransBase);

    return (0);
}
