/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <graphics/view.h>
#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/graphics.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <stdlib.h>
#include <string.h>

#include <zune/qrcode.h>
#include "qrcode_private.h"
#include "qrcodegen.h"

#define QRCODE_TEXT_LIMIT 7089UL
#define QRCODE_BYTE_LIMIT 2953UL
#define QRCODE_QUIET_MODULES 4

/* Native MUI pen specifications: text for modules, area background for
   empty modules and the standard four-module quiet zone. */
static const struct MUI_PenSpec defaultForeground = { "m5" }; /* MPEN_TEXT */
static const struct MUI_PenSpec defaultBackground = { "m2" }; /* MPEN_BACKGROUND */

/* Free the current encoded matrix and set status to Empty.
   Preserve the copied input, input mode, and requested ECC level. */
static void clearMatrix(struct QRCode_DATA *data)
{
    if (data->matrix != NULL)
    {
        FreeVec(data->matrix);
        data->matrix = NULL;
    }
    data->status = MUIV_QRCode_Status_Empty;
}

/* Replace the owned input and discard the previous matrix before validation.
   An empty payload succeeds and leaves the object Empty. A nonempty
   payload is copied, with a private terminator for text input. Invalid
   length/pointer or allocation failure returns FALSE and sets status;
   the previous input is not restored. */
static BOOL replaceInput(struct QRCode_DATA *data, const UBYTE *bytes,
                         ULONG length, BOOL isText)
{
    UBYTE *replacement = NULL;
    ULONG limit = isText ? QRCODE_TEXT_LIMIT : QRCODE_BYTE_LIMIT;

    clearMatrix(data);
    if (data->input != NULL)
        FreeVec(data->input);
    data->input = NULL;
    data->length = 0;
    data->textInput = isText;

    if (length == 0)
        return TRUE;
    if (bytes == NULL || length > limit)
    {
        data->status = MUIV_QRCode_Status_Invalid;
        return FALSE;
    }
    replacement = AllocVec(length + (isText ? 1 : 0), MEMF_ANY);
    if (replacement == NULL)
    {
        data->status = MUIV_QRCode_Status_NoMemory;
        return FALSE;
    }
    CopyMem((APTR)bytes, replacement, length);
    if (isText)
        replacement[length] = 0;
    data->input = replacement;
    data->length = length;
    return TRUE;
}

/* Return the byte count of a NUL-terminated string, up to the limit.
   NULL means empty. Return limit + 1 if no terminator appears within
   7,090 inspected bytes. The caller must supply readable storage for
   the examined range; this function cannot validate an arbitrary pointer. */
static ULONG textLength(const char *text)
{
    ULONG n;
    if (text == NULL)
        return 0;
    for (n = 0; n <= QRCODE_TEXT_LIMIT; n++)
    {
        if (text[n] == '\0')
            return n;
    }
    return QRCODE_TEXT_LIMIT + 1;
}

/* Replace the current matrix using the owned input and exact ECC level.
   Empty input stays Empty; invalid ECC sets Invalid. Allocate separate
   scratch and output buffers, freeing scratch on every completed path.
   On success, retain the output matrix; on failure, release it and
   report NoMemory or NoCapacity. The copied input remains available
   for subsequent encoding attempts. No drawing occurs here. */
static void encodeCurrent(struct QRCode_DATA *data)
{
    UBYTE *tmp;
    UBYTE *qr;
    BOOL ok;

    clearMatrix(data);
    if (data->ecc > MUIV_QRCode_ECC_H)
    {
        data->status = MUIV_QRCode_Status_Invalid;
        return;
    }
    if (data->length == 0)
        return;
    tmp = AllocVec(qrcodegen_BUFFER_LEN_MAX, MEMF_ANY);
    qr = AllocVec(qrcodegen_BUFFER_LEN_MAX, MEMF_ANY);
    if (tmp == NULL || qr == NULL)
    {
        if (tmp != NULL) FreeVec(tmp);
        if (qr != NULL) FreeVec(qr);
        data->status = MUIV_QRCode_Status_NoMemory;
        return;
    }
    if (data->textInput)
    {
        ok = qrcodegen_encodeText((const char *)data->input, tmp, qr,
                                  (enum qrcodegen_Ecc)data->ecc,
                                  qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX,
                                  qrcodegen_Mask_AUTO, FALSE);
    }
    else
    {
        CopyMem(data->input, tmp, data->length);
        ok = qrcodegen_encodeBinary(tmp, (int)data->length, qr,
                                    (enum qrcodegen_Ecc)data->ecc,
                                    qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX,
                                    qrcodegen_Mask_AUTO, FALSE);
    }
    FreeVec(tmp);
    if (ok)
    {
        data->matrix = qr;
        data->status = MUIV_QRCode_Status_Ready;
    }
    else
    {
        FreeVec(qr);
        data->status = MUIV_QRCode_Status_NoCapacity;
    }
}

