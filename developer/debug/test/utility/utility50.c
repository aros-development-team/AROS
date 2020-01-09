#include <proto/utility.h>
#include <stdio.h>
#include <string.h>

// Test for the V50 functions of utility.library

int main(void)
{
    TEXT buffer[12];
    
    IPTR args[2];
    args[0] = (IPTR)"XYZ";
    args[1] = 12345;
    
    // strlen must always be 11
    
    LONG count = VSNPrintf(buffer, sizeof buffer, "ab%scd%ldef", (RAWARG)args);
    printf("buffer %s len %d count %d\n", buffer, strlen(buffer), count);
    
    SetMem(buffer + 2, 'o', 4);
    printf("buffer %s len %d\n", buffer, strlen(buffer));
    
    Strlcpy(buffer, "12345678901234567890", sizeof buffer);
    printf("buffer %s len %d\n", buffer, strlen(buffer));

    Strlcpy(buffer, "12345678", sizeof buffer);
    Strlcat(buffer, "abcdefghijklmnopqr", sizeof buffer);
    printf("buffer %s len %d\n", buffer, strlen(buffer));
    
    return 0;
}