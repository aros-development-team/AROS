#include <exec/types.h>
#include <dos/dos.h>
#include <libraries/locale.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/locale.h>

struct Library *LocaleBase = NULL;

int main(int argc, char **av)
{
    struct Locale *new;

    if(argc != 2)
    {
	PrintFault(ERROR_REQUIRED_ARG_MISSING, "SetLocale");
	return 20;
    }

    LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library", 38);
    if(LocaleBase)
    {
	new = OpenLocale("piglatin.prefs" /* av[1] */);
	FPuts(Output(), "Locale opened\n");
	if(new)
	{
	    struct Locale *old = NULL;
	    old = PrefsUpdate(new);
	    FPuts(Output(), "Locale set\n");
	    CloseLocale(old);
	    CloseLocale(new);
	}
	else
	{
	    PrintFault(IoErr(), "SetLocale");
	}
	CloseLibrary((struct Library *)LocaleBase);
    }
    return 0;
}
