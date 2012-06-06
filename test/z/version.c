#include <zlib.h>
#include <stdio.h>

int main(void)
{
    printf("Version %s\n", zlibVersion());
    return 0;
}
