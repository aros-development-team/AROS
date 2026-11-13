#ifndef BTCORE_SDP_H
#define BTCORE_SDP_H

#include <btcore/buffer.h>
#include <btcore/status.h>
#include <btcore/types.h>

/*
 * SDP (Service Discovery Protocol), project.md Fase 6. Unlike HCI/L2CAP,
 * SDP's PDU header and Data Elements are BIG-ENDIAN -- project.md flags
 * this explicitly as the one place where the "everything is LE" instinct
 * from the rest of the stack would be wrong.
 *
 * Scope reduction, documented: only the Data Element types needed to
 * search for and read service records are modeled -- Nil, unsigned
 * integers (1/2/4-byte), UUID-16, UUID-128, and Sequence (a nestable
 * ordered list). Signed integers, text strings, booleans, alternatives,
 * URLs, and 32-bit UUIDs from the spec aren't implemented. This module
 * covers wire encode/parse only -- an SDP client that opens an L2CAP
 * channel to BT_SDP_PSM and drives a request/response exchange (looping
 * on continuation state) is separate, not-yet-written follow-up work.
 */

#define BT_SDP_PSM 0x0001u

#define BT_SDP_HEADER_LEN 5 /* pdu_id(1) + transaction_id(2) + param_len(2) */

#define BT_SDP_PDU_ERROR_RESPONSE 0x01u
#define BT_SDP_PDU_SERVICE_SEARCH_REQUEST 0x02u
#define BT_SDP_PDU_SERVICE_SEARCH_RESPONSE 0x03u
#define BT_SDP_PDU_SERVICE_ATTRIBUTE_REQUEST 0x04u
#define BT_SDP_PDU_SERVICE_ATTRIBUTE_RESPONSE 0x05u

struct bt_sdp_header
{
    uint8_t pdu_id;
    uint16_t transaction_id;
    uint16_t param_len;
};

bt_status_t bt_sdp_encode_header(struct bt_buf_writer *w, uint8_t pdu_id, uint16_t transaction_id,
                                  uint16_t param_len);
bt_status_t bt_sdp_parse_header(struct bt_buf_reader *r, struct bt_sdp_header *out);

enum bt_sdp_element_type
{
    BT_SDP_ELEM_NIL,
    BT_SDP_ELEM_UINT,    /* value in .uint, byte width in .width (1, 2, or 4) */
    BT_SDP_ELEM_UUID16,
    BT_SDP_ELEM_UUID128,
    BT_SDP_ELEM_SEQUENCE, /* .seq_data/.seq_len: nested elements' raw bytes; walk with
                           * bt_sdp_element_iter */
    BT_SDP_ELEM_ALTERNATIVE, /* like SEQUENCE (data element alternative) */
    BT_SDP_ELEM_TEXT,     /* .seq_data/.seq_len: the string bytes (not NUL terminated) */
    BT_SDP_ELEM_URL,      /* .seq_data/.seq_len: the URL bytes */
    BT_SDP_ELEM_BOOL,     /* .uint 0/1 */
    BT_SDP_ELEM_SINT,     /* value in .uint (two's complement, width 1/2/4) */
    BT_SDP_ELEM_UUID32,   /* value in .uint */
    BT_SDP_ELEM_OTHER     /* wider integers: bytes in .seq_data/.seq_len */
};

struct bt_sdp_element
{
    enum bt_sdp_element_type type;
    uint8_t width;
    uint32_t uint;
    uint16_t uuid16;
    uint8_t uuid128[16]; /* wire byte order */
    const uint8_t *seq_data;
    size_t seq_len;
};

/* Encoders each append exactly one Data Element to w. */
bt_status_t bt_sdp_encode_uint(struct bt_buf_writer *w, uint32_t value, uint8_t width);
bt_status_t bt_sdp_encode_uuid16(struct bt_buf_writer *w, uint16_t uuid);
bt_status_t bt_sdp_encode_uuid128(struct bt_buf_writer *w, const uint8_t uuid[16]);

/* Writes a Sequence element's header for a nested block of nested_len
 * bytes -- follow immediately with nested_len bytes' worth of further
 * bt_sdp_encode_*() calls building the nested elements. */
bt_status_t bt_sdp_encode_sequence_header(struct bt_buf_writer *w, uint16_t nested_len);

/* Parses exactly one Data Element from r. For BT_SDP_ELEM_SEQUENCE,
 * seq_data points into r's underlying buffer and r is advanced past the
 * entire nested block (walk seq_data separately with bt_sdp_element_iter). */
bt_status_t bt_sdp_parse_element(struct bt_buf_reader *r, struct bt_sdp_element *out);

struct bt_sdp_element_iter
{
    struct bt_buf_reader r;
};

void bt_sdp_element_iter_init(struct bt_sdp_element_iter *it, const uint8_t *data, size_t len);
/* Returns BT_ERR_BUFFER_UNDERFLOW once exhausted (not a real error). */
bt_status_t bt_sdp_element_iter_next(struct bt_sdp_element_iter *it, struct bt_sdp_element *out);

/* ContinuationState: an opaque blob a server may return when a response
 * doesn't fit in one PDU; echo it back in a follow-up request to get the
 * rest. Capped at 16 bytes here (real servers' continuation state is
 * small in practice; a bigger one would need this bumped). */
struct bt_sdp_continuation
{
    uint8_t len;
    uint8_t data[16];
};

/* service_search_pattern must already be one complete, encoded Data
 * Element Sequence (bt_sdp_encode_sequence_header() followed by nested
 * bt_sdp_encode_uuid16()/_uuid128() calls), header included. cont may be
 * NULL (equivalent to a zero-length continuation, i.e. "first request"). */
bt_status_t bt_sdp_encode_service_search_request(struct bt_buf_writer *w, uint16_t transaction_id,
                                                   const uint8_t *service_search_pattern,
                                                   size_t pattern_len, uint16_t max_record_count,
                                                   const struct bt_sdp_continuation *cont);

struct bt_sdp_service_search_response
{
    uint16_t total_record_count;
    uint16_t current_record_count;
    const uint8_t *handles; /* current_record_count 4-byte big-endian handles, back to back */
    struct bt_sdp_continuation continuation;
};

bt_status_t bt_sdp_parse_service_search_response(const uint8_t *params, size_t params_len,
                                                   struct bt_sdp_service_search_response *out);

/* attribute_id_list must already be one complete, encoded Data Element
 * Sequence of AttributeIDs (uint16, via bt_sdp_encode_uint(..., 2)) and/or
 * AttributeID ranges (uint32, high 16 bits = first ID, low 16 = last). */
bt_status_t bt_sdp_encode_service_attribute_request(struct bt_buf_writer *w,
                                                      uint16_t transaction_id,
                                                      uint32_t service_record_handle,
                                                      uint16_t max_attribute_byte_count,
                                                      const uint8_t *attribute_id_list,
                                                      size_t attribute_id_list_len,
                                                      const struct bt_sdp_continuation *cont);

struct bt_sdp_service_attribute_response
{
    /* One Data Element Sequence's nested bytes: alternating
     * AttributeID (uint) / AttributeValue (any element) pairs. Walk with
     * bt_sdp_element_iter, taking two elements per attribute. */
    const uint8_t *attribute_list;
    size_t attribute_list_len;
    struct bt_sdp_continuation continuation;
};

bt_status_t bt_sdp_parse_service_attribute_response(const uint8_t *params, size_t params_len,
                                                      struct bt_sdp_service_attribute_response *out);

#endif /* BTCORE_SDP_H */
