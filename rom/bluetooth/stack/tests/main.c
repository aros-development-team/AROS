#include "support/test.h"
#include "endian/test_endian.h"
#include "buffer/test_buffer.h"
#include "hci/test_hci.h"
#include "h4/test_h4.h"
#include "virtual_transport/test_virtual_transport.h"
#include "queue/test_queue.h"
#include "timer/test_timer.h"
#include "controller/test_command_queue.h"
#include "controller/test_controller.h"
#include "manager/test_manager.h"
#include "addr/test_addr.h"
#include "device/test_device_registry.h"
#include "discovery/test_discovery.h"
#include "l2cap/test_l2cap.h"
#include "l2cap/test_signaling.h"
#include "l2cap/test_channel.h"
#include "sdp/test_sdp.h"
#include "sdp/test_sdp_client.h"
#include "att/test_att.h"
#include "gatt/test_gatt_client.h"
#include "smp/test_smp.h"
#include "security/test_bond_store.h"
#include "security/test_smp_crypto.h"
#include "security/test_smp_pairing.h"
#include "security/test_smp_manager.h"
#include "hid/test_hid_report.h"
#include "hid/test_hid_input.h"
#include "hid/test_hogp_client.h"
#include "hid/test_aros_input_bridge.h"
#include "vendor_init/test_vendor_init.h"

#include <stdio.h>

int main(void)
{
    run_endian_tests();
    run_buffer_tests();
    run_hci_tests();
    run_h4_tests();
    run_virtual_transport_tests();
    run_queue_tests();
    run_timer_tests();
    run_command_queue_tests();
    run_controller_tests();
    run_manager_tests();
    run_addr_tests();
    run_device_registry_tests();
    run_discovery_tests();
    run_l2cap_tests();
    run_l2cap_signaling_tests();
    run_l2cap_channel_tests();
    run_sdp_tests();
    run_sdp_client_tests();
    run_att_tests();
    run_gatt_client_tests();
    run_smp_tests();
    run_bond_store_tests();
    run_smp_crypto_tests();
    run_smp_pairing_tests();
    run_smp_manager_tests();
    run_hid_report_tests();
    run_hid_input_tests();
    run_hogp_client_tests();
    run_aros_input_bridge_tests();
    run_vendor_init_tests();

    printf("%d/%d checks passed\n", bt_test_count - bt_test_failures, bt_test_count);

    return bt_test_failures == 0 ? 0 : 1;
}
