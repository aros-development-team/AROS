/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <netmidi/queue.h>
#include "rtpmidi/receiver.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static void report(struct rtpmidi_receiver *rx, const char *format, ...)
{
    char text[200];
    va_list arguments;

    if (!rx->output->report)
        return;
    va_start(arguments, format);
    vsnprintf(text, sizeof(text), format, arguments);
    va_end(arguments);
    rx->output->report(rx->context, text);
}

void rtpmidi_receiver_init(struct rtpmidi_receiver *rx,
                      const struct rtpmidi_receiver_output *output, void *context,
                      struct netmidi_event_queue *queue)
{
    memset(rx, 0, sizeof(*rx));
    rx->output = output;
    rx->context = context;
    rx->queue = queue;
    rtpmidi_receiver_reset_channels(rx);
}

void rtpmidi_receiver_forget_notes(struct rtpmidi_receiver *rx)
{
    memset(rx->active_notes, 0, sizeof(rx->active_notes));
}

static struct rtpmidi_parameter_counter *parameter_counter(
    struct rtpmidi_receiver *rx, unsigned int channel, int nrpn, uint16_t number,
    int create)
{
    size_t i;
    struct rtpmidi_parameter_counter *free_entry = NULL;
    for (i = 0; i < RTPMIDI_PARAMETER_STATES; ++i) {
        struct rtpmidi_parameter_counter *entry = &rx->parameter_counts[i];
        if (entry->used && entry->channel == channel &&
            entry->nrpn == nrpn && entry->number == number)
            return entry;
        if (!entry->used && !free_entry) free_entry = entry;
    }
    if (!create || !free_entry) return NULL;
    memset(free_entry, 0, sizeof(*free_entry));
    free_entry->used = 1;
    free_entry->channel = (uint8_t)channel;
    free_entry->nrpn = (uint8_t)nrpn;
    free_entry->number = number;
    return free_entry;
}

static int count_controller(uint8_t number)
{
    return number == 120 || (number >= 123 && number <= 125) ||
           number == 127;
}

