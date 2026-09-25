#ifndef X11_KEYSTATE_H
#define X11_KEYSTATE_H

/* Private X11 transition bookkeeping. No Xlib calls, allocation or callbacks.
 * key[] is zero when up, raw code + 1 when delivered, or 0xffff when a
 * held key is unmapped/suppressed. Counts belong to this backend only.
 */
struct x11_keystate
{
    unsigned short key[256];
    unsigned short count[128];
};

static inline int x11_key_press(struct x11_keystate *s, unsigned int key, int raw)
{
    if (key >= 256 || s->key[key])
        return -1;
    if (raw < 0 || raw >= 128)
    {
        s->key[key] = 0xffff;
        return -1;
    }
    s->key[key] = raw + 1;
    return s->count[raw]++ == 0 ? raw : -1;
}

static inline int x11_key_release(struct x11_keystate *s, unsigned int key)
{
    unsigned int held;
    if (key >= 256 || !(held = s->key[key]))
        return -1;
    s->key[key] = 0;
    if (held == 0xffff)
        return -1;
    return --s->count[held - 1] == 0 ? (int)((held - 1) | 0x80) : -1;
}

#endif
