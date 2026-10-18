/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef NETMIDI2_H
#define NETMIDI2_H

#include <stddef.h>
#include <stdint.h>

/* Network MIDI 2.0 (UDP), MMA/AMEI M2-124-UM v1.0. The section numbers in
 * this file and in netmidi2.c are that document's; the code was read
 * against it on 2026-10-12. A UDP packet is the signature "MIDI" followed
 * by Command Packets: a 32-bit header (command code, payload length in
 * words, two command specific bytes) and the payload (section 5).
 * Integers are big-endian. No OS calls occur here: the caller owns the
 * socket, addresses, mDNS and the clock. */

#define NETMIDI2_SIGNATURE 0x4d494449u
#define NETMIDI2_MAX_PACKET 1400      /* avoid IP fragmentation (5.1.1) */
#define NETMIDI2_MAX_UMP_WORDS 64     /* per UMP Data command (7.1) */
#define NETMIDI2_NAME_MAX 98
#define NETMIDI2_PRODUCT_ID_MAX 42

enum netmidi2_command {
    NETMIDI2_INVITATION = 0x01,
    NETMIDI2_INVITATION_AUTH = 0x02,
    NETMIDI2_INVITATION_USER_AUTH = 0x03,
    NETMIDI2_INVITATION_ACCEPTED = 0x10,
    NETMIDI2_INVITATION_PENDING = 0x11,
    NETMIDI2_INVITATION_AUTH_REQUIRED = 0x12,
    NETMIDI2_INVITATION_USER_AUTH_REQUIRED = 0x13,
    NETMIDI2_PING = 0x20,
    NETMIDI2_PING_REPLY = 0x21,
    NETMIDI2_RETRANSMIT_REQUEST = 0x80,
    NETMIDI2_RETRANSMIT_ERROR = 0x81,
    NETMIDI2_SESSION_RESET = 0x82,
    NETMIDI2_SESSION_RESET_REPLY = 0x83,
    NETMIDI2_NAK = 0x8f,
    NETMIDI2_BYE = 0xf0,
    NETMIDI2_BYE_REPLY = 0xf1,
    NETMIDI2_UMP_DATA = 0xff
};

enum netmidi2_bye_reason {
    NETMIDI2_BYE_UNKNOWN = 0x00,
    NETMIDI2_BYE_USER_TERMINATED = 0x01,
    NETMIDI2_BYE_POWER_DOWN = 0x02,
    NETMIDI2_BYE_TOO_MANY_MISSING = 0x03,
    NETMIDI2_BYE_TIMEOUT = 0x04,
    NETMIDI2_BYE_NOT_ESTABLISHED = 0x05,
    NETMIDI2_BYE_NO_PENDING_SESSION = 0x06,
    NETMIDI2_BYE_PROTOCOL_ERROR = 0x07,
    /* from a Host only */
    NETMIDI2_BYE_TOO_MANY_SESSIONS = 0x40,
    NETMIDI2_BYE_AUTH_WITHOUT_INVITATION = 0x41,
    NETMIDI2_BYE_USER_REFUSED = 0x42,
    NETMIDI2_BYE_AUTH_FAILED = 0x43,
    NETMIDI2_BYE_USER_NOT_FOUND = 0x44,
    NETMIDI2_BYE_AUTH_NOT_SUPPORTED = 0x45, /* No Matching Authentication Method */
    /* from a Client only */
    NETMIDI2_BYE_INVITATION_CANCELED = 0x80
};

enum netmidi2_nak_reason {
    NETMIDI2_NAK_OTHER = 0x00,
    NETMIDI2_NAK_NOT_SUPPORTED = 0x01,
    NETMIDI2_NAK_NOT_EXPECTED = 0x02,
    NETMIDI2_NAK_MALFORMED = 0x03,
    NETMIDI2_NAK_BAD_PING_REPLY = 0x20
};

/* One parsed Command Packet. payload points into the UDP packet. */
struct netmidi2_command_packet {
    uint8_t code;
    uint8_t csd1;
    uint8_t csd2;
    const uint8_t *payload;
    size_t payload_words;
};

/* Splits a UDP payload into its commands. Returns the number found, or -1
 * when the signature is wrong or a command runs past the packet; commands
 * before the damage are still reported through count. */
int netmidi2_parse(const uint8_t *packet, size_t length,
                 struct netmidi2_command_packet *commands, size_t capacity);

/* Builds a UDP payload. Every add returns 0, or -1 when it does not fit. */
struct netmidi2_writer {
    uint8_t *data;
    size_t capacity;
    size_t length;
};

int netmidi2_writer_init(struct netmidi2_writer *writer, uint8_t *data, size_t capacity);
int netmidi2_add(struct netmidi2_writer *writer, uint8_t code, uint8_t csd1, uint8_t csd2,
               const uint8_t *payload, size_t payload_bytes);
/* Invitation, Invitation Reply Accepted or Pending: name and product
 * instance id, each padded to whole words. */