void rtpmidi_receiver_deliver(struct rtpmidi_receiver *rx, const uint8_t *bytes,
                          size_t length)
{
    uint8_t status = bytes[0];
    unsigned int channel = status & 0x0f;
    if (status == 0xff && length == 1) {
        rx->reset_count = (uint8_t)((rx->reset_count + 1) & 0x7f);
        rx->tune_request_count = 0;
        rx->active_sense_count = 0;
        rx->song_select_known = 0;
        rx->sequencer_known = 0;
        rx->sequencer_running = 0;
        rx->sequencer_downbeat = 0;
        rx->sequencer_start_at_zero = 0;
        rx->sequencer_clock = 0;
        rx->mtc_full_known = 0;
        memset(rx->active_notes, 0, sizeof(rx->active_notes));
        memset(rx->note_count, 0, sizeof(rx->note_count));
        rtpmidi_receiver_reset_channels(rx);
    } else if (status == 0xf6 && length == 1) {
        rx->tune_request_count =
            (uint8_t)((rx->tune_request_count + 1) & 0x7f);
    } else if (status == 0xfe && length == 1) {
        rx->active_sense_count =
            (uint8_t)((rx->active_sense_count + 1) & 0x7f);
    } else if (status == 0xf3 && length == 2) {
        rx->song_select = bytes[1];
        rx->song_select_known = 1;
    } else if (status == 0xf2 && length == 3) {
        rx->sequencer_known = 1;
        rx->sequencer_downbeat = 0;
        rx->sequencer_start_at_zero = 0;
        rx->sequencer_clock =
            ((((uint32_t)bytes[2] << 7) | bytes[1]) * 6) & 0x7ffff;
    } else if (status == 0xfa && length == 1) {
        rx->sequencer_known = 1;
        rx->sequencer_running = 1;
        rx->sequencer_downbeat = 0;
        rx->sequencer_start_at_zero = 1;
        rx->sequencer_clock = 0;
    } else if (status == 0xfb && length == 1) {
        rx->sequencer_known = 1;
        rx->sequencer_running = 1;
        rx->sequencer_start_at_zero = 0;
    } else if (status == 0xfc && length == 1) {
        rx->sequencer_known = 1;
        rx->sequencer_running = 0;
        rx->sequencer_start_at_zero = 0;
    } else if (status == 0xf8 && length == 1) {
        rx->sequencer_known = 1;
        rx->sequencer_start_at_zero = 0;
        if (rx->sequencer_running) {
            if (!rx->sequencer_downbeat)
                rx->sequencer_downbeat = 1;
            else
                rx->sequencer_clock = (rx->sequencer_clock + 1) & 0x7ffff;
        }
    }
    if (length == 3 && status >= 0x80 && status <= 0x9f) {
        int note_on = (status & 0xf0) == 0x90 && bytes[2];
        if (note_on) {
            if (rx->note_count[channel][bytes[1]] < 127)
                ++rx->note_count[channel][bytes[1]];
            rx->active_notes[channel][bytes[1]] = bytes[2];
        } else {
            if (rx->note_count[channel][bytes[1]])
                --rx->note_count[channel][bytes[1]];
            rx->active_notes[channel][bytes[1]] = 0;
        }
    } else if (length == 3 && (status & 0xf0) == 0xb0 &&
               (bytes[1] == 120 || bytes[1] >= 123)) {
        /* All Sound Off, and 123 to 127, which all work as All Notes
           Off (MIDI 1.0, pages 20 and 24, table IV). Not 121 and 122. */
        memset(rx->active_notes[channel], 0,
               sizeof(rx->active_notes[channel]));
        memset(rx->note_count[channel], 0,
               sizeof(rx->note_count[channel]));
    }
    if (length == 2 && (status & 0xf0) == 0xc0) {
        rx->program[channel] = bytes[1];
        rx->program_known[channel] = 1;
    } else if (length == 3 && (status & 0xf0) == 0xb0) {
        uint8_t controller = bytes[1];
        rx->controller_count[channel][bytes[1]] =
            (uint8_t)((rx->controller_count[channel][bytes[1]] + 1) & 0x3f);
        if (bytes[1] >= 64 && bytes[1] <= 69 &&
            (rx->controllers[channel][bytes[1]] >= 64) != (bytes[2] >= 64)) {
            unsigned int switch_index = bytes[1] - 64;
            rx->switch_toggles[channel][switch_index] = (uint8_t)(
                (rx->switch_toggles[channel][switch_index] + 1) & 0x3f);
        }
        rx->controllers[channel][bytes[1]] = bytes[2];
        rx->controller_known[channel][bytes[1]] = 1;
        /* MIDI 1.0, page 17: a parameter is chosen by the two numbers,
           99 and 98 or 101 and 100, and Data Entry, Increment and
           Decrement then apply to it. A number of which only the first
           half has come is no parameter here until the second does.
           That text's rule 4 has an instrument take either half alone;
           the journal does not, and on purpose: RFC 6295 appendix A.4
           calls a number with only its MSB sent pending, carries it in
           chapter M's PENDING field, and begins a transaction only at
           the LSB. This state follows the journal's model. */
        if (controller == 101 || controller == 99) {
            rx->parameter_nrpn[channel] = controller == 99;
            rx->parameter_type_known[channel] = 1;
            rx->parameter_msb[channel] = bytes[2];
            rx->parameter_pending[channel] = 1;
            rx->parameter_valid[channel] = 0;
        } else if ((controller == 100 || controller == 98) &&
                   rx->parameter_type_known[channel] &&
                   rx->parameter_nrpn[channel] == (controller == 98)) {
            rx->parameter_lsb[channel] = bytes[2];
            rx->parameter_pending[channel] = 0;
            rx->parameter_valid[channel] =
                rx->parameter_msb[channel] != 0x7f || bytes[2] != 0x7f;
            if (rx->parameter_valid[channel]) {
                uint16_t number = (uint16_t)(
                    ((uint16_t)rx->parameter_msb[channel] << 7) | bytes[2]);
                struct rtpmidi_parameter_counter *counter = parameter_counter(
                    rx, channel, rx->parameter_nrpn[channel], number, 1);
                if (counter)
                    counter->count = (uint8_t)((counter->count + 1) & 0x7f);
            }
        } else if (controller == 121) {
            rx->parameter_pending[channel] = 0;
            rx->parameter_valid[channel] = 0;
        }
        if (bytes[1] == 0) {
            rx->bank_msb[channel] = bytes[2];
            rx->bank_msb_known[channel] = 1;
        } else if (bytes[1] == 32) {
            rx->bank_lsb[channel] = bytes[2];
            rx->bank_lsb_known[channel] = 1;
        } else if (bytes[1] == 121) {
            rx->pitch[channel] = 0x2000;
            rx->channel_pressure[channel] = 0;
            rx->channel_pressure_known[channel] = 1;
            memset(rx->poly_pressure[channel], 0,
                   sizeof(rx->poly_pressure[channel]));
            memset(rx->poly_pressure_known[channel], 1,
                   sizeof(rx->poly_pressure_known[channel]));
            unsigned int switch_index;
            for (switch_index = 0; switch_index < 6; ++switch_index) {
                unsigned int number = switch_index + 64;
                if (rx->controllers[channel][number] >= 64)
                    rx->switch_toggles[channel][switch_index] = (uint8_t)(
                        (rx->switch_toggles[channel][switch_index] + 1) &
                        0x3f);
                rx->controllers[channel][number] = 0;
                rx->controller_known[channel][number] = 1;
            }
        }
    } else if (length == 2 && (status & 0xf0) == 0xd0) {
        rx->channel_pressure[channel] = bytes[1];
        rx->channel_pressure_known[channel] = 1;
    } else if (length == 3 && (status & 0xf0) == 0xa0) {
        rx->poly_pressure[channel][bytes[1]] = bytes[2];
        rx->poly_pressure_known[channel][bytes[1]] = 1;
    } else if (length == 3 && (status & 0xf0) == 0xe0) {
        rx->pitch[channel] = (uint16_t)(bytes[1] | (bytes[2] << 7));
    }
    rx->output->message(rx->context, bytes, length);
}

void rtpmidi_receiver_deliver_sysex(struct rtpmidi_receiver *rx, const uint8_t *message,
                          size_t length)
{
    int mtc = length == 10 && message[0] == 0xf0 && message[1] == 0x7f &&
              message[3] == 0x01 && message[4] == 0x01 &&
              message[9] == 0xf7;
    if (mtc) {
        size_t i;
        for (i = 1; i < 9 && !(message[i] & 0x80); ++i) {}
        if (i == 9) {
            memcpy(rx->mtc_full_frame, message + 5, 4);
            rx->mtc_full_known = 1;
        }
    } else if (length >= 2 && message[0] == 0xf0 &&
               message[length - 1] == 0xf7 &&
               length - 2 <= RTPMIDI_SYSEX_JOURNAL_MAX)
        /* the sender's COUNT leaves out what chapter X never holds */
        ++rx->sysex_count;
    rx->output->sysex(rx->context, message, length);
}

