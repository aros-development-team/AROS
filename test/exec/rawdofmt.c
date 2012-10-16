#include <exec/rawfmt.h>
#include <proto/exec.h>

#include <stdio.h>
#include <string.h>

int __nocommandline = 1;

#define TEST(x)         \
if (x)                  \
    printf("Passed\n"); \
else                    \
{                       \
    printf("Failed\n"); \
    result |= 1;        \
}

int main(void)
{
    int result = 0;
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
    TEST(count == 41)
    NewRawDoFmt("%s plus %s will be %u, next are %u, %u, %u", (APTR)RAWFMTFUNC_STRING, buf, "one", "two", 3, 4, 5, 6);
    printf("Formatted string is: %s\n", buf);
    TEST(!strcmp(buf, "one plus two will be 3, next are 4, 5, 6"))
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
    TEST(count == 41)
    RawDoFmt("%s plus %s will be %lu, next are %lu, %lu, %lu", args, (APTR)RAWFMTFUNC_STRING, buf);
    printf("Formatted string is: %s\n", buf);
    TEST(!strcmp(buf, "one plus two will be 3, next are 4, 5, 6"))
    RawDoFmt("%s plus %s will be %lu, next are %lu, %lu, %lu\n", args, (APTR)RAWFMTFUNC_SERIAL, NULL);
    printf("Serial output done\n");

    /* Now check correct sign interpretation. Specifier is intentionally %d, not %u! */
    NewRawDoFmt("This should be positive: %d",  (APTR)RAWFMTFUNC_STRING, buf, 40960);
    printf("NewRawDoFmt sign test: %s\n", buf);
    TEST(!strcmp(buf, "This should be positive: 40960"))

     /* Don't depend on endianess, sign-extend on 64 bits */
    args[0] = (SIPTR)0xA0A0A0A0;

    /* Intentionally %d with no 'l'! UWORD argument! */
    RawDoFmt("This should be negative: %d", args, (APTR)RAWFMTFUNC_STRING, buf);
    printf("RawDoFmt sign test: %s\n", buf);
    TEST(!strcmp(buf, "This should be negative: -24416"))

    /* This is actually implemented by locale.library's patch */
    NewRawDoFmt("%s %llx %llx", (APTR)RAWFMTFUNC_STRING, buf, "Hello", 0x1122334455667788ULL, 0xAABBCCDDEEFF9988ULL);
    printf("String and two QUADs: %s\n", buf);
    TEST(!strcmp(buf, "Hello 1122334455667788 AABBCCDDEEFF9988"))

    return result;
}
