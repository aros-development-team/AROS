/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* Network MIDI 2.0 (UDP) as a transport of netmidid: a Host on a port of
 * its own, and the Client of the Host the configuration names. A peer's
 * UMPs become MIDI 1.0 for CAMD by the default translation, and what CAMD
 * sends becomes UMPs with the MIDI 1.0 protocol: this endpoint declares no
 * other. Each session has a translator of its own and a port of the driver
 * networkmidi2. The protocol is the link library netmidi2's. */


#include "netmidid.h"

#include <netmidi2/netmidi2.h>
#include <ump/ump.h>
#include <rtpmidi/ports.h>
#include <rtpmidi/rtpmidi.h>
#include <rtpmidi/timing.h>
/* for the names peers give and how long to wait between invitations */
#include <rtpmidi/applemidi.h>
#include <rtpmidi/peers.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto/bsdsocket.h>

#define PRODUCT_ID_MAX 42   /* M2-124-UM section 4.4 */

/* what the core hands over fits the buffers SysEx is gathered into */
typedef char sysex_fits[NETMIDID_SYSEX_MAX <= RTPMIDI_SYSEX_MAX ? 1 : -1];


struct midi2_runtime {
    struct netmidid *d;
    int socket;
    uint16_t port;
    struct netmidi2_table midi2;
    struct ump_to_midi1 midi2_in[NETMIDI2_SESSIONS_MAX];
    /* where each session's SysEx messages are gathered: there while the
       session is, since it is 32 KiB a session */
    uint8_t *midi2_sysex[NETMIDI2_SESSIONS_MAX];
    int midi2_known[NETMIDI2_SESSIONS_MAX];
    unsigned long midi2_gaps[NETMIDI2_SESSIONS_MAX];
    unsigned long midi2_resets[NETMIDI2_SESSIONS_MAX];
    /* Each peer's clock against ours, from its JR Clock messages, and what
       its timestamps have us hold until its time (M2-104-UM 7.2.2) */
    struct ump_jr_clock midi2_clock[NETMIDI2_SESSIONS_MAX];
    struct rtpmidi_event_queue midi2_queue[NETMIDI2_SESSIONS_MAX];
    int midi2_current;              /* the session whose UMP is passing */
    /* the port each session's peer has, of the networkmidi2 driver's,
       numbered through with the other driver's; -1 for none */
    int midi2_port[NETMIDI2_SESSIONS_MAX];
    /* A Host to invite, as its Client */
    int midi2_host;                 /* its address and port are known */
    uint64_t midi2_host_key;
    uint64_t midi2_tried;
    uint64_t midi2_wait;
    struct netmidi_port_counts midi2_count[NETMIDI2_SESSIONS_MAX];
    char product_id[PRODUCT_ID_MAX + 1];
    unsigned long midi2_timed;
    unsigned long midi2_late;
};

static uint64_t midi2_key(const struct sockaddr_in *address)
{
    return ((uint64_t)ntohl(address->sin_addr.s_addr) << 16) |
           ntohs(address->sin_port);
}

static void midi2_send(struct midi2_runtime *rt, uint64_t key,
                       const struct netmidi2_writer *packet)
{
    struct sockaddr_in address;
    if (packet->length <= 4)
        return;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl((uint32_t)(key >> 16));
    address.sin_port = htons((uint16_t)key);
    netmidi_network_send(&rt->d->network, rt->socket, packet->data,
                         packet->length, &address);
}

static uint64_t now_us(void)
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return (uint64_t)time.tv_sec * 1000000 + (uint64_t)time.tv_usec;
}

static void midi2_deliver(struct midi2_runtime *rt, int index,
                          const uint8_t *bytes, size_t length)
{
    if (rt->midi2_port[index] >= 0)
        netmidi_camd_deliver_port(&rt->d->camd, rt->midi2_port[index], bytes,
                                  length);
}

/* A message translated from a peer's UMPs. One that came with a JR
 * Timestamp, from a peer whose clock is known, waits for its time and the
 * latency asked for; any other is for at once (7.2.2.3). */
