#include <exec/rawfmt.h>
#include <proto/exec.h>

#include <stdio.h>

int __nocommandline = 1;

int main(void)
{
    char buf[256];
    ULONG count = 0;
    IPTR args[] = {
        (IPTR)"one",
	(IPTR)"two",
	3
    };

    printf("Checking NewRawDoFmt...\n");
    NewRawDoFmt("%s plus %s will be %lu", (VOID_FUNC)RAWFMTFUNC_COUNT, &count, "one", "two", 3);
    printf("Count is %u\n", count);
    NewRawDoFmt("%s plus %s will be %lu", (VOID_FUNC)RAWFMTFUNC_STRING, buf, "one", "two", 3);
    printf("Formatted string is: %s\n", buf);
    NewRawDoFmt("%s plus %s will be %lu\n", (VOID_FUNC)RAWFMTFUNC_SERIAL, NULL, "one", "two", 3);
    printf("Serial output done\n");

    count = 0;
    printf("Checking RawDoFmt...\n");
    RawDoFmt("%s plus %s will be %lu", args, (VOID_FUNC)RAWFMTFUNC_COUNT, &count);
    printf("Count is %u\n", count);
    RawDoFmt("%s plus %s will be %lu", args, (VOID_FUNC)RAWFMTFUNC_STRING, buf);
    printf("Formatted string is: %s\n", buf);
    RawDoFmt("%s plus %s will be %lu\n", args, (VOID_FUNC)RAWFMTFUNC_SERIAL, NULL);
    printf("Serial output done\n");

    return 0;
}
