#include <btcore/att.h>
#include <btcore/gatt_server.h>

#include <string.h>

#define ATT_OP_FIND_BY_TYPE_VALUE_REQUEST 0x06u
#define ATT_OP_FIND_BY_TYPE_VALUE_RESPONSE 0x07u
#define ATT_OP_WRITE_COMMAND 0x52u

#define ATT_ERR_INVALID_HANDLE 0x01u
#define ATT_ERR_READ_NOT_PERMITTED 0x02u
#define ATT_ERR_WRITE_NOT_PERMITTED 0x03u
#define ATT_ERR_INVALID_PDU 0x04u
#define ATT_ERR_REQUEST_NOT_SUPPORTED 0x06u
#define ATT_ERR_UNSUPPORTED_GROUP_TYPE 0x10u

#define GATT_UUID_PRIMARY_SERVICE 0x2800u

static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

static void wr16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

void bt_gatt_uuid16_to_128(uint16_t uuid16, uint8_t out[16])
{
    static const uint8_t base[16] = {0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
                                     0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    memcpy(out, base, 16);
    wr16(out + 12, uuid16);
}

/* UUIDs are equal whichever of the two forms each side used. */
static bool uuid_equal(const uint8_t *a, size_t alen, const uint8_t *b, size_t blen)
{
    uint8_t la[16], lb[16];

    if ((alen != 2 && alen != 16) || (blen != 2 && blen != 16))
        return false;
    if (alen == blen)
        return memcmp(a, b, alen) == 0;
    if (alen == 2)
        bt_gatt_uuid16_to_128(rd16(a), la);
    else
        memcpy(la, a, 16);
    if (blen == 2)
        bt_gatt_uuid16_to_128(rd16(b), lb);
    else
        memcpy(lb, b, 16);
    return memcmp(la, lb, 16) == 0;
}

static size_t error_rsp(uint8_t *rsp, uint8_t opcode, uint16_t handle, uint8_t code)
{
    rsp[0] = BT_ATT_OPCODE_ERROR_RESPONSE;
    rsp[1] = opcode;
    wr16(rsp + 2, handle);
    rsp[4] = code;
    return 5;
}

/* Walks the attributes of [from, end]; false once past the end. */
static bool next_in_range(struct bt_gatt_server *s, uint32_t *cursor, uint16_t end,
                          struct bt_gatt_server_attr *a)
{
    if (*cursor > end)
        return false;
    memset(a, 0, sizeof(*a));
    if (!s->next(s->context, (uint16_t)*cursor, a) || a->handle > end || a->handle < *cursor)
        return false;
    *cursor = (uint32_t)a->handle + 1;
    return true;
}

static bool find_attr(struct bt_gatt_server *s, uint16_t handle, struct bt_gatt_server_attr *a)
{
    uint32_t cursor = handle;

    return handle != 0 && next_in_range(s, &cursor, handle, a);
}

static bool range_ok(uint16_t start, uint16_t end)
{
    return start != 0 && start <= end;
}

static size_t do_find_information(struct bt_gatt_server *s, const uint8_t *p, size_t len,
                                  uint8_t *rsp)
{
    struct bt_gatt_server_attr a;
    uint16_t start, end;
    uint32_t cursor;
    size_t n = 2;
    uint8_t tlen = 0;

    if (len != 5)
        return error_rsp(rsp, p[0], 0, ATT_ERR_INVALID_PDU);
    start = rd16(p + 1);
    end = rd16(p + 3);
    if (!range_ok(start, end))
        return error_rsp(rsp, p[0], start, ATT_ERR_INVALID_HANDLE);
    cursor = start;
    while (next_in_range(s, &cursor, end, &a))
    {
        if (tlen == 0)
            tlen = a.type_len;
        if (a.type_len != tlen || n + 2u + tlen > s->mtu)
            break;
        wr16(rsp + n, a.handle);
        memcpy(rsp + n + 2, a.type, tlen);
        n += 2u + tlen;
    }
    if (n == 2)
        return error_rsp(rsp, p[0], start, BT_ATT_ERROR_ATTRIBUTE_NOT_FOUND);
    rsp[0] = BT_ATT_OPCODE_FIND_INFORMATION_RESPONSE;
    rsp[1] = (tlen == 2) ? 0x01 : 0x02;
    return n;
}

static size_t do_find_by_type_value(struct bt_gatt_server *s, const uint8_t *p, size_t len,
                                    uint8_t *rsp)
{
    struct bt_gatt_server_attr a;
    uint16_t start, end;
    uint32_t cursor;
    size_t n = 1;

    if (len < 7)
        return error_rsp(rsp, p[0], 0, ATT_ERR_INVALID_PDU);
    start = rd16(p + 1);
    end = rd16(p + 3);
    if (!range_ok(start, end))
        return error_rsp(rsp, p[0], start, ATT_ERR_INVALID_HANDLE);
    cursor = start;
    while (next_in_range(s, &cursor, end, &a))
    {
        bool match;

        if (!uuid_equal(a.type, a.type_len, p + 5, 2) || !(a.flags & BT_GATT_ATTR_READ))
            continue;
        if (a.flags & BT_GATT_ATTR_SERVICE)
            match = uuid_equal(a.value, a.value_len, p + 7, len - 7);
        else
            match = (a.value_len == len - 7) && memcmp(a.value, p + 7, len - 7) == 0;
        if (!match)
            continue;
        if (n + 4 > s->mtu)
            break;
        wr16(rsp + n, a.handle);
        wr16(rsp + n + 2, (a.flags & BT_GATT_ATTR_SERVICE) ? a.group_end : a.handle);
        n += 4;
    }
    if (n == 1)
        return error_rsp(rsp, p[0], start, BT_ATT_ERROR_ATTRIBUTE_NOT_FOUND);
    rsp[0] = ATT_OP_FIND_BY_TYPE_VALUE_RESPONSE;
    return n;
}

/* Read By Type and Read By Group Type: lists of equally long entries. */
static size_t do_read_by_type(struct bt_gatt_server *s, const uint8_t *p, size_t len, uint8_t *rsp,
                              bool group)
{
    struct bt_gatt_server_attr a;
    uint16_t start, end;
    uint32_t cursor;
    size_t n = 2;
    size_t head = group ? 4 : 2;
    size_t vlen = 0;

    if (len != 7 && len != 21)
        return error_rsp(rsp, p[0], 0, ATT_ERR_INVALID_PDU);
    start = rd16(p + 1);
    end = rd16(p + 3);
    if (!range_ok(start, end))
        return error_rsp(rsp, p[0], start, ATT_ERR_INVALID_HANDLE);
    if (group && !(len == 7 && rd16(p + 5) == GATT_UUID_PRIMARY_SERVICE))
        return error_rsp(rsp, p[0], start, ATT_ERR_UNSUPPORTED_GROUP_TYPE);
    cursor = start;
    while (next_in_range(s, &cursor, end, &a))
    {
        size_t alen;

        if (!uuid_equal(a.type, a.type_len, p + 5, len - 5))
            continue;
        if (group && !(a.flags & BT_GATT_ATTR_SERVICE))
            continue;
        if (!(a.flags & BT_GATT_ATTR_READ))
        {
            if (n == 2)
                return error_rsp(rsp, p[0], a.handle, ATT_ERR_READ_NOT_PERMITTED);
            break;
        }
        alen = a.value_len;
        if (n == 2)
        {
            /* the first entry sets the length; a long value is cut short */
            vlen = alen;
            if (vlen > (size_t)s->mtu - 2 - head)
                vlen = (size_t)s->mtu - 2 - head;
            if (vlen > 255 - head)
                vlen = 255 - head;
        }
        else if (alen != vlen)
        {
            break;
        }
        if (n + head + vlen > s->mtu)
            break;
        wr16(rsp + n, a.handle);
        if (group)
            wr16(rsp + n + 2, a.group_end);
        if (vlen)
            memcpy(rsp + n + head, a.value, vlen);
        n += head + vlen;
    }
    if (n == 2)
        return error_rsp(rsp, p[0], start, BT_ATT_ERROR_ATTRIBUTE_NOT_FOUND);
    rsp[0] = group ? BT_ATT_OPCODE_READ_BY_GROUP_TYPE_RESPONSE : BT_ATT_OPCODE_READ_BY_TYPE_RESPONSE;
    rsp[1] = (uint8_t)(head + vlen);
    return n;
}

static size_t do_read(struct bt_gatt_server *s, const uint8_t *p, size_t len, uint8_t *rsp,
                      bool blob)
{
    struct bt_gatt_server_attr a;
    uint16_t handle, offset = 0;
    size_t n;

    if (len != (blob ? 5u : 3u))
        return error_rsp(rsp, p[0], 0, ATT_ERR_INVALID_PDU);
    handle = rd16(p + 1);
    if (blob)
        offset = rd16(p + 3);
    if (!find_attr(s, handle, &a))
        return error_rsp(rsp, p[0], handle, ATT_ERR_INVALID_HANDLE);
    if (!(a.flags & BT_GATT_ATTR_READ))
        return error_rsp(rsp, p[0], handle, ATT_ERR_READ_NOT_PERMITTED);
    if (offset > a.value_len)
        return error_rsp(rsp, p[0], handle, BT_ATT_ERROR_INVALID_OFFSET);
    n = (size_t)a.value_len - offset;
    if (n > (size_t)s->mtu - 1)
        n = (size_t)s->mtu - 1;
    rsp[0] = blob ? BT_ATT_OPCODE_READ_BLOB_RESPONSE : BT_ATT_OPCODE_READ_RESPONSE;
    if (n)
        memcpy(rsp + 1, a.value + offset, n);
    return n + 1;
}

static size_t do_write(struct bt_gatt_server *s, const uint8_t *p, size_t len, uint8_t *rsp,
                       bool command)
{
    struct bt_gatt_server_attr a;
    uint16_t handle;
    uint8_t err;

    if (len < 3)
        return command ? 0 : error_rsp(rsp, p[0], 0, ATT_ERR_INVALID_PDU);
    handle = rd16(p + 1);
    if (!find_attr(s, handle, &a))
        return command ? 0 : error_rsp(rsp, p[0], handle, ATT_ERR_INVALID_HANDLE);
    if (!(a.flags & (command ? (BT_GATT_ATTR_WRITE_NR | BT_GATT_ATTR_WRITE) : BT_GATT_ATTR_WRITE)) ||
        s->write == NULL)
        return command ? 0 : error_rsp(rsp, p[0], handle, ATT_ERR_WRITE_NOT_PERMITTED);
    err = s->write(s->context, &a, p + 3, len - 3);
    if (command)
        return 0;
    if (err)
        return error_rsp(rsp, p[0], handle, err);
    rsp[0] = BT_ATT_OPCODE_WRITE_RESPONSE;
    return 1;
}

void bt_gatt_server_init(struct bt_gatt_server *server, bt_gatt_server_next_fn next,
                          bt_gatt_server_write_fn write, void *context)
{
    memset(server, 0, sizeof(*server));
    server->next = next;
    server->write = write;
    server->context = context;
    server->mtu = BT_ATT_DEFAULT_MTU;
}

size_t bt_gatt_server_handle_pdu(struct bt_gatt_server *server, const uint8_t *pdu, size_t len,
                                  uint8_t *rsp, size_t rsp_max)
{
    if (len < 1 || rsp_max < BT_ATT_DEFAULT_MTU || server->next == NULL)
        return 0;
    if (server->mtu > rsp_max)
        server->mtu = (uint16_t)rsp_max;

    switch (pdu[0])
    {
    case BT_ATT_OPCODE_EXCHANGE_MTU_REQUEST:
    {
        uint16_t ours = (rsp_max < BT_GATT_SERVER_RX_MTU) ? (uint16_t)rsp_max : BT_GATT_SERVER_RX_MTU;
        uint16_t theirs;

        if (len != 3)
            return error_rsp(rsp, pdu[0], 0, ATT_ERR_INVALID_PDU);
        theirs = rd16(pdu + 1);
        if (theirs < BT_ATT_DEFAULT_MTU)
            theirs = BT_ATT_DEFAULT_MTU;
        rsp[0] = BT_ATT_OPCODE_EXCHANGE_MTU_RESPONSE;
        wr16(rsp + 1, ours);
        /* only valid for what follows this response */
        server->mtu = (theirs < ours) ? theirs : ours;
        return 3;
    }
    case BT_ATT_OPCODE_FIND_INFORMATION_REQUEST:
        return do_find_information(server, pdu, len, rsp);
    case ATT_OP_FIND_BY_TYPE_VALUE_REQUEST:
        return do_find_by_type_value(server, pdu, len, rsp);
    case BT_ATT_OPCODE_READ_BY_TYPE_REQUEST:
        return do_read_by_type(server, pdu, len, rsp, false);
    case BT_ATT_OPCODE_READ_BY_GROUP_TYPE_REQUEST:
        return do_read_by_type(server, pdu, len, rsp, true);
    case BT_ATT_OPCODE_READ_REQUEST:
        return do_read(server, pdu, len, rsp, false);
    case BT_ATT_OPCODE_READ_BLOB_REQUEST:
        return do_read(server, pdu, len, rsp, true);
    case BT_ATT_OPCODE_WRITE_REQUEST:
        return do_write(server, pdu, len, rsp, false);
    case ATT_OP_WRITE_COMMAND:
        return do_write(server, pdu, len, rsp, true);
    case BT_ATT_OPCODE_HANDLE_VALUE_CONFIRMATION:
        server->indication_pending = false;
        return 0;
    default:
        if (pdu[0] & 0x40u)
            return 0; /* a command we do not know: no answer */
        return error_rsp(rsp, pdu[0], 0, ATT_ERR_REQUEST_NOT_SUPPORTED);
    }
}

size_t bt_gatt_server_encode_handle_value(struct bt_gatt_server *server, uint16_t handle,
                                           const uint8_t *value, size_t len, bool indicate,
                                           uint8_t *out, size_t out_max)
{
    size_t max = server->mtu;

    if (max > out_max)
        max = out_max;
    if (max < 3 || (indicate && server->indication_pending))
        return 0;
    if (len > max - 3)
        len = max - 3;
    out[0] = indicate ? BT_ATT_OPCODE_HANDLE_VALUE_INDICATION : BT_ATT_OPCODE_HANDLE_VALUE_NOTIFICATION;
    wr16(out + 1, handle);
    if (len)
        memcpy(out + 3, value, len);
    if (indicate)
        server->indication_pending = true;
    return len + 3;
}