static void midi2_to_camd(void *context, const uint8_t *bytes, size_t length)
{
    struct midi2_runtime *rt = context;
    int index = rt->midi2_current;
    const struct ump_jr *jr = &rt->midi2_in[index].jr;
    uint64_t due_us;

    ++rt->midi2_count[index].messages_in;
    if (jr->have_timestamp &&
        ump_jr_clock_due(&rt->midi2_clock[index], jr->timestamp, &due_us)) {
        uint64_t now = netmidid_now();
        uint64_t due = due_us / 100 + rt->d->latency;

        ++rt->midi2_timed;
        ++rt->midi2_count[index].timed;
        if (due > now && due - now <= RTPMIDI_QUEUE_HORIZON &&
            (bytes[0] == 0xf0 ?
                 rtpmidi_queue_push_sysex(&rt->midi2_queue[index], due, bytes,
                                          length) :
                 rtpmidi_queue_push(&rt->midi2_queue[index], due, bytes,
                                    length)) == 0)
            return;
        if (due <= now) {
            ++rt->midi2_late;
            ++rt->midi2_count[index].late;
        }
    }
    midi2_deliver(rt, index, bytes, length);
}

/* What was held and is now due. */
static void midi2_release(struct midi2_runtime *rt, uint64_t now)
{
    struct rtpmidi_queued_event event;
    int index;

    for (index = 0; index < NETMIDI2_SESSIONS_MAX; ++index)
        while (rtpmidi_queue_pop_due(&rt->midi2_queue[index], now, &event)) {
            if (event.sysex)
                midi2_deliver(rt, index, event.sysex, event.sysex_length);
            else
                midi2_deliver(rt, index, event.bytes, event.length);
            rtpmidi_queue_event_release(&event);
        }
}

static void midi2_sink(void *context, int index, const uint32_t *words,
                       size_t count)
{
    struct midi2_runtime *rt = context;
    size_t i = 0;
    ++rt->midi2_count[index].packets_in;
    while (i < count) {
        unsigned n = ump_words(words[i]);
        if (i + n > count)
            break;
        {
            unsigned long clocks = rt->midi2_in[index].jr.clocks;

            rt->midi2_current = index;
            ump_to_midi1(&rt->midi2_in[index], words + i, n, midi2_to_camd,
                         rt);
            if (rt->midi2_in[index].jr.clocks != clocks)
                ump_jr_clock_seen(&rt->midi2_clock[index],
                                  rt->midi2_in[index].jr.clock, now_us());
        }
        i += n;
    }
}

/* A session ended, was reset or lost data for good: nothing here knows
 * which notes its peer left sounding, so every channel of its port is told
 * to stop. */
static void midi2_all_notes_off(struct midi2_runtime *rt, int index)
{
    uint8_t message[3] = {0xb0, 123, 0};
    unsigned int channel;
    if (rt->midi2_port[index] < 0)
        return;
    for (channel = 0; channel < 16; ++channel) {
        message[0] = (uint8_t)(0xb0 | channel);
        netmidi_camd_deliver_port(&rt->d->camd, rt->midi2_port[index],
                                  message, sizeof(message));
    }
}

static void midi2_receive(struct netmidid *d, void *self, int number)
{
    static uint8_t in[2048], out[NETMIDI2_MAX_PACKET];
    struct midi2_runtime *rt = self;
    struct sockaddr_in source;
    socklen_t source_length = sizeof(source);
    struct netmidi2_writer packet;
    int index;
    int count = netmidi_network_receive(&d->network, number, in, sizeof(in),
                                        &source, &source_length);
    if (count <= 0 || source_length < sizeof(source) ||
        source.sin_family != AF_INET)
        return;
    netmidi2_writer_init(&packet, out, sizeof(out));
    if (netmidi2_table_receive(&rt->midi2, midi2_key(&source), in,
                               (size_t)count, (uint32_t)(netmidid_now() / 10),
                               &packet, &index) == 0)
        midi2_send(rt, midi2_key(&source), &packet);
}