/* Stage a complete pair of pen specifications. When set up, obtain both
   new pens first; never discard the active pair on a bad specification,
   failed allocation or indistinguishable final screen pen indices.
   NULL resets the requested component to the normal MUI theme pen. */
static BOOL updateColours(struct QRCode_DATA *data, Object *obj,
                          BOOL hasForeground, const struct MUI_PenSpec *foreground,
                          BOOL hasBackground, const struct MUI_PenSpec *background,
                          BOOL hasTransparent, BOOL transparent)
{
    struct MUI_PenSpec nextForeground = data->foregroundSpec;
    struct MUI_PenSpec nextBackground = data->backgroundSpec;
    BOOL nextTransparent = hasTransparent ? transparent : data->transparent;

    if (hasForeground)
    {
        if (foreground && !memchr(foreground->buf, 0, sizeof(foreground->buf)))
            return FALSE;
        nextForeground = foreground ? *foreground : defaultForeground;
    }
    if (hasBackground)
    {
        if (background && !memchr(background->buf, 0, sizeof(background->buf)))
            return FALSE;
        nextBackground = background ? *background : defaultBackground;
    }
    if (data->darkPen != -1)
    {
        LONG nextDark = MUI_ObtainPen(muiRenderInfo(obj), &nextForeground, 0);
        LONG nextLight = -1;
        if (nextDark == -1)
            return FALSE;
        if (!nextTransparent)
        {
            nextLight = MUI_ObtainPen(muiRenderInfo(obj), &nextBackground, 0);
            if (nextLight == -1 || MUIPEN(nextDark) == MUIPEN(nextLight))
            {
                MUI_ReleasePen(muiRenderInfo(obj), nextDark);
                if (nextLight != -1)
                    MUI_ReleasePen(muiRenderInfo(obj), nextLight);
                return FALSE;
            }
        }
        MUI_ReleasePen(muiRenderInfo(obj), data->darkPen);
        MUI_ReleasePen(muiRenderInfo(obj), data->lightPen);
        data->darkPen = nextDark;
        data->lightPen = nextLight;
    }
    data->foregroundSpec = nextForeground;
    data->backgroundSpec = nextBackground;
    data->transparent = nextTransparent;
    return TRUE;
}

/* Collect QRCode attributes from one tag list. For repeated attributes,
   the last value wins. Color-only updates exchange pens and redraw without
   re-encoding; invalid pen specs leave the active colors intact. Supplying
   both Text and Data invalidates and clears the old content. An input
   replacement discards the previous content before validation, so it does
   not provide rollback on failure. Return TRUE if state changed. */
static BOOL applyTags(struct QRCode_DATA *data, Object *obj, struct TagItem *tags)
{
    struct TagItem *state = tags;
    struct TagItem *tag;
    const char *text = NULL;
    const struct MUI_QRCodePayload *payload = NULL;
    BOOL hasText = FALSE;
    BOOL hasData = FALSE;
    BOOL changed = FALSE;
    BOOL reencode = FALSE;
    BOOL hasForeground = FALSE, hasBackground = FALSE;
    BOOL hasTransparent = FALSE, transparent = FALSE;
    const struct MUI_PenSpec *foreground = NULL, *background = NULL;

    while ((tag = NextTagItem(&state)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_QRCode_ECC:
                data->ecc = (ULONG)tag->ti_Data;
                reencode = TRUE;
                changed = TRUE;
                break;
            case MUIA_QRCode_Text:
                text = (const char *)tag->ti_Data;
                hasText = TRUE;
                reencode = TRUE;
                changed = TRUE;
                break;
            case MUIA_QRCode_Data:
                payload = (const struct MUI_QRCodePayload *)tag->ti_Data;
                hasData = TRUE;
                reencode = TRUE;
                changed = TRUE;
                break;
            case MUIA_QRCode_ForegroundPenSpec:
                foreground = (const struct MUI_PenSpec *)tag->ti_Data;
                hasForeground = TRUE;
                break;
            case MUIA_QRCode_BackgroundPenSpec:
                background = (const struct MUI_PenSpec *)tag->ti_Data;
                hasBackground = TRUE;
                break;
            case MUIA_QRCode_Transparent:
                transparent = (BOOL)(tag->ti_Data != 0);
                hasTransparent = TRUE;
                break;
        }
    }
    if ((hasForeground || hasBackground || hasTransparent) &&
        updateColours(data, obj, hasForeground, foreground,
                      hasBackground, background, hasTransparent, transparent))
        changed = TRUE;
    if (!reencode)
        return changed;
    if (hasText && hasData)
    {
        replaceInput(data, NULL, 1, FALSE);
        return TRUE;
    }
    if (hasText)
    {
        if (!replaceInput(data, (const UBYTE *)text, textLength(text), TRUE))
            return TRUE;
    }
    else if (hasData)
    {
        if (payload == NULL)
            replaceInput(data, NULL, 0, FALSE);
        else if (!replaceInput(data, payload->Bytes, payload->Length, FALSE))
            return TRUE;
    }
    encodeCurrent(data);
    return TRUE;
}

