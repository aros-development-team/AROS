#include "test_blemidi.h"
#include "../support/test.h"
#include "../../../classes/btmidi/blemidi.h"

#include <stdint.h>
#include <string.h>

/* btmidi.class's codec against "Specification for MIDI over Bluetooth Low
   Energy (BLE-MIDI)" 1.0, MMA/AMEI RP-052: the packets its figures draw,
   decoded, and the same messages written. */

struct bytes {
    uint8_t data[1024];
    uint16_t times[1024];
    size_t count;
};

static int take_byte(void *context, uint16_t time, uint8_t byte)
{
    struct bytes *bytes = context;

    if (bytes->count >= sizeof(bytes->data))
        return -1;
    bytes->data[bytes->count] = byte;
    bytes->times[bytes->count++] = time;
    return 0;
}

#define PACKETS_MAX 64

struct packets {
    uint8_t data[PACKETS_MAX][BLEMIDI_PACKET_MAX];
    size_t length[PACKETS_MAX];
    size_t count;
};

static int take_packet(void *context, const uint8_t *packet, size_t length)
{
    struct packets *packets = context;

    if (packets->count >= PACKETS_MAX || length > BLEMIDI_PACKET_MAX)
        return -1;
    memcpy(packets->data[packets->count], packet, length);
    packets->length[packets->count++] = length;
    return 0;
}

/* The checks name the line of the case, not of the helper. */
#define CHECK_AT(cond, line) bt_test_check((cond), #cond, __FILE__, (line))

static struct bytes decodes(struct blemidi_decoder *decoder,
                            const uint8_t *packet, size_t length,
                            const uint8_t *expected, size_t expected_length,
                            int line)
{
    struct bytes got;

    got.count = 0;
    CHECK_AT(blemidi_check(decoder, packet, length) == 0, line);
    CHECK_AT(blemidi_decode(decoder, packet, length, take_byte, &got) == 0,
             line);
    CHECK_AT(got.count == expected_length &&
             memcmp(got.data, expected, expected_length) == 0, line);
    return got;
}

#define DECODES(decoder, packet, expected) \
    decodes(decoder, packet, sizeof(packet), expected, sizeof(expected), \
            __LINE__)

#define REFUSED(decoder, packet) \
    BT_CHECK(blemidi_check(decoder, packet, sizeof(packet)) == -1)

