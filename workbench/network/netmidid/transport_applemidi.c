/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* AppleMIDI (RTP-MIDI) as a transport of netmidid: the sessions of up to
 * APPLEMIDI_PEERS_MAX peers on a control and a data port, their clocks,
 * recovery journals and feedback, each peer on a port of the driver
 * applemidi. The protocol is the link library rtpmidi's. */


#include "netmidid.h"

#include <rtpmidi/applemidi.h>
#include <rtpmidi/clock.h>
#include <rtpmidi/peers.h>
#include <rtpmidi/ports.h>
#include <rtpmidi/receiver.h>
#include <rtpmidi/rtpmidi.h>
#include <rtpmidi/session.h>
#include <rtpmidi/sender.h>
#include <rtpmidi/timing.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto/bsdsocket.h>

/* what the core hands over fits what the protocol sends */
typedef char sysex_fits[NETMIDID_SYSEX_MAX <= RTPMIDI_SYSEX_MAX ? 1 : -1];

struct applemidi_runtime;

/* What is kept for one peer's stream, beside its session in the peer
 * table at the same index. */
struct stream {
    int initiating;             /* we sent the invitation */
    struct applemidi_clock clock;
    int have_received_sequence;
    uint16_t sequence;          /* of the next packet to this peer */
    uint32_t received_sequence;
    uint64_t last_invite;
    uint64_t last_rtp_send;
    uint64_t last_activity;
    unsigned int retries;
    struct rtpmidi_sysex_assembler sysex;
    struct rtpmidi_event_queue queue;
    struct rtpmidi_sender sender;
    struct rtpmidi_receiver receiver;
    unsigned long timed;        /* events that came with a time to keep */
    unsigned long late;         /* of those, the ones already past it */
    struct applemidi_runtime *rt;
    int port;                   /* the port its peer was given, or -1 */
    unsigned long sysex_dropped;    /* SysEx messages too long, or broken */
    uint64_t feedback_sent;     /* when the peer was last told how far we are */
    int feedback_owed;          /* packets came since */
    /* what has passed since the session began, for whoever asks */
    struct netmidi_port_counts count;
};

struct applemidi_runtime {
    struct netmidid *d;
    int socket[2];              /* the control port's, the data port's */
    uint16_t local_port;        /* the control port */
    int configured;             /* a peer was named to invite */
    uint32_t configured_address;
    uint16_t configured_port;
    /* when it was last invited, and how long until it is again */
    uint64_t again_tried;
    uint64_t again_wait;
    struct applemidi_peers peers;
    struct stream stream[APPLEMIDI_PEERS_MAX];
    unsigned long unknown_commands; /* AppleMIDI commands not used */
    /* What CAMD handed over in one turn of the loop and has not gone out
       yet: it leaves together, one packet to a peer where it fits. */
    struct rtpmidi_timed_message outgoing[64];
    uint8_t outgoing_port[64];  /* the port each was sent to */
    size_t outgoing_count;
};

static void peer_address(const struct applemidi_peer *peer, int data_port,
                         struct sockaddr_in *address)
{
    memset(address, 0, sizeof(*address));
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = htonl(peer->address);
    address->sin_port = htons((uint16_t)(peer->control_port +
                                         (data_port ? 1 : 0)));
}

static int send_to(struct applemidi_runtime *rt, int data_port,
                   const void *bytes, size_t length,
                   const struct sockaddr_in *address)
{
    return netmidi_network_send(&rt->d->network, rt->socket[data_port ? 1 : 0],
                                bytes, length, address);
}

static int send_apple_to(struct applemidi_runtime *rt, int data_port,
                         const struct sockaddr_in *address,
                         const struct applemidi_packet *packet)
{
    uint8_t bytes[128];
    size_t length;
    if (applemidi_encode(packet, bytes, sizeof(bytes), &length) != 0)
        return -1;
    return send_to(rt, data_port, bytes, length, address);
}

static int send_apple(struct applemidi_runtime *rt, int index, int data_port,
                      const struct applemidi_packet *packet)
{
    struct sockaddr_in address;
    peer_address(&rt->peers.peer[index], data_port, &address);
    return send_apple_to(rt, data_port, &address, packet);
}

static void send_sync(struct applemidi_runtime *rt, int index, uint64_t now)
{
    struct applemidi_packet sync;
    if (applemidi_clock_poll(&rt->stream[index].clock, rt->peers.local_ssrc,
                             now, &sync))
        send_apple(rt, index, 1, &sync);
}

static void drop_peer(struct applemidi_runtime *rt, int index, uint64_t now);

static void output_message(void *context, const uint8_t *bytes,
                           size_t length)
{
    struct stream *stream = context;
    struct netmidid *d = stream->rt->d;
    ++stream->count.messages_in;
    if (stream->port >= 0)
        netmidi_camd_deliver_port(&d->camd, stream->port, bytes, length);
}

