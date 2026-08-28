#include <btcore/hci.h>
#include <string.h>

bt_status_t bt_hci_encode_command(struct bt_buf_writer *w, uint16_t opcode,
                                   const uint8_t *params, size_t params_len)
{
    bt_status_t st;

    if (params_len > BT_HCI_MAX_PARAM_LEN)
        return BT_ERR_INVALID_ARGUMENT;

    st = bt_buf_writer_write_le16(w, opcode);
    if (st != BT_OK)
        return st;

    st = bt_buf_writer_write_u8(w, (uint8_t)params_len);
    if (st != BT_OK)
        return st;

    if (params_len == 0)
        return BT_OK;

    return bt_buf_writer_write_bytes(w, params, params_len);
}

bt_status_t bt_hci_parse_event_header(struct bt_buf_reader *r, struct bt_hci_event_header *out)
{
    uint8_t event_code;
    uint8_t param_len;
    bt_status_t st;

    st = bt_buf_reader_read_u8(r, &event_code);
    if (st != BT_OK)
        return st;

    st = bt_buf_reader_read_u8(r, &param_len);
    if (st != BT_OK)
        return st;

    if (bt_buf_reader_remaining(r) < param_len)
        return BT_ERR_BUFFER_UNDERFLOW;

    out->event_code = event_code;
    out->param_len = param_len;
    return BT_OK;
}

bt_status_t bt_hci_parse_command_complete(struct bt_buf_reader *r, uint8_t param_len,
                                           struct bt_hci_command_complete *out)
{
    uint8_t num_pkts;
    uint16_t opcode;
    size_t return_len;
    const uint8_t *p;
    bt_status_t st;

    /* num_hci_command_packets(1) + command_opcode(2) is the fixed prefix. */
    if (param_len < 3)
        return BT_ERR_BUFFER_UNDERFLOW;

    st = bt_buf_reader_read_u8(r, &num_pkts);
    if (st != BT_OK)
        return st;

    st = bt_buf_reader_read_le16(r, &opcode);
    if (st != BT_OK)
        return st;

    return_len = (size_t)param_len - 3;
    p = bt_buf_reader_peek(r, return_len);
    if (p == NULL)
        return BT_ERR_BUFFER_UNDERFLOW;

    st = bt_buf_reader_skip(r, return_len);
    if (st != BT_OK)
        return st;

    out->num_hci_command_packets = num_pkts;
    out->command_opcode = opcode;
    out->return_params = p;
    out->return_params_len = return_len;
    return BT_OK;
}

bt_status_t bt_hci_parse_command_status(struct bt_buf_reader *r, uint8_t param_len,
                                         struct bt_hci_command_status *out)
{
    uint8_t status;
    uint8_t num_pkts;
    uint16_t opcode;
    bt_status_t st;

    if (param_len != 4)
        return BT_ERR_INVALID_ARGUMENT;

    st = bt_buf_reader_read_u8(r, &status);
    if (st != BT_OK)
        return st;

    st = bt_buf_reader_read_u8(r, &num_pkts);
    if (st != BT_OK)
        return st;

    st = bt_buf_reader_read_le16(r, &opcode);
    if (st != BT_OK)
        return st;

    out->status = status;
    out->num_hci_command_packets = num_pkts;
    out->command_opcode = opcode;
    return BT_OK;
}

bt_status_t bt_hci_encode_acl_header(struct bt_buf_writer *w, uint16_t handle, uint8_t pb_flag,
                                      uint8_t bc_flag, uint16_t data_len)
{
    uint16_t handle_and_flags;
    bt_status_t st;

    if (handle > 0x0fffu || pb_flag > 0x03u || bc_flag > 0x03u)
        return BT_ERR_INVALID_ARGUMENT;

    handle_and_flags = (uint16_t)(handle | ((uint16_t)pb_flag << 12) | ((uint16_t)bc_flag << 14));

    st = bt_buf_writer_write_le16(w, handle_and_flags);
    if (st != BT_OK)
        return st;

    return bt_buf_writer_write_le16(w, data_len);
}

bt_status_t bt_hci_parse_acl_header(struct bt_buf_reader *r, struct bt_hci_acl_header *out)
{
    uint16_t handle_and_flags;
    uint16_t data_len;
    bt_status_t st;

    st = bt_buf_reader_read_le16(r, &handle_and_flags);
    if (st != BT_OK)
        return st;

    st = bt_buf_reader_read_le16(r, &data_len);
    if (st != BT_OK)
        return st;

    out->handle = (uint16_t)(handle_and_flags & 0x0fffu);
    out->pb_flag = (uint8_t)((handle_and_flags >> 12) & 0x03u);
    out->bc_flag = (uint8_t)((handle_and_flags >> 14) & 0x03u);
    out->data_len = data_len;
    return BT_OK;
}

