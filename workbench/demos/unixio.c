#define AROS_ALMOST_COMPATIBLE
#include <hidd/unixio.h>
#include <proto/exec.h>
#include <proto/boopsi.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/intuition.h>

struct Library * BOOPSIBase;
struct DosLibrary * DOSBase;
struct IntuitionBase * IntuitionBase;

int main (int argc, char ** argv)
{
    APTR hidd;
    LONG ret;
    int fd;
    IPTR vpa[1];

    BOOPSIBase = OpenLibrary (BOOPSINAME, 0);

    if (!BOOPSIBase)
    {
	vpa[0] = (IPTR)BOOPSINAME;
	VPrintf("Can't open library \"%s\"\n", vpa);
	return 10;
    }

    DOSBase = (struct DosLibrary *) OpenLibrary (DOSNAME, 0);

    if (!DOSBase)
    {
	CloseLibrary (BOOPSIBase);

	vpa[0] = (IPTR)DOSNAME;
	VPrintf("Can't open library \"%s\"\n", vpa);
	return 10;
    }

    IntuitionBase = (struct IntuitionBase *) OpenLibrary (INTUITIONNAME, 0);

    if (!IntuitionBase)
    {
	CloseLibrary ((struct Library *)DOSBase);
	CloseLibrary (BOOPSIBase);

	vpa[0] = (IPTR)INTUITIONNAME;
	VPrintf("Can't open library \"%s\"\n", vpa);
	return 10;
    }

    hidd = NewObject (NULL, UNIXIOCLASS, TAG_END);

    if (!hidd)
    {
	CloseLibrary (BOOPSIBase);
	CloseLibrary ((struct Library *)DOSBase);
	CloseLibrary ((struct Library *)IntuitionBase);

	vpa[0] = (IPTR)UNIXIOCLASS;
	VPrintf("Need \"%s\" class\n", vpa);
	return 10;
    }

    fd = 0;
    ret = DoMethod (hidd, HIDDM_WaitForIO, fd, HIDDV_UnixIO_Read);

    vpa[0] = ret;
    VPrintf ("return code = %ld\n", vpa);

    DisposeObject (hidd);

    CloseLibrary ((struct Library *)IntuitionBase);
    CloseLibrary ((struct Library *)DOSBase);
    CloseLibrary (BOOPSIBase);

    return 0;
}