static void output_sysex(void *context, const uint8_t *message, size_t length)
{
    struct stream *stream = context;
    ++stream->count.messages_in;
    if (stream->port >= 0)
        netmidi_camd_deliver_port(&stream->rt->d->camd, stream->port, message,
                                  length);
}

static void output_report(void *context, const char *text)
{
    (void)context;
    fputs(TAG "", stdout);
    fputs(text, stdout);
}

static const struct rtpmidi_receiver_output receiver_output = {
    output_message, output_sysex, output_report
};

/* A slot of the peer table has a new occupant: nothing of the last one's
 * stream may be left in it. */
static void stream_begin(struct applemidi_runtime *rt, int index,
                         int initiating, uint64_t now)
{
    struct stream *stream = &rt->stream[index];
    /* where its SysEx messages are gathered: lent from the first peer the
       slot has, and kept for the next */
    uint8_t *sysex_buffer = stream->sysex.bytes;

    rtpmidi_queue_reset(&stream->queue);
    memset(stream, 0, sizeof(*stream));
    if (!sysex_buffer)
        sysex_buffer = malloc(RTPMIDI_SYSEX_MAX);
    rtpmidi_sysex_buffer(&stream->sysex, sysex_buffer, RTPMIDI_SYSEX_MAX);
    stream->rt = rt;
    stream->port = -1;
    stream->initiating = initiating;
    stream->last_invite = now;
    stream->last_activity = now;
    stream->sequence = (uint16_t)netmidid_random(now);
    applemidi_clock_init(&stream->clock, initiating, now);
    rtpmidi_sysex_reset(&stream->sysex);
    rtpmidi_queue_reset(&stream->queue);
    rtpmidi_sender_reset(&stream->sender);
    rtpmidi_receiver_init(&stream->receiver, &receiver_output, stream,
                          &stream->queue);
}

/* The session is over, or began again: what the peer left sounding is
 * released and what was kept for its stream goes. */
static void stream_forget(struct applemidi_runtime *rt, int index,
                          uint64_t now)
{
    struct stream *stream = &rt->stream[index];
    if (stream->timed) {
        printf(TAG "%lu of %lu timed events came after their time\n",
               stream->late, stream->timed);
        stream->timed = stream->late = 0;
    }
    if (stream->sysex_dropped) {
        printf(TAG "%lu SysEx messages were dropped, too long or broken\n",
               stream->sysex_dropped);
        stream->sysex_dropped = 0;
    }
    applemidi_clock_init(&stream->clock, stream->initiating, now);
    stream->have_received_sequence = 0;
    rtpmidi_sysex_reset(&stream->sysex);
    rtpmidi_queue_reset(&stream->queue);
    if (stream->receiver.output) {
        rtpmidi_receiver_release_notes(&stream->receiver);
        rtpmidi_receiver_reset_channels(&stream->receiver);
        rtpmidi_receiver_reset_system(&stream->receiver);
    }
    rtpmidi_sender_reset(&stream->sender);
    /* after the notes were released on it */
    netmidid_port_release(rt->d, NETMIDID_APPLEMIDI, index + 1);
    stream->port = -1;
}

static int invite_configured(struct applemidi_runtime *rt, uint64_t now)
{
    struct applemidi_packet invite;
    int index = applemidi_peers_invite(&rt->peers, rt->configured_address,
                                       rt->configured_port,
                                       netmidid_random(now),
                                       (uint32_t)now, &invite);
    if (index < 0)
        return -1;
    stream_begin(rt, index, 1, now);
    return send_apple(rt, index, 0, &invite);
}

static int configured_index(const struct applemidi_runtime *rt)
{
    return rt->configured ?
           applemidi_peers_find_control(&rt->peers, rt->configured_address,
                                        rt->configured_port) : -1;
}

/* Tells the peer how far we have received: what it may drop from its
   journal's history. */
static void feedback_now(struct applemidi_runtime *rt, int index,
                         uint64_t now)
{
    struct stream *stream = &rt->stream[index];
    struct applemidi_packet feedback;

    memset(&feedback, 0, sizeof(feedback));
    feedback.command = APPLEMIDI_RS;
    feedback.ssrc = rt->peers.local_ssrc;
    feedback.feedback_sequence =
        applemidi_feedback_field((uint16_t)stream->received_sequence);
    send_apple(rt, index, 0, &feedback);
    stream->feedback_sent = now ? now : 1;
    stream->feedback_owed = 0;
}

/* A packet came. The peer is told at once after a gap, since its journal
   was just used, and when one that came before comes again, since it may
   be asking; otherwise once in RTPMIDI_FEEDBACK_INTERVAL while packets
   arrive, not for each of them. Returns 0 for a packet already had. */
