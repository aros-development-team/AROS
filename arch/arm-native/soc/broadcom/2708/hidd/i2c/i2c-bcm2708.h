/*
    Copyright © 2010-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef I2C_BCM2708_H
#define I2C_BCM2708_H

#include <aros/macros.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/nodes.h>
#include <exec/lists.h>

#include <dos/bptr.h>

#include <oop/oop.h>

#include <aros/arossupportbase.h>
#include <exec/execbase.h>

#include <hardware/bcm2708.h>

static inline ULONG rd32le(IPTR iobase) {
    ULONG val;
    val = AROS_LE2LONG(*(volatile ULONG *)(iobase));
    return val;
}

static inline UWORD rd16le(IPTR iobase) {
    UWORD val;
    val = AROS_LE2WORD(*(volatile UWORD *)(iobase));
    return val;
}

static inline UBYTE rd8(IPTR iobase) {
    UBYTE val;
    val = *(volatile UBYTE *)(iobase);
    return val;
}

static inline void wr32le(IPTR iobase, ULONG value) {
    *(volatile ULONG *)(iobase) = AROS_LONG2LE(value);
}

static inline void wr16le(IPTR iobase, UWORD value) {
    *(volatile UWORD *)(iobase) = AROS_WORD2LE(value);
}

static inline void wr8be(IPTR iobase, UBYTE value) {
    *(volatile UBYTE *)(iobase) = value;
}

struct i2cbcm2708base {
    struct Library i2c_LibNode;
    OOP_Class *		i2c_DrvClass;
};

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define METHOD_NAME(base, id, name) \
  base ## __ ## id ## __ ## name

#define METHOD_NAME_S(base, id, name) \
  # base "__" # id "__" # name

#define BASE(lib) ((struct pcibase*)(lib))

#endif /* I2C_BCM2708_H */
