#include <stdio.h>

int main()
{
    int a, b, c;
    
    a = 3;
    b = 0;
    printf("Attempting to divide by zero...\n");
    c = a/b;
    printf("Done, result is: %d!\n", c);
    return 0;
}