static int send_feedback(struct applemidi_runtime *rt, int index,
                         uint16_t sequence)
{
    struct stream *stream = &rt->stream[index];
    uint64_t now = netmidid_now();
    int16_t advance = 1;
    int fresh = 1;

    if (!stream->have_received_sequence) {
        stream->received_sequence = sequence;
        stream->have_received_sequence = 1;
    } else {
        advance = (int16_t)(sequence - (uint16_t)stream->received_sequence);
        if (advance <= 0)
            fresh = 0;
        else
            stream->received_sequence += (uint16_t)advance;
    }
    if (!fresh || advance > 1 || !stream->feedback_sent ||
        netmidid_elapsed(now, stream->feedback_sent) >=
            RTPMIDI_FEEDBACK_INTERVAL)
        feedback_now(rt, index, now);
    else
        stream->feedback_owed = 1;
    return fresh;
}

/* An event's time in our clock is known: adds the latency and says
 * whether that moment is still ahead. One that is not is played at once,
 * and counted, since it means the latency is too short for this path. */
static int stream_due(const struct applemidi_runtime *rt,
                      struct stream *stream, uint64_t now, uint64_t *due)
{
    *due += rt->d->latency;
    ++stream->timed;
    ++stream->count.timed;
    if (*due > now)
        return 1;
    ++stream->late;
    ++stream->count.late;
    return 0;
}

static void receive_rtp(struct applemidi_runtime *rt, int index,
                        const uint8_t *bytes, size_t count)
{
    struct stream *stream = &rt->stream[index];
    struct rtpmidi_packet midi;
    struct rtpmidi_journal journal;
    struct rtpmidi_journal_system_state system_state;
    struct rtpmidi_journal_sysex sysex_state;
    struct rtpmidi_reader reader;
    struct rtpmidi_event event;
    const uint8_t *sysex_message;
    size_t sysex_length;
    int action;
    int system_decoded = 0;
    int16_t sequence_advance = 0;
    uint16_t previous_sequence = 0;
    uint64_t now;

    if (rtpmidi_decode(bytes, count, &midi) != 0 ||
        midi.ssrc != rt->peers.peer[index].session.peer_ssrc)
        return;
    if (midi.journal &&
        rtpmidi_journal_decode(midi.journal, midi.journal_length,
                          &journal) != 0) {
        puts(TAG "malformed recovery journal discarded");
        return;
    }
    if (midi.journal && journal.system) {
        system_decoded = rtpmidi_journal_decode_system(&journal, &system_state);
        if (system_decoded < 0) {
            puts(TAG "malformed system recovery journal discarded");
            return;
        }
        if (system_state.sysex &&
            rtpmidi_journal_decode_sysex(&system_state, &sysex_state) < 0) {
            puts(TAG "malformed SysEx recovery journal discarded");
            return;
        }
    }
    rtpmidi_reader_init(&reader, &midi);
    while ((action = rtpmidi_reader_next(&reader, &event)) == 1) {}
    if (action < 0) {
        puts(TAG "unsupported or malformed MIDI command");
        return;
    }
    if (stream->have_received_sequence) {
        previous_sequence = (uint16_t)stream->received_sequence;
        sequence_advance = (int16_t)(midi.sequence -
                                     previous_sequence);
        if (sequence_advance > 1) {
            stream->count.packets_lost += (unsigned long)(sequence_advance - 1);
            printf(TAG "%u RTP packet(s) lost; journal %s\n",
                   (unsigned int)(sequence_advance - 1),
                   !midi.journal ? "unavailable" :
                   rtpmidi_journal_covers_gap(
                       previous_sequence, midi.sequence, journal.checkpoint) ?
                                             "covers gap" :
                                             "does not cover gap");
        }
    }
    if (!send_feedback(rt, index, midi.sequence))
        return; /* A duplicate or older packet must not replay MIDI events. */
    rtpmidi_sysex_packet(&stream->sysex, midi.sequence);
    if (sequence_advance > 1 && midi.journal &&
        rtpmidi_journal_covers_gap(previous_sequence, midi.sequence,
                              journal.checkpoint)) {
        struct rtpmidi_receiver_clock clock;
        clock.synchronized = stream->clock.synchronized;
        clock.peer_to_local = stream->clock.peer_to_local;
        clock.now = netmidid_now();
        rtpmidi_receiver_recover(&stream->receiver, &journal,
                            system_decoded > 0 ? &system_state : NULL,
                            sequence_advance == 2, midi.timestamp, &clock);
    }
    else if (sequence_advance > 1) {
        /* Packets are gone and no journal says what was in them: among
           them may be the end of a note that is sounding, and it would
           sound for ever. The notes this peer has on are ended. Its
           controllers are left as they are: nothing is known to be wrong
           with them, and peers that send no journal lose a packet now and
           then. */
        puts(TAG "ending the notes this peer left on");
        rtpmidi_receiver_release_notes(&stream->receiver);
    }
    if (!midi.midi_length)
        return; /* Guard packet: acknowledge its journal without MIDI output. */
    ++stream->count.packets_in;
    rtpmidi_reader_init(&reader, &midi);
    while ((action = rtpmidi_reader_next(&reader, &event)) == 1) {
        if (event.sysex) {
            int sysex_result = rtpmidi_sysex_feed(&stream->sysex, &event,
                                          midi.sequence,
                                          &sysex_message, &sysex_length);
            if (sysex_result == 1) {
                uint64_t due;
                now = netmidid_now();
                if (stream->clock.synchronized &&
                    rtpmidi_clock_due(stream->sysex.timestamp, now,
                                 stream->clock.peer_to_local, &due) == 0 &&
                    stream_due(rt, stream, now, &due)) {
                    if (due - now > RTPMIDI_QUEUE_HORIZON ||
                        rtpmidi_queue_push_sysex(&stream->queue, due,
                                            sysex_message,
                                            sysex_length) != 0)
                        puts(TAG "future SysEx event discarded");
                    continue;
                }
                rtpmidi_receiver_deliver_sysex(&stream->receiver,
                                               sysex_message, sysex_length);
            } else if (sysex_result < 0) {
                ++stream->sysex_dropped;
                ++stream->count.sysex_dropped;
                puts(TAG "SysEx segment discarded");
            }
        } else {
            uint64_t due;
            now = netmidid_now();
            if (stream->clock.synchronized &&
                rtpmidi_clock_due(event.timestamp, now,
                             stream->clock.peer_to_local, &due) == 0 &&
                stream_due(rt, stream, now, &due)) {
                if (due - now > RTPMIDI_QUEUE_HORIZON ||
                    rtpmidi_queue_push(&stream->queue, due, event.bytes,
                                  event.length) != 0)
                    puts(TAG "future MIDI event discarded");
                continue;
            }
            rtpmidi_receiver_deliver(&stream->receiver, event.bytes,
                                     event.length);
        }
    }
    stream->count.undefined += reader.undefined;
    if (action < 0)
        puts(TAG "unsupported or malformed MIDI command");
}