void rtpmidi_receiver_reset_channels(struct rtpmidi_receiver *rx)
{
    unsigned int channel;
    memset(rx->program_known, 0, sizeof(rx->program_known));
    memset(rx->bank_msb_known, 0, sizeof(rx->bank_msb_known));
    memset(rx->bank_lsb_known, 0, sizeof(rx->bank_lsb_known));
    memset(rx->controller_known, 0, sizeof(rx->controller_known));
    memset(rx->controllers, 0, sizeof(rx->controllers));
    memset(rx->switch_toggles, 0, sizeof(rx->switch_toggles));
    memset(rx->controller_count, 0, sizeof(rx->controller_count));
    memset(rx->parameter_type_known, 0,
           sizeof(rx->parameter_type_known));
    memset(rx->parameter_pending, 0, sizeof(rx->parameter_pending));
    memset(rx->parameter_valid, 0, sizeof(rx->parameter_valid));
    memset(rx->parameter_counts, 0, sizeof(rx->parameter_counts));
    memset(rx->channel_pressure_known, 0,
           sizeof(rx->channel_pressure_known));
    memset(rx->poly_pressure_known, 0, sizeof(rx->poly_pressure_known));
    for (channel = 0; channel < 16; ++channel)
        rx->pitch[channel] = 0x2000;
}

void rtpmidi_receiver_reset_system(struct rtpmidi_receiver *rx)
{
    rx->reset_count = 0;
    rx->tune_request_count = 0;
    rx->active_sense_count = 0;
    rx->song_select = 0;
    rx->song_select_known = 0;
    rx->sequencer_known = 0;
    rx->sequencer_running = 0;
    rx->sequencer_downbeat = 0;
    rx->sequencer_start_at_zero = 0;
    rx->sequencer_clock = 0;
    rx->mtc_full_known = 0;
    rx->sysex_count = 0;
}

void rtpmidi_receiver_release_notes(struct rtpmidi_receiver *rx)
{
    uint8_t message[3];
    unsigned int channel;
    unsigned int note;
    message[2] = 0;
    for (channel = 0; channel < 16; ++channel) {
        message[0] = (uint8_t)(0x80 | channel);
        for (note = 0; note < 128; ++note) {
            message[1] = (uint8_t)note;
            while (rx->note_count[channel][note])
                rtpmidi_receiver_deliver(rx, message, sizeof(message));
            rx->active_notes[channel][note] = 0;
        }
    }
}

static void recover_channel_state(struct rtpmidi_receiver *rx,
                                   const struct rtpmidi_journal *journal,
                                   int single_loss)
{
    struct rtpmidi_journal_channel_state state;
    uint8_t message[3];
    size_t i;
    unsigned int channel;
    int bank_changed;
    uint16_t pitch;

    if (single_loss && journal->single_packet_safe)
        return;
    for (i = 0; i < journal->channel_count; ++i) {
        const struct rtpmidi_journal_channel *entry = &journal->channels[i];
        if (single_loss && entry->single_packet_safe)
            continue;
        if (rtpmidi_journal_decode_channel_state(entry, &state) <= 0)
            continue;
        channel = entry->number;
        bank_changed = 0;
        if (state.has_program &&
            (!single_loss || !state.program_single_packet_safe)) {
            message[0] = (uint8_t)(0xb0 | channel);
            if (state.has_bank) {
                if (!rx->bank_msb_known[channel] ||
                    rx->bank_msb[channel] != state.bank_msb) {
                    message[1] = 0;
                    message[2] = state.bank_msb;
                    rtpmidi_receiver_deliver(rx, message, 3);
                    bank_changed = 1;
                }
                if (!rx->bank_lsb_known[channel] ||
                    rx->bank_lsb[channel] != state.bank_lsb) {
                    message[1] = 32;
                    message[2] = state.bank_lsb;
                    rtpmidi_receiver_deliver(rx, message, 3);
                    bank_changed = 1;
                }
            }
            if (bank_changed || !rx->program_known[channel] ||
                rx->program[channel] != state.program) {
                message[0] = (uint8_t)(0xc0 | channel);
                message[1] = state.program;
                rtpmidi_receiver_deliver(rx, message, 2);
                report(rx, "recovered Program Change channel=%u program=%u\n",
                       channel, (unsigned int)state.program);
            }
        }
        pitch = (uint16_t)(state.pitch_lsb | (state.pitch_msb << 7));
        if (state.has_pitch &&
            (!single_loss || !state.pitch_single_packet_safe) &&
            rx->pitch[channel] != pitch) {
            message[0] = (uint8_t)(0xe0 | channel);
            message[1] = state.pitch_lsb;
            message[2] = state.pitch_msb;
            rtpmidi_receiver_deliver(rx, message, 3);
            report(rx, "recovered Pitch Bend channel=%u value=%u\n",
                   channel, (unsigned int)pitch);
        }
    }
}

