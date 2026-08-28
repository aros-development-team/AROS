#ifndef BTCORE_ADDR_H
#define BTCORE_ADDR_H

#include <btcore/types.h>

#define BT_ADDR_LEN 6

/*
 * A Bluetooth device address, stored exactly as it appears on the wire
 * (b[0] is the first byte transmitted, i.e. the address's LSB per HCI
 * convention). Never cast to/from a native integer type -- compare and
 * copy byte-wise (project.md: "endereços Bluetooth ... exigem
 * representação explícita").
 */
struct bt_addr
{
    uint8_t b[BT_ADDR_LEN];
};

bool bt_addr_equal(const struct bt_addr *a, const struct bt_addr *b);

#endif /* BTCORE_ADDR_H */
