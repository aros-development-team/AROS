#include "test_vendor_init.h"
#include "../support/test.h"

#include <vendor_init/dummy/dummy_vendor_init.h>

static void test_dummy_never_matches_by_id(void)
{
    BT_CHECK(!bt_vendor_init_dummy_ops.matches_usb_id(0x0000, 0x0000));
    BT_CHECK(!bt_vendor_init_dummy_ops.matches_usb_id(0x0a12, 0x0001));
}

static void test_dummy_run_is_a_noop(void)
{
    bt_status_t status = bt_vendor_init_dummy_ops.run(NULL, NULL, 0);

    BT_CHECK(status == BT_OK);
    BT_CHECK(bt_vendor_init_dummy_ops.name != NULL);
}

void run_vendor_init_tests(void)
{
    test_dummy_never_matches_by_id();
    test_dummy_run_is_a_noop();
}
