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

    return 0;
}

void cleanup(void) {}
