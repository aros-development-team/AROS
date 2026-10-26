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

/* Collect QRCode attributes from one tag list and re-encode at most once.
   For repeated attributes of one kind, the last value wins. Supplying
   both Text and Data invalidates and clears the old content. An input
   replacement discards the previous content before validation, so this
   operation does not provide rollback on failure. Return TRUE if any
   recognized QRCode attribute was supplied, FALSE otherwise. */
static BOOL applyTags(struct QRCode_DATA *data, struct TagItem *tags)
{
    struct TagItem *state = tags;
    struct TagItem *tag;
    const char *text = NULL;
    const struct MUI_QRCodePayload *payload = NULL;
    BOOL hasText = FALSE;
    BOOL hasData = FALSE;
    BOOL changed = FALSE;

    while ((tag = NextTagItem(&state)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_QRCode_ECC:
                data->ecc = (ULONG)tag->ti_Data;
                changed = TRUE;
                break;
            case MUIA_QRCode_Text:
                text = (const char *)tag->ti_Data;
                hasText = TRUE;
                changed = TRUE;
                break;
            case MUIA_QRCode_Data:
                payload = (const struct MUI_QRCodePayload *)tag->ti_Data;
                hasData = TRUE;
                changed = TRUE;
                break;
        }
    }
    if (!changed)
        return FALSE;
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
   Start with ECC M, Empty status, and no owned input, matrix, or pens.
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
    applyTags(data, msg->ops_AttrList);
    return (IPTR)obj;
}

/* Apply QRCode attributes, then forward OM_SET to the superclass.
   Request a full object redraw if a recognized QRCode attribute was
   present. Return the superclass result; encoding failures are
   reported through MUIA_QRCode_Status rather than this return value. */
IPTR QRCode__OM_SET(Class *cl, Object *obj, struct opSet *msg)
{
    struct QRCode_DATA *data = INST_DATA(cl, obj);
    BOOL changed = applyTags(data, msg->ops_AttrList);
    IPTR result = DoSuperMethodA(cl, obj, (Msg)msg);

    if (changed)
        MUI_Redraw(obj, MADF_DRAWOBJECT);
    return result;
}

/* Return the current requested ECC value or encoding Status to the caller.
   Return TRUE for either supported attribute; forward other queries
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

/* Set up the superclass, then acquire dark and light screen pens.
   Fail if either reservation fails or both resolve to one pen index.
   Release each successful reservation (including equal indices),
   reset both sentinels, and clean up the superclass before returning
   FALSE. On success, Cleanup owns the two pen reservations. */
IPTR QRCode__MUIM_Setup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct QRCode_DATA *data = INST_DATA(cl, obj);
    struct ColorMap *cm;

    if (!DoSuperMethodA(cl, obj, (Msg)msg))
        return FALSE;
    cm = _screen(obj)->ViewPort.ColorMap;
    data->darkPen = ObtainBestPen(cm, 0, 0, 0, TAG_DONE);
    data->lightPen = ObtainBestPen(cm, 0xffffffffUL, 0xffffffffUL,
                                   0xffffffffUL, TAG_DONE);
    if (data->darkPen < 0 || data->lightPen < 0 ||
        data->darkPen == data->lightPen)
    {
        if (data->darkPen >= 0) ReleasePen(cm, data->darkPen);
        if (data->lightPen >= 0) ReleasePen(cm, data->lightPen);
        data->darkPen = -1;
        data->lightPen = -1;
        DoSuperMethod(cl, obj, MUIM_Cleanup);
        return FALSE;
    }
    return TRUE;
}

/* Release each acquired screen pen against the active screen colormap.
   Reset both pen sentinels, then delegate Zune cleanup to the
   superclass. Each successful pen obtain owns one release. */
IPTR QRCode__MUIM_Cleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    struct QRCode_DATA *data = INST_DATA(cl, obj);
    struct ColorMap *cm = _screen(obj)->ViewPort.ColorMap;

    if (data->darkPen >= 0) ReleasePen(cm, data->darkPen);
    if (data->lightPen >= 0) ReleasePen(cm, data->lightPen);
    data->darkPen = -1;
    data->lightPen = -1;
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/* Draw the superclass first. On relevant draw requests, fill the area
   with the light pen. Render only a Ready matrix that fits with four
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
    if (data->lightPen < 0 || data->darkPen < 0)
        return result;
    width = _mwidth(obj);
    height = _mheight(obj);
    if (width <= 0 || height <= 0)
        return result;
    SetAPen(_rp(obj), data->lightPen);
    RectFill(_rp(obj), _mleft(obj), _mtop(obj),
             _mleft(obj) + width - 1, _mtop(obj) + height - 1);
    if (data->matrix == NULL || data->status != MUIV_QRCode_Status_Ready)
        return result;
    size = qrcodegen_getSize(data->matrix);
    side = size + 2 * QRCODE_QUIET_MODULES;
    unit = (width < height ? width : height) / side;
    if (unit < 1)
        return result;
    originX = _mleft(obj) + (width - unit * side) / 2 + QRCODE_QUIET_MODULES * unit;
    originY = _mtop(obj) + (height - unit * side) / 2 + QRCODE_QUIET_MODULES * unit;

    SetAPen(_rp(obj), data->darkPen);
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
