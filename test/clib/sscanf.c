/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <string.h>

#include "test.h"

int main(void)
{
    char s[10];
    static char text1[] = "1.3 1 string";
    static char text2[] = "NO_NUMBERS_TEXT";
    static char text3[] = "FSOMETHING";
    static char text4[] = "0xAF"; /* Hex integer */
    static char text5[] = "xAF"; /* "Hex integer" without 0 */
    static char text6[] = "AF"; /* "Hex integer" without 0x */
    int i, cnt;
    float f;

    cnt = sscanf(text1, "%f %d %s", &f, &i, s);
    TEST(cnt == 3);
    TEST(f == 1.3f);
    TEST(i == 1);
    TEST(strcmp(s, "string") == 0);

    i = 123456;
    cnt = sscanf(text2, "%i", &i);
    TEST(cnt == 0);
    TEST(i == 123456);

    i = 123456;
    cnt = sscanf(text3, "%i", &i);
    TEST(cnt == 0);
    TEST(i == 123456);
    
    i = 123456;
    cnt = sscanf(text4, "%i", &i);
    TEST(cnt == 1);
    TEST(i == 0xAF);
    
    i = 123456;
    cnt = sscanf(text5, "%i", &i);
    TEST(cnt == 0);
    TEST(i == 123456);

    i = 123456;
    cnt = sscanf(text6, "%i", &i);
    TEST(cnt == 0);
    TEST(i == 123456);

    cnt = sscanf("0.1", "%f", &f);
    TEST(cnt == 1);

    cnt = sscanf(".1", "%f", &f);
    TEST(cnt == 1);

    cnt = sscanf("1", "%f", &f);
    TEST(cnt == 1);

    cnt = sscanf("-.1", "%f", &f);
    TEST(cnt == 1);

    cnt = sscanf("x", "%f", &f);
    TEST(cnt == 0);

    return 0;
}

void cleanup(void) {}