static void receive_packet(struct applemidi_runtime *rt, int data_port)
{
    struct sockaddr_in source;
    socklen_t source_length = sizeof(source);
    uint8_t bytes[1500];
    int count = netmidi_network_receive(&rt->d->network,
                                        rt->socket[data_port ? 1 : 0], bytes,
                                        sizeof(bytes), &source,
                                        &source_length);
    struct applemidi_packet packet;
    struct applemidi_packet response;
    enum applemidi_peer_event event;
    uint32_t address;
    uint16_t port;
    int decoded;
    unsigned int collisions;
    int response_port;
    int index;
    int action;
    uint64_t now;

    if (count <= 0 || source_length < sizeof(source) ||
        source.sin_family != AF_INET)
        return;
    address = ntohl(source.sin_addr.s_addr);
    port = ntohs(source.sin_port);
    if (port == 65535 || (data_port && port == 0))
        return;
    now = netmidid_now();
    decoded = applemidi_decode(bytes, (size_t)count, &packet);
    if (decoded == APPLEMIDI_UNKNOWN_COMMAND) {
        /* AppleMIDI, and a command we have no use for (the bit-rate
           limit, RL, is one): not RTP, and not an error */
        ++rt->unknown_commands;
        return;
    }
    if (decoded != 0) {
        index = data_port ?
                applemidi_peers_find_data(&rt->peers, address, port) : -1;
        if (index >= 0) {
            rt->stream[index].last_activity = now;
            applemidi_peers_seen(&rt->peers, index, (uint32_t)now);
            receive_rtp(rt, index, bytes, (size_t)count);
        }
        return;
    }
    if (packet.command == APPLEMIDI_CK) {
        index = data_port ?
                applemidi_peers_find_data(&rt->peers, address, port) : -1;
        if (index < 0 ||
            packet.ssrc != rt->peers.peer[index].session.peer_ssrc)
            return;
        rt->stream[index].last_activity = now;
        applemidi_peers_seen(&rt->peers, index, (uint32_t)now);
        {
            unsigned int completed = rt->stream[index].clock.completed;

            if (applemidi_clock_receive(&rt->stream[index].clock,
                                        rt->peers.local_ssrc, &packet, now,
                                        &response))
                send_apple(rt, index, 1, &response);
            if (rt->stream[index].clock.completed != completed)
                puts(TAG "clock exchange completed");
        }
        return;
    }
    if (packet.command == APPLEMIDI_RS) {
        index = data_port ? -1 :
                applemidi_peers_find_control(&rt->peers, address, port);
        if (index >= 0 &&
            rt->peers.peer[index].session.phase ==
                APPLEMIDI_SESSION_CONNECTED &&
            packet.ssrc == rt->peers.peer[index].session.peer_ssrc)
            rtpmidi_sender_ack(&rt->stream[index].sender,
                applemidi_feedback_acked(packet.feedback_sequence,
                    (uint16_t)(rt->stream[index].sequence - 1)));
        return;
    }

    /* IN, OK, NO, BY: the peer table's business. Whether the packet made a
     * new peer is seen from the slot it comes back with. */
    index = applemidi_peers_find_control(&rt->peers, address,
                (uint16_t)(data_port ? port - 1 : port));
    collisions = rt->peers.ssrc_collisions;
    {
        int known = index;
        action = applemidi_peers_receive(&rt->peers, address, port, data_port,
                                         &packet, (uint32_t)now,
                                         &response, &response_port, &index,
                                         &event);
        if (action < 0)
            return;
        if (known < 0 && index >= 0)
            stream_begin(rt, index, 0, now);
    }
    if (action == 1) {
        /* a refusal goes to someone who has no place in the table */
        source.sin_port = htons((uint16_t)(data_port == response_port ?
            port : response_port ? port + 1 : port - 1));
        send_apple_to(rt, response_port, &source, &response);
    }
    if (rt->peers.ssrc_collisions != collisions) {
        puts(TAG "peer has our SSRC");
        if (applemidi_peers_renew(&rt->peers, netmidid_random(now)) == 0) {
            puts(TAG "taking another SSRC");
            if (rt->configured)
                invite_configured(rt, now);
        }
        return;
    }
    if (index < 0)
        return;
    rt->stream[index].last_activity = now;
    if (packet.command == APPLEMIDI_OK && action == 1 &&
        response.command == APPLEMIDI_IN) {
        rt->stream[index].last_invite = now;
        rt->stream[index].retries = 0;
    }
    if (event == APPLEMIDI_PEER_RESTARTED) {
        puts(TAG "peer began again");
        stream_forget(rt, index, now);
    } else if (event == APPLEMIDI_PEER_CONNECTED) {
        /* The peer gets a port: the one kept for its name, or the first
           that is free and kept for no one. Where every peer shares the
           one pair of clusters there is nothing to give out. */
        rt->stream[index].port =
            netmidid_port_take(rt->d, NETMIDID_APPLEMIDI, packet.name,
                               packet.name_length, index + 1);
        if (rt->stream[index].port < 0) {
            puts(TAG "no port for this peer");
            drop_peer(rt, index, now);
            return;
        }
        printf(TAG "session connected port=%d\n",
               rt->stream[index].port);
        {
            size_t n = applemidi_name_fit(
                packet.name, packet.name_length,
                sizeof(rt->stream[index].count.name) - 1);

            memcpy(rt->stream[index].count.name, packet.name, n);
            rt->stream[index].count.name[n] = 0;
        }
        applemidi_clock_init(&rt->stream[index].clock,
                             rt->stream[index].initiating, now);
        send_sync(rt, index, now);
    } else if (event == APPLEMIDI_PEER_LEFT) {
        puts(packet.command == APPLEMIDI_NO ?
             TAG "invitation refused" : TAG "peer disconnected");
        stream_forget(rt, index, now);
    }
}

