#include <bzlib.h>
#include <stdio.h>

int main(void)
{
    printf("BZ2Base %p\n", BZ2Base);
    printf("Version %s\n", BZ2_bzlibVersion());
    return 0;
}
