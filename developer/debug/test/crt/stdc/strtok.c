/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <stdio.h>
#include <string.h>
#include "test.h"


int main(void)
{
    {
        // strtok()
        char str[] = "This;is,a test"; // must be array
        char del[] = ",;";
        char *ptr;
        int cnt = 0;

        ptr = strtok(str, del);
        while (ptr)
        {
            cnt++;
            switch(cnt)
            {
                case 1:
                    TEST( strcmp(ptr, "This") == 0 );
                    break;
                case 2:
                    TEST( strcmp(ptr, "is") == 0 );
                    break;
                case 3:
                    TEST( strcmp(ptr, "a test") == 0 );
                    break;
                default:
                    TEST(0);
                    break;
            }
            printf("%s\n", ptr);
            ptr = strtok(NULL, del);
        }
    }

    return OK;
}



void cleanup()
{
    /* Nothing to clean up */
}
