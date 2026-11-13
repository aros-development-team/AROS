#include "virtual_transport.h"

#include <btcore/buffer.h>
#include <btcore/hci.h>

#include <string.h>

static struct bt_virtual_transport *vt_of(struct bt_hci_transport *transport)
{
    return (struct bt_virtual_transport *)transport->impl;
}

static int vt_open(struct bt_hci_transport *transport)
{
    struct bt_virtual_transport *vt = vt_of(transport);

    vt->is_open = true;
    vt->reset_done = false;
    return 0;
}

static void vt_close(struct bt_hci_transport *transport)
{
    vt_of(transport)->is_open = false;
}

static void emit_event(struct bt_hci_transport *transport, struct bt_virtual_transport *vt,
                        const uint8_t *event, size_t event_len)
{
    if (vt->recv != NULL)
        vt->recv(transport, BT_HCI_PACKET_EVENT, event, event_len, vt->recv_user_data);
}

/* Emits a Command Complete event: num_hci_command_packets=1, the given
 * opcode, and whatever return_params the caller already built (status
 * byte included, since its position/meaning varies per command). */
static void emit_command_complete(struct bt_hci_transport *transport, struct bt_virtual_transport *vt,
                                   uint16_t opcode, const uint8_t *return_params,
                                   uint8_t return_params_len)
{
    uint8_t event[BT_HCI_EVENT_HEADER_LEN + 3 + 16];
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, event, sizeof(event));
    bt_buf_writer_write_u8(&w, BT_HCI_EVENT_COMMAND_COMPLETE);
    bt_buf_writer_write_u8(&w, (uint8_t)(3 + return_params_len));
    bt_buf_writer_write_u8(&w, 1); /* num_hci_command_packets */
    bt_buf_writer_write_le16(&w, opcode);
    bt_buf_writer_write_bytes(&w, return_params, return_params_len);

    emit_event(transport, vt, event, bt_buf_writer_len(&w));
}

/* Emits a Command Status event: some commands (Inquiry, LE scan enable in
 * spirit though it actually completes) only ack this way. */
static void emit_command_status(struct bt_hci_transport *transport, struct bt_virtual_transport *vt,
                                 uint16_t opcode, uint8_t status)
{
    uint8_t event[BT_HCI_EVENT_HEADER_LEN + 4];
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, event, sizeof(event));
    bt_buf_writer_write_u8(&w, BT_HCI_EVENT_COMMAND_STATUS);
    bt_buf_writer_write_u8(&w, 4);
    bt_buf_writer_write_u8(&w, status);
    bt_buf_writer_write_u8(&w, 1); /* num_hci_command_packets */
    bt_buf_writer_write_le16(&w, opcode);

    emit_event(transport, vt, event, bt_buf_writer_len(&w));
}

/* Simulates one Classic inquiry finding a single fake device, then
 * finishing -- Inquiry Result followed by Inquiry Complete. */
static void emit_fake_inquiry_sequence(struct bt_hci_transport *transport,
                                        struct bt_virtual_transport *vt)
{
    static const uint8_t fake_addr[BT_ADDR_LEN] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    uint8_t result[BT_HCI_EVENT_HEADER_LEN + 1 + 14];
    uint8_t complete[BT_HCI_EVENT_HEADER_LEN + 1];
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, result, sizeof(result));
    bt_buf_writer_write_u8(&w, BT_HCI_EVENT_INQUIRY_RESULT);
    bt_buf_writer_write_u8(&w, 1 + 14); /* num_responses(1) + one 14-byte entry */
    bt_buf_writer_write_u8(&w, 1);      /* num_responses */
    bt_buf_writer_write_bytes(&w, fake_addr, BT_ADDR_LEN);
    bt_buf_writer_write_u8(&w, 0x01);       /* page_scan_repetition_mode */
    bt_buf_writer_write_le16(&w, 0x0000);   /* reserved */
    bt_buf_writer_write_le24(&w, 0x1F0104); /* fake class of device */
    bt_buf_writer_write_le16(&w, 0x0000);   /* clock_offset */
    emit_event(transport, vt, result, bt_buf_writer_len(&w));

    bt_buf_writer_init(&w, complete, sizeof(complete));
    bt_buf_writer_write_u8(&w, BT_HCI_EVENT_INQUIRY_COMPLETE);
    bt_buf_writer_write_u8(&w, 1);
    bt_buf_writer_write_u8(&w, 0x00); /* status: success */
    emit_event(transport, vt, complete, bt_buf_writer_len(&w));
}

/* Simulates one LE advertising report from a fake device. */
static void emit_fake_le_adv_report(struct bt_hci_transport *transport,
                                     struct bt_virtual_transport *vt)
{
    static const uint8_t fake_addr[BT_ADDR_LEN] = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16};
    uint8_t event[BT_HCI_EVENT_HEADER_LEN + 2 + 10]; /* subevent+num_reports + one 0-data report */
    struct bt_buf_writer w;

    bt_buf_writer_init(&w, event, sizeof(event));
    bt_buf_writer_write_u8(&w, BT_HCI_EVENT_LE_META);
    bt_buf_writer_write_u8(&w, 2 + 10);
    bt_buf_writer_write_u8(&w, BT_HCI_LE_META_SUBEVENT_ADVERTISING_REPORT);
    bt_buf_writer_write_u8(&w, 1); /* num_reports */
    bt_buf_writer_write_u8(&w, 0x00); /* event_type: ADV_IND */
    bt_buf_writer_write_u8(&w, 0x00); /* address_type: public */
    bt_buf_writer_write_bytes(&w, fake_addr, BT_ADDR_LEN);
    bt_buf_writer_write_u8(&w, 0x00);          /* data_len */
    bt_buf_writer_write_u8(&w, (uint8_t)-55);  /* rssi */

    emit_event(transport, vt, event, bt_buf_writer_len(&w));
}

