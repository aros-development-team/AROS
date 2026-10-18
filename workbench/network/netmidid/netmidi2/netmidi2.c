/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include "netmidi2/netmidi2.h"
#include "ump/ump.h"

#include <string.h>

/* Timing. The specification fixes one value and bounds another; the rest
 * are this implementation's. */
#define IDLE_FIRST_MS 100       /* first Zero Length UMP Data: within 300 ms
                                   of the last data (7.2.1) */
#define IDLE_FEC_MS 20          /* then one per FEC repeat (7.2.2), ours */
#define IDLE_STOP_MS 8000       /* and none after this long, ours: the
                                   intervals grow and end (7.2.1) */
#define PING_IDLE_MS 5000       /* ping a peer this long quiet, ours */
#define PING_LIMIT 3            /* unanswered pings before Bye, Timeout
                                   (6.13 says "multiple") */
#define RETRY_MS 1000           /* repeats of Invitation, Bye and Session
                                   Reset: 300 ms to 2 s (6.2) */
#define RETRY_LIMIT 10          /* ours */

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static uint32_t get32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

/* Sections 5.2 to 5.4. */
int netmidi2_parse(const uint8_t *packet, size_t length,
                 struct netmidi2_command_packet *commands, size_t capacity)
{
    size_t offset = 4, count = 0;

    if (length < 4 || get32(packet) != NETMIDI2_SIGNATURE)
        return -1;
    while (offset < length) {
        size_t words;
        if (length - offset < 4)
            return -1;
        words = packet[offset + 1];
        if (length - offset - 4 < words * 4)
            return -1;
        if (count < capacity) {
            commands[count].code = packet[offset];
            commands[count].csd1 = packet[offset + 2];
            commands[count].csd2 = packet[offset + 3];
            commands[count].payload = packet + offset + 4;
            commands[count].payload_words = words;
        }
        ++count;
        offset += 4 + words * 4;
    }
    return (int)(count < capacity ? count : capacity);
}

int netmidi2_writer_init(struct netmidi2_writer *writer, uint8_t *data, size_t capacity)
{
    writer->data = data;
    writer->capacity = capacity < NETMIDI2_MAX_PACKET ? capacity : NETMIDI2_MAX_PACKET;
    writer->length = 0;
    if (writer->capacity < 4)
        return -1;
    put32(data, NETMIDI2_SIGNATURE);
    writer->length = 4;
    return 0;
}

int netmidi2_add(struct netmidi2_writer *writer, uint8_t code, uint8_t csd1, uint8_t csd2,
               const uint8_t *payload, size_t payload_bytes)
{
    size_t words = (payload_bytes + 3) / 4;
    uint8_t *p;

    if (words > 255 || writer->capacity - writer->length < 4 + words * 4)
        return -1;
    p = writer->data + writer->length;
    p[0] = code;
    p[1] = (uint8_t)words;
    p[2] = csd1;
    p[3] = csd2;
    memset(p + 4, 0, words * 4);
    if (payload_bytes)
        memcpy(p + 4, payload, payload_bytes);
    writer->length += 4 + words * 4;
    return 0;
}

/* The length of a string that ends in a zero byte, looked for in its first
 * limit + 1 bytes: limit + 1 when there is none there, which is "too
 * long" to whoever asked. */
static size_t text_length(const char *text, size_t limit)
{
    size_t length = 0;

    while (length <= limit && text[length])
        ++length;
    return length;
}

int netmidi2_add_identity(struct netmidi2_writer *writer, uint8_t code, uint8_t csd2,
                        const char *name, const char *product_id)
{
    uint8_t payload[(NETMIDI2_NAME_MAX + 3) / 4 * 4 + (NETMIDI2_PRODUCT_ID_MAX + 3) / 4 * 4];
    size_t name_length = text_length(name, NETMIDI2_NAME_MAX);
    size_t id_length = text_length(product_id, NETMIDI2_PRODUCT_ID_MAX);
    size_t name_words = (name_length + 3) / 4;

    if (name_length > NETMIDI2_NAME_MAX || id_length > NETMIDI2_PRODUCT_ID_MAX)
        return -1;
    memset(payload, 0, sizeof(payload));
    memcpy(payload, name, name_length);
    memcpy(payload + name_words * 4, product_id, id_length);
    return netmidi2_add(writer, code, (uint8_t)name_words, csd2, payload,
                      name_words * 4 + id_length);
}

int netmidi2_add_ping(struct netmidi2_writer *writer, uint8_t code, uint32_t id)
{
    uint8_t payload[4];
    put32(payload, id);
    return netmidi2_add(writer, code, 0, 0, payload, 4);
}