/* One message from CAMD to one peer. */
static int send_midi_to(struct applemidi_runtime *rt, int index,
                        const uint8_t *message, size_t length)
{
    struct stream *stream = &rt->stream[index];
    struct sockaddr_in data;
    uint8_t wire[1100];
    uint8_t segment[1002];
    size_t wire_length;
    size_t data_length;
    size_t offset;
    uint32_t timestamp = (uint32_t)netmidid_now();
    uint16_t first_sequence;

    peer_address(&rt->peers.peer[index], 1, &data);
    if (message[0] != 0xf0) {
        if (rtpmidi_sender_packet(&stream->sender, stream->sequence, timestamp,
                                  rt->peers.local_ssrc, message, length,
                                  wire, sizeof(wire), &wire_length) != 0 ||
            send_to(rt, 1, wire, wire_length, &data) != 0)
            return -1;
        ++stream->sequence;
        ++stream->count.packets_out;
        stream->last_rtp_send = netmidid_now();
        return 0;
    }
    first_sequence = stream->sequence;
    if (length <= 1002) {
        if (rtpmidi_encode_list(stream->sequence, timestamp,
                               rt->peers.local_ssrc, message, length,
                               wire, sizeof(wire), &wire_length) != 0 ||
            send_to(rt, 1, wire, wire_length, &data) != 0)
            return -1;
        ++stream->sequence;
        ++stream->count.packets_out;
        stream->last_rtp_send = netmidid_now();
        rtpmidi_sender_record_sysex(&stream->sender, first_sequence, timestamp,
                                    message, length);
        return 0;
    }
    data_length = length - 2;
    offset = 0;
    while (offset < data_length) {
        size_t chunk = data_length - offset;

        if (chunk > 1000)
            chunk = 1000;
        /* F0 begins the message and F7 ends it; between segments it is
           the other way round (RFC 6295 section 3.2) */
        segment[0] = offset == 0 ? 0xf0 : 0xf7;
        memcpy(segment + 1, message + 1 + offset, chunk);
        segment[chunk + 1] = offset + chunk == data_length ? 0xf7 : 0xf0;
        if (rtpmidi_encode_list(stream->sequence, timestamp,
                               rt->peers.local_ssrc, segment,
                               chunk + 2, wire, sizeof(wire),
                               &wire_length) != 0 ||
            send_to(rt, 1, wire, wire_length, &data) != 0)
            return -1;
        ++stream->sequence;
        ++stream->count.packets_out;
        stream->last_rtp_send = netmidid_now();
        offset += chunk;
    }
    rtpmidi_sender_record_sysex(&stream->sender, first_sequence, timestamp,
                                message, length);
    return 0;
}

