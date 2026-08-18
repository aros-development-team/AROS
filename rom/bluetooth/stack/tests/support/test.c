#include "test.h"

#include <stdio.h>

int bt_test_failures = 0;
int bt_test_count = 0;

void bt_test_check(int cond, const char *expr, const char *file, int line)
{
    bt_test_count++;
    if (!cond)
    {
        bt_test_failures++;
        fprintf(stderr, "FAIL: %s at %s:%d\n", expr, file, line);
    }
}
