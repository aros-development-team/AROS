#include <proto/example.h>
#include <proto/exec.h>

#include <stdio.h>

char buf[100];

// SPrintfA() uses RawDoFormat()
#pragma pack(2)
struct Data
{
    LONG longval;
    WORD wordval;
    STRPTR str;
} data = {10000000, 1001, "Hello"};
#pragma pack()

int main(void)
{
    puts(SayHelloOS3());
    puts(SayHelloOS4());
    puts(SayHelloMOS());
    puts(Uppercase("hElLo ArOs!!!"));

    puts(SPrintfA(buf, "TEST LONG %ld WORD %d STRING %s", &data));
    
    // variadic; again special treatment because of RawDoFmt()
    puts(SPrintf(buf, "TEST LONG %ld WORD %ld STRING %s", 10000000, (LONG)1001, (IPTR)"Hello"));

    return 0;
}