/* Sends what was gathered to every connected peer: to each as few
 * packets as hold it, each message with the time it was taken at. */
static void applemidi_flush(struct netmidid *d, void *self)
{
    struct applemidi_runtime *rt = self;
    int index;

    (void)d;
    for (index = 0; index < APPLEMIDI_PEERS_MAX; ++index) {
        struct stream *stream = &rt->stream[index];
        struct sockaddr_in data;
        uint8_t wire[1100];
        size_t wire_length;
        size_t taken;
        size_t done = 0;
        size_t count = 0;
        size_t i;
        struct rtpmidi_timed_message mine[64];

        if (!rt->peers.peer[index].used ||
            rt->peers.peer[index].session.phase != APPLEMIDI_SESSION_CONNECTED)
            continue;
        /* what was sent to this peer's port */
        for (i = 0; i < rt->outgoing_count; ++i)
            if (rt->d->shared_port || rt->outgoing_port[i] == stream->port)
                mine[count++] = rt->outgoing[i];
        peer_address(&rt->peers.peer[index], 1, &data);
        while (done < count) {
            if (rtpmidi_sender_packet_list(&stream->sender, stream->sequence,
                                           rt->peers.local_ssrc,
                                           mine + done, count - done,
                                           wire, sizeof(wire), &wire_length,
                                           &taken) != 0) {
                ++done;         /* not a message we can send */
                continue;
            }
            done += taken;
            stream->count.messages_out += taken;
            if (send_to(rt, 1, wire, wire_length, &data) != 0)
                break;
            ++stream->sequence;
            ++stream->count.packets_out;
            stream->last_rtp_send = netmidid_now();
        }
    }
    rt->outgoing_count = 0;
}

/* What a CAMD program sends goes to every connected peer of the port,
 * each with its own sequence numbers and its own journal. */
static int applemidi_send(struct netmidid *d, void *self, int port,
                          const uint8_t *message, size_t length)
{
    struct applemidi_runtime *rt = self;
    int index;
    int result = -1;

    if (message[0] != 0xf0 && length <= 3) {
        /* gathered, and sent when CAMD has nothing more this turn */
        struct rtpmidi_timed_message *m;

        if (rt->outgoing_count ==
            sizeof(rt->outgoing) / sizeof(rt->outgoing[0]))
            applemidi_flush(d, self);
        rt->outgoing_port[rt->outgoing_count] = (uint8_t)port;
        m = &rt->outgoing[rt->outgoing_count++];
        m->timestamp = (uint32_t)netmidid_now();
        m->length = (uint8_t)length;
        memcpy(m->bytes, message, length);
        for (index = 0; index < APPLEMIDI_PEERS_MAX; ++index)
            if (rt->peers.peer[index].used &&
                rt->peers.peer[index].session.phase ==
                    APPLEMIDI_SESSION_CONNECTED &&
                (d->shared_port || rt->stream[index].port == port))
                result = 0;
    } else {
        /* a SysEx message has packets of its own, after what came first */
        applemidi_flush(d, self);
        for (index = 0; index < APPLEMIDI_PEERS_MAX; ++index)
            if (rt->peers.peer[index].used &&
                rt->peers.peer[index].session.phase ==
                    APPLEMIDI_SESSION_CONNECTED &&
                (d->shared_port || rt->stream[index].port == port) &&
                send_midi_to(rt, index, message, length) == 0) {
                ++rt->stream[index].count.messages_out;
                result = 0;
            }
    }
    return result;
}

static void send_guard(struct applemidi_runtime *rt, int index, uint64_t now)
{
    struct stream *stream = &rt->stream[index];
    struct sockaddr_in data;
    uint8_t wire[1100];
    size_t wire_length;
    uint32_t timestamp = (uint32_t)now;
    if (rtpmidi_sender_guard(&stream->sender, stream->sequence, timestamp,
                             rt->peers.local_ssrc, wire, sizeof(wire),
                             &wire_length) != 0)
        return;
    peer_address(&rt->peers.peer[index], 1, &data);
    if (send_to(rt, 1, wire, wire_length, &data) == 0) {
        ++stream->sequence;
        /* a guard packet carries no message and is not counted as one that does */
        stream->last_rtp_send = now;
    }
}


/* Ends a peer from our side, with BY when a session had been reached. */
static void drop_peer(struct applemidi_runtime *rt, int index, uint64_t now)
{
    struct applemidi_packet goodbye;
    struct sockaddr_in control;
    peer_address(&rt->peers.peer[index], 0, &control);
    stream_forget(rt, index, now);
    if (applemidi_peers_end(&rt->peers, index, &goodbye) == 0)
        send_apple_to(rt, 0, &control, &goodbye);
}

