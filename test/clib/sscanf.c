#include <stdio.h>
#include <string.h>

#include "test.h"

int main(void)
{
    char s[10];
    static char text[] = "1.3 1 string";
    int i, cnt;
    float f;

    cnt = sscanf(text, "%f %d %s", &f, &i, s);
    TEST(cnt == 3);
    TEST(f == 1.3);
    TEST(i == 1);
    TEST(strcmp(s, "string") == 0);

    return 0;
}

void cleanup(void) {}
