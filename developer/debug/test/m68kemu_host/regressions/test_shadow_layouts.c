/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/
/* Host fixture of run_shadow_layouts.py. It is built once for every layout of
   the sample structures, each time around the same header that
   gen_shadow_layouts.py wrote for them, and reports what the tables hold. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t UBYTE;
typedef int8_t BYTE;
typedef uint16_t UWORD;
typedef uint32_t ULONG;
typedef void *APTR;

/* the SF_ types, M68KFieldMap and M68KStructLayout of m68kemu_shadow.h */
#include "shadow_types.inc"
/* the generator's output; it includes the sample structures itself */
#include "sample_layouts.h"

/* What a table must hold, written without the generator's macros. */
struct Expected
{
    const char *layout;
    const char *member;
    unsigned m68k_off;
    size_t native_off;
    int type;
};

#define POINTER (sizeof(void *) == 8 ? SF_PTR : SF_LONG)
#define VALUE (sizeof(SampleValue) == 8 ? SF_PTR : SF_LONG)
#define FIELD(m68k_off, type, name, member) \
    { #name, #member, m68k_off, offsetof(struct name, member), type }

static const struct Expected expected[] = {
    FIELD( 0, POINTER, SampleBase, sb_Lib.sl_Node.sn_Succ),
    FIELD( 4, POINTER, SampleBase, sb_Lib.sl_Node.sn_Pred),
    FIELD( 8, SF_BYTE, SampleBase, sb_Lib.sl_Node.sn_Type),
    FIELD( 9, SF_BYTE, SampleBase, sb_Lib.sl_Node.sn_Pri),
    FIELD(10, POINTER, SampleBase, sb_Lib.sl_Node.sn_Name),
    FIELD(14, SF_BYTE, SampleBase, sb_Lib.sl_Flags),
    FIELD(15, SF_BYTE, SampleBase, sb_Lib.sl_Pad),
    FIELD(16, SF_WORD, SampleBase, sb_Lib.sl_Version),
    FIELD(18, SF_LONG, SampleBase, sb_Lib.sl_Sum),
    FIELD(22, SF_WORD, SampleBase, sb_Lib.sl_OpenCnt),
    FIELD(24, SF_WORD, SampleBase, sb_Count),
    FIELD(26, SF_LONG, SampleBase, sb_Flags),
    FIELD(30, POINTER, SampleBase, sb_Data),
    FIELD(34, VALUE,   SampleBase, sb_Value),
    FIELD( 0, POINTER, SampleLibrary, sl_Node.sn_Succ),
    FIELD( 4, POINTER, SampleLibrary, sl_Node.sn_Pred),
    FIELD( 8, SF_BYTE, SampleLibrary, sl_Node.sn_Type),
    FIELD( 9, SF_BYTE, SampleLibrary, sl_Node.sn_Pri),
    FIELD(10, POINTER, SampleLibrary, sl_Node.sn_Name),
    FIELD(14, SF_BYTE, SampleLibrary, sl_Flags),
    FIELD(15, SF_BYTE, SampleLibrary, sl_Pad),
    FIELD(16, SF_WORD, SampleLibrary, sl_Version),
    FIELD(18, SF_LONG, SampleLibrary, sl_Sum),
    FIELD(22, SF_WORD, SampleLibrary, sl_OpenCnt),
    FIELD( 0, POINTER, SampleNode, sn_Succ),
    FIELD( 4, POINTER, SampleNode, sn_Pred),
    FIELD( 8, SF_BYTE, SampleNode, sn_Type),
    FIELD( 9, SF_BYTE, SampleNode, sn_Pri),
    FIELD(10, POINTER, SampleNode, sn_Name),
};
#define EXPECTED (sizeof(expected) / sizeof(expected[0]))

/* name, size on m68k, and the type number: 20 onwards in the order of the names */
static const struct { const char *name; unsigned m68k_size, shadow_type; } layouts[] = {
    { "SampleBase", 70, 20 }, { "SampleLibrary", 24, 21 }, { "SampleNode", 14, 22 },
};
#define LAYOUTS (sizeof(layouts) / sizeof(layouts[0]))

static unsigned checks, failures;

static void check(int ok, const char *layout, const char *member, const char *what)
{
    checks++;
    if (!ok)
    {
        failures++;
        printf("FAILED %s %s: %s\n", layout, member, what);
    }
}

static const char *type_name(int type)
{
    switch (type)
    {
    case SF_BYTE: return "SF_BYTE";
    case SF_WORD: return "SF_WORD";
    case SF_LONG: return "SF_LONG";
    case SF_PTR:  return "SF_PTR";
    }
    return "?";
}

int main(void)
{
    size_t next = 0;
    unsigned i;

    printf("BUILD pointer %u value %u lock %u\n", (unsigned)sizeof(void *),
           (unsigned)sizeof(SampleValue), (unsigned)sizeof(struct SampleLock));
    check(SHADOW_GEN_LAYOUT_COUNT == LAYOUTS, "table", "", "the count is that of the structs with a member");
    for (i = 0; i < LAYOUTS; i++)
    {
        const struct M68KStructLayout *layout = &shadow_gen_layouts[i];
        const struct M68KFieldMap *f;

        check(layout->name && !strcmp(layout->name, layouts[i].name), layouts[i].name, "", "its place in the table");
        if (!layout->name)
            break;
        check(layout->m68k_size == layouts[i].m68k_size, layouts[i].name, "", "the m68k size");
        check(layout->shadow_type == layouts[i].shadow_type, layouts[i].name, "", "the type number");
        for (f = layout->fields; f->type != SF_END; f++)
        {
            const struct Expected *e = next < EXPECTED ? &expected[next] : NULL;

            check(e && !strcmp(e->layout, layouts[i].name), layouts[i].name, e ? e->member : "", "a field that is expected here");
            if (!e)
                break;
            next++;
            check(f->m68k_off == e->m68k_off, e->layout, e->member, "the m68k offset");
            check(f->native_off == e->native_off, e->layout, e->member, "the native offset is that of this build");
            check(f->type == e->type, e->layout, e->member, "the type follows the native size");
            check(!f->shadow_type && !f->sub_alloc && !f->sub_layout, e->layout, e->member, "nothing else is set");
            printf("FIELD %s %s %u %u %s\n", e->layout, e->member, (unsigned)f->m68k_off,
                   (unsigned)f->native_off, type_name(f->type));
        }
        check(!f->m68k_off && !f->native_off && !f->sub_layout, layouts[i].name, "", "the end entry is empty");
    }
    check(next == EXPECTED, "table", "", "every expected field was there");
    check(!shadow_gen_layouts[LAYOUTS].name && !shadow_gen_layouts[LAYOUTS].fields, "table", "", "it ends after the last struct");
    printf("shadow layouts: %u checks, %u failed\n", checks, failures);
    return failures != 0;
}