static void recover_controls(struct rtpmidi_receiver *rx,
                              const struct rtpmidi_journal *journal,
                              int single_loss)
{
    struct rtpmidi_journal_controls controls;
    uint8_t message[3];
    size_t i;
    size_t j;
    unsigned int channel;
    if (single_loss && journal->single_packet_safe)
        return;
    for (i = 0; i < journal->channel_count; ++i) {
        const struct rtpmidi_journal_channel *entry = &journal->channels[i];
        if (single_loss && entry->single_packet_safe)
            continue;
        if (rtpmidi_journal_decode_controls(entry, &controls) <= 0)
            continue;
        channel = entry->number;
        message[0] = (uint8_t)(0xb0 | channel);
        if (controls.enhanced) {
            uint8_t handled[128] = {0};
            size_t first;

            for (first = 0; first < controls.count; ++first) {
                uint8_t number = controls.logs[first].number;
                uint8_t highest = 0;
                uint8_t command;
                uint8_t target = 0;
                int target_known = 0;

                if (handled[number]) continue;
                handled[number] = 1;
                for (j = first; j < controls.count; ++j)
                    if (controls.logs[j].number == number &&
                        controls.logs[j].command > highest)
                        highest = controls.logs[j].command;
                /* A single command does not need enhanced matching. Let the
                 * ordinary tool handling below process it. */
                if (!highest) {
                    handled[number] = 0;
                    continue;
                }
                for (command = 0; command <= highest; ++command) {
                    const struct rtpmidi_journal_control_log *value = NULL;
                    const struct rtpmidi_journal_control_log *toggle = NULL;
                    const struct rtpmidi_journal_control_log *count = NULL;
                    uint8_t missing;
                    int all_safe = 1;

                    for (j = first; j < controls.count; ++j) {
                        const struct rtpmidi_journal_control_log *log =
                            &controls.logs[j];
                        if (log->number != number || log->command != command)
                            continue;
                        if (!log->single_packet_safe) all_safe = 0;
                        if (!log->alternate) value = log;
                        else if (log->count_tool) count = log;
                        else toggle = log;
                    }
                    if (count) {
                        target = count->value;
                        target_known = 1;
                    } else if (target_known) {
                        target = (uint8_t)((target + 1) & 0x3f);
                    } else {
                        continue;
                    }
                    if (single_loss && all_safe) continue;
                    missing = (uint8_t)(
                        (target - rx->controller_count[channel][number]) &
                        0x3f);
                    if (!missing) continue;
                    message[1] = number;
                    while (missing > 1) {
                        message[2] = 0;
                        rtpmidi_receiver_deliver(rx, message, sizeof(message));
                        --missing;
                    }
                    if (value) {
                        message[2] = value->value;
                        rtpmidi_receiver_deliver(rx, message, sizeof(message));
                    } else if (toggle) {
                        int desired = toggle->value & 1;
                        int current = rx->controllers[channel][number] >= 64;
                        if (current == desired) {
                            message[2] = desired ? 0 : 127;
                            rtpmidi_receiver_deliver(rx, message, sizeof(message));
                        }
                        message[2] = desired ? 127 : 0;
                        rtpmidi_receiver_deliver(rx, message, sizeof(message));
                    } else {
                        message[2] = 0;
                        rtpmidi_receiver_deliver(rx, message, sizeof(message));
                    }
                    /* Multiple tools describe one command. Compensate for a
                     * toggle's two-message reconstruction and retain the
                     * count identity from the journal. */
                    rx->controller_count[channel][number] = target;
                    report(rx, "recovered enhanced controller channel=%u controller=%u command=%u count=%u\n",
                           channel, (unsigned int)number,
                           (unsigned int)command, (unsigned int)target);
                }
            }
        }
        for (j = 0; j < controls.count; ++j) {
            const struct rtpmidi_journal_control_log *log = &controls.logs[j];
            if (controls.enhanced) {
                size_t k;
                int repeated = 0;

                for (k = 0; k < controls.count; ++k)
                    if (controls.logs[k].number == log->number &&
                        controls.logs[k].command != log->command) {
                        repeated = 1;
                        break;
                    }
                if (repeated) continue;
            }
            if (single_loss && log->single_packet_safe)
                continue;
            if (log->alternate) {
                if (log->number >= 64 && log->number <= 69 &&
                    !log->count_tool &&
                    rx->switch_toggles[channel][log->number - 64] !=
                    log->value) {
                    message[1] = log->number;
                    if (rx->controllers[channel][log->number] >= 64) {
                        message[2] = 0;
                        rtpmidi_receiver_deliver(rx, message, sizeof(message));
                    }
                    if (log->value & 1) {
                        message[2] = 127;
                        rtpmidi_receiver_deliver(rx, message, sizeof(message));
                    }
                    rx->switch_toggles[channel][log->number - 64] = log->value;
                    report(rx, "recovered controller toggle channel=%u controller=%u count=%u\n",
                           channel, (unsigned int)log->number,
                           (unsigned int)log->value);
                } else if (log->count_tool &&
                           count_controller(log->number) &&
                           rx->controller_count[channel][log->number] !=
                           log->value) {
                    uint8_t missing = (uint8_t)(
                        (log->value -
                         rx->controller_count[channel][log->number]) & 0x3f);
                    message[1] = log->number;
                    message[2] = 0;
                    while (missing--)
                        rtpmidi_receiver_deliver(rx, message, sizeof(message));
                    report(rx, "recovered controller count channel=%u controller=%u count=%u\n",
                           channel, (unsigned int)log->number,
                           (unsigned int)log->value);
                }
                continue;
            }
            if (rx->controller_known[channel][log->number] &&
                rx->controllers[channel][log->number] == log->value)
                continue;
            message[1] = log->number;
            message[2] = log->value;
            rtpmidi_receiver_deliver(rx, message, sizeof(message));
            report(rx, "recovered Control Change channel=%u controller=%u value=%u\n",
                   channel, (unsigned int)log->number,
                   (unsigned int)log->value);
        }
    }
}

