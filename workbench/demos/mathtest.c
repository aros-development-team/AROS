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
    LONG res;

    if (!(MathBase = OpenLibrary("mathffp.library", 0L)))
    {
	FPrintf((BPTR)stderr, "Couldn't open mathffp.library\n");
	return (0);
    }

    printf("Basic mathffp functionality test...\n");

    /* this should set the zero-bit*/
    if ( 0 != SPAbs(0))
	Printf("Error with the SPAbs-function!\n");
    else
	Printf("SPAbs-function ok!\n");

    CloseLibrary(MathBase);


    if (!(MathTransBase = OpenLibrary("mathtrans.library", 0L)))
    {
	fprintf (stderr, "Couldn't open mathtrans.library\n");
	return (0);
    }

    printf("Basic mathtrans functionality test...\n");

    res = SPLog (0x80000042);

    if (res != 0xb1721840)
	Printf ("Error with the SPLog-function (got=%08lx expected=0xb1721840)\n",
	    res);
    else
	Printf ("SPLog-function ok!\n");

    CloseLibrary(MathTransBase);

    return (0);
}