int netmidi2_add_ump(struct netmidi2_writer *writer, uint16_t sequence,
                   const uint32_t *words, size_t count)
{
    /* with no words it goes as it is: a UMP Data command may be empty */
    uint8_t payload[NETMIDI2_MAX_UMP_WORDS * 4] = {0};
    size_t i;

    if (count > NETMIDI2_MAX_UMP_WORDS)
        return -1;
    for (i = 0; i < count; ++i)
        put32(payload + i * 4, words[i]);
    return netmidi2_add(writer, NETMIDI2_UMP_DATA, (uint8_t)(sequence >> 8),
                      (uint8_t)sequence, payload, count * 4);
}

/* Section 6.15, table 24: the first word of the command refused. */
int netmidi2_add_nak(struct netmidi2_writer *writer, uint8_t reason,
                   const struct netmidi2_command_packet *original)
{
    uint8_t header[4];
    header[0] = original->code;
    header[1] = (uint8_t)original->payload_words;
    header[2] = original->csd1;
    header[3] = original->csd2;
    return netmidi2_add(writer, NETMIDI2_NAK, reason, 0, header, 4);
}

static void copy_text(char *to, size_t size, const uint8_t *from, size_t length)
{
    size_t i;
    for (i = 0; i < length && i + 1 < size && from[i]; ++i)
        to[i] = (char)from[i];
    to[i] = 0;
}

int netmidi2_identity(const struct netmidi2_command_packet *command,
                    char *name, size_t name_size, char *product_id, size_t id_size)
{
    size_t name_words = command->csd1;

    if (!name || !product_id || !name_size || !id_size ||
        name_words > command->payload_words)
        return -1;
    copy_text(name, name_size, command->payload, name_words * 4);
    copy_text(product_id, id_size, command->payload + name_words * 4,
              (command->payload_words - name_words) * 4);
    return 0;
}

/* Sessions */

void netmidi2_session_init(struct netmidi2_session *session, int host,
                         const char *name, const char *product_id,
                         netmidi2_ump_sink sink, void *context)
{
    memset(session, 0, sizeof(*session));
    session->host = host;
    /* copy_text stops at the string's end, or where there is no more room */
    copy_text(session->name, sizeof(session->name), (const uint8_t *)name,
              sizeof(session->name));
    copy_text(session->product_id, sizeof(session->product_id),
              (const uint8_t *)product_id, sizeof(session->product_id));
    session->protocols = UMP_PROTOCOL_MIDI1;
    session->ump_sink = sink;
    session->ump_context = context;
}

static void set_state(struct netmidi2_session *session, enum netmidi2_state state)
{
    session->state = state;
    session->state_since_ms = session->now_ms;
    session->retries = 0;
}

/* Both directions count from zero and nothing sent before is repeated:
 * a new session, and a reset one (6.11, "How to Reset the Session"). */
static void restart_streams(struct netmidi2_session *session)
{
    session->tx_sequence = 0;
    session->sent_count = 0;
    session->kept_count = session->kept_next = 0;
    session->rx_started = 1;
    session->rx_expected = 0;
    session->last_send_ms = session->now_ms;
    session->idle_sent = 0;
    session->next_idle_ms = session->now_ms + IDLE_STOP_MS;
}

static void start_session(struct netmidi2_session *session)
{
    set_state(session, NETMIDI2_ESTABLISHED);
    restart_streams(session);
    session->rx_lost = 0;
    session->last_receive_ms = session->now_ms;
    session->pings_unanswered = 0;
}

int netmidi2_invite(struct netmidi2_session *session, uint32_t now_ms,
                  struct netmidi2_writer *out)
{
    return netmidi2_invite_to(session, 0, now_ms, out);
}

int netmidi2_invite_to(struct netmidi2_session *session, uint64_t peer,
                     uint32_t now_ms, struct netmidi2_writer *out)
{
    if (session->host)
        return -1;
    session->peer = peer;
    session->now_ms = now_ms;
    set_state(session, NETMIDI2_INVITING);
    return netmidi2_add_identity(out, NETMIDI2_INVITATION, 0, session->name,
                               session->product_id);
}

