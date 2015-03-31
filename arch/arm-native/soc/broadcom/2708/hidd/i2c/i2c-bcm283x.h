/*
    Copyright © 2010-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef I2C_BCM283X_H
#define I2C_BCM283X_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/nodes.h>
#include <exec/lists.h>

#include <dos/bptr.h>

#include <oop/oop.h>

#include <aros/arossupportbase.h>
#include <exec/execbase.h>

#include <hardware/bcm283x.h>

struct i2cbcm283xbase {
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

#endif /* I2C_BCM283X_H */