/* Section 7, the figures of pages 3 and 4. */
static void figures_of_section_7(void)
{
    struct blemidi_decoder decoder;
    struct bytes got;

    blemidi_decoder_init(&decoder);
    {   /* "BLE Packet with One MIDI Message" */
        static const uint8_t packet[] = {0x81, 0x85, 0x90, 60, 100};
        static const uint8_t midi[] = {0x90, 60, 100};
        got = DECODES(&decoder, packet, midi);
        BT_CHECK(got.times[0] == ((1 << 7) | 5));
    }
    {   /* "BLE Packet with Two MIDI Messages" */
        static const uint8_t packet[] = {0x81, 0x85, 0x90, 60, 100,
                                         0x86, 0x80, 60, 0};
        static const uint8_t midi[] = {0x90, 60, 100, 0x80, 60, 0};
        got = DECODES(&decoder, packet, midi);
        BT_CHECK(got.times[2] == 133 && got.times[3] == 134);
    }
    {   /* "2 MIDI Messages with Running Status" */
        static const uint8_t packet[] = {0x80, 0x80, 0x90, 60, 100, 64, 90};
        static const uint8_t midi[] = {0x90, 60, 100, 0x90, 64, 90};
        DECODES(&decoder, packet, midi);
    }
    {   /* the same with a message of two bytes */
        static const uint8_t packet[] = {0x80, 0x80, 0xc0, 5, 6, 0x81, 7};
        static const uint8_t midi[] = {0xc0, 5, 0xc0, 6, 0xc0, 7};
        DECODES(&decoder, packet, midi);
    }
    {   /* "Multiple MIDI Messages, mixed type": full, full, running
           status, running status with a new timestamp, full */
        static const uint8_t packet[] = {0x80, 0x81, 0x90, 60, 100,
                                         0x82, 0xb0, 7, 90, 10, 64,
                                         0x83, 11, 127,
                                         0x84, 0x80, 60, 0};
        static const uint8_t midi[] = {0x90, 60, 100, 0xb0, 7, 90,
                                       0xb0, 10, 64, 0xb0, 11, 127,
                                       0x80, 60, 0};
        got = DECODES(&decoder, packet, midi);
        BT_CHECK(got.times[6] == 2 && got.times[9] == 3 &&
                 got.times[12] == 4);
    }
    {   /* "System Messages Do Not Cancel Running Status": the data
           after the system message is of the status before it, and has
           a timestamp of its own (rule 3) */
        static const uint8_t common[] = {0x80, 0x81, 0x90, 60, 100, 64, 90,
                                         0x82, 0xf3, 5, 0x83, 67, 80};
        static const uint8_t common_midi[] = {0x90, 60, 100, 0x90, 64, 90,
                                              0xf3, 5, 0x90, 67, 80};
        static const uint8_t realtime[] = {0x80, 0x81, 0x90, 60, 100,
                                           0x82, 0xf8, 0x83, 67, 80};
        static const uint8_t realtime_midi[] = {0x90, 60, 100, 0xf8,
                                                0x90, 67, 80};
        DECODES(&decoder, common, common_midi);
        DECODES(&decoder, realtime, realtime_midi);
    }
    {   /* rule 4: the end of a packet cancels running status; rule 1: a
           running status message needs a full one before it */
        static const uint8_t first[] = {0x80, 0x80, 0x90, 60, 100};
        static const uint8_t midi[] = {0x90, 60, 100};
        static const uint8_t second[] = {0x80, 0x81, 64, 90};
        DECODES(&decoder, first, midi);
        REFUSED(&decoder, second);
    }
    {   /* rule 2: every status byte has a timestamp byte before it. The
           second 0x90 here is read as a timestamp and the data after it
           as running status: what was written is not what is read, and
           no receiver can tell. */
        static const uint8_t packet[] = {0x80, 0x80, 0x90, 60, 100,
                                         0x90, 64, 90};
        static const uint8_t midi[] = {0x90, 60, 100, 0x90, 64, 90};
        got = DECODES(&decoder, packet, midi);
        BT_CHECK(got.times[3] == 0x10);
    }
    {   /* the header byte: bit 7 set; bit 6 is reserved, sent as 0 and
           not looked at when received */
        static const uint8_t no_header[] = {0x01, 0x85, 0x90, 60, 100};
        static const uint8_t reserved[] = {0xc1, 0x85, 0x90, 60, 100};
        static const uint8_t no_timestamp[] = {0x81, 0x05, 0x90, 60, 100};
        static const uint8_t short_message[] = {0x81, 0x85, 0x90, 60};
        static const uint8_t midi[] = {0x90, 60, 100};
        REFUSED(&decoder, no_header);
        got = DECODES(&decoder, reserved, midi);
        BT_CHECK(got.times[0] == 133);
        REFUSED(&decoder, no_timestamp);
        REFUSED(&decoder, short_message);
    }
}

/* Section 8, the figures of page 6, and its two lists of rules. */
static void figures_of_section_8(void)
{
    struct blemidi_decoder decoder;

    blemidi_decoder_init(&decoder);
    {   /* "System Exclusive Start & End in 1 Packet" */
        static const uint8_t packet[] = {0x80, 0x81, 0xf0, 0x7d, 1, 2,
                                         0x82, 0xf7};
        static const uint8_t midi[] = {0xf0, 0x7d, 1, 2, 0xf7};
        DECODES(&decoder, packet, midi);
    }
    {   /* "System Exclusive Split Across 3 Packets": the second has no
           timestamp byte, which is how it is known for a continuation */
        static const uint8_t first[] = {0x80, 0x81, 0xf0, 0x7d, 1};
        static const uint8_t first_midi[] = {0xf0, 0x7d, 1};
        static const uint8_t second[] = {0x80, 2, 3, 4};
        static const uint8_t second_midi[] = {2, 3, 4};
        static const uint8_t third[] = {0x80, 5, 0x83, 0xf7};
        static const uint8_t third_midi[] = {5, 0xf7};
        DECODES(&decoder, first, first_midi);
        DECODES(&decoder, second, second_midi);
        DECODES(&decoder, third, third_midi);
    }
    {   /* the end alone in the last packet */
        static const uint8_t first[] = {0x80, 0x81, 0xf0, 0x7d, 1};
        static const uint8_t first_midi[] = {0xf0, 0x7d, 1};
        static const uint8_t last[] = {0x80, 0x83, 0xf7};
        static const uint8_t last_midi[] = {0xf7};
        DECODES(&decoder, first, first_midi);
        DECODES(&decoder, last, last_midi);
    }
    {   /* rules 4 and 5: System Real Time inside, with a timestamp byte,
           and the data going on after it, also at a packet's start */
        static const uint8_t first[] = {0x80, 0x81, 0xf0, 0x7d, 1,
                                        0x82, 0xf8, 2};
        static const uint8_t first_midi[] = {0xf0, 0x7d, 1, 0xf8, 2};
        static const uint8_t second[] = {0x80, 0x83, 0xfa, 3, 0x84, 0xf7};
        static const uint8_t second_midi[] = {0xfa, 3, 0xf7};
        DECODES(&decoder, first, first_midi);
        DECODES(&decoder, second, second_midi);
    }
    {   /* "only System Real-Time messages are allowed to precede its
           completion": a channel message inside is refused, and the
           decoder is as it was */
        static const uint8_t first[] = {0x80, 0x81, 0xf0, 0x7d, 1};
        static const uint8_t first_midi[] = {0xf0, 0x7d, 1};
        static const uint8_t note[] = {0x80, 0x82, 0x90, 60, 100};
        static const uint8_t last[] = {0x80, 2, 0x83, 0xf7};
        static const uint8_t last_midi[] = {2, 0xf7};
        DECODES(&decoder, first, first_midi);
        REFUSED(&decoder, note);
        DECODES(&decoder, last, last_midi);
    }
}