static void midi2_sessions(struct midi2_runtime *rt, uint64_t now)
{
    static uint8_t out[NETMIDI2_MAX_PACKET];
    struct netmidi2_writer packet;
    int index;

    for (index = 0; index < NETMIDI2_SESSIONS_MAX; ++index) {
        const struct netmidi2_session *session =
            netmidi2_table_session(&rt->midi2, index);
        uint64_t peer;
        if (!session) {
            if (rt->midi2_known[index]) {
                puts(TAG "MIDI 2.0 session closed");
                if (rt->midi2_timed) {
                    printf(TAG "%lu of %lu timed MIDI 2.0 events came after their time\n",
                           rt->midi2_late, rt->midi2_timed);
                    rt->midi2_timed = rt->midi2_late = 0;
                }
                /* what was still held would sound after the notes off */
                rtpmidi_queue_reset(&rt->midi2_queue[index]);
                midi2_all_notes_off(rt, index);
                netmidid_port_release(rt->d, NETMIDID_NETWORKMIDI2,
                                      index + 1);
                rt->midi2_port[index] = -1;
                ump_to_midi1_buffer(&rt->midi2_in[index], NULL, 0);
                free(rt->midi2_sysex[index]);
                rt->midi2_sysex[index] = NULL;
                rt->midi2_known[index] = 0;
            }
            continue;
        }
        peer = session->peer;
        if (session->state == NETMIDI2_ESTABLISHED &&
            !rt->midi2_known[index]) {
            /* a port of the driver, by the same rule as an AppleMIDI
               peer's: kept for its endpoint's name, or the first free.
               Where all share the clusters, port 0. */
            rt->midi2_port[index] = netmidid_port_take(
                rt->d, NETMIDID_NETWORKMIDI2,
                (const uint8_t *)session->peer_name,
                applemidi_text_length(session->peer_name,
                                      sizeof(session->peer_name) - 1),
                index + 1);
            printf(TAG "MIDI 2.0 session established peer=\"%s\" port=%d\n",
                   session->peer_name,
                   rt->midi2_port[index] < 0 ? -1 :
                   rt->d->shared_port ? 0 :
                   rt->midi2_port[index] - NETMIDI_CAMD_MIDI2);
            memset(&rt->midi2_count[index], 0, sizeof(rt->midi2_count[index]));
            {
                size_t n = applemidi_name_fit(
                    (const uint8_t *)session->peer_name,
                    applemidi_text_length(session->peer_name,
                        sizeof(session->peer_name) - 1),
                    sizeof(rt->midi2_count[index].name) - 1);

                memcpy(rt->midi2_count[index].name, session->peer_name, n);
            }
            rt->midi2_known[index] = 1;
            rt->midi2_gaps[index] = session->gaps;
            rt->midi2_resets[index] = session->resets;
            ump_to_midi1_init(&rt->midi2_in[index], 0);
            if (!rt->midi2_sysex[index])
                rt->midi2_sysex[index] = malloc(RTPMIDI_SYSEX_MAX);
            ump_to_midi1_buffer(&rt->midi2_in[index], rt->midi2_sysex[index],
                                RTPMIDI_SYSEX_MAX);
            ump_jr_clock_init(&rt->midi2_clock[index]);
        }
        if (rt->midi2_known[index] &&
            (session->gaps != rt->midi2_gaps[index] ||
             session->resets != rt->midi2_resets[index])) {
            rt->midi2_count[index].packets_lost +=
                session->gaps - rt->midi2_gaps[index];
            puts(TAG "MIDI 2.0 data lost or session reset");
            rt->midi2_gaps[index] = session->gaps;
            rt->midi2_resets[index] = session->resets;
            ump_to_midi1_buffer(&rt->midi2_in[index], rt->midi2_sysex[index],
                                RTPMIDI_SYSEX_MAX);
            midi2_all_notes_off(rt, index);
        }
        netmidi2_writer_init(&packet, out, sizeof(out));
        if (netmidi2_table_tick(&rt->midi2, index, (uint32_t)(now / 10),
                                &packet))
            midi2_send(rt, peer, &packet);
    }
}

static int midi2_periodic(struct netmidid *d, void *self, uint64_t now)
{
    struct midi2_runtime *rt = self;

    (void)d;
    /* The Host we are to be a Client of is known and we have no session
       with it: invite it, and again more slowly while it does not answer. */
    if (rt->midi2_host &&
        netmidi2_table_find(&rt->midi2, rt->midi2_host_key) < 0 &&
        (!rt->midi2_tried ||
         netmidid_elapsed(now, rt->midi2_tried) >= rt->midi2_wait)) {
        static uint8_t out[NETMIDI2_MAX_PACKET];
        struct netmidi2_writer packet;

        netmidi2_writer_init(&packet, out, sizeof(out));
        if (netmidi2_table_invite(&rt->midi2, rt->midi2_host_key,
                                  (uint32_t)(now / 10), &packet) >= 0) {
            puts(TAG "inviting a MIDI 2.0 host");
            midi2_send(rt, rt->midi2_host_key, &packet);
        }
        rt->midi2_tried = now ? now : 1;
        rt->midi2_wait = rt->midi2_wait ? rt->midi2_wait * 2 :
                         APPLEMIDI_WANTED_FIRST_WAIT;
        if (rt->midi2_wait > APPLEMIDI_WANTED_LONGEST_WAIT)
            rt->midi2_wait = APPLEMIDI_WANTED_LONGEST_WAIT;
    }
    midi2_sessions(rt, now);
    midi2_release(rt, netmidid_now());
    return 0;
}

