#include <proto/utility.h>
#include <stdio.h>
#include <string.h>

// Test for the V50 functions of utility.library

#ifndef Strlen
#define Strlen strlen
#endif

int main(void)
{
    TEXT buffer[12];
    
    IPTR args[2];
    args[0] = (IPTR)"XYZ";
    args[1] = 12345;
    
    // strlen must always be 11
    
    LONG count = VSNPrintf(buffer, sizeof buffer, "ab%scd%ldef", (RAWARG)args);
    printf("buffer %s len %d count %d\n", buffer, Strlen(buffer), count);

    Strlcpy(buffer, "12345678901234567890", sizeof buffer);
    printf("buffer %s len %d\n", buffer, Strlen(buffer));

    Strlcpy(buffer, "12345678", sizeof buffer);
    Strlcat(buffer, "abcdefghijklmnopqr", sizeof buffer);
    printf("buffer %s len %d\n", buffer, Strlen(buffer));
    
    return 0;
}