static void recover_parameters(struct rtpmidi_receiver *rx,
                               const struct rtpmidi_journal *journal,
                               int single_loss)
{
    struct rtpmidi_journal_parameters parameters;
    uint8_t message[3];
    size_t i;
    size_t j;
    int replayed = 0;
    int last_nrpn = 0;
    char count_text[16];

    if (single_loss && journal->single_packet_safe) return;
    for (i = 0; i < journal->channel_count; ++i) {
        const struct rtpmidi_journal_channel *channel = &journal->channels[i];
        if (single_loss && channel->single_packet_safe) continue;
        if (rtpmidi_journal_decode_parameters(channel, &parameters) <= 0)
            continue;
        message[0] = (uint8_t)(0xb0 | channel->number);
        replayed = 0;
        for (j = 0; j < parameters.count; ++j) {
            const struct rtpmidi_journal_parameter_log *log = &parameters.logs[j];
            struct rtpmidi_parameter_counter *counter = parameter_counter(
                rx, channel->number, log->nrpn, log->number, 0);
            int had_counter = counter != NULL;
            uint8_t previous_count = counter ? counter->count : 0;
            int count_mismatch = log->has_count &&
                (!counter || counter->count != log->count);
            if ((single_loss && log->single_packet_safe) ||
                (!log->has_entry_msb && !log->has_entry_lsb &&
                 !log->has_adjust && !count_mismatch))
                continue;
            message[1] = (uint8_t)(log->nrpn ? 99 : 101);
            message[2] = (uint8_t)(log->number >> 7);
            rtpmidi_receiver_deliver(rx, message, sizeof(message));
            message[1] = (uint8_t)(log->nrpn ? 98 : 100);
            message[2] = (uint8_t)(log->number & 0x7f);
            rtpmidi_receiver_deliver(rx, message, sizeof(message));
            if (log->has_entry_msb) {
                message[1] = 6;
                message[2] = log->entry_msb;
                rtpmidi_receiver_deliver(rx, message, sizeof(message));
            }
            if (log->has_entry_lsb) {
                message[1] = 38;
                message[2] = log->entry_lsb;
                rtpmidi_receiver_deliver(rx, message, sizeof(message));
            }
            if (log->has_adjust) {
                unsigned int count = (unsigned int)(
                    log->adjust < 0 ? -log->adjust : log->adjust);
                message[1] = (uint8_t)(log->adjust < 0 ? 97 : 96);
                message[2] = 0;
                while (count--) rtpmidi_receiver_deliver(rx, message, sizeof(message));
            }
            if (log->has_count) {
                counter = parameter_counter(
                    rx, channel->number, log->nrpn, log->number, 1);
                if (counter) counter->count = log->count;
            } else {
                counter = parameter_counter(
                    rx, channel->number, log->nrpn, log->number, 0);
                if (counter) {
                    counter->count = previous_count;
                    if (!had_counter) counter->used = 0;
                }
            }
            replayed = 1;
            last_nrpn = log->nrpn;
            if (log->has_count)
                snprintf(count_text, sizeof(count_text), " count=%u",
                         (unsigned int)log->count);
            else
                count_text[0] = '\0';
            report(rx, "recovered %s parameter=%u entry=%s%u%s adjust=%d%s\n",
                   log->nrpn ? "NRPN" : "RPN",
                   (unsigned int)log->number,
                   log->has_entry_msb ? "MSB:" : "",
                   (unsigned int)(log->has_entry_msb ? log->entry_msb :
                                  log->entry_lsb),
                   log->has_entry_lsb ? "+LSB" : "", (int)log->adjust,
                   count_text);
        }
        if (parameters.has_pending &&
            (!single_loss || !parameters.single_packet_safe)) {
            message[1] = (uint8_t)(parameters.pending_nrpn ? 99 : 101);
            message[2] = parameters.pending_msb;
            rtpmidi_receiver_deliver(rx, message, sizeof(message));
            report(rx, "recovered pending %s MSB=%u\n",
                   parameters.pending_nrpn ? "NRPN" : "RPN",
                   (unsigned int)parameters.pending_msb);
        } else if (replayed && !parameters.transaction_open) {
            message[1] = (uint8_t)(last_nrpn ? 99 : 101);
            message[2] = 0x7f;
            rtpmidi_receiver_deliver(rx, message, sizeof(message));
            message[1] = (uint8_t)(last_nrpn ? 98 : 100);
            rtpmidi_receiver_deliver(rx, message, sizeof(message));
        }
    }
}