static int append_ump_command(struct netmidi2_session *session, const uint32_t *words,
                              size_t count, struct netmidi2_writer *out)
{
    unsigned i;
    struct netmidi2_sent_ump *slot;
    size_t before = out->length;
    /* FEC: the previous UMP Data commands first, in the order sent
       (7.2.2). All of it or none: a packet half written is no packet. */
    for (i = 0; i < session->sent_count; ++i)
        if (netmidi2_add_ump(out, session->sent[i].sequence, session->sent[i].data,
                           session->sent[i].words)) {
            out->length = before;
            return -1;
        }
    if (netmidi2_add_ump(out, session->tx_sequence, words, count)) {
        out->length = before;
        return -1;
    }
    if (session->sent_count == NETMIDI2_FEC_REPEATS) {
        memmove(&session->sent[0], &session->sent[1],
                sizeof(session->sent[0]) * (NETMIDI2_FEC_REPEATS - 1));
        --session->sent_count;
    }
    slot = &session->sent[session->sent_count++];
    slot->sequence = session->tx_sequence;
    slot->words = (uint8_t)count;
    if (count)
        memcpy(slot->data, words, count * sizeof(*words));
    /* and for whoever asks for it again later (7.2.3) */
    {
        struct netmidi2_kept_ump *kept = &session->kept[session->kept_next];

        kept->sequence = session->tx_sequence;
        if (count <= NETMIDI2_KEPT_WORDS) {
            kept->words = (uint8_t)count;
            if (count)
                memcpy(kept->data, words, count * sizeof(*words));
        } else {
            kept->words = NETMIDI2_KEPT_TOO_LONG;
        }
        session->kept_next = (session->kept_next + 1) % NETMIDI2_KEPT;
        if (session->kept_count < NETMIDI2_KEPT)
            ++session->kept_count;
    }
    ++session->tx_sequence;
    return 0;
}

static void retransmit_error(struct netmidi2_writer *out, uint16_t sequence)
{
    uint8_t payload[4];

    payload[0] = (uint8_t)(sequence >> 8);
    payload[1] = (uint8_t)sequence;
    payload[2] = payload[3] = 0;
    (void)netmidi2_add(out, NETMIDI2_RETRANSMIT_ERROR,
                       NETMIDI2_RETRANSMIT_NOT_KEPT, 0, payload, 4);
}

/* A Retransmit Request (7.2.3): the commands from the one asked for to
 * the newest, as they were sent, as far as the packet holds; the count in
 * the request may be ignored, and is. One no longer kept is answered with
 * a Retransmit Error naming it, and what follows it is still sent. */
static void retransmit(struct netmidi2_session *session, uint16_t from,
                       struct netmidi2_writer *out)
{
    unsigned oldest = (session->kept_next + NETMIDI2_KEPT -
                       session->kept_count) % NETMIDI2_KEPT;
    unsigned i, first = session->kept_count;
    int told = 0;

    for (i = 0; i < session->kept_count; ++i)
        if (session->kept[(oldest + i) % NETMIDI2_KEPT].sequence == from) {
            first = i;
            break;
        }
    if (first == session->kept_count) {
        /* not there: the first we could still send again is told */
        ++session->not_retransmitted;
        retransmit_error(out, session->kept_count ?
                         session->kept[oldest].sequence :
                         session->tx_sequence);
        return;
    }
    for (i = first; i < session->kept_count; ++i) {
        const struct netmidi2_kept_ump *kept =
            &session->kept[(oldest + i) % NETMIDI2_KEPT];

        if (kept->words == NETMIDI2_KEPT_TOO_LONG) {
            if (!told) {
                ++session->not_retransmitted;
                retransmit_error(out, kept->sequence);
                told = 1;
            }
            continue;
        }
        if (netmidi2_add_ump(out, kept->sequence, kept->data, kept->words))
            break;              /* the packet is full: it will ask again */
        ++session->retransmitted;
    }
}

int netmidi2_send_ump(struct netmidi2_session *session, const uint32_t *words,
                    size_t count, uint32_t now_ms, struct netmidi2_writer *out)
{
    if (session->state != NETMIDI2_ESTABLISHED || count == 0 ||
        count > NETMIDI2_MAX_UMP_WORDS)
        return -1;
    session->now_ms = now_ms;
    if (append_ump_command(session, words, count, out))
        return -1;
    session->last_send_ms = now_ms;
    session->idle_sent = 0;
    session->next_idle_ms = now_ms + IDLE_FIRST_MS;
    return 0;
}

static void answer_stream(struct netmidi2_session *session, const uint32_t *words,
                          struct netmidi2_writer *out)
{
    uint32_t reply[4 * 8];
    unsigned status = ump_stream_status(words), n;
    unsigned filter;