struct midi2_outgoing {
    struct midi2_runtime *rt;
    unsigned long messages;     /* MIDI messages in what is gathered */
    int port;                   /* the port it was sent to; -1 for all */
    uint32_t words[NETMIDI2_MAX_UMP_WORDS];
    size_t count;
};

/* One UMP Data command to the session of the port, or to every
 * established session where all share. */
static void midi2_flush(struct midi2_outgoing *outgoing)
{
    static uint8_t out[NETMIDI2_MAX_PACKET];
    struct midi2_runtime *rt = outgoing->rt;
    struct netmidi2_writer packet;
    int index;

    if (!outgoing->count)
        return;
    for (index = 0; index < NETMIDI2_SESSIONS_MAX; ++index) {
        struct netmidi2_session *session =
            netmidi2_table_session(&rt->midi2, index);
        if (!session || session->state != NETMIDI2_ESTABLISHED ||
            (outgoing->port >= 0 && rt->midi2_port[index] != outgoing->port))
            continue;
        netmidi2_writer_init(&packet, out, sizeof(out));
        if (netmidi2_send_ump(session, outgoing->words, outgoing->count,
                              (uint32_t)(netmidid_now() / 10),
                              &packet) == 0) {
            midi2_send(rt, session->peer, &packet);
            ++rt->midi2_count[index].packets_out;
            rt->midi2_count[index].messages_out += outgoing->messages;
        }
    }
    outgoing->count = 0;
}

/* A UMP is never split between two commands (section 7.1). */
static void midi2_collect(void *context, const uint32_t *words,
                          unsigned count)
{
    struct midi2_outgoing *outgoing = context;
    if (outgoing->count + count > NETMIDI2_MAX_UMP_WORDS)
        midi2_flush(outgoing);
    memcpy(outgoing->words + outgoing->count, words,
           count * sizeof(*words));
    outgoing->count += count;
}

static int midi2_send_midi(struct netmidid *d, void *self, int port,
                           const uint8_t *message, size_t length)
{
    static struct midi2_outgoing outgoing;
    struct ump_from_midi1 from;
    size_t i;

    /* only what was sent to a port of this driver, or to the one all
       share */
    if (!d->shared_port && port < NETMIDI_CAMD_MIDI2)
        return -1;
    outgoing.rt = self;
    outgoing.port = d->shared_port ? -1 : port;
    outgoing.count = 0;
    outgoing.messages = 1;
    ump_from_midi1_init(&from, 0);
    for (i = 0; i < length; ++i)
        ump_from_midi1_byte(&from, message[i], midi2_collect, &outgoing);
    midi2_flush(&outgoing);
    return 0;
}

static int midi2_next_due(struct netmidid *d, void *self, uint64_t *due)
{
    struct midi2_runtime *rt = self;
    uint64_t next;
    int found = 0;
    int index;

    (void)d;
    for (index = 0; index < NETMIDI2_SESSIONS_MAX; ++index)
        if (rtpmidi_queue_next_due(&rt->midi2_queue[index], &next) &&
            (!found || next < *due)) {
            *due = next;
            found = 1;
        }
    return found;
}

static unsigned long midi2_counts(struct netmidid *d, void *self,
                                  struct netmidi_port_counts *counts)
{
    struct midi2_runtime *rt = self;
    int index;

    (void)d;
    for (index = 0; index < NETMIDI2_SESSIONS_MAX; ++index) {
        int port = rt->midi2_port[index];

        if (!rt->midi2_known[index] || port < 0)
            continue;
        counts[port] = rt->midi2_count[index];
        counts[port].present = 1;
    }
    return 0;
}


/* The Product Instance Id tells one device from another and is how a
   system knows a device again (M2-124-UM section 4.4): it has to differ
   between machines and stay the same on one. It is made once, of random
   bits, and kept in a file; the Linux build, which the tests run, has the
   one name always. */
