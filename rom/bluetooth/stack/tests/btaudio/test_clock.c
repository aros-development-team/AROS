#include "../support/test.h"
#include "test_clock.h"
#include "../../../classes/btaudio/clock.h"

void run_clock_tests(void)
{
    struct bAudioClock clock = {1234567, 0};
    unsigned i;
    /* One hour of 44.1 kHz audio, including fractional packet periods. */
    for(i = 0; i < 248062; i++) bClockAdvance(&clock, 640, 44100);
    bClockAdvance(&clock, 320, 44100);
    BT_CHECK(clock.next == 1234567ULL + 3600000000ULL);
    BT_CHECK(clock.remainder == 0);
    /* Processing and scheduling delays never become part of the deadline. */
    clock.next = 1000000; clock.remainder = 0;
    bClockAdvance(&clock, 640, 48000);
    BT_CHECK(clock.next == 1013333);
    for(i = 0; clock.next <= 1100000; i++) bClockAdvance(&clock, 640, 48000);
    BT_CHECK(i == 7);
    BT_CHECK(clock.next == 1106666);
    /* A blocked write leaves the deadline unchanged until a packet fits. */
    BT_CHECK(clock.remainder == 32000);

    /* A multi-second stall cannot turn into seconds of burst output. */
    clock.next = 1000000; clock.remainder = 123;
    bClockLimitDebt(&clock, 11000000, 640, 44100, 8);
    BT_CHECK(clock.next == 11000000 - 116099);
    BT_CHECK(clock.remainder == 0);
    for(i = 0; clock.next < 11000000; i++) bClockAdvance(&clock, 640, 44100);
    BT_CHECK(i == 8);
    /* Do not perturb a short delay, a future deadline or early uptime. */
    clock.next = 20000000; clock.remainder = 321;
    bClockLimitDebt(&clock, 20010000, 640, 44100, 8);
    BT_CHECK(clock.next == 20000000 && clock.remainder == 321);
    bClockLimitDebt(&clock, 19900000, 640, 44100, 8);
    BT_CHECK(clock.next == 20000000 && clock.remainder == 321);
    clock.next = 0;
    bClockLimitDebt(&clock, 100, 640, 48000, 8);
    BT_CHECK(clock.next == 0);
}