static void recover_notes(struct rtpmidi_receiver *rx,
                          const struct rtpmidi_journal *journal,
                          int single_loss, uint32_t timestamp,
                          const struct rtpmidi_receiver_clock *clock)
{
    struct rtpmidi_journal_notes notes;
    struct rtpmidi_journal_note_extras extras;
    uint8_t message[3];
    uint8_t release_velocity[128];
    uint8_t desired_count[128];
    uint8_t count_known[128];
    size_t i;
    size_t log_index;
    unsigned int octave;
    unsigned int bit;
    unsigned int note;
    size_t cancelled;
    int decoded;
    int extras_decoded;
    uint64_t due;
    uint64_t now;
    int timely;

    if (single_loss && journal->single_packet_safe)
        return;
    for (i = 0; i < journal->channel_count; ++i) {
        const struct rtpmidi_journal_channel *channel = &journal->channels[i];
        if (single_loss && channel->single_packet_safe)
            continue;
        decoded = rtpmidi_journal_decode_notes(channel, &notes);
        extras_decoded = rtpmidi_journal_decode_note_extras(channel, &extras);
        if (decoded < 0 || extras_decoded < 0 ||
            (!decoded && !extras_decoded))
            continue;
        memset(release_velocity, 64, sizeof(release_velocity));
        memset(desired_count, 0, sizeof(desired_count));
        memset(count_known, 0, sizeof(count_known));
        for (log_index = 0; log_index < extras.count; ++log_index) {
            const struct rtpmidi_journal_note_extra *extra =
                &extras.logs[log_index];
            if (single_loss && extra->single_packet_safe)
                continue;
            if (extra->velocity)
                release_velocity[extra->number] = extra->value;
            else {
                desired_count[extra->number] = extra->value;
                count_known[extra->number] = 1;
            }
        }
        message[0] = (uint8_t)(0x80 | channel->number);
        for (octave = 0; octave < 16 &&
                         (!single_loss || !notes.offbits_single_packet_safe);
             ++octave) {
            for (bit = 0; bit < 8; ++bit) {
                note = octave * 8 + bit;
                if (!(notes.offbits[octave] & (0x80u >> bit)))
                    continue;
                cancelled = netmidi_queue_cancel_note_on(rx->queue,
                                                    channel->number,
                                                    (uint8_t)note);
                if (cancelled)
                    report(rx, "cancelled %lu future Note On event(s) channel=%u note=%u\n",
                           (unsigned long)cancelled,
                           (unsigned int)channel->number, note);
                if (!rx->active_notes[channel->number][note] &&
                    !rx->note_count[channel->number][note])
                    continue;
                message[1] = (uint8_t)note;
                message[2] = release_velocity[note];
                rtpmidi_receiver_deliver(rx, message, sizeof(message));
                report(rx, "recovered Note Off channel=%u note=%u\n",
                       (unsigned int)channel->number, note);
            }
        }
        now = clock ? clock->now : 0;
        timely = clock && clock->synchronized &&
                 rtpmidi_clock_due(timestamp, now, clock->peer_to_local,
                              &due) == 0 &&
                 (due >= now || now - due <= 200);
        for (log_index = 0; log_index < notes.log_count; ++log_index) {
            const struct rtpmidi_journal_note_log *log = &notes.logs[log_index];
            if (single_loss && log->single_packet_safe)
                continue;
            if (rx->active_notes[channel->number][log->number] ==
                log->velocity ||
                netmidi_queue_has_note_on(rx->queue, channel->number,
                                     log->number))
                continue;
            if (rx->active_notes[channel->number][log->number]) {
                message[0] = (uint8_t)(0x80 | channel->number);
                message[1] = log->number;
                message[2] = release_velocity[log->number];
                rtpmidi_receiver_deliver(rx, message, sizeof(message));
            }
            if (!log->simultaneous || !timely)
                continue;
            message[0] = (uint8_t)(0x90 | channel->number);
            message[1] = log->number;
            message[2] = log->velocity;
            if (due > now) {
                if (due - now > 100000 ||
                    netmidi_queue_push(rx->queue, due, message,
                                  sizeof(message)) != 0)
                    continue;
            } else {
                rtpmidi_receiver_deliver(rx, message, sizeof(message));
            }
            report(rx, "recovered Note On channel=%u note=%u velocity=%u\n",
                   (unsigned int)channel->number,
                   (unsigned int)log->number,
                   (unsigned int)log->velocity);
        }
        for (note = 0; note < 128; ++note) {
            uint8_t velocity = rx->active_notes[channel->number][note];
            if (!count_known[note]) continue;
            if (!velocity) {
                for (log_index = 0; log_index < notes.log_count; ++log_index)
                    if (notes.logs[log_index].number == note) {
                        velocity = notes.logs[log_index].velocity;
                        break;
                    }
            }
            if (!velocity) velocity = 64;
            while (rx->note_count[channel->number][note] <
                   desired_count[note]) {
                message[0] = (uint8_t)(0x90 | channel->number);
                message[1] = (uint8_t)note;
                message[2] = velocity;
                rtpmidi_receiver_deliver(rx, message, sizeof(message));
                report(rx, "recovered duplicate Note On channel=%u note=%u count=%u\n",
                       (unsigned int)channel->number, note,
                       (unsigned int)rx->note_count[channel->number][note]);
            }
            while (rx->note_count[channel->number][note] >
                   desired_count[note]) {
                message[0] = (uint8_t)(0x80 | channel->number);
                message[1] = (uint8_t)note;
                message[2] = release_velocity[note];
                rtpmidi_receiver_deliver(rx, message, sizeof(message));
                report(rx, "recovered counted Note Off channel=%u note=%u count=%u velocity=%u\n",
                       (unsigned int)channel->number, note,
                       (unsigned int)rx->note_count[channel->number][note],
                       (unsigned int)message[2]);
            }
        }
    }
}

