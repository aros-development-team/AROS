#include <stdio.h>
#include <stdlib.h>

#include <aros/debug.h>

static unsigned int level = 0;

int main(void)
{
    bug("Nest level: %u\n", ++level);

    if (level < 20)
    	main();

    exit(0);

    bug("Exit() did not work!\n");
    return 20;
}