    if (status == UMP_ENDPOINT_DISCOVERY) {
        filter = words[1] & 0xff;
        if (filter & UMP_FILTER_ENDPOINT_INFO) {
            ump_endpoint_info(reply, session->protocols, session->jr_offered);
            (void)netmidi2_send_ump(session, reply, 4, session->now_ms, out);
        }
        if (filter & UMP_FILTER_ENDPOINT_NAME) {
            n = ump_stream_text(reply, sizeof(reply) / sizeof(reply[0]),
                                   UMP_ENDPOINT_NAME, session->name,
                                   text_length(session->name,
                                               NETMIDI2_NAME_MAX));
            if (n)
                (void)netmidi2_send_ump(session, reply, n, session->now_ms, out);
        }
        if (filter & UMP_FILTER_PRODUCT_INSTANCE_ID) {
            n = ump_stream_text(reply, sizeof(reply) / sizeof(reply[0]),
                                   UMP_PRODUCT_INSTANCE_ID, session->product_id,
                                   text_length(session->product_id,
                                               NETMIDI2_PRODUCT_ID_MAX));
            if (n)
                (void)netmidi2_send_ump(session, reply, n, session->now_ms, out);
        }
        if (filter & UMP_FILTER_STREAM_CONFIG) {
            ump_stream_config(reply, UMP_PROTOCOL_MIDI1, session->jr);
            (void)netmidi2_send_ump(session, reply, 4, session->now_ms, out);
        }
    } else if (status == UMP_FUNCTION_BLOCK_DISCOVERY) {
        /* Block 0, or 0xFF for all of them, which is the same one (UMP
           specification 7.1.7). Each filter bit is a reply of its own. */
        unsigned block = (words[0] >> 8) & 0xff;

        filter = words[0] & 0xff;
        if (block != 0 && block != 0xff)
            return;
        if (filter & UMP_FUNCTION_BLOCK_FILTER_INFO) {
            ump_function_block_info(reply, 0);
            (void)netmidi2_send_ump(session, reply, 4, session->now_ms, out);
        }
        if (filter & UMP_FUNCTION_BLOCK_FILTER_NAME) {
            size_t length = text_length(session->name, NETMIDI2_NAME_MAX);

            /* the block is called as the endpoint is, as far as fits */
            if (length > UMP_FUNCTION_BLOCK_NAME_MAX)
                length = UMP_FUNCTION_BLOCK_NAME_MAX;
            n = ump_function_block_name(reply,
                                        sizeof(reply) / sizeof(reply[0]),
                                        session->name, length);
            if (n)
                (void)netmidi2_send_ump(session, reply, n, session->now_ms,
                                        out);
        }
    } else if (status == UMP_STREAM_CONFIG_REQUEST) {
        /* This endpoint speaks the MIDI 1.0 protocol in UMP only. It
           takes the peer's timestamps when the caller uses them, and
           sends none: what cannot be changed is answered as it is
           (7.1.6.1, step 3). */
        session->jr = words[0] & session->jr_offered & UMP_JR_RECEIVE;
        ump_stream_config(reply, UMP_PROTOCOL_MIDI1, session->jr);
        (void)netmidi2_send_ump(session, reply, 4, session->now_ms, out);
    }
}

static void receive_ump(struct netmidi2_session *session,
                        const struct netmidi2_command_packet *command,
                        struct netmidi2_writer *out)
{
    uint16_t sequence = (uint16_t)((command->csd1 << 8) | command->csd2);
    uint32_t words[NETMIDI2_MAX_UMP_WORDS];
    size_t count = command->payload_words, i;

    if (count > NETMIDI2_MAX_UMP_WORDS)
        return;
    if (session->rx_started) {
        int16_t ahead = (int16_t)(sequence - session->rx_expected);
        if (ahead < 0)
            return;                 /* an FEC repeat already handled */
        if (ahead > 0) {
            /* Lost beyond what the repeats carry, and we ask for no
               retransmission: the caller is told, for what may hang. */
            session->rx_lost += (unsigned long)ahead;
            ++session->gaps;
        }
    }
    session->rx_started = 1;
    session->rx_expected = (uint16_t)(sequence + 1);
    for (i = 0; i < count; ++i)
        words[i] = get32(command->payload + i * 4);
    for (i = 0; i < count; ) {
        unsigned n = ump_words(words[i]);
        if (i + n > count)
            break;
        if ((words[i] >> 28) == UMP_STREAM)
            answer_stream(session, words + i, out);
        else if (session->ump_sink)
            session->ump_sink(session->ump_context, words + i, n);
        i += n;
    }
}

static void bye(struct netmidi2_writer *out, uint8_t reason)
{
    (void)netmidi2_add(out, NETMIDI2_BYE, reason, 0, NULL, 0);
}

