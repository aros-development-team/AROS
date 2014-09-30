/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "test.h"
#include <stdio.h>
#include <string.h>

#define TESTNUMBER1         11
#define TESTNUMBER1STRLEN   2
#define TEST1RESULT         "11"
#define TEST2RESULT         "11"
#define TEST3RESULT         "11"

#define BUFSIZE 10

static void cleanbuffer(char * buf)
{
    memset(buf, 0xff, BUFSIZE);
}

static int stringsame(char * c1, char * c2, int size)
{
    int i;
    for(i = 0; i < size; i++)
        if (c1[i] != c2[i]) return 0;
    return 1;
}

int main()
{
    char buf[BUFSIZE];
    int n1 = TESTNUMBER1;
    long long n2 = TESTNUMBER1;
    long long n3 = TESTNUMBER1;


    /* check standard %d conversion */
    cleanbuffer(buf);
    TEST((sprintf(buf, "%d", n1) == TESTNUMBER1STRLEN));
    TEST((stringsame(buf, TEST1RESULT, TESTNUMBER1STRLEN) == 1));

    /* check standard %qd conversion */
    cleanbuffer(buf);
    TEST((sprintf(buf, "%qd", n2) == TESTNUMBER1STRLEN));
    TEST((stringsame(buf, TEST2RESULT, TESTNUMBER1STRLEN) == 1));

    /* check standard %lld conversion */
    cleanbuffer(buf);
    TEST((sprintf(buf, "%lld", n3) == TESTNUMBER1STRLEN));
    TEST((stringsame(buf, TEST3RESULT, TESTNUMBER1STRLEN) == 1));

    return OK;
}

void cleanup()
{

}
