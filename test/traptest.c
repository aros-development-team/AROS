#include <stdio.h>

int main()
{
    int a, b, c;
    int *ptr;
    
    a = 3;
    b = 0;
    printf("Attempting to divide by zero...\n");
    c = a/b;
    printf("Done, result is: %d!\n", c);

    printf("Trying illegal access now...\n");
    ptr = (unsigned int *)0xABADC0DE;
    a = *ptr;
    printf("Done, result is: %d!\n", a);

    return 0;
}
