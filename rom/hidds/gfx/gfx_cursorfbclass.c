/*
    Copyright (C) 2016-2026, The AROS Development Team. All rights reserved.

    Desc: Software-cursor framebuffer wrapper bitmap class (CursorFB).

    This class wraps the real framebuffer / displayable bitmap of a display
    driver that has no hardware sprite. It forwards every bitmap operation to
    the wrapped (real) bitmap, but brackets the pixel-writing and pixel-reading
    operations with a hide/show of the software mouse pointer (rendered by the
    Display class). This keeps the pointer from being corrupted when the desktop
    is drawn underneath it, without requiring any changes to the display drivers.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <exec/memory.h>
#include <oop/oop.h>
#include <hidd/gfx.h>

#include <stddef.h>

#include "gfx_intern.h"

/* In gfx_intern.h, 'csd' is a macro expanding to CSD(cl). This class takes an
   explicit class_static_data argument in its factory/helper functions and the
   CFB_INIT macro declares a local 'csd', so drop the macro here. CSD(cl) stays. */
#undef csd

struct CursorFBData
{
    OOP_Object *realbm;     /* The wrapped real framebuffer / screen bitmap */
    OOP_Object *display;    /* The Display object owning the software pointer */
};

/*
 * Bracketing helpers. The pointer is hidden before an operation that overlaps
 * it, and shown again afterwards. The whole bracket is serialised by the
 * Display's cursor semaphore so the framebuffer update is atomic.
 */
#define CFB_INIT                                                        \
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;                      \
    struct CursorFBData *data = OOP_INST_DATA(cl, o);                   \
    struct class_static_data *csd = CSD(cl);                            \
    struct SignalSemaphore *sem = GfxDisplay_CursorSem(csd, data->display); \
    BOOL inside = FALSE;                                                \
    IPTR retval;                                                        \
    ObtainSemaphore(sem);

#define CFB_REMOVE(x1, y1, x2, y2)                                      \
    if (GfxDisplay_CursorIntersects(csd, data->display, data->realbm, (x1), (y1), (x2), (y2))) { \
        GfxDisplay_CursorRemove(csd, data->display);                   \
        inside = TRUE;                                                  \
    }

/* Conservative bracket for operations whose bounds are not cheaply known: if
   this bitmap currently carries the pointer, remove it for the whole op. */
#define CFB_REMOVE_ALL                                                  \
    if (GfxDisplay_CursorIntersects(csd, data->display, data->realbm, -16384, -16384, 16383, 16383)) { \
        GfxDisplay_CursorRemove(csd, data->display);                   \
        inside = TRUE;                                                  \
    }

#define CFB_FORWARD                                                     \
    retval = OOP_DoMethod(data->realbm, (OOP_Msg)msg);

#define CFB_EXIT                                                        \
    if (inside)                                                         \
        GfxDisplay_CursorRender(csd, data->display);                   \
    ReleaseSemaphore(sem);                                             \
    return retval;

/******************************************************************************/

static OOP_Object *cursorfb_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct CursorFBData *data = OOP_INST_DATA(cl, o);
        data->realbm = NULL;
        data->display = NULL;
    }
    return o;
}

static VOID cursorfb_dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct CursorFBData *data = OOP_INST_DATA(cl, o);

    if (data->realbm)
    {
        OOP_DisposeObject(data->realbm);
        data->realbm = NULL;
    }
    OOP_DoSuperMethod(cl, o, msg);
}

static void cursorfb_get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct CursorFBData *data = OOP_INST_DATA(cl, o);

    /* Everything else comes from the wrapped real bitmap */
    OOP_DoMethod(data->realbm, (OOP_Msg)msg);
}

static IPTR cursorfb_set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct class_static_data *csd = CSD(cl);
    struct Library *UtilityBase = csd->cs_UtilityBase;
    struct CursorFBData *data = OOP_INST_DATA(cl, o);
    struct TagItem *tag, *tstate = msg->attrList;
    IPTR ret;

    /* Forward first, so the real bitmap programs its mode / scanout. */
    ret = OOP_DoMethod(data->realbm, (OOP_Msg)msg);

    /*
     * React to visibility changes: when the wrapped bitmap becomes the displayed
     * one, it is the framebuffer that carries the software pointer; when it is
     * hidden, the pointer must be detached from it. This is the driver-agnostic
     * way for the Display class to learn the active framebuffer without the
     * drivers having to do anything.
     */
    while ((tag = NextTagItem(&tstate)))
    {
        if (tag->ti_Tag == (HiddBitMapAttrBase + aoHidd_BitMap_Visible))
        {
            if (tag->ti_Data)
                GfxDisplay_CursorSetTarget(csd, data->display, data->realbm);
            else
                GfxDisplay_CursorUntarget(csd, data->display, data->realbm);
        }
    }

    return ret;
}