static void periodic_peer(struct applemidi_runtime *rt, int index,
                          uint64_t now)
{
    struct stream *stream = &rt->stream[index];
    const struct applemidi_session *session =
        &rt->peers.peer[index].session;
    struct applemidi_packet packet;
    struct rtpmidi_queued_event event;
    int data_port;

    while (rtpmidi_queue_pop_due(&stream->queue, now, &event)) {
        if (event.sysex)
            rtpmidi_receiver_deliver_sysex(&stream->receiver, event.sysex,
                                           event.sysex_length);
        else
            rtpmidi_receiver_deliver(&stream->receiver, event.bytes,
                                     event.length);
        rtpmidi_queue_event_release(&event);
    }

    if (session->phase == APPLEMIDI_SESSION_INVITING_CONTROL ||
        session->phase == APPLEMIDI_SESSION_INVITING_DATA) {
        if (netmidid_elapsed(now, stream->last_invite) >=
                APPLEMIDI_INVITE_INTERVAL) {
            if (++stream->retries > APPLEMIDI_INVITE_TRIES) {
                puts(TAG "invitation timed out");
                drop_peer(rt, index, now);
            } else if (applemidi_peers_retry(&rt->peers, index, &packet,
                                             &data_port) == 0) {
                send_apple(rt, index, data_port, &packet);
                stream->last_invite = now;
            }
        }
    } else if (session->phase == APPLEMIDI_SESSION_CONNECTED) {
        if (applemidi_clock_expired(&stream->clock, now)) {
            puts(TAG "peer synchronization timed out");
            drop_peer(rt, index, now);
            return;
        }
        send_sync(rt, index, now);
        if (stream->feedback_owed &&
            netmidid_elapsed(now, stream->feedback_sent) >=
                RTPMIDI_FEEDBACK_INTERVAL)
            feedback_now(rt, index, now);
        if (stream->sender.count &&
            netmidid_elapsed(now, stream->last_rtp_send) >=
                RTPMIDI_GUARD_INTERVAL)
            send_guard(rt, index, now);
    } else if (session->phase == APPLEMIDI_SESSION_WAITING_DATA &&
               netmidid_elapsed(now, stream->last_activity) >=
                   APPLEMIDI_DATA_INVITE_TIMEOUT) {
        puts(TAG "data invitation timed out");
        drop_peer(rt, index, now);
    }
}


static int applemidi_periodic(struct netmidid *d, void *self, uint64_t now)
{
    struct applemidi_runtime *rt = self;
    int index;

    (void)d;
    for (index = 0; index < APPLEMIDI_PEERS_MAX; ++index)
        if (rt->peers.peer[index].used)
            periodic_peer(rt, index, now);
    /* A peer not heard from for as long as a clock exchange may take has
       gone without BY, whichever side invited: it is ended, and a
       configured one is then invited again. */
    index = applemidi_peers_expired(&rt->peers, (uint32_t)now,
                                    APPLEMIDI_CLOCK_TIMEOUT);
    if (index >= 0) {
        puts(TAG "peer timed out");
        drop_peer(rt, index, now);
    }
    /* The peer given by its address is invited again when there is no
       session with it, be it that it never answered or that it left:
       more slowly each time, up to once a minute, and from the start
       again after a session. */
    if (rt->configured) {
        now = netmidid_now();
        index = configured_index(rt);
        if (index >= 0) {
            if (rt->peers.peer[index].session.phase ==
                    APPLEMIDI_SESSION_CONNECTED) {
                rt->again_tried = now;
                rt->again_wait = APPLEMIDI_WANTED_FIRST_WAIT;
            }
        } else if (netmidid_elapsed(now, rt->again_tried) >=
                   rt->again_wait) {
            invite_configured(rt, now);
            rt->again_tried = now;
            rt->again_wait *= 2;
            if (rt->again_wait > APPLEMIDI_WANTED_LONGEST_WAIT)
                rt->again_wait = APPLEMIDI_WANTED_LONGEST_WAIT;
        }
    }
    return 0;
}

static void applemidi_receive(struct netmidid *d, void *self, int number)
{
    struct applemidi_runtime *rt = self;

    (void)d;
    receive_packet(rt, number == rt->socket[1]);
}

static int applemidi_next_due(struct netmidid *d, void *self, uint64_t *due)
{
    struct applemidi_runtime *rt = self;
    uint64_t next;
    int found = 0;
    int index;

    (void)d;
    for (index = 0; index < APPLEMIDI_PEERS_MAX; ++index)
        if (rt->peers.peer[index].used &&
            rtpmidi_queue_next_due(&rt->stream[index].queue, &next) &&
            (!found || next < *due)) {
            *due = next;
            found = 1;
        }
    return found;
}

static unsigned long applemidi_counts(struct netmidid *d, void *self,
                                      struct netmidi_port_counts *counts)
{
    struct applemidi_runtime *rt = self;
    int index;