/* A command from someone the session is not with. */
static void receive_stranger(const struct netmidi2_session *session,
                             const struct netmidi2_command_packet *c,
                             struct netmidi2_writer *out)
{
    switch (c->code) {
    case NETMIDI2_INVITATION:
    case NETMIDI2_INVITATION_AUTH:
    case NETMIDI2_INVITATION_USER_AUTH:
        if (session->host)
            bye(out, NETMIDI2_BYE_TOO_MANY_SESSIONS);       /* 6.4 */
        else
            (void)netmidi2_add_nak(out, NETMIDI2_NAK_NOT_EXPECTED, c);
        break;
    case NETMIDI2_INVITATION_ACCEPTED:
    case NETMIDI2_INVITATION_PENDING:
        bye(out, NETMIDI2_BYE_NO_PENDING_SESSION);          /* 6.5, 6.6 */
        break;
    case NETMIDI2_INVITATION_AUTH_REQUIRED:
    case NETMIDI2_INVITATION_USER_AUTH_REQUIRED:
        (void)netmidi2_add_nak(out, NETMIDI2_NAK_NOT_EXPECTED, c);
        break;
    case NETMIDI2_PING:                                     /* 6.13 */
        if (c->payload_words >= 1)
            (void)netmidi2_add_ping(out, NETMIDI2_PING_REPLY, get32(c->payload));
        break;
    case NETMIDI2_BYE:                                      /* 6.16 */
        (void)netmidi2_add(out, NETMIDI2_BYE_REPLY, 0, 0, NULL, 0);
        break;
    case NETMIDI2_UMP_DATA:
    case NETMIDI2_SESSION_RESET:
    case NETMIDI2_SESSION_RESET_REPLY:
    case NETMIDI2_RETRANSMIT_REQUEST:
    case NETMIDI2_RETRANSMIT_ERROR:
        bye(out, NETMIDI2_BYE_NOT_ESTABLISHED);
        break;
    case NETMIDI2_PING_REPLY:
    case NETMIDI2_BYE_REPLY:
    case NETMIDI2_NAK:
        break;
    default:
        (void)netmidi2_add_nak(out, NETMIDI2_NAK_NOT_SUPPORTED, c);
        break;
    }
}

int netmidi2_receive(struct netmidi2_session *session, const uint8_t *packet,
                   size_t length, uint32_t now_ms, struct netmidi2_writer *out)
{
    return netmidi2_receive_from(session, session->peer, packet, length,
                                 now_ms, out);
}

