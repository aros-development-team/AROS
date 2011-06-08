#include <stdio.h>
#include <stdlib.h>

static unsigned int level = 0;

int main(void)
{
    printf("Nest level: %u\n", ++level);

    if (level < 20)
    	main();

    exit(0);
    printf("Exit() did not work!\n");
}