/* Initialize one class instance after successful superclass creation.
   Start with ECC M, theme-aware color specs, Empty status, and no pens.
   Apply the initial tag list before returning the object. Encoding
   errors remain observable through Status; they do not cause OM_NEW
   to discard an otherwise successfully created instance. */
IPTR QRCode__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    struct QRCode_DATA *data;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (obj == NULL)
        return 0;
    data = INST_DATA(cl, obj);
    memset(data, 0, sizeof(*data));
    data->ecc = MUIV_QRCode_ECC_M;
    data->darkPen = -1;
    data->lightPen = -1;
    data->foregroundSpec = defaultForeground;
    data->backgroundSpec = defaultBackground;
    applyTags(data, obj, msg->ops_AttrList);
    return (IPTR)obj;
}

/* Apply QRCode attributes, then forward OM_SET to the superclass.
   Request a full object redraw when the QR matrix or colors changed.
   present. Return the superclass result; encoding failures are
   reported through MUIA_QRCode_Status rather than this return value. */
IPTR QRCode__OM_SET(Class *cl, Object *obj, struct opSet *msg)
{
    struct QRCode_DATA *data = INST_DATA(cl, obj);
    BOOL changed = applyTags(data, obj, msg->ops_AttrList);
    IPTR result = DoSuperMethodA(cl, obj, (Msg)msg);

    if (changed)
        MUI_Redraw(obj, MADF_DRAWOBJECT);
    return result;
}

/* Return the current ECC, encoding Status or borrowed pen specifications.
   Return TRUE for recognized attributes; forward other queries
   to the superclass. Status describes encoding, not draw visibility. */
IPTR QRCode__OM_GET(Class *cl, Object *obj, struct opGet *msg)
{
    struct QRCode_DATA *data = INST_DATA(cl, obj);

    if (msg->opg_AttrID == MUIA_QRCode_Status)
    {
        *msg->opg_Storage = (IPTR)data->status;
        return TRUE;
    }
    if (msg->opg_AttrID == MUIA_QRCode_ECC)
    {
        *msg->opg_Storage = (IPTR)data->ecc;
        return TRUE;
    }
    if (msg->opg_AttrID == MUIA_QRCode_ForegroundPenSpec)
    {
        *msg->opg_Storage = (IPTR)&data->foregroundSpec;
        return TRUE;
    }
    if (msg->opg_AttrID == MUIA_QRCode_BackgroundPenSpec)
    {
        *msg->opg_Storage = (IPTR)&data->backgroundSpec;
        return TRUE;
    }
    if (msg->opg_AttrID == MUIA_QRCode_Transparent)
    {
        *msg->opg_Storage = (IPTR)data->transparent;
        return TRUE;
    }
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/* Free the owned matrix and input before forwarding OM_DISPOSE.
   Screen pens belong to the Zune Setup/Cleanup lifecycle and are not
   acquired or released by this method. */
IPTR QRCode__OM_DISPOSE(Class *cl, Object *obj, Msg msg)
{
    struct QRCode_DATA *data = INST_DATA(cl, obj);

    clearMatrix(data);
    if (data->input != NULL)
        FreeVec(data->input);
    data->input = NULL;
    return DoSuperMethodA(cl, obj, msg);
}

/* Increase superclass minimum dimensions by 29 pixels and default
   dimensions by 185 pixels; permit unrestricted maximum growth.
   These bounds do not depend on the current encoded QR version. */
IPTR QRCode__MUIM_AskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    DoSuperMethodA(cl, obj, (Msg)msg);
    msg->MinMaxInfo->MinWidth += 29;
    msg->MinMaxInfo->MinHeight += 29;
    msg->MinMaxInfo->DefWidth += 185;
    msg->MinMaxInfo->DefHeight += 185;
    msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;
    return TRUE;
}

