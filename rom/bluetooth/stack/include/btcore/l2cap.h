#ifndef BTCORE_L2CAP_H
#define BTCORE_L2CAP_H

#include <btcore/buffer.h>
#include <btcore/hci.h>
#include <btcore/status.h>
#include <btcore/types.h>

/* project.md, Fase 5. An L2CAP PDU (header + payload) travels inside one
 * or more HCI ACL Data packets -- never cast a raw buffer to a struct;
 * always go through the encode/parse pairs below. */

#define BT_L2CAP_HEADER_LEN 4 /* length(2) + cid(2) */

#define BT_L2CAP_CID_SIGNALING_CLASSIC 0x0001u
#define BT_L2CAP_CID_ATT 0x0004u
#define BT_L2CAP_CID_SIGNALING_LE 0x0005u
#define BT_L2CAP_CID_SMP 0x0006u
#define BT_L2CAP_CID_DYNAMIC_START 0x0040u /* first CID assignable to a new channel */

struct bt_l2cap_header
{
    uint16_t length; /* bytes of payload following this header */
    uint16_t cid;
};

bt_status_t bt_l2cap_encode_header(struct bt_buf_writer *w, uint16_t length, uint16_t cid);
bt_status_t bt_l2cap_parse_header(struct bt_buf_reader *r, struct bt_l2cap_header *out);

/*
 * Reassembles HCI ACL Data fragments belonging to one connection handle
 * back into a complete L2CAP PDU. One reassembler per handle -- fragments
 * from different handles must never be fed into the same instance.
 *
 * Fixed-size, no allocation: BT_L2CAP_REASSEMBLY_MAX bounds the largest
 * L2CAP PDU (header included) this reassembler can hold.
 */
#ifndef BT_L2CAP_REASSEMBLY_MAX
#define BT_L2CAP_REASSEMBLY_MAX 1708 /* header + BNEP-wrapped ethernet frame */
#endif

struct bt_l2cap_reassembler
{
    uint8_t buf[BT_L2CAP_REASSEMBLY_MAX];
    size_t have; /* bytes accumulated so far, including the header */
    size_t want; /* total bytes expected once the header is known; 0 = not known yet */
};

void bt_l2cap_reassembler_init(struct bt_l2cap_reassembler *ra);

enum bt_l2cap_reassembly_result
{
    BT_L2CAP_REASSEMBLY_MORE,     /* fragment accepted, PDU not complete yet */
    BT_L2CAP_REASSEMBLY_COMPLETE, /* fragment accepted, PDU now complete -- call take() */
    BT_L2CAP_REASSEMBLY_ERROR     /* malformed input; reassembler reset to idle */
};

/* pb_flag is the ACL header's Packet_Boundary_Flag: 0x01 means
 * "continuing fragment", anything else means "first fragment of a new
 * PDU" (0x00/0x02 differ only in flushability, irrelevant here). Detects,
 * as BT_L2CAP_REASSEMBLY_ERROR: a continuation with nothing in progress;
 * accumulated data exceeding BT_L2CAP_REASSEMBLY_MAX; a declared L2CAP
 * length that would need more space than that; or more bytes arriving
 * than the declared length allows. A new start fragment while a PDU is
 * already in progress silently abandons the old one (a well-behaved peer
 * never interleaves like this; there's nothing sensible to salvage). */
enum bt_l2cap_reassembly_result bt_l2cap_reassembler_feed(struct bt_l2cap_reassembler *ra,
                                                           uint8_t pb_flag, const uint8_t *data,
                                                           size_t len);

/* Valid only right after feed() returns BT_L2CAP_REASSEMBLY_COMPLETE.
 * Returns the full PDU (header + payload) via the reassembler's own
 * buffer and resets it to idle -- copy out or fully finish using it
 * before the next feed() call, which will start overwriting it. */
const uint8_t *bt_l2cap_reassembler_take(struct bt_l2cap_reassembler *ra, size_t *out_len);

/*
 * Splits one outbound L2CAP PDU into a sequence of ACL Data fragments no
 * larger than the controller's negotiated ACL data length (from HCI Read
 * Buffer Size).
 */
struct bt_l2cap_fragmenter
{
    const uint8_t *data;
    size_t len;
    size_t pos;
    uint16_t handle;
    size_t frag_size;
};

/* frag_size must be > 0 (the controller's acl_data_packet_length). */
void bt_l2cap_fragmenter_init(struct bt_l2cap_fragmenter *fr, uint16_t handle, size_t frag_size,
                               const uint8_t *l2cap_pdu, size_t l2cap_pdu_len);

/* Writes the next ACL header + payload chunk into w. Returns
 * BT_ERR_BUFFER_UNDERFLOW once nothing is left to send (not a real error
 * -- means "done", call this in a loop until you see it). */
bt_status_t bt_l2cap_fragmenter_next(struct bt_l2cap_fragmenter *fr, struct bt_buf_writer *w);

/*
 * L2CAP signaling (carried on BT_L2CAP_CID_SIGNALING_CLASSIC/_LE).
 * Scope reduction, documented: configuration only negotiates MTU
 * (Configuration Option type 0x01) -- flush timeout, QoS, and other
 * option types from the spec aren't modeled. Command Reject only exposes
 * the Reason code; the reason-specific trailing data (varies per reason)
 * is left as an opaque blob for the caller to inspect if it cares.
 */