int netmidi2_add_identity(struct netmidi2_writer *writer, uint8_t code, uint8_t csd2,
                        const char *name, const char *product_id);
int netmidi2_add_ping(struct netmidi2_writer *writer, uint8_t code, uint32_t id);
int netmidi2_add_ump(struct netmidi2_writer *writer, uint16_t sequence,
                   const uint32_t *words, size_t count);
int netmidi2_add_nak(struct netmidi2_writer *writer, uint8_t reason,
                   const struct netmidi2_command_packet *original);

/* Reads the UMP Endpoint Name and Product Instance Id of an Invitation or
 * Invitation Reply into NUL-terminated buffers. Both outputs must be
 * non-NULL with nonzero capacity. Returns -1 without changing either
 * output if the capacities/pointers or the encoded lengths are invalid. */
int netmidi2_identity(const struct netmidi2_command_packet *command,
                    char *name, size_t name_size, char *product_id, size_t id_size);

/* A session with one peer, as Host or Client. */
enum netmidi2_state {
    NETMIDI2_IDLE,
    NETMIDI2_INVITING,        /* Client: Invitation sent */
    NETMIDI2_ESTABLISHED,
    NETMIDI2_CLOSING,         /* Bye sent, waiting for Bye Reply */
    NETMIDI2_RESETTING        /* Session Reset sent, waiting for its reply */
};

#define NETMIDI2_FEC_REPEATS 2

struct netmidi2_sent_ump {
    uint16_t sequence;
    uint8_t words;
    uint32_t data[NETMIDI2_MAX_UMP_WORDS];
};

/* What was sent lately, for a peer that asks for it again (7.2.3): the
 * last NETMIDI2_KEPT UMP Data commands of up to NETMIDI2_KEPT_WORDS words.
 * A longer command is remembered as sent and not kept: asked for, it is
 * answered with a Retransmit Error. */
#define NETMIDI2_KEPT 32
#define NETMIDI2_KEPT_WORDS 8
#define NETMIDI2_KEPT_TOO_LONG 0xff
struct netmidi2_kept_ump {
    uint16_t sequence;
    uint8_t words;
    uint32_t data[NETMIDI2_KEPT_WORDS];
};

enum netmidi2_retransmit_error {
    NETMIDI2_RETRANSMIT_UNKNOWN = 0x00,
    NETMIDI2_RETRANSMIT_NOT_KEPT = 0x01     /* table 32 */
};

typedef void (*netmidi2_ump_sink)(void *context, const uint32_t *words, size_t count);

struct netmidi2_session {
    enum netmidi2_state state;
    int host;
    char name[NETMIDI2_NAME_MAX + 1];
    char product_id[NETMIDI2_PRODUCT_ID_MAX + 1];
    char peer_name[NETMIDI2_NAME_MAX + 1];
    char peer_product_id[NETMIDI2_PRODUCT_ID_MAX + 1];
    unsigned protocols;             /* UMP_PROTOCOL_* this endpoint takes */
    unsigned jr_offered;            /* UMP_JR_RECEIVE when the caller uses
                                       the peer's timestamps */
    unsigned jr;                    /* what was agreed in this session */

    uint16_t tx_sequence;
    struct netmidi2_sent_ump sent[NETMIDI2_FEC_REPEATS];
    unsigned sent_count;
    struct netmidi2_kept_ump kept[NETMIDI2_KEPT];
    unsigned kept_count;
    unsigned kept_next;
    unsigned long retransmitted;    /* commands sent again when asked */
    unsigned long not_retransmitted;    /* requests we could not serve */

    int rx_started;
    uint16_t rx_expected;
    unsigned long rx_lost;          /* UMP Data commands never received */

    uint32_t now_ms;
    uint32_t last_send_ms;          /* last UMP Data with data */
    uint32_t last_receive_ms;
    uint32_t idle_sent;             /* zero length commands since data */
    uint32_t next_idle_ms;
    uint32_t ping_id;
    uint32_t ping_sent_ms;
    unsigned pings_unanswered;
    uint32_t state_since_ms;
    unsigned retries;
    uint8_t bye_reason;             /* repeated until Bye Reply */
    int refuse;                     /* a Host with no room: an Invitation
                                       gets Bye, "too many sessions" */
    /* Who the session is with: a number the caller makes of the peer's
     * address and port, which is what identifies a session (3.2, 6.4). */
    uint64_t peer;
    /* Counted for the caller, which compares before and after a call. */
    unsigned long gaps;             /* times UMP Data was lost for good:
                                       what was sounding may hang */
    unsigned long resets;           /* times the session was reset: both
                                       directions begin again (6.11) */

    netmidi2_ump_sink ump_sink;
    void *ump_context;
};

void netmidi2_session_init(struct netmidi2_session *session, int host,
                         const char *name, const char *product_id,
                         netmidi2_ump_sink sink, void *context);

/* Client: start a session. Writes the Invitation into out. The second
 * form names the Host; the first is for a caller with one peer only. */
