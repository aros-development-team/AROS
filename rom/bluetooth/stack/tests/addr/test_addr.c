#include "test_addr.h"
#include "../support/test.h"

#include <btcore/addr.h>

static void test_equal(void)
{
    struct bt_addr a = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
    struct bt_addr b = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
    struct bt_addr c = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x07}};

    BT_CHECK(bt_addr_equal(&a, &b));
    BT_CHECK(!bt_addr_equal(&a, &c));
}

void run_addr_tests(void)
{
    test_equal();
}