static int vt_send_command(struct bt_hci_transport *transport, const uint8_t *data, size_t length)
{
    struct bt_virtual_transport *vt = vt_of(transport);
    struct bt_buf_reader r;
    uint16_t opcode;
    uint8_t param_len;
    uint8_t rp[16];
    struct bt_buf_writer rpw;

    if (!vt->is_open)
        return -1;

    bt_buf_reader_init(&r, data, length);
    if (bt_buf_reader_read_le16(&r, &opcode) != BT_OK)
        return -1;
    if (bt_buf_reader_read_u8(&r, &param_len) != BT_OK)
        return -1;
    if (bt_buf_reader_remaining(&r) != param_len)
        return -1; /* malformed command: declared length doesn't match payload */

    bt_buf_writer_init(&rpw, rp, sizeof(rp));

    switch (opcode)
    {
    case BT_HCI_OPCODE_RESET:
        vt->reset_done = true;
        bt_buf_writer_write_u8(&rpw, 0x00); /* status */
        break;

    case BT_HCI_OPCODE_READ_LOCAL_VERSION_INFO:
        bt_buf_writer_write_u8(&rpw, 0x00);   /* status */
        bt_buf_writer_write_u8(&rpw, 0x0c);   /* hci_version */
        bt_buf_writer_write_le16(&rpw, 0x0000); /* hci_revision */
        bt_buf_writer_write_u8(&rpw, 0x0c);   /* lmp_pal_version */
        bt_buf_writer_write_le16(&rpw, 0xffff); /* manufacturer_name (test value) */
        bt_buf_writer_write_le16(&rpw, 0x0001); /* lmp_pal_subversion */
        break;

    case BT_HCI_OPCODE_READ_LOCAL_SUPPORTED_FEATURES:
        {
            static const uint8_t features[8] = {0xff, 0xfe, 0x8d, 0xfe, 0x9b, 0xf9, 0x00, 0x80};
            bt_buf_writer_write_u8(&rpw, 0x00); /* status */
            bt_buf_writer_write_bytes(&rpw, features, sizeof(features));
        }
        break;

    case BT_HCI_OPCODE_READ_BUFFER_SIZE:
        bt_buf_writer_write_u8(&rpw, 0x00);      /* status */
        bt_buf_writer_write_le16(&rpw, 200);     /* acl_data_packet_length */
        bt_buf_writer_write_u8(&rpw, 0);         /* sco_data_packet_length */
        bt_buf_writer_write_le16(&rpw, 8);       /* total_num_acl_data_packets */
        bt_buf_writer_write_le16(&rpw, 0);       /* total_num_sco_data_packets */
        break;

    case BT_HCI_OPCODE_INQUIRY:
        /* Real Inquiry only ever acks via Command Status, then results
         * stream in asynchronously -- no Command Complete for this one. */
        emit_command_status(transport, vt, opcode, 0x00);
        emit_fake_inquiry_sequence(transport, vt);
        return 0;

    case BT_HCI_OPCODE_LE_SET_SCAN_PARAMETERS:
        bt_buf_writer_write_u8(&rpw, 0x00); /* status */
        break;

    case BT_HCI_OPCODE_LE_SET_SCAN_ENABLE:
        bt_buf_writer_write_u8(&rpw, 0x00); /* status */
        emit_command_complete(transport, vt, opcode, rp, (uint8_t)bt_buf_writer_len(&rpw));
        emit_fake_le_adv_report(transport, vt);
        return 0;

    default:
        /* Not modelled: answer like a real controller would for a command it
         * accepts, so the command queue is never left waiting. */
        bt_buf_writer_write_u8(&rpw, 0x00); /* status */
        break;
    }

    emit_command_complete(transport, vt, opcode, rp, (uint8_t)bt_buf_writer_len(&rpw));
    return 0;
}

static int vt_send_unsupported(struct bt_hci_transport *transport, const uint8_t *data, size_t length)
{
    (void)transport;
    (void)data;
    (void)length;
    return -1; /* ACL/SCO/ISO not modeled by the virtual controller yet */
}

static int vt_start_receive(struct bt_hci_transport *transport, bt_hci_transport_recv_fn recv,
                             void *user_data)
{
    struct bt_virtual_transport *vt = vt_of(transport);

    vt->recv = recv;
    vt->recv_user_data = user_data;
    return 0;
}

static void vt_stop_receive(struct bt_hci_transport *transport)
{
    struct bt_virtual_transport *vt = vt_of(transport);

    vt->recv = NULL;
    vt->recv_user_data = NULL;
}

static const struct bt_hci_transport_ops bt_virtual_transport_ops = {
    .open = vt_open,
    .close = vt_close,
    .send_command = vt_send_command,
    .send_acl = vt_send_unsupported,
    .send_sco = vt_send_unsupported,
    .send_iso = vt_send_unsupported,
    .start_receive = vt_start_receive,
    .stop_receive = vt_stop_receive,
};

void bt_virtual_transport_init(struct bt_virtual_transport *vt)
{
    memset(vt, 0, sizeof(*vt));
    vt->base.ops = &bt_virtual_transport_ops;
    vt->base.impl = vt;
}
