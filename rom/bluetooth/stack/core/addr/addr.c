#include <btcore/addr.h>

#include <string.h>

bool bt_addr_equal(const struct bt_addr *a, const struct bt_addr *b)
{
    return memcmp(a->b, b->b, BT_ADDR_LEN) == 0;
}
