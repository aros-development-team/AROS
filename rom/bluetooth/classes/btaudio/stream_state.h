#ifndef BTAUDIO_STREAM_STATE_H
#define BTAUDIO_STREAM_STATE_H

#include <stdbool.h>
#include <btcore/a2dp_source.h>

/* An open transport may wait indefinitely for local audio or a remote START. */
static inline bool bStreamWaiting(enum bt_a2dp_source_state state, bool media_ready)
{
    return state == BT_A2DP_SUSPENDED ||
        (state == BT_A2DP_MEDIA_PENDING && media_ready);
}

#endif