/* Section 9: thirteen bits; within a packet a low part smaller than the
   one before means the high part went up by one. */
static void timestamps_of_section_9(void)
{
    struct blemidi_decoder decoder;
    struct bytes got;
    static const uint8_t packet[] = {0x81, 0xfe, 0x90, 60, 100,
                                     0x83, 0x80, 60, 0};
    static const uint8_t midi[] = {0x90, 60, 100, 0x80, 60, 0};
    static const uint8_t top[] = {0xbf, 0xff, 0x90, 60, 100,
                                  0x80, 0x80, 60, 0};

    blemidi_decoder_init(&decoder);
    got = DECODES(&decoder, packet, midi);
    BT_CHECK(got.times[0] == ((1 << 7) | 0x7e));
    BT_CHECK(got.times[3] == ((2 << 7) | 3));
    got = DECODES(&decoder, top, midi);
    BT_CHECK(got.times[0] == 8191 && got.times[3] == 0);
}

#define WRITES(packets, index, expected) \
    BT_CHECK((packets).count > (index) && \
             (packets).length[index] == sizeof(expected) && \
             memcmp((packets).data[index], expected, sizeof(expected)) == 0)

static int put(struct blemidi_writer *writer, uint16_t timestamp,
               const uint8_t *midi, size_t length)
{
    size_t i;

    for (i = 0; i < length; i++)
        if (blemidi_writer_put(writer, timestamp, midi[i]) != 0)
            return -1;
    return 0;
}

/* F9 and FD, which MIDI 1.0 leaves undefined, are in neither direction's
   stream; the messages around them are. */
static void undefined_statuses(void)
{
    struct blemidi_decoder decoder;
    static struct blemidi_writer writer;
    static struct packets packets;
    static const uint8_t packet[] = {0x80, 0x80, 0x90, 60, 100, 0x81, 0xf9,
                                     0x82, 0xfd, 0x83, 0xf8, 0x84, 64, 90};
    static const uint8_t midi[] = {0x90, 60, 100, 0xf8, 0x90, 64, 90};
    static const uint8_t stream[] = {0x90, 60, 0xf9, 100, 0xfd, 0xf8};
    static const uint8_t written[] = {0x80, 0x80, 0x90, 60, 100, 0x80, 0xf8};

    blemidi_decoder_init(&decoder);
    DECODES(&decoder, packet, midi);
    packets.count = 0;
    blemidi_writer_init(&writer, take_packet, &packets);
    BT_CHECK(put(&writer, 0, stream, sizeof(stream)) == 0);
    BT_CHECK(blemidi_writer_flush(&writer) == 0);
    BT_CHECK(packets.count == 1);
    WRITES(packets, 0, written);
}

/* What the writer makes of a MIDI byte stream is one of the forms the
   figures draw, and the decoder gives the stream back. */