#define BT_L2CAP_SIG_HEADER_LEN 4 /* code(1) + identifier(1) + length(2) */

#define BT_L2CAP_SIG_COMMAND_REJECT 0x01u
#define BT_L2CAP_SIG_CONNECTION_REQUEST 0x02u
#define BT_L2CAP_SIG_CONNECTION_RESPONSE 0x03u
#define BT_L2CAP_SIG_CONFIGURE_REQUEST 0x04u
#define BT_L2CAP_SIG_CONFIGURE_RESPONSE 0x05u
#define BT_L2CAP_SIG_DISCONNECTION_REQUEST 0x06u
#define BT_L2CAP_SIG_DISCONNECTION_RESPONSE 0x07u

#define BT_L2CAP_CONFIG_OPTION_MTU 0x01u

#define BT_L2CAP_CONN_RESULT_SUCCESS 0x0000u
#define BT_L2CAP_CONN_RESULT_PENDING 0x0001u
#define BT_L2CAP_CONN_RESULT_REFUSED_PSM 0x0002u

#define BT_L2CAP_CONFIG_RESULT_SUCCESS 0x0000u

struct bt_l2cap_sig_header
{
    uint8_t code;
    uint8_t identifier;
    uint16_t length; /* bytes of command-specific data following this header */
};

bt_status_t bt_l2cap_sig_encode_header(struct bt_buf_writer *w, uint8_t code, uint8_t identifier,
                                        uint16_t length);
bt_status_t bt_l2cap_sig_parse_header(struct bt_buf_reader *r, struct bt_l2cap_sig_header *out);

bt_status_t bt_l2cap_sig_encode_connection_request(struct bt_buf_writer *w, uint8_t identifier,
                                                     uint16_t psm, uint16_t source_cid);

struct bt_l2cap_connection_request
{
    uint16_t psm;
    uint16_t source_cid;
};

bt_status_t bt_l2cap_sig_parse_connection_request(const uint8_t *cmd_data, size_t cmd_data_len,
                                                    struct bt_l2cap_connection_request *out);

bt_status_t bt_l2cap_sig_encode_connection_response(struct bt_buf_writer *w, uint8_t identifier,
                                                      uint16_t destination_cid,
                                                      uint16_t source_cid, uint16_t result,
                                                      uint16_t status);

struct bt_l2cap_connection_response
{
    uint16_t destination_cid;
    uint16_t source_cid;
    uint16_t result;
    uint16_t status;
};

bt_status_t bt_l2cap_sig_parse_connection_response(const uint8_t *cmd_data, size_t cmd_data_len,
                                                     struct bt_l2cap_connection_response *out);

/* mtu of 0 omits the MTU option entirely (an empty Configuration Request,
 * meaning "accept the peer's default"). */
bt_status_t bt_l2cap_sig_encode_configure_request(struct bt_buf_writer *w, uint8_t identifier,
                                                    uint16_t destination_cid, uint16_t flags,
                                                    uint16_t mtu);

struct bt_l2cap_configure_request
{
    uint16_t destination_cid;
    uint16_t flags;
    bool has_mtu;
    uint16_t mtu;
};

bt_status_t bt_l2cap_sig_parse_configure_request(const uint8_t *cmd_data, size_t cmd_data_len,
                                                   struct bt_l2cap_configure_request *out);

bt_status_t bt_l2cap_sig_encode_configure_response(struct bt_buf_writer *w, uint8_t identifier,
                                                     uint16_t source_cid, uint16_t flags,
                                                     uint16_t result, uint16_t mtu);

struct bt_l2cap_configure_response
{
    uint16_t source_cid;
    uint16_t flags;
    uint16_t result;
    bool has_mtu;
    uint16_t mtu;
};

bt_status_t bt_l2cap_sig_parse_configure_response(const uint8_t *cmd_data, size_t cmd_data_len,
                                                    struct bt_l2cap_configure_response *out);

bt_status_t bt_l2cap_sig_encode_disconnection_request(struct bt_buf_writer *w, uint8_t identifier,
                                                        uint16_t destination_cid,
                                                        uint16_t source_cid);
bt_status_t bt_l2cap_sig_encode_disconnection_response(struct bt_buf_writer *w, uint8_t identifier,
                                                         uint16_t destination_cid,
                                                         uint16_t source_cid);

struct bt_l2cap_disconnection
{
    uint16_t destination_cid;
    uint16_t source_cid;
};

/* Same wire layout for both the request and the response. */
bt_status_t bt_l2cap_sig_parse_disconnection(const uint8_t *cmd_data, size_t cmd_data_len,
                                              struct bt_l2cap_disconnection *out);

bt_status_t bt_l2cap_sig_encode_command_reject(struct bt_buf_writer *w, uint8_t identifier,
                                                uint16_t reason);

struct bt_l2cap_command_reject
{
    uint16_t reason;
    const uint8_t *data; /* reason-specific trailing bytes, opaque here */
    size_t data_len;
};

bt_status_t bt_l2cap_sig_parse_command_reject(const uint8_t *cmd_data, size_t cmd_data_len,
                                               struct bt_l2cap_command_reject *out);

#endif /* BTCORE_L2CAP_H */
