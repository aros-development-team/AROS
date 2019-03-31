#ifndef P96GFX_HIDD_H
#define P96GFX_HIDD_H

#include <exec/lists.h>
#include <oop/oop.h>

#include <interface/Hidd_P96Gfx.h>

#include "p96gfx_intern.h"

#define P96GFX_LIBNAME                                          "p96gfx.hidd"
#define CLID_Hidd_Gfx_P96                                       IID_Hidd_P96Gfx

/* Private instance data for P96Gfx hidd class */
struct P96GfxData
{
    struct MinList                                              bitmaps;                        /* Currently shown bitmap objects       */
    OOP_Object                                                  *spriteColors;
    OOP_Object                                                  *pfo;
    struct p96gfx_carddata                                      *cardData;        
};

#endif /* P96GFX_HIDD_H */
