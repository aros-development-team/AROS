#include <stdlib.h>

#include <aros/debug.h>

int main(void)
{
    abort();

    bug("Abort() did not work!\n");

    return 20;
}
