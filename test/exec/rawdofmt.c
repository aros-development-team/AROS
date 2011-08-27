#include <exec/rawfmt.h>
#include <proto/exec.h>

#include <stdio.h>

int __nocommandline = 1;

int main(void)
{
    char buf[256];
    ULONG count = 0;
    IPTR args[] =
    {
	(IPTR)"one",
	(IPTR)"two",
	3,
	4,
	5,
	6,
    };

    /*
     * NewRawDoFmt() should not need 'l' modifier for ULONGs.
     * This is verified under MorphOS.
     */
    printf("Checking NewRawDoFmt...\n");
    NewRawDoFmt("%s plus %s will be %u, next are %u, %u, %u", (APTR)RAWFMTFUNC_COUNT, &count, "one", "two", 3, 4, 5, 6);
    printf("Count is %u\n", (unsigned)count);
    NewRawDoFmt("%s plus %s will be %u, next are %u, %u, %u", (APTR)RAWFMTFUNC_STRING, buf, "one", "two", 3, 4, 5, 6);
    printf("Formatted string is: %s\n", buf);
    NewRawDoFmt("%s plus %s will be %u, next are %u, %u, %u\n", (APTR)RAWFMTFUNC_SERIAL, NULL, "one", "two", 3, 4, 5, 6);
    printf("Serial output done\n");

    count = 0;
    
    /*
     * RawDoFmt() historically assumes UWORD argument
     * without 'l' modifier. In AROS 'l' here stands for IPTR,
     * not ULONG, for 64 bit compatibility.
     */
    printf("Checking RawDoFmt...\n");
    RawDoFmt("%s plus %s will be %lu, next are %lu, %lu, %lu", args, (APTR)RAWFMTFUNC_COUNT, &count);
    printf("Count is %u\n", (unsigned)count);
    RawDoFmt("%s plus %s will be %lu, next are %lu, %lu, %lu", args, (APTR)RAWFMTFUNC_STRING, buf);
    printf("Formatted string is: %s\n", buf);
    RawDoFmt("%s plus %s will be %lu, next are %lu, %lu, %lu\n", args, (APTR)RAWFMTFUNC_SERIAL, NULL);
    printf("Serial output done\n");

    return 0;
}
