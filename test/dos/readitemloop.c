#include <dos/rdargs.h>
#include <proto/dos.h>

#include <stdio.h>

int __nocommandline = 1;

/* No newline in the end!!! Important!!! */
static char text[] = "WORD1 WORD2 WORD3";
static char buf[256];

int main(void)
{
    struct CSource cs;
    int i;
    LONG res;

    cs.CS_Buffer = text;
    cs.CS_Length = sizeof(text) - 1;
    cs.CS_CurChr = 0;

    i = 1;
    do
    {
	res = ReadItem(buf, sizeof(buf), &cs);

	printf("Step %d, result %d, buffer %s, CurChr %d\n", i++, res, buf, cs.CS_CurChr);
	if (i == 10)
	{
	    printf("ERROR: Unrecoverable loop detected!\n");
	    break;
	}
    } while (res != ITEM_NOTHING);
    
    return 0;
}
