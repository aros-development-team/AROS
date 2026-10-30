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
          without damaging anything else in the length tests. The invalid
          capacity tests can crash an unfixed class; use a test system.

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
    if (strlen((char *)buffer) != (size_t)expect || strncmp((char *)buffer, text, expect) != 0)
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

    gad = NewObject(NULL, (UBYTE *)STRGCLASS,
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

/* The edit buffer itself (or a suffix) may be passed back to OM_SET. */
static int run_overlap(void)
{
    Object *gad;
    int fail;

    memset(buffer, 0xA5, sizeof(buffer));
    gad = NewObject(NULL, (UBYTE *)STRGCLASS,
                    STRINGA_Buffer, (IPTR)buffer,
                    STRINGA_MaxChars, MAXCHARS,
                    STRINGA_TextVal, (IPTR)"abcdefghijk",
                    TAG_DONE);
    if (!gad)
        return 1;
    SetAttrs(gad, STRINGA_TextVal, (IPTR)buffer, TAG_DONE);
    fail = check("OM_SET same buffer", 11, gad, "abcdefghijk");
    SetAttrs(gad, STRINGA_TextVal, (IPTR)(buffer + 2), TAG_DONE);
    fail |= check("OM_SET overlapping suffix", 9, gad, "cdefghijk");
    DisposeObject(gad);
    return fail;
}

/* Supply all buffers so invalid capacities do not request allocations.
   On an unfixed class these LongVal cases can crash; run on a test system. */
static int run_limit(int limit, int numeric)
{
    UBYTE guarded[32], expected[32], undo[32], work[32];
    Object *gad;
    int fail;
    Tag value_tag = numeric ? STRINGA_LongVal : STRINGA_TextVal;
    IPTR value = numeric ? 123 : (IPTR)"abc";

    memset(guarded, 0xA5, sizeof(guarded));
    guarded[8] = 0;
    memcpy(expected, guarded, sizeof(expected));
    gad = NewObject(NULL, (UBYTE *)STRGCLASS,
                    STRINGA_Buffer, (IPTR)(guarded + 8),
                    STRINGA_UndoBuffer, (IPTR)undo,
                    STRINGA_WorkBuffer, (IPTR)work,
                    STRINGA_MaxChars, (IPTR)limit,
                    value_tag, value,
                    TAG_DONE);
    if (!gad)
        return 1;
    fail = memcmp(guarded, expected, sizeof(guarded)) != 0;
    SetAttrs(gad, value_tag, value, TAG_DONE);
    fail |= memcmp(guarded, expected, sizeof(guarded)) != 0;
    printf("%s MaxChars %d, %s, OM_NEW and OM_SET\n",
           fail ? "FAIL" : "ok", limit, numeric ? "LongVal" : "TextVal");
    DisposeObject(gad);
    return fail;
}

int main(void)
{
    static const int lengths[] = { 0, 5, MAXCHARS - 1, MAXCHARS, LONGTEXT };
    size_t i;
    int fail = 0;

    for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++)
        fail |= run(lengths[i]);

    fail |= run_overlap();
    fail |= run_limit(1, 0);
    fail |= run_limit(1, 1);
    fail |= run_limit(0, 0);
    fail |= run_limit(0, 1);
    fail |= run_limit(-1, 0);
    fail |= run_limit(-1, 1);

    printf("%s\n", fail ? "FAIL" : "PASS");
    return fail ? 20 : 0;
}