int netmidi2_invite(struct netmidi2_session *session, uint32_t now_ms,
                  struct netmidi2_writer *out);
int netmidi2_invite_to(struct netmidi2_session *session, uint64_t peer,
                     uint32_t now_ms, struct netmidi2_writer *out);

/* Handles one UDP payload from the peer; replies, if any, are added to
 * out (initialised by the caller). Received UMPs reach the sink in order,
 * each once even with FEC; Endpoint Discovery is answered here. Returns
 * -1 when the packet was not Network MIDI 2.0. */
int netmidi2_receive(struct netmidi2_session *session, const uint8_t *packet,
                   size_t length, uint32_t now_ms, struct netmidi2_writer *out);
/* The same, saying where the packet came from. Whatever is added to out
 * is for that sender. A packet from someone the session is not with
 * leaves the session as it is: an Invitation is refused with Bye, "too
 * many sessions"; UMP Data and the commands of a session with Bye, "not
 * established"; a Bye and a Ping are answered and nothing more. */
int netmidi2_receive_from(struct netmidi2_session *session, uint64_t peer,
                        const uint8_t *packet, size_t length,
                        uint32_t now_ms, struct netmidi2_writer *out);
/* Resets the session (6.11): no UMP Data is sent until the peer's reply,
 * after which both directions count from zero again. For when the two
 * ends are out of step and nothing else will repair it. */
int netmidi2_reset(struct netmidi2_session *session, uint32_t now_ms,
                 struct netmidi2_writer *out);

/* Sends UMPs: a UMP Data command preceded by the previous ones for FEC.
 * Returns -1 when not established or it does not fit. */
int netmidi2_send_ump(struct netmidi2_session *session, const uint32_t *words,
                    size_t count, uint32_t now_ms, struct netmidi2_writer *out);

/* Call regularly: idle (zero length) UMP Data, pings, retries and
 * timeouts. Returns 1 when out holds a packet to send. */
int netmidi2_tick(struct netmidi2_session *session, uint32_t now_ms,
                struct netmidi2_writer *out);

/* Ends the session: writes a Bye. */
int netmidi2_bye(struct netmidi2_session *session, uint8_t reason, uint32_t now_ms,
               struct netmidi2_writer *out);

/* Several sessions on one UDP port: a Host with its Clients, each known by
 * the address and port it sends from (section 3.2), and the sessions this
 * side began as a Client. The caller still owns the socket and the clock.
 * A slot is in use from an Invitation, sent or accepted, until its session
 * is idle again. */
#define NETMIDI2_SESSIONS_MAX 8

struct netmidi2_table;

/* UMPs received in the session at index. */
typedef void (*netmidi2_table_sink)(void *context, int index,
                                    const uint32_t *words, size_t count);

struct netmidi2_table_slot {
    struct netmidi2_table *table;
    int index;
    int used;
    struct netmidi2_session session;
};

struct netmidi2_table {
    char name[NETMIDI2_NAME_MAX + 1];
    char product_id[NETMIDI2_PRODUCT_ID_MAX + 1];
    int accept;                     /* act as a Host: take Invitations */
    unsigned jr_offered;            /* for every session: UMP_JR_RECEIVE
                                       when the caller uses timestamps */
    netmidi2_table_sink sink;
    void *context;
    struct netmidi2_table_slot slot[NETMIDI2_SESSIONS_MAX];
    struct netmidi2_session nobody; /* answers for those with no session */
};

void netmidi2_table_init(struct netmidi2_table *table, const char *name,
                         const char *product_id, netmidi2_table_sink sink,
                         void *context);
/* The slot of the session with peer, or -1. */
int netmidi2_table_find(const struct netmidi2_table *table, uint64_t peer);
/* As a Client: begins a session with the Host at peer. Returns its slot,
 * -1 when there is none free or one with that peer exists. */
int netmidi2_table_invite(struct netmidi2_table *table, uint64_t peer,
                          uint32_t now_ms, struct netmidi2_writer *out);
/* One UDP payload from peer. Whatever is added to out is for that peer.
 * *index is the session it belonged to or made, -1 when none. Returns -1
 * when the packet was not Network MIDI 2.0. */
int netmidi2_table_receive(struct netmidi2_table *table, uint64_t peer,
                           const uint8_t *packet, size_t length,
                           uint32_t now_ms, struct netmidi2_writer *out,
                           int *index);
/* Call regularly for every slot in use; out is for that slot's peer.
 * Returns 1 when out holds a packet. A session that has ended gives its
 * slot back here. */
int netmidi2_table_tick(struct netmidi2_table *table, int index,
                        uint32_t now_ms, struct netmidi2_writer *out);
/* The session in a slot, or NULL: for its state, its peer, its counters,
 * and for netmidi2_send_ump(), netmidi2_bye() and netmidi2_reset(). */
struct netmidi2_session *netmidi2_table_session(struct netmidi2_table *table,
                                                int index);

#endif
