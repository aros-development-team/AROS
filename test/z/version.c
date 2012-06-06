#include <zlib.h>
#include <stdio.h>

int main(void)
{
    printf("ZBase %p\n", ZBase);
    printf("Version %s\n", zlibVersion());
    return 0;
}