static void product_instance_id(char *id, uint64_t now)
{
    static const char *const path[2] = {"ENVARC:NetMIDI.id", "ENV:NetMIDI.id"};
    int which;
    size_t i;

    for (which = 0; which < 2; ++which) {
        FILE *file = fopen(path[which], "r");
        size_t length = 0;

        if (!file)
            continue;
        length = fread(id, 1, PRODUCT_ID_MAX, file);
        fclose(file);
        while (length && (id[length - 1] == '\n' || id[length - 1] == '\r'))
            --length;
        id[length] = 0;
        /* ASCII 32 to 126, and not empty */
        for (i = 0; i < length; ++i)
            if (id[i] < 32 || id[i] > 126)
                break;
        if (length && i == length)
            return;
    }
    snprintf(id, PRODUCT_ID_MAX + 1, "AROS-%08lx%08lx",
             (unsigned long)netmidid_random(now),
             (unsigned long)netmidid_random(now + 1));
    for (which = 0; which < 2; ++which) {
        FILE *file = fopen(path[which], "w");

        if (file) {
            fputs(id, file);
            fclose(file);
        }
    }
}

static int midi2_open(struct netmidid *d, void *self)
{
    struct midi2_runtime *rt = self;
    const struct netmidi_midi2_config *config = &d->config.midi2;
    int index;

    /* the core gave it zeroed */
    rt->d = d;
    rt->socket = -1;
    for (index = 0; index < NETMIDI2_SESSIONS_MAX; ++index)
        rt->midi2_port[index] = -1;
    /* Network MIDI 2.0 is off unless a port is given for it. */
    rt->port = config->port;
    if (!rt->port)
        return 1;
    rt->socket = netmidid_socket(d, &netmidid_midi2, rt->port);
    if (rt->socket < 0) {
        fputs(TAG "cannot bind the Network MIDI 2.0 port\n", stderr);
        return -1;
    }
    product_instance_id(rt->product_id, netmidid_now());
    netmidi2_table_init(&rt->midi2, d->session_name, rt->product_id,
                        midi2_sink, rt);
    /* a peer may send its timestamps: they are used */
    rt->midi2.jr_offered = UMP_JR_RECEIVE;
    for (index = 0; index < NETMIDI2_SESSIONS_MAX; ++index)
        rtpmidi_queue_reset(&rt->midi2_queue[index]);
    for (index = 0; index < 16; ++index)
        if (config->port_name[index][0] &&
            netmidid_port_keep(d, NETMIDID_NETWORKMIDI2, index,
                               config->port_name[index]) != 0)
            printf(TAG "networkmidi2 port %d cannot be kept for \"%s\"\n",
                   index, config->port_name[index]);
    if (config->peer_ip[0]) {
        uint32_t address;

        if (netmidid_ipv4(config->peer_ip, &address) == 0) {
            rt->midi2_host_key = ((uint64_t)address << 16) |
                                 config->peer_port;
            rt->midi2_host = 1;
        }
    }
    return 0;
}

static void midi2_close(struct netmidid *d, void *self)
{
    struct midi2_runtime *rt = self;
    int index;

    (void)d;
    for (index = 0; index < NETMIDI2_SESSIONS_MAX; ++index) {
        static uint8_t out[NETMIDI2_MAX_PACKET];
        struct netmidi2_writer packet;
        struct netmidi2_session *session =
            netmidi2_table_session(&rt->midi2, index);
        netmidi2_writer_init(&packet, out, sizeof(out));
        if (session && netmidi2_bye(session, NETMIDI2_BYE_USER_TERMINATED,
                                    (uint32_t)(netmidid_now() / 10),
                                    &packet) == 0)
            midi2_send(rt, session->peer, &packet);
    }
}

static void midi2_free(struct netmidid *d, void *self)
{
    struct midi2_runtime *rt = self;
    int index;

    (void)d;
    for (index = 0; index < NETMIDI2_SESSIONS_MAX; ++index) {
        free(rt->midi2_sysex[index]);
        rt->midi2_sysex[index] = NULL;
    }
}

const struct netmidid_transport netmidid_midi2 = {
    sizeof(struct midi2_runtime),
    midi2_open,
    NULL,
    midi2_receive,
    midi2_next_due,
    midi2_periodic,
    midi2_send_midi,
    NULL,
    midi2_counts,
    midi2_close,
    midi2_free
};
