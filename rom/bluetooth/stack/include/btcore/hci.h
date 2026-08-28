#ifndef BTCORE_HCI_H
#define BTCORE_HCI_H

#include <btcore/addr.h>
#include <btcore/buffer.h>
#include <btcore/status.h>
#include <btcore/types.h>

/* HCI opcodes pack a 6-bit OGF and a 10-bit OCF: opcode = (ogf << 10) | ocf. */
#define BT_HCI_OPCODE(ogf, ocf) ((uint16_t)(((uint16_t)(ogf) << 10) | ((uint16_t)(ocf) & 0x03ffu)))

#define BT_HCI_OGF_CONTROLLER_BASEBAND 0x03u
#define BT_HCI_OCF_RESET 0x0003u
#define BT_HCI_OPCODE_RESET BT_HCI_OPCODE(BT_HCI_OGF_CONTROLLER_BASEBAND, BT_HCI_OCF_RESET)

#define BT_HCI_OCF_WRITE_INQUIRY_MODE 0x0045u
#define BT_HCI_OPCODE_WRITE_INQUIRY_MODE \
    BT_HCI_OPCODE(BT_HCI_OGF_CONTROLLER_BASEBAND, BT_HCI_OCF_WRITE_INQUIRY_MODE)
/* Mode 2: results carry RSSI and the Extended Inquiry Response, so a Classic
 * device's name arrives inline and no separate name request is needed. */
#define BT_HCI_INQUIRY_MODE_RSSI_EIR 0x02u

#define BT_HCI_OGF_LINK_CONTROL 0x01u
#define BT_HCI_OCF_INQUIRY 0x0001u
#define BT_HCI_OPCODE_INQUIRY BT_HCI_OPCODE(BT_HCI_OGF_LINK_CONTROL, BT_HCI_OCF_INQUIRY)
#define BT_HCI_GIAC_LAP 0x9E8B33u /* General Inquiry Access Code */

#define BT_HCI_OGF_LE_CONTROLLER 0x08u
#define BT_HCI_OCF_LE_SET_SCAN_PARAMETERS 0x000Bu
#define BT_HCI_OCF_LE_SET_SCAN_ENABLE 0x000Cu
#define BT_HCI_OCF_LE_ENCRYPT 0x0017u
#define BT_HCI_OCF_LE_RAND 0x0018u
#define BT_HCI_OCF_LE_ENABLE_ENCRYPTION 0x0019u
#define BT_HCI_OCF_LE_READ_LOCAL_P256_PUBLIC_KEY 0x0025u
#define BT_HCI_OCF_LE_GENERATE_DHKEY 0x0026u
#define BT_HCI_OPCODE_LE_SET_SCAN_PARAMETERS \
    BT_HCI_OPCODE(BT_HCI_OGF_LE_CONTROLLER, BT_HCI_OCF_LE_SET_SCAN_PARAMETERS)
#define BT_HCI_OPCODE_LE_SET_SCAN_ENABLE \
    BT_HCI_OPCODE(BT_HCI_OGF_LE_CONTROLLER, BT_HCI_OCF_LE_SET_SCAN_ENABLE)
#define BT_HCI_OPCODE_LE_ENCRYPT BT_HCI_OPCODE(BT_HCI_OGF_LE_CONTROLLER, BT_HCI_OCF_LE_ENCRYPT)
#define BT_HCI_OPCODE_LE_RAND BT_HCI_OPCODE(BT_HCI_OGF_LE_CONTROLLER, BT_HCI_OCF_LE_RAND)
#define BT_HCI_OPCODE_LE_ENABLE_ENCRYPTION \
    BT_HCI_OPCODE(BT_HCI_OGF_LE_CONTROLLER, BT_HCI_OCF_LE_ENABLE_ENCRYPTION)
#define BT_HCI_OPCODE_LE_READ_LOCAL_P256_PUBLIC_KEY \
    BT_HCI_OPCODE(BT_HCI_OGF_LE_CONTROLLER, BT_HCI_OCF_LE_READ_LOCAL_P256_PUBLIC_KEY)
