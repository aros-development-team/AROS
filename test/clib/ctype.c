/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <ctype.h>
#include <stdio.h>

#include "test.h"

int main(void)
{
    char c;

    c = '1';
    TEST(isdigit(c));
    TEST(!isalpha(c));
    TEST(isalnum(c));
    c = 'a';
    TEST(!isdigit(c));
    TEST(isalpha(c));
    TEST(isalnum(c));
    c = '.';
    TEST(!isdigit(c));
    TEST(!isalpha(c));
    TEST(!isalnum(c));
    TEST(toupper('a') == 'A');
    TEST(toupper('A') == 'A');
    TEST(tolower('Z') == 'z');
    TEST(tolower('z') == 'z');

    return 0;
}

void cleanup(void) {}