/* Set up the superclass and resolve the owned foreground/background MUI
   PenSpecs. Reject allocation failure or indistinguishable screen pens;
   release complete MUI pen tokens before unwinding superclass Setup. */
IPTR QRCode__MUIM_Setup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct QRCode_DATA *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl, obj, (Msg)msg))
        return FALSE;
    data->darkPen = MUI_ObtainPen(muiRenderInfo(obj), &data->foregroundSpec, 0);
    if (!data->transparent)
        data->lightPen = MUI_ObtainPen(muiRenderInfo(obj), &data->backgroundSpec, 0);
    if (data->darkPen == -1 ||
        (!data->transparent && (data->lightPen == -1 ||
         MUIPEN(data->darkPen) == MUIPEN(data->lightPen))))
    {
        MUI_ReleasePen(muiRenderInfo(obj), data->darkPen);
        MUI_ReleasePen(muiRenderInfo(obj), data->lightPen);
        data->darkPen = -1;
        data->lightPen = -1;
        DoSuperMethod(cl, obj, MUIM_Cleanup);
        return FALSE;
    }
    return TRUE;
}

/* Release both complete MUI pen tokens while render info is still valid.
   Theme pen tokens are borrowed; RGB allocations are released by MUI. */
IPTR QRCode__MUIM_Cleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct QRCode_DATA *data = INST_DATA(cl, obj);

    MUI_ReleasePen(muiRenderInfo(obj), data->darkPen);
    MUI_ReleasePen(muiRenderInfo(obj), data->lightPen);
    data->darkPen = -1;
    data->lightPen = -1;
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/* Draw the superclass first. On relevant draw requests, fill the area
   with the background pen. Render only a Ready matrix that fits with four
   quiet modules on every side at an integral pixel scale. Each run
   of dark modules is one filled rectangle; scaling never interpolates.
   Insufficient display size leaves the area blank without changing
   the encoding status or allocating another matrix. */
IPTR QRCode__MUIM_Draw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct QRCode_DATA *data = INST_DATA(cl, obj);
    LONG width, height, side, unit, size, originX, originY;
    LONG x, y;
    IPTR result = DoSuperMethodA(cl, obj, (Msg)msg);

    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        return result;
    if (data->darkPen < 0 || (!data->transparent && data->lightPen < 0))
        return result;
    width = _mwidth(obj);
    height = _mheight(obj);
    if (width <= 0 || height <= 0)
        return result;
    if (data->transparent)
    {
        /* Repaint the inherited Zune background on each full or update draw,
           preventing stale QR modules after data or color changes. */
        DoMethod(obj, MUIM_DrawBackground, _mleft(obj), _mtop(obj),
                 width, height, _mleft(obj), _mtop(obj), 0);
    }
    else
    {
        SetAPen(_rp(obj), MUIPEN(data->lightPen));
        RectFill(_rp(obj), _mleft(obj), _mtop(obj),
                 _mleft(obj) + width - 1, _mtop(obj) + height - 1);
    }
    if (data->matrix == NULL || data->status != MUIV_QRCode_Status_Ready)
        return result;
    size = qrcodegen_getSize(data->matrix);
    side = size + 2 * QRCODE_QUIET_MODULES;
    unit = (width < height ? width : height) / side;
    if (unit < 1)
        return result;
    originX = _mleft(obj) + (width - unit * side) / 2 + QRCODE_QUIET_MODULES * unit;
    originY = _mtop(obj) + (height - unit * side) / 2 + QRCODE_QUIET_MODULES * unit;

    SetAPen(_rp(obj), MUIPEN(data->darkPen));
    for (y = 0; y < size; y++)
    {
        for (x = 0; x < size; )
        {
            LONG start;
            while (x < size && !qrcodegen_getModule(data->matrix, x, y)) x++;
            start = x;
            while (x < size && qrcodegen_getModule(data->matrix, x, y)) x++;
            if (start < x)
                RectFill(_rp(obj), originX + start * unit,
                         originY + y * unit, originX + x * unit - 1,
                         originY + (y + 1) * unit - 1);
        }
    }
    return result;
}