static void recover_aftertouch(struct rtpmidi_receiver *rx,
                               const struct rtpmidi_journal *journal,
                               int single_loss)
{
    struct rtpmidi_journal_aftertouch aftertouch;
    uint8_t message[3];
    size_t i;
    size_t j;
    unsigned int channel;
    if (single_loss && journal->single_packet_safe) return;
    for (i = 0; i < journal->channel_count; ++i) {
        const struct rtpmidi_journal_channel *entry = &journal->channels[i];
        if (single_loss && entry->single_packet_safe) continue;
        if (rtpmidi_journal_decode_aftertouch(entry, &aftertouch) <= 0) continue;
        channel = entry->number;
        if (aftertouch.has_channel_pressure &&
            (!single_loss || !aftertouch.channel_single_packet_safe) &&
            (!rx->channel_pressure_known[channel] ||
             rx->channel_pressure[channel] != aftertouch.channel_pressure)) {
            message[0] = (uint8_t)(0xd0 | channel);
            message[1] = aftertouch.channel_pressure;
            rtpmidi_receiver_deliver(rx, message, 2);
            report(rx, "recovered Channel Aftertouch channel=%u pressure=%u\n",
                   channel, (unsigned int)message[1]);
        }
        for (j = 0; j < aftertouch.poly_count; ++j) {
            const struct rtpmidi_journal_poly_pressure *log = &aftertouch.poly[j];
            if ((single_loss && log->single_packet_safe) ||
                log->reset_notes || !rx->active_notes[channel][log->number] ||
                (rx->poly_pressure_known[channel][log->number] &&
                 rx->poly_pressure[channel][log->number] == log->pressure))
                continue;
            message[0] = (uint8_t)(0xa0 | channel);
            message[1] = log->number;
            message[2] = log->pressure;
            rtpmidi_receiver_deliver(rx, message, 3);
            report(rx, "recovered Poly Aftertouch channel=%u note=%u pressure=%u\n",
                   channel, (unsigned int)log->number,
                   (unsigned int)log->pressure);
        }
    }
}