/* Default: forward to the real bitmap unchanged */
static IPTR cursorfb_fwd(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct Library *OOPBase = CSD(cl)->cs_OOPBase;
    struct CursorFBData *data = OOP_INST_DATA(cl, o);

    return OOP_DoMethod(data->realbm, msg);
}

/******************************************************************************/
/* Pixel-writing / reading operations, bracketed with pointer hide/show       */
/******************************************************************************/

static IPTR cursorfb_putpixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    CFB_INIT
    CFB_REMOVE(msg->x, msg->y, msg->x, msg->y)
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_drawpixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawPixel *msg)
{
    CFB_INIT
    CFB_REMOVE(msg->x, msg->y, msg->x, msg->y)
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_getpixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    CFB_INIT
    CFB_REMOVE(msg->x, msg->y, msg->x, msg->y)
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_drawline(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawLine *msg)
{
    WORD x1 = msg->x1, y1 = msg->y1, x2 = msg->x2, y2 = msg->y2, t;
    CFB_INIT
    if (x1 > x2) { t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { t = y1; y1 = y2; y2 = t; }
    CFB_REMOVE(x1, y1, x2, y2)
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_putimage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImage *msg)
{
    CFB_INIT
    CFB_REMOVE(msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1)
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_putalphaimage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutAlphaImage *msg)
{
    CFB_INIT
    CFB_REMOVE(msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1)
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_puttemplate(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutTemplate *msg)
{
    CFB_INIT
    CFB_REMOVE(msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1)
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_putalphatemplate(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutAlphaTemplate *msg)
{
    CFB_INIT
    CFB_REMOVE(msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1)
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_putpattern(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPattern *msg)
{
    CFB_INIT
    CFB_REMOVE(msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1)
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_getimage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{
    CFB_INIT
    CFB_REMOVE(msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1)
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_putimagelut(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImageLUT *msg)
{
    CFB_INIT
    CFB_REMOVE(msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1)
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_puttranspimagelut(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutTranspImageLUT *msg)
{
    CFB_INIT
    CFB_REMOVE(msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1)
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_getimagelut(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImageLUT *msg)
{
    CFB_INIT
    CFB_REMOVE(msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1)
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_drawrect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    CFB_INIT
    CFB_REMOVE(msg->minX, msg->minY, msg->maxX, msg->maxY)
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_fillrect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    CFB_INIT
    CFB_REMOVE(msg->minX, msg->minY, msg->maxX, msg->maxY)
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_drawellipse(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawEllipse *msg)
{
    CFB_INIT
    CFB_REMOVE(msg->x - msg->rx, msg->y - msg->ry, msg->x + msg->rx, msg->y + msg->ry)
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_fillellipse(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawEllipse *msg)
{
    CFB_INIT
    CFB_REMOVE(msg->x - msg->rx, msg->y - msg->ry, msg->x + msg->rx, msg->y + msg->ry)
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_drawpolygon(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawPolygon *msg)
{
    /* Be conservative: the polygon may be anywhere, always remove the pointer */
    CFB_INIT
    CFB_REMOVE_ALL
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_fillpolygon(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawPolygon *msg)
{
    CFB_INIT
    CFB_REMOVE_ALL
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_drawtext(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawText *msg)
{
    /* Text extent is not trivially known here; be safe and remove the pointer */
    CFB_INIT
    CFB_REMOVE_ALL
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_filltext(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawText *msg)
{
    CFB_INIT
    CFB_REMOVE_ALL
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_clear(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_Clear *msg)
{
    CFB_INIT
    CFB_REMOVE_ALL
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_blitcolexp(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_BlitColorExpansion *msg)
{
    CFB_INIT
    CFB_REMOVE(msg->destX, msg->destY, msg->destX + msg->width - 1, msg->destY + msg->height - 1)
    CFB_FORWARD
    CFB_EXIT
}

static IPTR cursorfb_scale(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_BitMapScale *msg)
{
    CFB_INIT
    CFB_REMOVE_ALL
    CFB_FORWARD
    CFB_EXIT
}

/******************************************************************************/
/* Class management                                                           */
/******************************************************************************/

OOP_Object *cursorfb_realbitmap(struct class_static_data *csd, OOP_Object *bm)
{
    struct Library *OOPBase = csd->cs_OOPBase;

    if (bm && (OOP_OCLASS(bm) == csd->cursorfbclass))
    {
        struct CursorFBData *data = OOP_INST_DATA(csd->cursorfbclass, bm);
        return data->realbm;
    }
    return NULL;
}

OOP_Object *cursorfb_display(struct class_static_data *csd, OOP_Object *bm)
{
    struct Library *OOPBase = csd->cs_OOPBase;

    if (bm && (OOP_OCLASS(bm) == csd->cursorfbclass))
    {
        struct CursorFBData *data = OOP_INST_DATA(csd->cursorfbclass, bm);
        return data->display;
    }
    return NULL;
}

OOP_Object *create_cursorfb(struct class_static_data *csd, OOP_Object *display, OOP_Object *realbm)
{
    struct Library *OOPBase = csd->cs_OOPBase;
    OOP_Object *o;

    if (!realbm)
        return NULL;

    o = OOP_NewObject(csd->cursorfbclass, NULL, NULL);
    if (o)
    {
        struct CursorFBData *data = OOP_INST_DATA(csd->cursorfbclass, o);
        data->realbm = realbm;
        data->display = display;
    }

    return o;
}

OOP_Class *init_cursorfbclass(struct class_static_data *csd)
{
    struct Library *OOPBase = csd->cs_OOPBase;
    OOP_Class *cl;

    struct OOP_MethodDescr root_descr[num_Root_Methods + 1] =
    {
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_new    , moRoot_New     },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_dispose, moRoot_Dispose },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_get    , moRoot_Get     },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_set    , moRoot_Set     },
        {NULL, 0UL }
    };

    struct OOP_MethodDescr bitmap_descr[num_Hidd_BitMap_Methods + 1] =
    {
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_SetColors                },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_putpixel         , moHidd_BitMap_PutPixel                 },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_drawpixel        , moHidd_BitMap_DrawPixel                },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_putimage         , moHidd_BitMap_PutImage                 },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_putalphaimage    , moHidd_BitMap_PutAlphaImage            },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_puttemplate      , moHidd_BitMap_PutTemplate              },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_putalphatemplate , moHidd_BitMap_PutAlphaTemplate         },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_putpattern       , moHidd_BitMap_PutPattern               },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_getimage         , moHidd_BitMap_GetImage                 },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_getpixel         , moHidd_BitMap_GetPixel                 },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_drawline         , moHidd_BitMap_DrawLine                 },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_drawrect         , moHidd_BitMap_DrawRect                 },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fillrect         , moHidd_BitMap_FillRect                 },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_drawellipse      , moHidd_BitMap_DrawEllipse              },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fillellipse      , moHidd_BitMap_FillEllipse              },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_drawpolygon      , moHidd_BitMap_DrawPolygon              },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fillpolygon      , moHidd_BitMap_FillPolygon              },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_drawtext         , moHidd_BitMap_DrawText                 },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_filltext         , moHidd_BitMap_FillText                 },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_FillSpan                 },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_clear            , moHidd_BitMap_Clear                    },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_blitcolexp       , moHidd_BitMap_BlitColorExpansion       },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_MapColor                 },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_UnmapPixel               },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_putimagelut      , moHidd_BitMap_PutImageLUT              },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_puttranspimagelut, moHidd_BitMap_PutTranspImageLUT        },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_getimagelut      , moHidd_BitMap_GetImageLUT              },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_BytesPerLine             },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_ConvertPixels            },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_FillMemRect8             },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_FillMemRect16            },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_FillMemRect24            },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_FillMemRect32            },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_InvertMemRect            },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_CopyMemBox8              },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_CopyMemBox16             },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_CopyMemBox24             },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_CopyMemBox32             },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_CopyLUTMemBox16          },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_CopyLUTMemBox24          },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_CopyLUTMemBox32          },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_PutMem32Image8           },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_PutMem32Image16          },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_PutMem32Image24          },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_GetMem32Image8           },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_GetMem32Image16          },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_GetMem32Image24          },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_PutMemTemplate8          },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_PutMemTemplate16         },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_PutMemTemplate24         },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_PutMemTemplate32         },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_PutMemPattern8           },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_PutMemPattern16          },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_PutMemPattern24          },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_PutMemPattern32          },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_SetColorMap              },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_ObtainDirectAccess       },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_ReleaseDirectAccess      },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_scale            , moHidd_BitMap_BitMapScale              },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_PrivateSet               },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_SetRGBConversionFunction },
        {(IPTR (*)(OOP_Class *, OOP_Object *, OOP_MethodID *))cursorfb_fwd              , moHidd_BitMap_UpdateRect               },
        {NULL, 0UL }
    };

    struct OOP_InterfaceDescr ifdescr[] =
    {
        {root_descr  , IID_Root        , num_Root_Methods        },
        {bitmap_descr, IID_Hidd_BitMap , num_Hidd_BitMap_Methods },
        {NULL        , NULL            , 0                       }
    };

    OOP_AttrBase MetaAttrBase = OOP_GetAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID        , (IPTR)CLID_Root                    },
        {aMeta_InterfaceDescr , (IPTR)ifdescr                      },
        {aMeta_InstSize       , (IPTR)sizeof(struct CursorFBData)  },
        {TAG_DONE             , 0UL                                }
    };

    cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
    if (cl)
    {
        /* CSD() recovers the library base from cl->UserData, so point it at the
           IntHIDDGraphicsBase that embeds this class_static_data. */
        cl->UserData = (APTR)((UBYTE *)csd - offsetof(struct IntHIDDGraphicsBase, hdg_csd));
        csd->cursorfbclass = cl;
    }
    return cl;
}