    for (index = 0; index < APPLEMIDI_PEERS_MAX; ++index) {
        struct stream *stream = &rt->stream[index];

        if (!rt->peers.peer[index].used || stream->port < 0 ||
            stream->port >= NETMIDI_CAMD_PORTS)
            continue;
        stream->count.journals_omitted = stream->sender.journals_omitted;
        counts[stream->port] = stream->count;
        counts[stream->port].sysex_dropped +=
            netmidi_camd_sysex_dropped(&d->camd, stream->port);
        counts[stream->port].present = 1;
    }
    return rt->peers.refused;
}

/* Who may connect: anyone (the default), nobody, or those listed, each an
 * IPv4 address or a session name, with commas between. */
static int accept_peers(struct applemidi_runtime *rt, const char *rule)
{
    static char list[512];
    char *entry;

    if (!rule || strcmp(rule, "anyone") == 0)
        return 0;
    rt->peers.accept = APPLEMIDI_ACCEPT_NOBODY;
    if (strcmp(rule, "nobody") == 0)
        return 0;
    rt->peers.accept = APPLEMIDI_ACCEPT_LISTED;
    snprintf(list, sizeof(list), "%s", rule);
    for (entry = strtok(list, ","); entry; entry = strtok(NULL, ",")) {
        uint32_t address;
        int allowed = netmidid_ipv4(entry, &address) == 0 ?
            applemidi_peers_allow(&rt->peers, address, NULL) :
            applemidi_peers_allow(&rt->peers, 0, entry);

        if (allowed != 0) {
            fputs(TAG "too many or bad entries in accept\n", stderr);
            return -1;
        }
    }
    return 0;
}

static int applemidi_open(struct netmidid *d, void *self)
{
    struct applemidi_runtime *rt = self;
    const struct netmidi_applemidi_config *config = &d->config.applemidi;
    uint64_t now;
    int index;

    /* the core gave it zeroed */
    rt->d = d;
    rt->socket[0] = rt->socket[1] = -1;
    rt->local_port = config->local_port;
    rt->configured = config->peer_ip[0] != 0;
    if (rt->configured) {
        if (netmidid_ipv4(config->peer_ip, &rt->configured_address) != 0) {
            fputs(TAG "peer must be an IPv4 address\n", stderr);
            return -1;
        }
        rt->configured_port = config->peer_port;
    }
    /* the data port is the one after the control port */
    if (rt->local_port == 65535 ||
        (rt->socket[0] = netmidid_socket(d, &netmidid_applemidi,
                                         rt->local_port)) < 0 ||
        (rt->socket[1] = netmidid_socket(d, &netmidid_applemidi,
                                         (uint16_t)(rt->local_port + 1))) < 0) {
        fputs(TAG "cannot bind UDP port pair\n", stderr);
        return -1;
    }
    for (index = 0; index < 16; ++index)
        if (config->port_name[index][0] &&
            netmidid_port_keep(d, NETMIDID_APPLEMIDI, index,
                               config->port_name[index]) != 0)
            printf(TAG "applemidi port %d cannot be kept for \"%s\"\n",
                   index, config->port_name[index]);
    now = netmidid_now();
    if (applemidi_peers_init(&rt->peers, netmidid_random(now),
                        (const uint8_t *)d->session_name,
                        applemidi_text_length(d->session_name,
                            sizeof(d->session_name) - 1)) != 0)
        return -1;
    if (config->max_peers)
        rt->peers.limit = config->max_peers;
    if (accept_peers(rt, config->accept[0] ? config->accept : NULL) != 0)
        return -1;
    return 0;
}

static int applemidi_start(struct netmidid *d, void *self, uint64_t now)
{
    struct applemidi_runtime *rt = self;

    (void)d;
    if (rt->configured && invite_configured(rt, now) != 0)
        return -1;
    rt->again_tried = now;
    rt->again_wait = APPLEMIDI_WANTED_FIRST_WAIT;
    printf(TAG "listening on UDP %u/%u\n", (unsigned int)rt->local_port,
           (unsigned int)rt->local_port + 1);
    fflush(stdout);
    return 0;
}

static void applemidi_close(struct netmidid *d, void *self)
{
    struct applemidi_runtime *rt = self;
    int index;

    (void)d;
    for (index = 0; index < APPLEMIDI_PEERS_MAX; ++index)
        if (rt->peers.peer[index].used)
            drop_peer(rt, index, netmidid_now());
}

static void applemidi_release(struct netmidid *d, void *self)
{
    struct applemidi_runtime *rt = self;
    int index;

    (void)d;
    for (index = 0; index < APPLEMIDI_PEERS_MAX; ++index) {
        free(rt->stream[index].sysex.bytes);
        rt->stream[index].sysex.bytes = NULL;
    }
}

const struct netmidid_transport netmidid_applemidi = {
    sizeof(struct applemidi_runtime),
    applemidi_open,
    applemidi_start,
    applemidi_receive,
    applemidi_next_due,
    applemidi_periodic,
    applemidi_send,
    applemidi_flush,
    applemidi_counts,
    applemidi_close,
    applemidi_release
};
