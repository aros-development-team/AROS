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
        FPrintf((BPTR)stderr, "Couldn't open mathffp.library\n");
        return (0);
    }

    printf("Basic mathffp functionality test...\n");

    /* this should set the zero-bit*/
    if ( 0 != SPAbs(0))
        Printf("Error with the SPAbs-function!\n");
    else
        Printf("SPAbs-function seems ok!\n");

    FFPOne = SPFlt(1);
    FFPTwo = SPAdd(FFPOne, FFPOne);
    FFPOnehalf = SPDiv(FFPTwo, FFPOne); 
        /* 0.5 = 1/2 ;the call to SPDiv is correct even if it seems wrong!*/

    if ( 0x80000041 != FFPOne)
        Printf("Error with the SPlt-function!\n");
    else
        Printf("SPFlt-function seems ok!\n");

    if ( 0x80000042 != FFPTwo)
    {
        Printf("Error with the SPAdd-function!\n Exiting!\n");
        return -1;
    }
    else
        Printf("SPAdd-function seems ok!\n");

    if ( 0x80000040 != FFPOnehalf)
    {
        Printf("Error with the SPDiv-function!\n Exiting!\n");
        return -1;
    }
    else
        Printf("SPDiv-function seems ok!\n");


    CloseLibrary(MathBase);


    if (!(MathTransBase = OpenLibrary("mathtrans.library", 0L)))
    {
        fprintf (stderr, "Couldn't open mathtrans.library\n");
        return (0);
    }

    printf("Very basic mathtrans functionality test...\n");

    res = SPLog (FFPTwo);
    if (res != 0xb1721840)
        Printf ("Error with the SPLog-function (got=%08lx expected=0xb1721840)\n",
            res);
    else
        Printf ("SPLog-function ok!\n");

    res = SPLog10(FFPTwo);
    if (res != 0x9a209b3f)
        Printf ("Error with the SPLog10-function (got=%08lx expected=0x9a209b3f)\n",
            res);
    else
        Printf ("SPLog10-function ok!\n");

    res = SPSin(FFPOne);
    if (res != 0xd76aa440)
        Printf ("Error with the SPSin-function (got=%08lx expected=0xd76aa440)\n",
            res);
    else
        Printf ("SPSin-function ok!\n");

    res = SPCos(FFPOne);
    if (res != 0x8a514040)
        Printf ("Error with the SPCos-function (got=%08lx expected=0x8a514040)\n",
            res);
    else
        Printf ("SPCos-function ok!\n");

    res = SPTan(FFPOne);
    if (res != 0xc7592341)
        Printf ("Error with the SPTan-function (got=%08lx expected=0xc7592341)\n",
            res);
    else
        Printf ("SPTan-function ok!\n");

    res = SPSinh(FFPOne);
    if (res != 0x966cfe41)
        Printf ("Error with the SPSinh-function (got=%08lx expected=0x966cfe41)\n",
            res);
    else
        Printf ("SPSinh-function ok!\n");

    res = SPCosh(FFPOne);
    if (res != 0xc583aa41)
        Printf ("Error with the SPCosh-function (got=%08lx expected=0xc583aa41)\n",
            res);
    else
        Printf ("SPCosh-function ok!\n");

    res = SPTanh(FFPOne);
    if (res != 0xc2f7d640)
        Printf ("Error with the SPTanh-function (got=%08lx expected=0xc2f7d640)\n",
            res);
    else
        Printf ("SPTanh-function ok!\n");

    res = SPExp (FFPTwo);
    if (res != 0xec732543)
        Printf ("Error with the SPExp-function (got=%08lx expected=0xec732543)\n",
            res);
    else
        Printf ("SPExp-function ok!\n");

    res = SPAsin(FFPOnehalf);
    if (res != 0x860a9240)
        Printf ("Error with the SPAsin-function (got=%08lx expected=0x860a9240)\n",
            res);
    else
        Printf ("SPAsin-function ok!\n");

    res = SPAcos (FFPOnehalf);
    if (res != 0x860a9241)
        Printf ("Error with the SPAcos-function (got=%08lx expected=0x860a9241)\n",
            res);
    else
        Printf ("SPAcos-function ok!\n");


    CloseLibrary(MathTransBase);

    return (0);
}