int netmidi2_receive_from(struct netmidi2_session *session, uint64_t peer,
                        const uint8_t *packet, size_t length,
                        uint32_t now_ms, struct netmidi2_writer *out)
{
    struct netmidi2_command_packet commands[64];
    int count, i;

    count = netmidi2_parse(packet, length, commands, 64);
    if (count < 0)
        return -1;
    session->now_ms = now_ms;
    if (session->state != NETMIDI2_IDLE && peer != session->peer) {
        for (i = 0; i < count; ++i)
            receive_stranger(session, &commands[i], out);
        return 0;
    }
    session->last_receive_ms = now_ms;
    for (i = 0; i < count; ++i) {
        const struct netmidi2_command_packet *c = &commands[i];
        int active = session->state == NETMIDI2_ESTABLISHED ||
                     session->state == NETMIDI2_RESETTING;
        switch (c->code) {
        case NETMIDI2_INVITATION:                           /* 6.4 */
            if (!session->host) {
                (void)netmidi2_add_nak(out, NETMIDI2_NAK_NOT_EXPECTED, c);
                break;
            }
            /* Table 27 gives a missing name or id as the example of a
               protocol error. */
            if (netmidi2_identity(c, session->peer_name, sizeof(session->peer_name),
                                session->peer_product_id,
                                sizeof(session->peer_product_id)) ||
                !session->peer_name[0] || !session->peer_product_id[0]) {
                bye(out, NETMIDI2_BYE_PROTOCOL_ERROR);
                break;
            }
            if (!active && session->refuse) {
                bye(out, NETMIDI2_BYE_TOO_MANY_SESSIONS);
                break;
            }
            /* An invitation within a session is accepted again. */
            if (!active) {
                session->peer = peer;
                start_session(session);
            }
            (void)netmidi2_add_identity(out, NETMIDI2_INVITATION_ACCEPTED, 0,
                                      session->name, session->product_id);
            break;
        case NETMIDI2_INVITATION_AUTH:
        case NETMIDI2_INVITATION_USER_AUTH:
            /* We never ask for authentication, so no invitation of ours
               came before this one (table 27, 0x41). */
            if (session->host)
                bye(out, NETMIDI2_BYE_AUTH_WITHOUT_INVITATION);
            else
                (void)netmidi2_add_nak(out, NETMIDI2_NAK_NOT_EXPECTED, c);
            break;
        case NETMIDI2_INVITATION_ACCEPTED:                  /* 6.5 */
            if (session->host)
                (void)netmidi2_add_nak(out, NETMIDI2_NAK_NOT_EXPECTED, c);
            else if (session->state == NETMIDI2_INVITING) {
                char peer_name[sizeof(session->peer_name)];
                char peer_product_id[sizeof(session->peer_product_id)];

                if (netmidi2_identity(c, peer_name, sizeof(peer_name),
                                     peer_product_id, sizeof(peer_product_id)) ||
                    !peer_name[0] || !peer_product_id[0]) {
                    (void)netmidi2_add_nak(out, NETMIDI2_NAK_MALFORMED, c);
                    break;
                }
                copy_text(session->peer_name, sizeof(session->peer_name),
                          (const uint8_t *)peer_name, sizeof(peer_name));
                copy_text(session->peer_product_id, sizeof(session->peer_product_id),
                          (const uint8_t *)peer_product_id, sizeof(peer_product_id));
                start_session(session);
            } else if (!active) {
                bye(out, NETMIDI2_BYE_NO_PENDING_SESSION);
            }
            break;
        case NETMIDI2_INVITATION_PENDING:                   /* 6.6 */
            if (session->host)
                (void)netmidi2_add_nak(out, NETMIDI2_NAK_NOT_EXPECTED, c);
            else if (session->state == NETMIDI2_INVITING)
                session->state_since_ms = now_ms;   /* keep waiting */
            else if (!active)
                bye(out, NETMIDI2_BYE_NO_PENDING_SESSION);
            break;
        case NETMIDI2_INVITATION_AUTH_REQUIRED:             /* 6.7, 6.8 */
        case NETMIDI2_INVITATION_USER_AUTH_REQUIRED:
            /* A Host may send these only to a Client that said it can
               authenticate, and we said we cannot. */
            if (!session->host && session->state == NETMIDI2_INVITING)
                (void)netmidi2_bye(session, NETMIDI2_BYE_INVITATION_CANCELED,
                                   now_ms, out);
            else
                (void)netmidi2_add_nak(out, NETMIDI2_NAK_NOT_EXPECTED, c);
            break;
        case NETMIDI2_PING:                                 /* 6.13, 6.14 */
            if (c->payload_words >= 1)
                (void)netmidi2_add_ping(out, NETMIDI2_PING_REPLY, get32(c->payload));
            break;
        case NETMIDI2_PING_REPLY:
            if (c->payload_words >= 1 && get32(c->payload) == session->ping_id)
                session->pings_unanswered = 0;
            break;
        case NETMIDI2_UMP_DATA:                             /* 7.1 */
            if (!active)
                bye(out, NETMIDI2_BYE_NOT_ESTABLISHED);
            else if (session->state == NETMIDI2_ESTABLISHED)
                receive_ump(session, c, out);
            /* while a reset is awaited, what comes is of before it */
            break;
        case NETMIDI2_SESSION_RESET:                        /* 6.11 */
            if (!active) {
                bye(out, NETMIDI2_BYE_NOT_ESTABLISHED);
                break;
            }
            (void)netmidi2_add(out, NETMIDI2_SESSION_RESET_REPLY, 0, 0, NULL, 0);
            /* a repeat of one already obeyed finds nothing sent since */
            if (session->tx_sequence || session->rx_expected ||
                session->state == NETMIDI2_RESETTING) {
                set_state(session, NETMIDI2_ESTABLISHED);
                restart_streams(session);
                ++session->resets;
            }
            break;
        case NETMIDI2_SESSION_RESET_REPLY:                  /* 6.12 */
            if (!active) {
                bye(out, NETMIDI2_BYE_NOT_ESTABLISHED);
            } else if (session->state == NETMIDI2_RESETTING) {
                set_state(session, NETMIDI2_ESTABLISHED);
                restart_streams(session);
                ++session->resets;
            } else {
                /* a reply to a reset we did not ask for: ask for one */
                (void)netmidi2_reset(session, now_ms, out);
            }
            break;
        case NETMIDI2_RETRANSMIT_REQUEST:                   /* 7.2.3 */
            if (!active)
                bye(out, NETMIDI2_BYE_NOT_ESTABLISHED);
            else
                retransmit(session, (uint16_t)((c->csd1 << 8) | c->csd2),
                           out);
            break;
        case NETMIDI2_RETRANSMIT_ERROR:                     /* 7.2.4 */
            if (!active)
                bye(out, NETMIDI2_BYE_NOT_ESTABLISHED);
            break;
        case NETMIDI2_BYE:                                  /* 6.16 */
            (void)netmidi2_add(out, NETMIDI2_BYE_REPLY, 0, 0, NULL, 0);
            set_state(session, NETMIDI2_IDLE);
            break;
        case NETMIDI2_BYE_REPLY:                            /* 6.17 */
            if (session->state == NETMIDI2_CLOSING)
                set_state(session, NETMIDI2_IDLE);
            break;
        case NETMIDI2_NAK:
            break;
        default:                                            /* 5.5 */
            (void)netmidi2_add_nak(out, NETMIDI2_NAK_NOT_SUPPORTED, c);
            break;
        }
    }
    return 0;
}

