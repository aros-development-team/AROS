#include <bzlib.h>
#include <stdio.h>

int main(void)
{
    printf("Version %s\n", BZ2_bzlibVersion());
    return 0;
}