#define BT_HCI_OPCODE_LE_GENERATE_DHKEY \
    BT_HCI_OPCODE(BT_HCI_OGF_LE_CONTROLLER, BT_HCI_OCF_LE_GENERATE_DHKEY)

#define BT_HCI_OGF_INFORMATIONAL 0x04u
#define BT_HCI_OCF_READ_LOCAL_VERSION_INFO 0x0001u
#define BT_HCI_OCF_READ_LOCAL_SUPPORTED_FEATURES 0x0003u
#define BT_HCI_OCF_READ_BUFFER_SIZE 0x0005u
#define BT_HCI_OPCODE_READ_LOCAL_VERSION_INFO \
    BT_HCI_OPCODE(BT_HCI_OGF_INFORMATIONAL, BT_HCI_OCF_READ_LOCAL_VERSION_INFO)
#define BT_HCI_OPCODE_READ_LOCAL_SUPPORTED_FEATURES \
    BT_HCI_OPCODE(BT_HCI_OGF_INFORMATIONAL, BT_HCI_OCF_READ_LOCAL_SUPPORTED_FEATURES)
#define BT_HCI_OPCODE_READ_BUFFER_SIZE \
    BT_HCI_OPCODE(BT_HCI_OGF_INFORMATIONAL, BT_HCI_OCF_READ_BUFFER_SIZE)

#define BT_HCI_EVENT_INQUIRY_COMPLETE 0x01u
#define BT_HCI_EVENT_INQUIRY_RESULT 0x02u
#define BT_HCI_EVENT_COMMAND_COMPLETE 0x0Eu
#define BT_HCI_EVENT_COMMAND_STATUS 0x0Fu
#define BT_HCI_EVENT_LE_META 0x3Eu
/* One response, and unlike 0x02 it carries RSSI and the EIR payload. Its
 * Reserved field is one byte where the plain result has two, so the layouts are
 * not interchangeable. */
#define BT_HCI_EVENT_EXTENDED_INQUIRY_RESULT 0x2Fu

#define BT_HCI_LE_META_SUBEVENT_ADVERTISING_REPORT 0x02u
#define BT_HCI_LE_META_SUBEVENT_READ_LOCAL_P256_PUBLIC_KEY_COMPLETE 0x08u
#define BT_HCI_LE_META_SUBEVENT_GENERATE_DHKEY_COMPLETE 0x09u

#define BT_HCI_COMMAND_HEADER_LEN 3 /* opcode(2) + parameter length(1) */
#define BT_HCI_EVENT_HEADER_LEN 2   /* event code(1) + parameter length(1) */
#define BT_HCI_ACL_HEADER_LEN 4     /* handle+flags(2) + data total length(2) */
#define BT_HCI_MAX_PARAM_LEN 255    /* HCI parameter length field is one byte wide */

/* Encodes a full HCI Command packet (header + parameters) into w. */
bt_status_t bt_hci_encode_command(struct bt_buf_writer *w, uint16_t opcode,
                                   const uint8_t *params, size_t params_len);

struct bt_hci_event_header
{
    uint8_t event_code;
    uint8_t param_len;
};

/* Parses the 2-byte event header. On success, param_len bytes are guaranteed
 * to remain in r (already bounds-checked), so callers may read them without
 * rechecking. */
bt_status_t bt_hci_parse_event_header(struct bt_buf_reader *r, struct bt_hci_event_header *out);

struct bt_hci_command_complete
{
    uint8_t num_hci_command_packets;
    uint16_t command_opcode;
    const uint8_t *return_params; /* points into the reader's underlying buffer */
    size_t return_params_len;
};

/* Parses a Command Complete event's parameters. Call after
 * bt_hci_parse_event_header() has confirmed event_code ==
 * BT_HCI_EVENT_COMMAND_COMPLETE, passing that header's param_len. */
