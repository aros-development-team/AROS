#ifndef BTAUDIO_CLOCK_H
#define BTAUDIO_CLOCK_H

#include <stdint.h>

/* Advance from the previous deadline, not from completion time. Carry the
   fractional microseconds so 44.1 kHz does not accumulate rounding drift. */
struct bAudioClock {
    uint64_t next;
    uint32_t remainder;
};

static inline void bClockAdvance(struct bAudioClock *clock, uint32_t frames, uint32_t rate)
{
    uint64_t delta = (uint64_t)frames * 1000000 + clock->remainder;
    clock->next += delta / rate;
    clock->remainder = delta % rate;
}

/* Keep at most one queue of catch-up work after a prolonged stall. Short
   scheduling delays retain their exact deadlines and fractional remainder. */
static inline void bClockLimitDebt(struct bAudioClock *clock, uint64_t now,
    uint32_t frames, uint32_t rate, uint32_t depth)
{
    uint64_t debt = (uint64_t)frames * 1000000 * depth / rate;
    if(now > clock->next && now - clock->next > debt) {
        clock->next = now - debt;
        clock->remainder = 0;
    }
}

#endif