static void recover_system(struct rtpmidi_receiver *rx,
                           const struct rtpmidi_journal_system_state *state,
                           int single_loss)
{
    uint8_t message[3];
    uint32_t clock;
    uint32_t remainder;
    int start_at_zero;
    if (state->has_reset &&
        (!single_loss || !state->reset_single_packet_safe) &&
        rx->reset_count != state->reset_count) {
        message[0] = 0xff;
        rtpmidi_receiver_deliver(rx, message, 1);
        rx->reset_count = state->reset_count;
        report(rx, "recovered System Reset\n");
    }
    if (state->has_tune_request &&
        (!single_loss || !state->tune_single_packet_safe) &&
        rx->tune_request_count != state->tune_count) {
        message[0] = 0xf6;
        rtpmidi_receiver_deliver(rx, message, 1);
        rx->tune_request_count = state->tune_count;
        report(rx, "recovered Tune Request\n");
    }
    if (state->has_song_select &&
        (!single_loss || !state->song_single_packet_safe) &&
        (!rx->song_select_known || rx->song_select != state->song)) {
        message[0] = 0xf3;
        message[1] = state->song;
        rtpmidi_receiver_deliver(rx, message, 2);
        report(rx, "recovered Song Select song=%u\n",
               (unsigned int)state->song);
    }
    if (state->has_active_sense &&
        (!single_loss || !state->active_sense_single_packet_safe) &&
        rx->active_sense_count != state->active_sense_count) {
        message[0] = 0xfe;
        rtpmidi_receiver_deliver(rx, message, 1);
        rx->active_sense_count = state->active_sense_count;
        report(rx, "recovered Active Sense\n");
    }
    if (state->has_mtc && state->mtc_has_complete &&
        !state->mtc_complete_quarter_frame &&
        (!single_loss || !state->mtc_single_packet_safe) &&
        (!rx->mtc_full_known ||
         memcmp(rx->mtc_full_frame, state->mtc_complete, 4) != 0)) {
        uint8_t full_frame[10] = {
            0xf0, 0x7f, 0x7f, 0x01, 0x01, 0, 0, 0, 0, 0xf7
        };
        memcpy(full_frame + 5, state->mtc_complete, 4);
        rtpmidi_receiver_deliver_sysex(rx, full_frame, sizeof(full_frame));
        report(rx, "recovered MTC Full Frame %02x:%02x:%02x:%02x\n",
               (unsigned int)state->mtc_complete[0],
               (unsigned int)state->mtc_complete[1],
               (unsigned int)state->mtc_complete[2],
               (unsigned int)state->mtc_complete[3]);
    }
    if (state->has_mtc && state->mtc_has_complete &&
        state->mtc_complete_quarter_frame &&
        (!single_loss || !state->mtc_single_packet_safe)) {
        uint8_t full_frame[10] = {
            0xf0, 0x7f, 0x7f, 0x01, 0x01, 0, 0, 0, 0, 0xf7
        };
        full_frame[5] = (uint8_t)(state->mtc_complete[6] |
                                  (state->mtc_complete[7] << 4));
        full_frame[6] = (uint8_t)(state->mtc_complete[4] |
                                  (state->mtc_complete[5] << 4));
        full_frame[7] = (uint8_t)(state->mtc_complete[2] |
                                  (state->mtc_complete[3] << 4));
        full_frame[8] = (uint8_t)(state->mtc_complete[0] |
                                  (state->mtc_complete[1] << 4));
        if (!rx->mtc_full_known ||
            memcmp(rx->mtc_full_frame, full_frame + 5, 4) != 0) {
            rtpmidi_receiver_deliver_sysex(rx, full_frame, sizeof(full_frame));
            report(rx, "recovered MTC Quarter Frame position "
                   "%02x:%02x:%02x:%02x\n",
                   (unsigned int)full_frame[5],
                   (unsigned int)full_frame[6],
                   (unsigned int)full_frame[7],
                   (unsigned int)full_frame[8]);
        }
    }
    if (state->has_mtc && state->mtc_has_partial &&
        (!single_loss || !state->mtc_single_packet_safe)) {
        int point = state->mtc_point;
        int type = state->mtc_reverse ? 7 : 0;
        int step = state->mtc_reverse ? -1 : 1;
        for (;;) {
            message[0] = 0xf1;
            message[1] = (uint8_t)((type << 4) |
                                    state->mtc_partial[type]);
            rtpmidi_receiver_deliver(rx, message, 2);
            if (type == point) break;
            type += step;
        }
        report(rx, "recovered MTC Quarter Frame partial direction=%s point=%u\n",
               state->mtc_reverse ? "reverse" : "forward",
               (unsigned int)state->mtc_point);
    }
    if (state->sysex) {
        struct rtpmidi_journal_sysex sysex;
        if (rtpmidi_journal_decode_sysex(state, &sysex) > 0) {
            size_t log_index;
            uint8_t last_count = 0;
            int have_count = 0;
            for (log_index = 0; log_index < sysex.count; ++log_index) {
                const struct rtpmidi_journal_sysex_log *log =
                    &sysex.logs[log_index];
                uint8_t recovered[RTPMIDI_SYSEX_JOURNAL_MAX + 2];
                size_t data_index;
                if (log->has_count) {
                    last_count = log->count;
                    have_count = 1;
                }
                /* COUNT numbers the sender's SysEx commands modulo 256
                 * (RFC 6295 appendix B.5.3): a log is one we missed when
                 * its number is ahead of the last one we have. */
                if ((single_loss && log->single_packet_safe) ||
                    log->status != 3 || log->has_first || !log->has_count ||
                    (uint8_t)(log->count - rx->sysex_count) == 0 ||
                    (uint8_t)(log->count - rx->sysex_count) >= 128 ||
                    log->data_length > RTPMIDI_SYSEX_JOURNAL_MAX)
                    continue;
                recovered[0] = 0xf0;
                for (data_index = 0; data_index < log->data_length;
                     ++data_index)
                    recovered[data_index + 1] =
                        (uint8_t)(log->data[data_index] & 0x7f);
                recovered[log->data_length + 1] = 0xf7;
                rtpmidi_receiver_deliver_sysex(rx, recovered, log->data_length + 2);
                rx->sysex_count = log->count;
                report(rx, "recovered SysEx bytes=%lu count=%u\n",
                       (unsigned long)(log->data_length + 2),
                       (unsigned int)log->count);
            }
            /* The final log's COUNT is there to take up the count again
             * after a loss: one whose log had already left the journal
             * is then not counted against every later one. */
            if (have_count && (uint8_t)(last_count - rx->sysex_count) < 128)
                rx->sysex_count = last_count;
        }
    }
    if (!state->has_sequencer ||
        (single_loss && state->sequencer_single_packet_safe) ||
        state->has_time_tools)
        return;
    clock = state->has_clock ? state->clock : 0;
    start_at_zero = state->sequencer_running &&
                    !state->downbeat_played && !state->has_clock;
    if (rx->sequencer_known &&
        rx->sequencer_running == state->sequencer_running &&
        rx->sequencer_downbeat == state->downbeat_played &&
        rx->sequencer_clock == clock &&
        rx->sequencer_start_at_zero == start_at_zero)
        return;

    /* C=0, N=1, D=0 is the RFC 6295 encoding for Start at zero. */
    if (start_at_zero) {
        message[0] = 0xfa;
        rtpmidi_receiver_deliver(rx, message, 1);
        report(rx, "recovered sequencer Start\n");
        return;
    }

    /* Song Position Pointer plus at most six Clocks recreates every position
     * representable by MIDI's 14-bit SPP command. */
    if (clock > 98303) {
        if (!state->sequencer_running && rx->sequencer_running) {
            message[0] = 0xfc;
            rtpmidi_receiver_deliver(rx, message, 1);
            report(rx, "recovered sequencer Stop; "
                       "position exceeds MIDI SPP range\n");
        }
        return;
    }
    if (rx->sequencer_running) {
        message[0] = 0xfc;
        rtpmidi_receiver_deliver(rx, message, 1);
    }
    message[0] = 0xf2;
    message[1] = (uint8_t)((clock / 6) & 0x7f);
    message[2] = (uint8_t)(((clock / 6) >> 7) & 0x7f);
    rtpmidi_receiver_deliver(rx, message, 3);
    remainder = clock % 6;
    if (state->downbeat_played) {
        unsigned int i;

        message[0] = 0xfb;
        rtpmidi_receiver_deliver(rx, message, 1);
        message[0] = 0xf8;
        for (i = 0; i <= remainder; ++i)
            rtpmidi_receiver_deliver(rx, message, 1);
        if (!state->sequencer_running) {
            message[0] = 0xfc;
            rtpmidi_receiver_deliver(rx, message, 1);
        }
    } else if (state->sequencer_running) {
        message[0] = 0xfb;
        rtpmidi_receiver_deliver(rx, message, 1);
    }
    report(rx, "recovered sequencer state running=%u clock=%lu downbeat=%u\n",
           (unsigned int)state->sequencer_running,
           (unsigned long)clock, (unsigned int)state->downbeat_played);
}

void rtpmidi_receiver_recover(struct rtpmidi_receiver *rx,
                         const struct rtpmidi_journal *journal,
                         const struct rtpmidi_journal_system_state *system,
                         int single_loss, uint32_t timestamp,
                         const struct rtpmidi_receiver_clock *clock)
{
    if (system && !(single_loss && journal->single_packet_safe))
        recover_system(rx, system, single_loss);
    recover_channel_state(rx, journal, single_loss);
    recover_notes(rx, journal, single_loss, timestamp, clock);
    recover_parameters(rx, journal, single_loss);
    recover_controls(rx, journal, single_loss);
    recover_aftertouch(rx, journal, single_loss);
}