bt_status_t bt_hci_parse_local_version(const uint8_t *return_params, size_t return_params_len,
                                        struct bt_hci_local_version *out)
{
    struct bt_buf_reader r;
    bt_status_t st;

    if (return_params_len != 9)
        return BT_ERR_INVALID_ARGUMENT;

    bt_buf_reader_init(&r, return_params, return_params_len);

    st = bt_buf_reader_read_u8(&r, &out->status);
    if (st != BT_OK)
        return st;
    st = bt_buf_reader_read_u8(&r, &out->hci_version);
    if (st != BT_OK)
        return st;
    st = bt_buf_reader_read_le16(&r, &out->hci_revision);
    if (st != BT_OK)
        return st;
    st = bt_buf_reader_read_u8(&r, &out->lmp_pal_version);
    if (st != BT_OK)
        return st;
    st = bt_buf_reader_read_le16(&r, &out->manufacturer_name);
    if (st != BT_OK)
        return st;
    return bt_buf_reader_read_le16(&r, &out->lmp_pal_subversion);
}

bt_status_t bt_hci_parse_local_features(const uint8_t *return_params, size_t return_params_len,
                                         struct bt_hci_local_features *out)
{
    struct bt_buf_reader r;
    bt_status_t st;

    if (return_params_len != 9)
        return BT_ERR_INVALID_ARGUMENT;

    bt_buf_reader_init(&r, return_params, return_params_len);

    st = bt_buf_reader_read_u8(&r, &out->status);
    if (st != BT_OK)
        return st;

    return bt_buf_reader_read_bytes(&r, out->features, sizeof(out->features));
}

bt_status_t bt_hci_parse_buffer_size(const uint8_t *return_params, size_t return_params_len,
                                      struct bt_hci_buffer_size *out)
{
    struct bt_buf_reader r;
    bt_status_t st;

    if (return_params_len != 8)
        return BT_ERR_INVALID_ARGUMENT;

    bt_buf_reader_init(&r, return_params, return_params_len);

    st = bt_buf_reader_read_u8(&r, &out->status);
    if (st != BT_OK)
        return st;
    st = bt_buf_reader_read_le16(&r, &out->acl_data_packet_length);
    if (st != BT_OK)
        return st;
    st = bt_buf_reader_read_u8(&r, &out->sco_data_packet_length);
    if (st != BT_OK)
        return st;
    st = bt_buf_reader_read_le16(&r, &out->total_num_acl_data_packets);
    if (st != BT_OK)
        return st;
    return bt_buf_reader_read_le16(&r, &out->total_num_sco_data_packets);
}

bt_status_t bt_hci_encode_inquiry(struct bt_buf_writer *w, uint32_t lap, uint8_t inquiry_length,
                                   uint8_t num_responses)
{
    uint8_t params[5];
    struct bt_buf_writer pw;
    bt_status_t st;

    bt_buf_writer_init(&pw, params, sizeof(params));
    st = bt_buf_writer_write_le24(&pw, lap);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_u8(&pw, inquiry_length);
    if (st != BT_OK)
        return st;
    st = bt_buf_writer_write_u8(&pw, num_responses);
    if (st != BT_OK)
        return st;

    return bt_hci_encode_command(w, BT_HCI_OPCODE_INQUIRY, params, bt_buf_writer_len(&pw));
}

bt_status_t bt_hci_inquiry_result_iter_init(struct bt_hci_inquiry_result_iter *it,
                                             const uint8_t *event_params, size_t event_params_len)
{
    if (it == NULL || event_params == NULL || event_params_len < 1u)
        return BT_ERR_BUFFER_UNDERFLOW;

    it->count = event_params[0];
    it->index = 0;
    it->base = event_params + 1;
    it->available = event_params_len - 1u;

    /* 14 bytes per response across the five arrays: 6 + 1 + 2 + 3 + 2. */
    if ((size_t)it->count * 14u > it->available)
        return BT_ERR_BUFFER_UNDERFLOW;
    return BT_OK;
}

bt_status_t bt_hci_inquiry_result_iter_next(struct bt_hci_inquiry_result_iter *it,
                                             struct bt_hci_inquiry_result_entry *out)
{
    const size_t n = it->count;
    const size_t i = it->index;
    const uint8_t *addrs, *psrm, *cod, *clock;
    size_t b;

