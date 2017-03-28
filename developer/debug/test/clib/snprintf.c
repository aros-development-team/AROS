/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "test.h"
#include <stdio.h>
#include <string.h>

#define TESTSTRING "test"
#define TESTSTRING2 "123456789"
#define TESTSTRING3 "0123456789"
#define TESTSTRING4 "a long test string"

#define BUFSIZE 10

int main()
{
    char buf[BUFSIZE+1] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    
    /* first check strings shorter than buffer */
    TEST((snprintf(buf, BUFSIZE, "%s", TESTSTRING) == strlen(TESTSTRING)));
    TEST((buf[strlen(TESTSTRING)] == 0));
    TEST((buf[strlen(TESTSTRING) + 1] == (char) 0xff));
    
    /* now strings with length equal to buffer size - 1 */
    TEST((snprintf(buf, BUFSIZE, "%s", TESTSTRING2) == strlen(TESTSTRING2)));
    TEST((buf[strlen(TESTSTRING2)] == 0));
    TEST((buf[BUFSIZE] == (char) 0xff));

    /* now strings with length equal to buffer size (no zero byte written) */
    TEST((snprintf(buf, BUFSIZE, "%s", TESTSTRING3) == strlen(TESTSTRING3)));
    TEST((buf[BUFSIZE-1] == TESTSTRING3[strlen(TESTSTRING3)-1]));
    TEST((buf[BUFSIZE] == (char) 0xff));
    
    /* now strings longer than buffer size */
    TEST((snprintf(buf, BUFSIZE, "%s", TESTSTRING4) == strlen(TESTSTRING4)));
    TEST((buf[BUFSIZE-1] == TESTSTRING4[BUFSIZE-1]));
    TEST((buf[BUFSIZE] == (char) 0xff));
    return OK;
}

void cleanup()
{
    
}