bt_status_t bt_hci_parse_command_complete(struct bt_buf_reader *r, uint8_t param_len,
                                           struct bt_hci_command_complete *out);

struct bt_hci_command_status
{
    uint8_t status;
    uint8_t num_hci_command_packets;
    uint16_t command_opcode;
};

/* Parses a Command Status event's parameters. Call after
 * bt_hci_parse_event_header() has confirmed event_code ==
 * BT_HCI_EVENT_COMMAND_STATUS. param_len must be exactly 4, per spec. */
bt_status_t bt_hci_parse_command_status(struct bt_buf_reader *r, uint8_t param_len,
                                         struct bt_hci_command_status *out);

struct bt_hci_acl_header
{
    uint16_t handle;  /* 12-bit connection handle */
    uint8_t pb_flag;  /* 2-bit Packet_Boundary_Flag */
    uint8_t bc_flag;  /* 2-bit Broadcast_Flag */
    uint16_t data_len;
};

bt_status_t bt_hci_encode_acl_header(struct bt_buf_writer *w, uint16_t handle, uint8_t pb_flag,
                                      uint8_t bc_flag, uint16_t data_len);
bt_status_t bt_hci_parse_acl_header(struct bt_buf_reader *r, struct bt_hci_acl_header *out);

/*
 * Response parsers for the Fase 2 initialization sequence. Each takes the
 * return_params slice a bt_hci_command_complete already bounded -- they
 * re-validate the exact expected length rather than trusting the caller.
 */

struct bt_hci_local_version
{
    uint8_t status;
    uint8_t hci_version;
    uint16_t hci_revision;
    uint8_t lmp_pal_version;
    uint16_t manufacturer_name;
    uint16_t lmp_pal_subversion;
};

bt_status_t bt_hci_parse_local_version(const uint8_t *return_params, size_t return_params_len,
                                        struct bt_hci_local_version *out);

struct bt_hci_local_features
{
    uint8_t status;
    uint8_t features[8];
};

bt_status_t bt_hci_parse_local_features(const uint8_t *return_params, size_t return_params_len,
                                         struct bt_hci_local_features *out);

struct bt_hci_buffer_size
{
    uint8_t status;
    uint16_t acl_data_packet_length;
    uint8_t sco_data_packet_length;
    uint16_t total_num_acl_data_packets;
    uint16_t total_num_sco_data_packets;
};

bt_status_t bt_hci_parse_buffer_size(const uint8_t *return_params, size_t return_params_len,
                                      struct bt_hci_buffer_size *out);

/*
 * Discovery (project.md, Fase 4). Inquiry and LE scanning stream results
 * back as a variable number of entries packed into a single event, so
 * these are read with a small stateful iterator instead of a one-shot
 * parse -- init once per event, then call *_next() until it reports
 * BT_ERR_BUFFER_UNDERFLOW (meaning "no more entries", not a real error).
 */

/* num_responses of 0 means "unlimited" (subject to inquiry_length). */
bt_status_t bt_hci_encode_inquiry(struct bt_buf_writer *w, uint32_t lap, uint8_t inquiry_length,
                                   uint8_t num_responses);

struct bt_hci_inquiry_result_entry
{
    struct bt_addr bd_addr;
    uint8_t page_scan_repetition_mode;
    uint32_t class_of_device; /* low 24 bits meaningful */
    uint16_t clock_offset;
};

/*
 * The Inquiry Result event is column-major, not a sequence of records: after
 * Num_Responses come all the BD_ADDRs, then all the Page_Scan_Repetition_Modes,
 * then all the Reserved fields, then all the Classes of Device, then all the
 * Clock_Offsets. With one response the two layouts coincide, which is why
 * reading it as records looks correct until a second device answers -- and then
 * every field after the first address comes from the wrong array.
 */