    if (it->index >= it->count)
        return BT_ERR_BUFFER_UNDERFLOW;

    /* Each field is an array of its own; index into the array, do not stride
     * over a record that does not exist. */
    addrs = it->base;
    psrm  = addrs + n * BT_ADDR_LEN;
    cod   = psrm + n * 1u + n * 2u;   /* Reserved[] sits between them */
    clock = cod + n * 3u;

    for (b = 0; b < BT_ADDR_LEN; b++)
        out->bd_addr.b[b] = addrs[i * BT_ADDR_LEN + b];
    out->page_scan_repetition_mode = psrm[i];
    out->class_of_device = (uint32_t)cod[i * 3u]
                         | ((uint32_t)cod[i * 3u + 1u] << 8)
                         | ((uint32_t)cod[i * 3u + 2u] << 16);
    out->clock_offset = (uint16_t)clock[i * 2u]
                      | ((uint16_t)clock[i * 2u + 1u] << 8);

    it->index++;
    return BT_OK;
}

bt_status_t bt_hci_le_adv_report_iter_init(struct bt_hci_le_adv_report_iter *it,
                                            const uint8_t *event_params, size_t event_params_len)
{
    uint8_t subevent_code;
    uint8_t num_reports;
    bt_status_t st;

    bt_buf_reader_init(&it->r, event_params, event_params_len);

    st = bt_buf_reader_read_u8(&it->r, &subevent_code);
    if (st != BT_OK)
        return st;
    if (subevent_code != BT_HCI_LE_META_SUBEVENT_ADVERTISING_REPORT)
        return BT_ERR_INVALID_ARGUMENT;

    st = bt_buf_reader_read_u8(&it->r, &num_reports);
    if (st != BT_OK)
        return st;

    it->remaining = num_reports;
    return BT_OK;
}

bt_status_t bt_hci_le_adv_report_iter_next(struct bt_hci_le_adv_report_iter *it,
                                            struct bt_hci_le_adv_report *out)
{
    uint8_t rssi_raw;
    bt_status_t st;

    if (it->remaining == 0)
        return BT_ERR_BUFFER_UNDERFLOW;

    st = bt_buf_reader_read_u8(&it->r, &out->event_type);
    if (st != BT_OK)
        return st;
    st = bt_buf_reader_read_u8(&it->r, &out->address_type);
    if (st != BT_OK)
        return st;
    st = bt_buf_reader_read_bytes(&it->r, out->address.b, BT_ADDR_LEN);
    if (st != BT_OK)
        return st;
    st = bt_buf_reader_read_u8(&it->r, &out->data_len);
    if (st != BT_OK)
        return st;

    out->data = bt_buf_reader_peek(&it->r, out->data_len);
    if (out->data == NULL)
        return BT_ERR_BUFFER_UNDERFLOW;
    st = bt_buf_reader_skip(&it->r, out->data_len);
    if (st != BT_OK)
        return st;

    st = bt_buf_reader_read_u8(&it->r, &rssi_raw);
    if (st != BT_OK)
        return st;
    out->rssi = (int8_t)rssi_raw; /* RSSI is a signed byte per spec */

    it->remaining--;
    return BT_OK;
}

/*
 * LE scan and LE security commands. Encoders produce a complete command
 * (header included), like bt_hci_encode_inquiry(); the parsers take the
 * return-parameter / event-parameter slice and re-validate its length.
 */

bt_status_t bt_hci_encode_le_set_scan_parameters(struct bt_buf_writer *w, uint8_t scan_type,
                                                  uint16_t scan_interval, uint16_t scan_window,
                                                  uint8_t own_address_type,
                                                  uint8_t scanning_filter_policy)
{
    uint8_t params[7];
    struct bt_buf_writer pw;

    bt_buf_writer_init(&pw, params, sizeof(params));
    bt_buf_writer_write_u8(&pw, scan_type);
    bt_buf_writer_write_le16(&pw, scan_interval);
    bt_buf_writer_write_le16(&pw, scan_window);
    bt_buf_writer_write_u8(&pw, own_address_type);
    if (bt_buf_writer_write_u8(&pw, scanning_filter_policy) != BT_OK)
        return BT_ERR_BUFFER_OVERFLOW;
    return bt_hci_encode_command(w, BT_HCI_OPCODE_LE_SET_SCAN_PARAMETERS, params,
                                 bt_buf_writer_len(&pw));
}

