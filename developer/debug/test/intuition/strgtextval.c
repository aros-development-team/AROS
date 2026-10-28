/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: A text given to a strgclass gadget with STRINGA_TextVal, at OM_NEW
          and with OM_SET, must be stored whole when it fits into
          STRINGA_MaxChars bytes and cut to MaxChars - 1 characters when it
          does not, and nothing may be written past the edit buffer. The
          buffer is the caller's (STRINGA_Buffer), followed by guard bytes
          that are checked afterwards. ASL's file requester hands its
          pattern to such a gadget, so a pattern longer than 256 characters
          overwrote the memory after the gadget's buffer. The guard area is
          longer than the longest text, so an unfixed gadget is detected
          without damaging anything else.

    Usage: strgtextval
    Result: prints PASS and returns 0, or prints FAIL and returns 20.
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <intuition/gadgetclass.h>
#include <intuition/classes.h>
#include <stdio.h>
#include <string.h>

#define MAXCHARS 16
#define LONGTEXT 300
#define GUARD    (LONGTEXT + 16)

static UBYTE buffer[MAXCHARS + GUARD];

static int check(const char *when, int len, Object *gad, const char *text)
{
    int expect = len < MAXCHARS ? len : MAXCHARS - 1;
    STRPTR val = NULL;
    int i;

    GetAttr(STRINGA_TextVal, gad, (IPTR *)&val);
    if (val != (STRPTR)buffer)
    {
        printf("FAIL %s, %d characters: TextVal %p is not the given buffer %p\n",
               when, len, val, buffer);
        return 1;
    }
    for (i = MAXCHARS; i < MAXCHARS + GUARD; i++)
    {
        if (buffer[i] != 0xA5)
        {
            printf("FAIL %s, %d characters: guard byte %d after the buffer was overwritten\n",
                   when, len, i - MAXCHARS);
            return 1;
        }
    }
    if (strlen((char *)buffer) != expect || strncmp((char *)buffer, text, expect) != 0)
    {
        printf("FAIL %s, %d characters: buffer holds %lu characters, expected %d\n",
               when, len, (unsigned long)strlen((char *)buffer), expect);
        return 1;
    }
    printf("ok %s, %d characters: %d stored\n", when, len, expect);
    return 0;
}

static int run(int len)
{
    char text[LONGTEXT + 1];
    Object *gad;
    int i, fail = 0;

    for (i = 0; i < len; i++)
        text[i] = 'a' + i % 26;
    text[len] = '\0';
    memset(buffer, 0xA5, sizeof(buffer));

    gad = NewObject(NULL, STRGCLASS,
                    STRINGA_Buffer, (IPTR)buffer,
                    STRINGA_MaxChars, MAXCHARS,
                    STRINGA_TextVal, (IPTR)text,
                    TAG_DONE);
    if (!gad)
    {
        printf("FAIL: could not create the gadget\n");
        return 1;
    }
    fail |= check("OM_NEW", len, gad, text);

    memset(buffer, 0xA5, sizeof(buffer));
    buffer[0] = '\0';
    if (len > 0)
        text[0] = 'Z';
    SetAttrs(gad, STRINGA_TextVal, (IPTR)text, TAG_DONE);
    fail |= check("OM_SET", len, gad, text);

    DisposeObject(gad);
    return fail;
}

int main(void)
{
    static const int lengths[] = { 0, 5, MAXCHARS - 1, MAXCHARS, LONGTEXT };
    int i, fail = 0;

    for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++)
        fail |= run(lengths[i]);

    printf("%s\n", fail ? "FAIL" : "PASS");
    return fail ? 20 : 0;
}