struct bt_hci_inquiry_result_iter
{
    const uint8_t *base;   /* first BD_ADDR */
    size_t available;      /* bytes from base to the end of the parameters */
    uint8_t count;         /* Num_Responses */
    uint8_t index;         /* next response to return */
};

/* event_params is an Inquiry Result event's parameters (after the 2-byte
 * event header), i.e. bt_hci_event_header.param_len bytes starting right
 * after event_code+param_len. */
bt_status_t bt_hci_inquiry_result_iter_init(struct bt_hci_inquiry_result_iter *it,
                                             const uint8_t *event_params, size_t event_params_len);
bt_status_t bt_hci_inquiry_result_iter_next(struct bt_hci_inquiry_result_iter *it,
                                             struct bt_hci_inquiry_result_entry *out);

bt_status_t bt_hci_encode_le_set_scan_parameters(struct bt_buf_writer *w, uint8_t scan_type,
                                                  uint16_t scan_interval, uint16_t scan_window,
                                                  uint8_t own_address_type,
                                                  uint8_t scanning_filter_policy);
bt_status_t bt_hci_encode_le_set_scan_enable(struct bt_buf_writer *w, uint8_t scan_enable,
                                              uint8_t filter_duplicates);

/* Security controller services. Byte arrays use HCI wire order (least
 * significant octet first), intentionally distinct from smp_crypto.h's
 * FIPS/spec-display order. */
bt_status_t bt_hci_encode_le_encrypt(struct bt_buf_writer *w, const uint8_t key[16],
                                     const uint8_t plaintext[16]);
bt_status_t bt_hci_parse_le_encrypt_return(const uint8_t *params, size_t params_len,
                                            uint8_t *out_status, uint8_t encrypted[16]);
bt_status_t bt_hci_encode_le_rand(struct bt_buf_writer *w);
bt_status_t bt_hci_parse_le_rand_return(const uint8_t *params, size_t params_len,
                                         uint8_t *out_status, uint8_t random[8]);
bt_status_t bt_hci_encode_le_enable_encryption(struct bt_buf_writer *w, uint16_t handle,
                                                const uint8_t random[8], uint16_t ediv,
                                                const uint8_t ltk[16]);
bt_status_t bt_hci_encode_le_read_local_p256_public_key(struct bt_buf_writer *w);
bt_status_t bt_hci_encode_le_generate_dhkey(struct bt_buf_writer *w,
                                             const uint8_t remote_x[32],
                                             const uint8_t remote_y[32]);

struct bt_hci_le_p256_public_key_complete
{
    uint8_t status;
    const uint8_t *x;
    const uint8_t *y;
};

bt_status_t bt_hci_parse_le_p256_public_key_complete(
    const uint8_t *event_params, size_t event_params_len,
    struct bt_hci_le_p256_public_key_complete *out);
bt_status_t bt_hci_parse_le_generate_dhkey_complete(const uint8_t *event_params,
                                                     size_t event_params_len,
                                                     uint8_t *out_status,
                                                     const uint8_t **out_dhkey);

struct bt_hci_le_adv_report
{
    uint8_t event_type;
    uint8_t address_type;
    struct bt_addr address;
    uint8_t data_len;
    const uint8_t *data; /* points into the event buffer passed to iter_init */
    int8_t rssi;
};

struct bt_hci_le_adv_report_iter
{
    struct bt_buf_reader r;
    uint8_t remaining;
};

/* event_params is a full LE Meta Event's parameters, i.e. starting with
 * the Subevent_Code byte. Returns BT_ERR_INVALID_ARGUMENT if the subevent
 * isn't an advertising report. */
bt_status_t bt_hci_le_adv_report_iter_init(struct bt_hci_le_adv_report_iter *it,
                                            const uint8_t *event_params, size_t event_params_len);
bt_status_t bt_hci_le_adv_report_iter_next(struct bt_hci_le_adv_report_iter *it,
                                            struct bt_hci_le_adv_report *out);

#endif /* BTCORE_HCI_H */