bt_status_t bt_hci_encode_le_set_scan_enable(struct bt_buf_writer *w, uint8_t scan_enable,
                                              uint8_t filter_duplicates)
{
    uint8_t params[2];

    params[0] = scan_enable;
    params[1] = filter_duplicates;
    return bt_hci_encode_command(w, BT_HCI_OPCODE_LE_SET_SCAN_ENABLE, params, sizeof(params));
}

bt_status_t bt_hci_encode_le_encrypt(struct bt_buf_writer *w, const uint8_t key[16],
                                     const uint8_t plaintext[16])
{
    uint8_t params[32];

    memcpy(params, key, 16);
    memcpy(params + 16, plaintext, 16);
    return bt_hci_encode_command(w, BT_HCI_OPCODE_LE_ENCRYPT, params, sizeof(params));
}

bt_status_t bt_hci_parse_le_encrypt_return(const uint8_t *params, size_t params_len,
                                            uint8_t *out_status, uint8_t encrypted[16])
{
    if (params == NULL || params_len != 17u)
        return BT_ERR_INVALID_ARGUMENT;
    *out_status = params[0];
    memcpy(encrypted, params + 1, 16);
    return BT_OK;
}

bt_status_t bt_hci_encode_le_rand(struct bt_buf_writer *w)
{
    return bt_hci_encode_command(w, BT_HCI_OPCODE_LE_RAND, NULL, 0);
}

bt_status_t bt_hci_parse_le_rand_return(const uint8_t *params, size_t params_len,
                                         uint8_t *out_status, uint8_t random[8])
{
    if (params == NULL || params_len != 9u)
        return BT_ERR_INVALID_ARGUMENT;
    *out_status = params[0];
    memcpy(random, params + 1, 8);
    return BT_OK;
}

bt_status_t bt_hci_encode_le_enable_encryption(struct bt_buf_writer *w, uint16_t handle,
                                                const uint8_t random[8], uint16_t ediv,
                                                const uint8_t ltk[16])
{
    uint8_t params[28];
    struct bt_buf_writer pw;

    bt_buf_writer_init(&pw, params, sizeof(params));
    bt_buf_writer_write_le16(&pw, handle);
    bt_buf_writer_write_bytes(&pw, random, 8);
    bt_buf_writer_write_le16(&pw, ediv);
    if (bt_buf_writer_write_bytes(&pw, ltk, 16) != BT_OK)
        return BT_ERR_BUFFER_OVERFLOW;
    return bt_hci_encode_command(w, BT_HCI_OPCODE_LE_ENABLE_ENCRYPTION, params,
                                 bt_buf_writer_len(&pw));
}

bt_status_t bt_hci_encode_le_read_local_p256_public_key(struct bt_buf_writer *w)
{
    return bt_hci_encode_command(w, BT_HCI_OPCODE_LE_READ_LOCAL_P256_PUBLIC_KEY, NULL, 0);
}

bt_status_t bt_hci_encode_le_generate_dhkey(struct bt_buf_writer *w,
                                             const uint8_t remote_x[32],
                                             const uint8_t remote_y[32])
{
    uint8_t params[64];

    memcpy(params, remote_x, 32);
    memcpy(params + 32, remote_y, 32);
    return bt_hci_encode_command(w, BT_HCI_OPCODE_LE_GENERATE_DHKEY, params, sizeof(params));
}

bt_status_t bt_hci_parse_le_p256_public_key_complete(
    const uint8_t *event_params, size_t event_params_len,
    struct bt_hci_le_p256_public_key_complete *out)
{
    if (event_params == NULL || event_params_len != 66u ||
        event_params[0] != BT_HCI_LE_META_SUBEVENT_READ_LOCAL_P256_PUBLIC_KEY_COMPLETE)
        return BT_ERR_INVALID_ARGUMENT;
    out->status = event_params[1];
    out->x = event_params + 2;
    out->y = event_params + 34;
    return BT_OK;
}

bt_status_t bt_hci_parse_le_generate_dhkey_complete(const uint8_t *event_params,
                                                     size_t event_params_len,
                                                     uint8_t *out_status,
                                                     const uint8_t **out_dhkey)
{
    if (event_params == NULL || event_params_len != 34u ||
        event_params[0] != BT_HCI_LE_META_SUBEVENT_GENERATE_DHKEY_COMPLETE)
        return BT_ERR_INVALID_ARGUMENT;
    *out_status = event_params[1];
    *out_dhkey = event_params + 2;
    return BT_OK;
}