int netmidi2_reset(struct netmidi2_session *session, uint32_t now_ms,
                 struct netmidi2_writer *out)
{
    session->now_ms = now_ms;
    if (session->state != NETMIDI2_ESTABLISHED &&
        session->state != NETMIDI2_RESETTING)
        return -1;
    if (session->state != NETMIDI2_RESETTING)
        set_state(session, NETMIDI2_RESETTING);
    return netmidi2_add(out, NETMIDI2_SESSION_RESET, 0, 0, NULL, 0);
}

int netmidi2_bye(struct netmidi2_session *session, uint8_t reason, uint32_t now_ms,
               struct netmidi2_writer *out)
{
    session->now_ms = now_ms;
    if (session->state == NETMIDI2_IDLE)
        return -1;
    if (session->state == NETMIDI2_INVITING)
        reason = NETMIDI2_BYE_INVITATION_CANCELED;
    set_state(session, NETMIDI2_CLOSING);
    session->bye_reason = reason;
    return netmidi2_add(out, NETMIDI2_BYE, reason, 0, NULL, 0);
}

int netmidi2_tick(struct netmidi2_session *session, uint32_t now_ms,
                struct netmidi2_writer *out)
{
    size_t before = out->length;
    uint32_t elapsed = now_ms - session->state_since_ms;

    session->now_ms = now_ms;
    switch (session->state) {
    case NETMIDI2_IDLE:
        break;
    case NETMIDI2_INVITING:
        if (elapsed >= RETRY_MS * (session->retries + 1)) {
            if (++session->retries >= RETRY_LIMIT)
                /* given up: Bye, Invitation Canceled (6.4) */
                (void)netmidi2_bye(session, NETMIDI2_BYE_INVITATION_CANCELED,
                                   now_ms, out);
            else
                (void)netmidi2_add_identity(out, NETMIDI2_INVITATION, 0, session->name,
                                          session->product_id);
        }
        break;
    case NETMIDI2_CLOSING:
        if (elapsed >= RETRY_MS * (session->retries + 1)) {
            if (++session->retries >= RETRY_LIMIT / 2)
                set_state(session, NETMIDI2_IDLE);
            else
                (void)netmidi2_add(out, NETMIDI2_BYE, session->bye_reason, 0, NULL, 0);
        }
        break;
    case NETMIDI2_RESETTING:
        /* repeated until its reply, then Bye, Timeout (6.11) */
        if (elapsed >= RETRY_MS * (session->retries + 1)) {
            if (++session->retries >= RETRY_LIMIT)
                (void)netmidi2_bye(session, NETMIDI2_BYE_TIMEOUT, now_ms, out);
            else
                (void)netmidi2_add(out, NETMIDI2_SESSION_RESET, 0, 0, NULL, 0);
        }
        break;
    case NETMIDI2_ESTABLISHED:
        /* Idle: tell the peer there is no data, repeating the last data
           for FEC first, then at growing intervals until stopping. */
        if ((int32_t)(now_ms - session->next_idle_ms) >= 0 &&
            now_ms - session->last_send_ms < IDLE_STOP_MS) {
            (void)append_ump_command(session, NULL, 0, out);
            ++session->idle_sent;
            session->next_idle_ms = now_ms +
                (session->idle_sent <= NETMIDI2_FEC_REPEATS ? IDLE_FEC_MS :
                 IDLE_FIRST_MS << (session->idle_sent - NETMIDI2_FEC_REPEATS));
        }
        /* Ping a quiet peer; give up after several unanswered pings. */
        if (now_ms - session->last_receive_ms >= PING_IDLE_MS &&
            now_ms - session->ping_sent_ms >= PING_IDLE_MS) {
            if (session->pings_unanswered >= PING_LIMIT) {
                (void)netmidi2_bye(session, NETMIDI2_BYE_TIMEOUT, now_ms, out);
                break;
            }
            session->ping_id = now_ms ^ 0x5a5a0000u;
            session->ping_sent_ms = now_ms;
            ++session->pings_unanswered;
            (void)netmidi2_add_ping(out, NETMIDI2_PING, session->ping_id);
        }
        break;
    }
    return out->length > before;
}