static void writer_follows_the_figures(void)
{
    static struct blemidi_writer writer;
    static struct packets packets;

    packets.count = 0;
    blemidi_writer_init(&writer, take_packet, &packets);
    {   /* one message; two; a third by running status in the stream,
           which is written in full: the specification lets a sender
           leave the status out, it does not ask for it */
        static const uint8_t midi[] = {0x90, 60, 100, 0x80, 60, 0, 64, 0};
        static const uint8_t packet[] = {0x81, 0x85, 0x90, 60, 100,
                                         0x85, 0x80, 60, 0,
                                         0x85, 0x80, 64, 0};
        BT_CHECK(put(&writer, 133, midi, sizeof(midi)) == 0);
        BT_CHECK(blemidi_writer_flush(&writer) == 0);
        BT_CHECK(packets.count == 1);
        WRITES(packets, 0, packet);
    }
    {   /* page 5, "System Real-Time Message in the middle of another
           MIDI Message": the two come apart. The figure puts the
           interrupted message first; this sends the Real Time byte
           first, since it came first. */
        static const uint8_t midi[] = {0x90, 60, 0xf8, 100};
        static const uint8_t packet[] = {0x80, 0x82, 0xf8,
                                         0x82, 0x90, 60, 100};
        packets.count = 0;
        BT_CHECK(put(&writer, 2, midi, sizeof(midi)) == 0);
        BT_CHECK(blemidi_writer_flush(&writer) == 0);
        BT_CHECK(packets.count == 1);
        WRITES(packets, 0, packet);
    }
    {   /* section 7: no packet larger than the link carries, and no
           message but SysEx over two packets. 20 bytes take a header
           and four notes with their timestamp bytes. */
        static const uint8_t note[] = {0x90, 60, 100};
        int i;

        packets.count = 0;
        for (i = 0; i < 5; i++)
            BT_CHECK(put(&writer, 0, note, sizeof(note)) == 0);
        BT_CHECK(blemidi_writer_flush(&writer) == 0);
        BT_CHECK(packets.count == 2 &&
                 packets.length[0] == 17 && packets.length[1] == 5);
    }
    {   /* section 8: a SysEx over three packets of 20 bytes, System
           Real Time inside it, the end with a timestamp byte */
        uint8_t midi[48];
        struct blemidi_decoder decoder;
        static struct bytes got;
        size_t i, n = 0;
        int decoded = 0;

        midi[n++] = 0xf0;
        for (i = 0; i < 20; i++)
            midi[n++] = (uint8_t)i;
        midi[n++] = 0xf8;
        for (i = 20; i < 40; i++)
            midi[n++] = (uint8_t)i;
        midi[n++] = 0xf7;
        packets.count = 0;
        BT_CHECK(put(&writer, 300, midi, n) == 0);
        BT_CHECK(blemidi_writer_flush(&writer) == 0);
        BT_CHECK(packets.count == 3);
        if (packets.count == 3) {
            BT_CHECK(packets.data[0][0] == 0x82 &&
                     packets.data[0][1] == 0xac &&
                     packets.data[0][2] == 0xf0);
            /* a continuation: header, then data */
            BT_CHECK(packets.data[1][0] == 0x82 &&
                     !(packets.data[1][1] & 0x80));
            BT_CHECK(packets.data[2][packets.length[2] - 2] == 0xac &&
                     packets.data[2][packets.length[2] - 1] == 0xf7);
        }
        blemidi_decoder_init(&decoder);
        got.count = 0;
        for (i = 0; i < packets.count; i++) {
            BT_CHECK(packets.length[i] <= BLEMIDI_PACKET_MIN);
            if (blemidi_decode(&decoder, packets.data[i], packets.length[i],
                               take_byte, &got) == 0)
                decoded++;
        }
        BT_CHECK(decoded == (int)packets.count);
        BT_CHECK(got.count == n && memcmp(got.data, midi, n) == 0);
    }
    {   /* a status that cuts a SysEx short: section 8 allows nothing but
           Real Time before the end, so the end is written first */
        static const uint8_t midi[] = {0xf0, 0x7d, 1, 0x90, 60, 100};
        static const uint8_t packet[] = {0x80, 0x81, 0xf0, 0x7d, 1,
                                         0x81, 0xf7, 0x81, 0x90, 60, 100};
        packets.count = 0;
        BT_CHECK(put(&writer, 1, midi, sizeof(midi)) == 0);
        BT_CHECK(blemidi_writer_flush(&writer) == 0);
        BT_CHECK(packets.count == 1);
        WRITES(packets, 0, packet);
    }
}

void run_blemidi_tests(void)
{
    figures_of_section_7();
    figures_of_section_8();
    timestamps_of_section_9();
    undefined_statuses();
    writer_follows_the_figures();
}
