/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder
*/
/* m68kemu_shadow.h — Generic struct shadow engine */
#ifndef M68KEMU_SHADOW_H
#define M68KEMU_SHADOW_H

#include <exec/types.h>

/* Field types */
enum {
    SF_END = 0,  /* terminator */
    SF_BYTE,     /* UBYTE */
    SF_WORD,     /* WORD/UWORD (16-bit) */
    SF_LONG,     /* LONG/ULONG (32-bit) */
    SF_PTR,      /* pointer — create sub-shadow */
};

/* One field mapping */
struct M68KFieldMap {
    UWORD m68k_off;       /* offset in m68k struct */
    UWORD native_off;     /* offsetof() in native struct */
    UBYTE type;           /* SF_* */
    UWORD shadow_type;    /* for SF_PTR: type tag in shadow table */
    UWORD sub_alloc;      /* for SF_PTR: m68k alloc size for pointee */
    const struct M68KStructLayout *sub_layout; /* for SF_PTR: recursive sync (or NULL) */
};

/* Struct layout descriptor */
struct M68KStructLayout {
    const char *name;
    UWORD m68k_size;
    UWORD shadow_type;
    const struct M68KFieldMap *fields;
};

struct M68KEmuContext;

/* Create a shadow: alloc m68k space, register, sync all fields */
ULONG shadow_create(struct M68KEmuContext *ctx,
                    const struct M68KStructLayout *layout,
                    void *native);

/* Re-sync fields from native to m68k */
void shadow_sync(struct M68KEmuContext *ctx,
                 const struct M68KStructLayout *layout,
                 ULONG m68k_addr, void *native);

/* Destroy shadow and free m68k space */
void shadow_destroy(struct M68KEmuContext *ctx,
                    const struct M68KStructLayout *layout,
                    ULONG m68k_addr);

/* Find a generated layout by struct name (e.g. "Window", "Library") */
const struct M68KStructLayout *shadow_find_layout(const char *name);

/* Create shadow by struct name — convenience wrapper */
ULONG shadow_create_by_name(struct M68KEmuContext *ctx,
                            const char *struct_name, void *native);

/* Init ExecBase shadow onto existing lib base */
ULONG shadow_init_execbase(struct M68KEmuContext *ctx, ULONG m68k_base, void *native_sysbase);


/* m68k→native translation */
struct TagItem *m68k_to_native_taglist(struct M68KEmuContext *ctx, ULONG m68k_addr);
void free_native_taglist(struct TagItem *tags, int count);
void *m68k_to_native_struct(struct M68KEmuContext *ctx,
                            const char *struct_name, ULONG m68k_addr);

#endif