/* Several sessions on one port */

static void slot_sink(void *context, const uint32_t *words, size_t count)
{
    struct netmidi2_table_slot *slot = context;
    if (slot->table->sink)
        slot->table->sink(slot->table->context, slot->index, words, count);
}

void netmidi2_table_init(struct netmidi2_table *table, const char *name,
                         const char *product_id, netmidi2_table_sink sink,
                         void *context)
{
    int i;
    memset(table, 0, sizeof(*table));
    copy_text(table->name, sizeof(table->name), (const uint8_t *)name,
              sizeof(table->name));
    copy_text(table->product_id, sizeof(table->product_id),
              (const uint8_t *)product_id, sizeof(table->product_id));
    table->accept = 1;
    table->sink = sink;
    table->context = context;
    for (i = 0; i < NETMIDI2_SESSIONS_MAX; ++i) {
        table->slot[i].table = table;
        table->slot[i].index = i;
    }
}

int netmidi2_table_find(const struct netmidi2_table *table, uint64_t peer)
{
    int i;
    for (i = 0; i < NETMIDI2_SESSIONS_MAX; ++i)
        if (table->slot[i].used && table->slot[i].session.peer == peer)
            return i;
    return -1;
}

static int free_slot(const struct netmidi2_table *table)
{
    int i;
    for (i = 0; i < NETMIDI2_SESSIONS_MAX; ++i)
        if (!table->slot[i].used)
            return i;
    return -1;
}

static void slot_begin(struct netmidi2_table *table, int index, int host)
{
    struct netmidi2_table_slot *slot = &table->slot[index];
    netmidi2_session_init(&slot->session, host, table->name,
                          table->product_id, slot_sink, slot);
    slot->session.jr_offered = table->jr_offered;
    slot->used = 1;
}

int netmidi2_table_invite(struct netmidi2_table *table, uint64_t peer,
                          uint32_t now_ms, struct netmidi2_writer *out)
{
    int index = free_slot(table);
    if (index < 0 || netmidi2_table_find(table, peer) >= 0)
        return -1;
    slot_begin(table, index, 0);
    if (netmidi2_invite_to(&table->slot[index].session, peer, now_ms, out)) {
        table->slot[index].used = 0;
        return -1;
    }
    return index;
}

int netmidi2_table_receive(struct netmidi2_table *table, uint64_t peer,
                           const uint8_t *packet, size_t length,
                           uint32_t now_ms, struct netmidi2_writer *out,
                           int *index)
{
    int found = netmidi2_table_find(table, peer);

    if (index)
        *index = found;
    if (found >= 0)
        return netmidi2_receive_from(&table->slot[found].session, peer,
                                     packet, length, now_ms, out);
    /* Someone we have no session with. An idle Host answers for the
       table, in a slot if there is one free and Invitations are taken;
       the slot is kept only if a session began. */
    found = table->accept ? free_slot(table) : -1;
    if (found >= 0) {
        int result;

        slot_begin(table, found, 1);
        result = netmidi2_receive_from(&table->slot[found].session, peer,
                                       packet, length, now_ms, out);
        if (table->slot[found].session.state == NETMIDI2_IDLE)
            table->slot[found].used = 0;
        else if (index)
            *index = found;
        return result;
    }
    netmidi2_session_init(&table->nobody, 1, table->name, table->product_id,
                          NULL, NULL);
    table->nobody.jr_offered = table->jr_offered;
    table->nobody.refuse = 1;
    return netmidi2_receive_from(&table->nobody, peer, packet, length,
                                 now_ms, out);
}

int netmidi2_table_tick(struct netmidi2_table *table, int index,
                        uint32_t now_ms, struct netmidi2_writer *out)
{
    struct netmidi2_table_slot *slot;
    int result;

    if (index < 0 || index >= NETMIDI2_SESSIONS_MAX ||
        !table->slot[index].used)
        return 0;
    slot = &table->slot[index];
    result = netmidi2_tick(&slot->session, now_ms, out);
    if (slot->session.state == NETMIDI2_IDLE)
        slot->used = 0;
    return result;
}

struct netmidi2_session *netmidi2_table_session(struct netmidi2_table *table,
                                                int index)
{
    if (index < 0 || index >= NETMIDI2_SESSIONS_MAX ||
        !table->slot[index].used)
        return NULL;
    return &table->slot[index].session;
}
