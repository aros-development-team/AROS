/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: PrefsObjects - strings, numbers, dates, binary data, arrays and
          dictionaries, reference counted, for application preferences.
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <utility/date.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <string.h>

#include "application_intern.h"
#include "application_po.h"
#include LC_LIBDEFS_FILE

/* ---------------------------------------------------------------------- */
/* Objects                                                                */

static struct PObj *po_Alloc(ULONG type)
{
    struct PObj *o;

    if (!(o = AllocVec(sizeof(*o), MEMF_ANY | MEMF_CLEAR)))
        return NULL;
    o->po_Type = type;
    o->po_Retain = 1;
    if (type == ALPOT_String)
    {
        if (!(o->u.str.s = AllocVec(1, MEMF_ANY | MEMF_CLEAR)))
        {
            FreeVec(o);
            return NULL;
        }
    }
    return o;
}

static void po_FreeContents(struct PObj *o)
{
    LONG i;

    switch (o->po_Type)
    {
    case ALPOT_String:
        FreeVec(o->u.str.s);
        o->u.str.s = NULL;
        o->u.str.len = 0;
        break;
    case ALPOT_Binary:
        FreeVec(o->u.bin.data);
        o->u.bin.data = NULL;
        o->u.bin.size = 0;
        break;
    case ALPOT_Array:
    case ALPOT_Dictionary:
        for (i = 0; i < o->u.con.count; i++)
        {
            po_Release((PrefsObject *)o->u.con.items[i]);
            if (o->u.con.keys)
                FreeVec(o->u.con.keys[i]);
        }
        o->u.con.count = 0;
        break;
    }
}

void po_Release(PrefsObject *obj)
{
    struct PObj *o = PO(obj);

    if (!o || --o->po_Retain > 0)
        return;
    po_FreeContents(o);
    if (o->po_Type == ALPOT_Array || o->po_Type == ALPOT_Dictionary)
    {
        FreeVec(o->u.con.items);
        FreeVec(o->u.con.keys);
    }
    FreeVec(o);
}

ULONG po_Type(PrefsObject *obj)
{
    return obj ? PO(obj)->po_Type : ALPOT_None;
}

static BOOL po_IsContainer(struct PObj *o)
{
    return o && (o->po_Type == ALPOT_Array || o->po_Type == ALPOT_Dictionary);
}

LONG po_Count(PrefsObject *obj)
{
    return po_IsContainer(PO(obj)) ? PO(obj)->u.con.count : 0;
}

PrefsObject *po_At(PrefsObject *obj, LONG index)
{
    struct PObj *o = PO(obj);

    if (!po_IsContainer(o) || index < 0 || index >= o->u.con.count)
        return NULL;
    return (PrefsObject *)o->u.con.items[index];
}

static BOOL po_Grow(struct PObj *o)
{
    struct PObj **items;
    STRPTR *keys = NULL;
    LONG cap;

    if (o->u.con.count < o->u.con.cap)
        return TRUE;
    cap = o->u.con.cap ? o->u.con.cap * 2 : 8;
    if (!(items = AllocVec(cap * sizeof(*items), MEMF_ANY)))
        return FALSE;
    if (o->po_Type == ALPOT_Dictionary && !(keys = AllocVec(cap * sizeof(*keys), MEMF_ANY)))
    {
        FreeVec(items);
        return FALSE;
    }
    if (o->u.con.count)
    {
        CopyMem(o->u.con.items, items, o->u.con.count * sizeof(*items));
        if (keys)
            CopyMem(o->u.con.keys, keys, o->u.con.count * sizeof(*keys));
    }
    FreeVec(o->u.con.items);
    FreeVec(o->u.con.keys);
    o->u.con.items = items;
    o->u.con.keys = keys;
    o->u.con.cap = cap;
    return TRUE;
}

/* Takes over the caller's reference to item; key is copied. */
static ULONG po_Insert(struct PObj *o, LONG index, struct PObj *item, CONST_STRPTR key)
{
    STRPTR kcopy = NULL;
    LONG i;

    if (!item)
        return ALPOEF_MISSING_OBJECT;
    if (index < 0 || index > o->u.con.count)
        return ALPOEF_ILLEGAL_PARAMETER;
    if (o->po_Type == ALPOT_Dictionary)
    {
        if (!key)
            return ALPOEF_ILLEGAL_PARAMETER;
        if (!(kcopy = app_StrDup(key)))
            return ALPOEF_MEMORY;
    }
    if (!po_Grow(o))
    {
        FreeVec(kcopy);
        return ALPOEF_MEMORY;
    }
    for (i = o->u.con.count; i > index; i--)
    {
        o->u.con.items[i] = o->u.con.items[i - 1];
        if (o->u.con.keys)
            o->u.con.keys[i] = o->u.con.keys[i - 1];
    }
    o->u.con.items[index] = item;
    if (o->u.con.keys)
        o->u.con.keys[index] = kcopy;
    o->u.con.count++;
    return 0;
}

/* Slot index takes over the caller's reference to item. The object already
   there may be item itself: then the extra reference is dropped, unless it
   is the container's only one, which a caller passing a borrowed pointer
   did not have. */
static void po_Replace(struct PObj *o, LONG index, struct PObj *item)
{
    if (o->u.con.items[index] == item)
    {
        if (item->po_Retain > 1)
            item->po_Retain--;
        return;
    }
    po_Release((PrefsObject *)o->u.con.items[index]);
    o->u.con.items[index] = item;
}

static void po_RemoveAt(struct PObj *o, LONG index)
{
    LONG i;

    po_Release((PrefsObject *)o->u.con.items[index]);
    if (o->u.con.keys)
        FreeVec(o->u.con.keys[index]);
    for (i = index; i < o->u.con.count - 1; i++)
    {
        o->u.con.items[i] = o->u.con.items[i + 1];
        if (o->u.con.keys)
            o->u.con.keys[i] = o->u.con.keys[i + 1];
    }
    o->u.con.count--;
}

static LONG po_KeyIndex(struct PObj *o, CONST_STRPTR key)
{
    LONG i;

    if (!key || o->po_Type != ALPOT_Dictionary)
        return -1;
    for (i = 0; i < o->u.con.count; i++)
    {
        if (strcmp(o->u.con.keys[i], key) == 0)
            return i;
    }
    return -1;
}

static LONG po_ObjIndex(struct PObj *o, struct PObj *item)
{
    LONG i;

    for (i = 0; i < o->u.con.count; i++)
    {
        if (o->u.con.items[i] == item)
            return i;
    }
    return -1;
}

static ULONG po_SetString(struct PObj *o, CONST_STRPTR s, LONG len)
{
    STRPTR copy;

    if (!s)
        s = "";
    if (len < 0)
        len = strlen(s);
    if (!(copy = AllocVec(len + 1, MEMF_ANY)))
        return ALPOEF_MEMORY;
    CopyMem((APTR)s, copy, len);
    copy[len] = '\0';
    FreeVec(o->u.str.s);
    o->u.str.s = copy;
    o->u.str.len = len;
    return 0;
}

static ULONG po_SetBinary(struct PObj *o, CONST UBYTE *data, LONG size)
{
    UBYTE *copy = NULL;

    if (size < 0)
        return ALPOEF_ILLEGAL_PARAMETER;
    if (size && !(copy = AllocVec(size, MEMF_ANY | MEMF_CLEAR)))
        return ALPOEF_MEMORY;
    if (copy && data)
        CopyMem((APTR)data, copy, size);
    FreeVec(o->u.bin.data);
    o->u.bin.data = copy;
    o->u.bin.size = size;
    return 0;
}

PrefsObject *po_Copy(PrefsObject *obj)
{
    struct PObj *o = PO(obj), *c;
    LONG i;

    if (!o || !(c = po_Alloc(o->po_Type)))
        return NULL;

    switch (o->po_Type)
    {
    case ALPOT_String:
        if (po_SetString(c, o->u.str.s, o->u.str.len))
            goto fail;
        break;
    case ALPOT_Number:
        c->u.num = o->u.num;
        break;
    case ALPOT_Date:
        c->u.date = o->u.date;
        break;
    case ALPOT_Binary:
        if (po_SetBinary(c, o->u.bin.data, o->u.bin.size))
            goto fail;
        break;
    case ALPOT_Array:
    case ALPOT_Dictionary:
        for (i = 0; i < o->u.con.count; i++)
        {
            struct PObj *item = PO(po_Copy((PrefsObject *)o->u.con.items[i]));

            if (!item)
                goto fail;
            if (po_Insert(c, i, item, o->u.con.keys ? o->u.con.keys[i] : NULL))
            {
                po_Release((PrefsObject *)item);
                goto fail;
            }
        }
        break;
    }
    return (PrefsObject *)c;

fail:
    po_Release((PrefsObject *)c);
    return NULL;
}

/* ---------------------------------------------------------------------- */
/* Helpers used inside the library                                         */

PrefsObject *po_New(ULONG type) { return (PrefsObject *)po_Alloc(type); }
PrefsObject *po_NewDict(void)   { return (PrefsObject *)po_Alloc(ALPOT_Dictionary); }
PrefsObject *po_NewArray(void)  { return (PrefsObject *)po_Alloc(ALPOT_Array); }

PrefsObject *po_NewString(CONST_STRPTR s)
{
    struct PObj *o = po_Alloc(ALPOT_String);

    if (o && po_SetString(o, s, -1))
    {
        po_Release((PrefsObject *)o);
        o = NULL;
    }
    return (PrefsObject *)o;
}

PrefsObject *po_NewLong(LONG v)
{
    struct PObj *o = po_Alloc(ALPOT_Number);

    if (o)
    {
        o->u.num.type = ALPONUMT_Long;
        o->u.num.l = v;
    }
    return (PrefsObject *)o;
}

static void po_SetNow(struct PObj *o)
{
    struct DateStamp ds;

    DateStamp(&ds);
    o->u.date.secs = ds.ds_Days * 86400 + ds.ds_Minute * 60 + ds.ds_Tick / TICKS_PER_SECOND;
    o->u.date.micro = (ds.ds_Tick % TICKS_PER_SECOND) * (1000000 / TICKS_PER_SECOND);
}

PrefsObject *po_NewNow(void)
{
    struct PObj *o = po_Alloc(ALPOT_Date);

    if (o)
        po_SetNow(o);
    return (PrefsObject *)o;
}

BOOL po_ArrayInsert(PrefsObject *arr, PrefsObject *item, LONG index)
{
    struct PObj *o = PO(arr);

    if (!o || o->po_Type != ALPOT_Array)
        return FALSE;
    if (index < 0 || index > o->u.con.count)
        index = o->u.con.count;
    return po_Insert(o, index, PO(item), NULL) == 0;
}

void po_ArrayRemove(PrefsObject *arr, LONG index)
{
    struct PObj *o = PO(arr);

    if (po_IsContainer(o) && index >= 0 && index < o->u.con.count)
        po_RemoveAt(o, index);
}

void po_Clear(PrefsObject *obj)
{
    if (po_IsContainer(PO(obj)))
        po_FreeContents(PO(obj));
}

BOOL po_DictSet(PrefsObject *dict, CONST_STRPTR key, PrefsObject *item)
{
    struct PObj *o = PO(dict);
    LONG i;

    if (!o || o->po_Type != ALPOT_Dictionary || !key || !item)
        return FALSE;
    if ((i = po_KeyIndex(o, key)) >= 0)
    {
        po_Replace(o, i, PO(item));
        return TRUE;
    }
    return po_Insert(o, o->u.con.count, PO(item), key) == 0;
}

PrefsObject *po_DictGet(PrefsObject *dict, CONST_STRPTR key)
{
    struct PObj *o = PO(dict);
    LONG i;

    if (!o || (i = po_KeyIndex(o, key)) < 0)
        return NULL;
    return (PrefsObject *)o->u.con.items[i];
}

CONST_STRPTR po_DictString(PrefsObject *dict, CONST_STRPTR key, CONST_STRPTR def)
{
    struct PObj *o = PO(po_DictGet(dict, key));

    return (o && o->po_Type == ALPOT_String) ? o->u.str.s : def;
}

/* a double as a LONG, saturated instead of undefined out of range */
static LONG po_DoubleToLong(double d)
{
    if (d >= 2147483647.0)
        return 0x7fffffff;
    if (d <= -2147483648.0)
        return (LONG)0x80000000;
    return (LONG)d;
}

static BOOL po_NumberLong(struct PObj *o, LONG *v)
{
    if (!o || o->po_Type != ALPOT_Number)
        return FALSE;
    *v = (o->u.num.type == ALPONUMT_Double) ? po_DoubleToLong(o->u.num.d) : o->u.num.l;
    return TRUE;
}

LONG po_DictLong(PrefsObject *dict, CONST_STRPTR key, LONG def)
{
    LONG v;

    return po_NumberLong(PO(po_DictGet(dict, key)), &v) ? v : def;
}

/* Swap the contents of two dictionaries, then empty 'from'. */
BOOL po_MoveContents(PrefsObject *to, PrefsObject *from)
{
    struct PObj *t = PO(to), *f = PO(from), tmp;

    if (!t || !f || t->po_Type != f->po_Type || !po_IsContainer(t))
        return FALSE;
    tmp.u.con = t->u.con;
    t->u.con = f->u.con;
    f->u.con = tmp.u.con;
    po_FreeContents(f);
    return TRUE;
}

/* ---------------------------------------------------------------------- */
/* Numbers and dates as text                                               */

void po_FormatLong(STRPTR buf, LONG v)
{
    TEXT tmp[16], *p = tmp + sizeof(tmp) - 1;
    ULONG u = v < 0 ? -(ULONG)v : (ULONG)v;

    *p = '\0';
    do
        *--p = '0' + (u % 10);
    while (u /= 10);
    if (v < 0)
        *--p = '-';
    strcpy(buf, p);
}

#define PO_DBL_MAX      1.7976931348623157e308
#define PO_EXP_MAX      400     /* beyond any double, after the digits */

/* v times 10^e in the widest type there is: with the x87's 64-bit
   mantissa a 19-digit integer and the powers up to 1e27 are exact, so the
   result rounds once; elsewhere long double may be a plain double. */
static long double po_Scale(long double v, LONG e)
{
    static const long double p10[] = { 1e0L, 1e1L, 1e2L, 1e3L, 1e4L, 1e5L, 1e6L, 1e7L, 1e8L,
                                       1e9L, 1e10L, 1e11L, 1e12L, 1e13L, 1e14L, 1e15L, 1e16L,
                                       1e17L, 1e18L, 1e19L, 1e20L, 1e21L, 1e22L, 1e23L, 1e24L,
                                       1e25L, 1e26L, 1e27L };

    while (e > 27)
    {
        v *= 1e27L;
        e -= 27;
    }
    while (e < -27)
    {
        v /= 1e27L;
        e += 27;
    }
    return e < 0 ? v / p10[-e] : v * p10[e];
}

static CONST_STRPTR po_SkipSpace(CONST_STRPTR s)
{
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
        s++;
    return s;
}

/* A decimal number, optionally with a fraction and exponent. Up to 19
   digits are gathered exactly in an integer and scaled once; an exponent
   beyond any double is clamped, so the value is finite and the loop
   short. */
BOOL po_ParseNumber(CONST_STRPTR s, LONG *l, double *d, BOOL *isdouble)
{
    UQUAD q = 0;
    double v;
    BOOL neg = FALSE, any = FALSE, frac = FALSE;
    LONG ex = 0, shift = 0, digits = 0;

    s = po_SkipSpace(s);
    if (*s == '-' || *s == '+')
        neg = (*s++ == '-');
    for (;;)
    {
        if (*s == '.' && !frac)
        {
            frac = TRUE;
            s++;
            continue;
        }
        if (*s < '0' || *s > '9')
            break;
        any = TRUE;
        if (digits < 19)
        {
            q = q * 10 + (*s - '0');
            if (q)
                digits++;
            if (frac)
                shift--;
        }
        else if (!frac)
            shift++;
        s++;
    }
    if (any && (*s == 'e' || *s == 'E'))
    {
        BOOL exneg = FALSE;

        s++;
        frac = TRUE;
        if (*s == '-' || *s == '+')
            exneg = (*s++ == '-');
        while (*s >= '0' && *s <= '9')
        {
            if (ex < PO_EXP_MAX)
                ex = ex * 10 + (*s - '0');
            s++;
        }
        if (exneg)
            ex = -ex;
    }
    if (!any || *po_SkipSpace(s))
        return FALSE;
    ex += shift;
    if (ex > PO_EXP_MAX)
        ex = PO_EXP_MAX;
    if (ex < -PO_EXP_MAX)
        ex = -PO_EXP_MAX;
    v = q ? (double)po_Scale((long double)q, ex) : 0;
    if (v > PO_DBL_MAX)
        v = PO_DBL_MAX;
    if (neg)
        v = -v;
    *d = v;
    *l = po_DoubleToLong(v);
    *isdouble = frac;
    return TRUE;
}

/* digits[0..n) with the decimal point after position e+1 (plain notation
   for -5 <= e < 17, else d.ddde<exp>), as po_ParseNumber() reads it; a
   whole number keeps a ".0" so that the text stays a real */
static void po_PutDigits(STRPTR buf, const TEXT *digits, LONG n, LONG e)
{
    LONG i, point;

    if (e >= -5 && e < 17)
    {
        point = e + 1;
        if (e < 0)
        {
            *buf++ = '0';
            *buf++ = '.';
            for (i = -1; i > e; i--)
                *buf++ = '0';
            point = 0;
        }
        else
        {
            for (i = 0; i < point; i++)
                *buf++ = i < n ? digits[i] : '0';
            *buf++ = '.';
            if (n <= point)
                *buf++ = '0';
        }
        for (i = point; i < n; i++)
            *buf++ = digits[i];
        *buf = '\0';
        return;
    }
    *buf++ = digits[0];
    if (n > 1)
    {
        *buf++ = '.';
        for (i = 1; i < n; i++)
            *buf++ = digits[i];
    }
    *buf++ = 'e';
    po_FormatLong(buf, e);
}

/* The shortest of 15, 16 and 17 significant digits that po_ParseNumber()
   reads back as the same double, then the 17-digit neighbours; the plain
   17 digits when none does. */
void po_FormatDouble(STRPTR buf, double d)
{
    static const LONG step[] = { 0, 1, -1, 2, -2 };
    UQUAD q, r;
    LONG e = 0, ee, n, i, prec, try;
    TEXT digits[18];
    STRPTR out = buf;
    double m, back;
    BOOL isd;
    LONG dummy;

    if (d != d)
    {
        strcpy(buf, "0");
        return;
    }
    if (d < 0)
    {
        *out++ = '-';
        d = -d;
    }
    if (d > PO_DBL_MAX)
        d = PO_DBL_MAX;
    if (d == 0)
    {
        strcpy(buf, "0");
        return;
    }
    m = d;
    while (m >= 10)
    {
        m = (double)po_Scale(m, -1);
        e++;
    }
    while (m < 1)
    {
        m = (double)po_Scale(m, 1);
        e--;
    }
    q = (UQUAD)(po_Scale(d, 16 - e) + 0.5L);
    if (q >= (UQUAD)100000000000000000ULL)
    {
        q /= 10;
        e++;
    }
    else if (q < (UQUAD)10000000000000000ULL)
    {
        q *= 10;
        e--;
    }
    for (try = 0; try < 7; try++)
    {
        UQUAD div = 1;

        /* 15, 16 and 17 digits, then 17 digits one or two units off */
        prec = try < 2 ? 15 + try : 17;
        ee = e;
        for (i = prec; i < 17; i++)
            div *= 10;
        r = (q + div / 2) / div;
        for (i = prec; i < 17; i++)
            r *= 10;
        if (try >= 2)
            r += step[try - 2];
        if (r >= (UQUAD)100000000000000000ULL)
        {
            r /= 10;
            ee++;
        }
        else if (r < (UQUAD)10000000000000000ULL)
        {
            r *= 10;
            ee--;
        }
        for (i = 16; i >= 0; i--)
        {
            digits[i] = '0' + (r % 10);
            r /= 10;
        }
        for (n = 17; n > 1 && digits[n - 1] == '0'; n--)
            ;
        po_PutDigits(out, digits, n, ee);
        if (po_ParseNumber(out, &dummy, &back, &isd) && back == d)
            return;
    }
    /* none read back exactly: the plain 17 digits */
    for (r = q, i = 16; i >= 0; i--)
    {
        digits[i] = '0' + (r % 10);
        r /= 10;
    }
    po_PutDigits(out, digits, 17, e);
}

/* "YYYY-MM-DD HH:MM:SS", local time like the rest of AmigaDOS */
void po_FormatDate(STRPTR buf, ULONG secs)
{
    struct ClockData cd;
    ULONG v[6];
    LONG i, w[6] = { 4, 2, 2, 2, 2, 2 };
    TEXT sep[6] = { '-', '-', ' ', ':', ':', '\0' };

    Amiga2Date(secs, &cd);
    v[0] = cd.year; v[1] = cd.month; v[2] = cd.mday;
    v[3] = cd.hour; v[4] = cd.min;   v[5] = cd.sec;
    for (i = 0; i < 6; i++)
    {
        LONG j;

        for (j = w[i] - 1; j >= 0; j--)
        {
            buf[j] = '0' + v[i] % 10;
            v[i] /= 10;
        }
        buf += w[i];
        *buf++ = sep[i];
    }
}

BOOL po_ParseDate(CONST_STRPTR s, ULONG *secs)
{
    struct ClockData cd;
    ULONG v[6] = { 0 };
    LONG i, n;

    s = po_SkipSpace(s);
    for (i = 0; i < 6; i++)
    {
        for (n = 0; *s >= '0' && *s <= '9'; n++)
            v[i] = v[i] * 10 + (*s++ - '0');
        if (!n)
            break;
        if (*s == '-' || *s == ':' || *s == ' ' || *s == 'T')
            s++;
    }
    if (i == 1)
    {
        /* a bare number: seconds since 1978 */
        *secs = v[0];
        return TRUE;
    }
    if (i < 3)
        return FALSE;
    cd.year = v[0]; cd.month = v[1]; cd.mday = v[2];
    cd.hour = v[3]; cd.min = v[4];   cd.sec = v[5];
    cd.wday = 0;
    if (!(*secs = CheckDate(&cd)) && !(cd.year == 1978 && cd.month == 1 && cd.mday == 1))
        return FALSE;
    *secs = Date2Amiga(&cd);
    return TRUE;
}

/* ---------------------------------------------------------------------- */
/* Methods                                                                 */

static ULONG po_SetWithString(struct PObj *o, CONST_STRPTR s)
{
    LONG l;
    double d;
    BOOL isd;
    ULONG secs;

    if (!s)
        return ALPOEF_ILLEGAL_PARAMETER;
    switch (o->po_Type)
    {
    case ALPOT_String:
        return po_SetString(o, s, -1);
    case ALPOT_Number:
        if (Stricmp(s, "TRUE") == 0 || Stricmp(s, "FALSE") == 0)
        {
            o->u.num.type = ALPONUMT_Bool;
            o->u.num.l = (Stricmp(s, "TRUE") == 0);
            return 0;
        }
        if (!po_ParseNumber(s, &l, &d, &isd))
            return ALPOEF_ILLEGAL_PARAMETER;
        o->u.num.type = isd ? ALPONUMT_Double : ALPONUMT_Long;
        o->u.num.l = l;
        o->u.num.d = d;
        return 0;
    case ALPOT_Date:
        if (!po_ParseDate(s, &secs))
            return ALPOEF_ILLEGAL_PARAMETER;
        o->u.date.secs = secs;
        o->u.date.micro = 0;
        return 0;
    }
    return ALPOEF_IMPOSSIBLE_OP;
}

static ULONG po_GetAsString(struct PObj *o, struct ALPOString *as)
{
    TEXT buf[40];
    CONST_STRPTR src = buf;
    ULONG len;

    if (!as || !as->string || !as->length)
        return ALPOEF_ILLEGAL_PARAMETER;
    switch (o->po_Type)
    {
    case ALPOT_String:
        src = o->u.str.s;
        break;
    case ALPOT_Number:
        if (o->u.num.type == ALPONUMT_Bool)
            strcpy(buf, o->u.num.l ? "TRUE" : "FALSE");
        else if (o->u.num.type == ALPONUMT_Double)
            po_FormatDouble(buf, o->u.num.d);
        else
            po_FormatLong(buf, o->u.num.l);
        break;
    case ALPOT_Date:
        po_FormatDate(buf, o->u.date.secs);
        break;
    default:
        return ALPOEF_IMPOSSIBLE_OP;
    }
    len = strlen(src);
    if (len > as->length - 1)
        len = as->length - 1;
    CopyMem((APTR)src, as->string, len);
    as->string[len] = '\0';
    as->length = len;
    return 0;
}

static ULONG po_StringMethod(struct PObj *o, struct TagItem *tag)
{
    struct ALPOUniString *us = (struct ALPOUniString *)tag->ti_Data;
    ULONG i, err;
    STRPTR tmp;

    switch (tag->ti_Tag)
    {
    case ALPOSTR_Clear:
        return po_SetString(o, "", 0);
    case ALPOSTR_GetLength:
        if (tag->ti_Data)
            *(LONG *)tag->ti_Data = o->u.str.len;
        return 0;
    case ALPOSTR_SetString:
        return po_SetString(o, (CONST_STRPTR)tag->ti_Data, -1);
    case ALPOSTR_GetString:
        if (tag->ti_Data)
            *(STRPTR *)tag->ti_Data = o->u.str.s;
        return 0;
    case ALPOSTR_SetUniString:
        /* Kept as 8 bits per character; others become '?'. */
        if (!us || (!us->string && us->length))
            return ALPOEF_ILLEGAL_PARAMETER;
        if (!(tmp = AllocVec(us->length + 1, MEMF_ANY)))
            return ALPOEF_MEMORY;
        for (i = 0; i < us->length; i++)
            tmp[i] = us->string[i] < 256 ? us->string[i] : '?';
        err = po_SetString(o, tmp, us->length);
        FreeVec(tmp);
        return err;
    case ALPOSTR_GetUniString:
        if (!us || !us->string)
            return ALPOEF_ILLEGAL_PARAMETER;
        for (i = 0; i < o->u.str.len && i < us->length; i++)
            us->string[i] = (UBYTE)o->u.str.s[i];
        us->length = i;
        return 0;
    }
    return ALPOEF_UNKNOWN_METHOD;
}

static ULONG po_NumberMethod(struct PObj *o, struct TagItem *tag)
{
    IPTR v = tag->ti_Data;

    switch (tag->ti_Tag)
    {
    case ALPONUM_GetType:
        if (v)
            *(LONG *)v = o->u.num.type;
        return 0;
    case ALPONUM_ConvertToType:
        if (v == ALPONUMT_Double && o->u.num.type != ALPONUMT_Double)
            o->u.num.d = o->u.num.l;
        else if (v == ALPONUMT_Long && o->u.num.type == ALPONUMT_Double)
            o->u.num.l = po_DoubleToLong(o->u.num.d);
        else if (v == ALPONUMT_Bool)
            o->u.num.l = (o->u.num.type == ALPONUMT_Double) ? (o->u.num.d != 0) : (o->u.num.l != 0);
        else if (v != ALPONUMT_Long && v != ALPONUMT_Double)
            return ALPOEF_ILLEGAL_PARAMETER;
        o->u.num.type = v;
        return 0;
    case ALPONUM_SetLong:
        o->u.num.type = ALPONUMT_Long;
        o->u.num.l = (LONG)v;
        return 0;
    case ALPONUM_SetBool:
        o->u.num.type = ALPONUMT_Bool;
        o->u.num.l = v ? TRUE : FALSE;
        return 0;
    case ALPONUM_SetDouble:
        if (!v)
            return ALPOEF_ILLEGAL_PARAMETER;
        o->u.num.type = ALPONUMT_Double;
        o->u.num.d = *(double *)v;
        return 0;
    case ALPONUM_GetLong:
        if (v)
            po_NumberLong(o, (LONG *)v);
        return 0;
    case ALPONUM_GetBool:
        if (v)
            *(BOOL *)v = (o->u.num.type == ALPONUMT_Double) ? (o->u.num.d != 0) : (o->u.num.l != 0);
        return 0;
    case ALPONUM_GetDouble:
        if (v)
            *(double *)v = (o->u.num.type == ALPONUMT_Double) ? o->u.num.d : (double)o->u.num.l;
        return 0;
    }
    return ALPOEF_UNKNOWN_METHOD;
}

static ULONG po_DateMethod(struct PObj *o, struct TagItem *tag)
{
    APTR v = (APTR)tag->ti_Data;

    if (tag->ti_Tag != ALPODAT_SetCurrentTime && tag->ti_Tag != ALPODAT_SetSecsSince1978 && !v)
        return ALPOEF_ILLEGAL_PARAMETER;

    switch (tag->ti_Tag)
    {
    case ALPODAT_SetCurrentTime:
        po_SetNow(o);
        return 0;
    case ALPODAT_SetTimeVal:
        o->u.date.secs = ((struct timeval *)v)->tv_secs;
        o->u.date.micro = ((struct timeval *)v)->tv_micro;
        return 0;
    case ALPODAT_GetTimeVal:
        ((struct timeval *)v)->tv_secs = o->u.date.secs;
        ((struct timeval *)v)->tv_micro = o->u.date.micro;
        return 0;
    case ALPODAT_SetSecsSince1978:
        o->u.date.secs = (ULONG)tag->ti_Data;
        o->u.date.micro = 0;
        return 0;
    case ALPODAT_GetSecsSince1978:
        *(ULONG *)v = o->u.date.secs;
        return 0;
    case ALPODAT_SetClockData:
        o->u.date.secs = Date2Amiga((struct ClockData *)v);
        o->u.date.micro = 0;
        return 0;
    case ALPODAT_GetClockData:
        Amiga2Date(o->u.date.secs, (struct ClockData *)v);
        return 0;
    case ALPODAT_SetDateStamp:
        o->u.date.secs = ((struct DateStamp *)v)->ds_Days * 86400
                         + ((struct DateStamp *)v)->ds_Minute * 60
                         + ((struct DateStamp *)v)->ds_Tick / TICKS_PER_SECOND;
        o->u.date.micro = (((struct DateStamp *)v)->ds_Tick % TICKS_PER_SECOND)
                          * (1000000 / TICKS_PER_SECOND);
        return 0;
    case ALPODAT_GetDateStamp:
        ((struct DateStamp *)v)->ds_Days = o->u.date.secs / 86400;
        ((struct DateStamp *)v)->ds_Minute = (o->u.date.secs % 86400) / 60;
        ((struct DateStamp *)v)->ds_Tick = (o->u.date.secs % 60) * TICKS_PER_SECOND
                                           + o->u.date.micro / (1000000 / TICKS_PER_SECOND);
        return 0;
    }
    return ALPOEF_UNKNOWN_METHOD;
}

static ULONG po_BinaryMethod(struct PObj *o, struct TagItem *tag)
{
    struct ALPOBinData *bd = (struct ALPOBinData *)tag->ti_Data;

    switch (tag->ti_Tag)
    {
    case ALPOBIN_ClearData:
        return po_SetBinary(o, NULL, 0);
    case ALPOBIN_GetData:
        if (tag->ti_Data)
            *(UBYTE **)tag->ti_Data = o->u.bin.data;
        return 0;
    case ALPOBIN_GetSize:
        if (tag->ti_Data)
            *(LONG *)tag->ti_Data = o->u.bin.size;
        return 0;
    case ALPOBIN_GetDataStruct:
        if (!bd)
            return ALPOEF_ILLEGAL_PARAMETER;
        bd->data = o->u.bin.data;
        bd->size = o->u.bin.size;
        return 0;
    case ALPOBIN_SetData:
        if (!bd)
            return ALPOEF_ILLEGAL_PARAMETER;
        return po_SetBinary(o, bd->data, bd->size);
    }
    return ALPOEF_UNKNOWN_METHOD;
}

static ULONG po_ArrayMethod(struct PObj *o, struct TagItem *tag)
{
    struct ALPOObjIndex *oi = (struct ALPOObjIndex *)tag->ti_Data;
    LONG i;

    switch (tag->ti_Tag)
    {
    case ALPOARR_Clear:
        po_FreeContents(o);
        return 0;
    case ALPOARR_GetCount:
        if (tag->ti_Data)
            *(LONG *)tag->ti_Data = o->u.con.count;
        return 0;
    case ALPOARR_AddObj:
        return po_Insert(o, o->u.con.count, PO(tag->ti_Data), NULL);
    case ALPOARR_RemoveObjAtIndex:
        i = (LONG)tag->ti_Data;
        if (i < 0 || i >= o->u.con.count)
            return ALPOEF_NOTFOUND;
        po_RemoveAt(o, i);
        return 0;
    case ALPOARR_RemoveObj:
        if ((i = po_ObjIndex(o, PO(tag->ti_Data))) < 0)
            return ALPOEF_NOTFOUND;
        po_RemoveAt(o, i);
        return 0;
    }

    if (!oi)
        return ALPOEF_ILLEGAL_PARAMETER;

    switch (tag->ti_Tag)
    {
    case ALPOARR_InsertObjAtIndex:
        return po_Insert(o, oi->index, PO(oi->obj), NULL);
    case ALPOARR_GetObjAtIndex:
        if (oi->index < 0 || oi->index >= o->u.con.count)
        {
            oi->obj = NULL;
            return ALPOEF_NOTFOUND;
        }
        oi->obj = (PrefsObject *)o->u.con.items[oi->index];
        return 0;
    case ALPOARR_ReplaceObjAtIndex:
        if (!oi->obj)
            return ALPOEF_MISSING_OBJECT;
        if (oi->index < 0 || oi->index >= o->u.con.count)
            return ALPOEF_NOTFOUND;
        po_Replace(o, oi->index, PO(oi->obj));
        return 0;
    case ALPOARR_GetIndexForObj:
        oi->index = po_ObjIndex(o, PO(oi->obj));
        return oi->index < 0 ? ALPOEF_NOTFOUND : 0;
    }
    return ALPOEF_UNKNOWN_METHOD;
}

static LONG po_KeyCmp(struct PObj *o, LONG a, LONG b)
{
    return strcmp(o->u.con.keys[a], o->u.con.keys[b]);
}

static void po_SortDict(struct PObj *o, BOOL descending)
{
    LONG i, j;

    for (i = 1; i < o->u.con.count; i++)
    {
        for (j = i; j > 0; j--)
        {
            LONG c = po_KeyCmp(o, j - 1, j);
            struct PObj *ti;
            STRPTR tk;

            if (descending ? c >= 0 : c <= 0)
                break;
            ti = o->u.con.items[j];
            o->u.con.items[j] = o->u.con.items[j - 1];
            o->u.con.items[j - 1] = ti;
            tk = o->u.con.keys[j];
            o->u.con.keys[j] = o->u.con.keys[j - 1];
            o->u.con.keys[j - 1] = tk;
        }
    }
}

static ULONG po_DictMethod(struct PObj *o, struct TagItem *tag)
{
    struct ALPOObjKey *ok = (struct ALPOObjKey *)tag->ti_Data;
    struct ALPOObjIndex *oi = (struct ALPOObjIndex *)tag->ti_Data;
    struct ALPOObjKeyIndex *oki = (struct ALPOObjKeyIndex *)tag->ti_Data;
    LONG i;

    switch (tag->ti_Tag)
    {
    case ALPODICT_Clear:
        po_FreeContents(o);
        return 0;
    case ALPODICT_SortAscending:
    case ALPODICT_SortDescending:
        po_SortDict(o, tag->ti_Tag == ALPODICT_SortDescending);
        return 0;
    case ALPODICT_GetCount:
        if (tag->ti_Data)
            *(LONG *)tag->ti_Data = o->u.con.count;
        return 0;
    case ALPODICT_RemoveObjForKey:
        if ((i = po_KeyIndex(o, (CONST_STRPTR)tag->ti_Data)) < 0)
            return ALPOEF_NOTFOUND;
        po_RemoveAt(o, i);
        return 0;
    case ALPODICT_RemoveObjAtIndex:
        i = (LONG)tag->ti_Data;
        if (i < 0 || i >= o->u.con.count)
            return ALPOEF_NOTFOUND;
        po_RemoveAt(o, i);
        return 0;
    case ALPODICT_RemoveObj:
        if ((i = po_ObjIndex(o, PO(tag->ti_Data))) < 0)
            return ALPOEF_NOTFOUND;
        po_RemoveAt(o, i);
        return 0;
    }

    if (!tag->ti_Data)
        return ALPOEF_ILLEGAL_PARAMETER;

    switch (tag->ti_Tag)
    {
    case ALPODICT_SetObjForKey:
        if (!ok->obj)
            return ALPOEF_MISSING_OBJECT;
        if (!ok->key)
            return ALPOEF_ILLEGAL_PARAMETER;
        return po_DictSet((PrefsObject *)o, ok->key, ok->obj) ? 0 : ALPOEF_MEMORY;
    case ALPODICT_GetObjForKey:
        ok->obj = po_DictGet((PrefsObject *)o, ok->key);
        return ok->obj ? 0 : ALPOEF_NOTFOUND;
    case ALPODICT_ReplaceObjForKey:
        if (!ok->obj)
            return ALPOEF_MISSING_OBJECT;
        if ((i = po_KeyIndex(o, ok->key)) < 0)
            return ALPOEF_NOTFOUND;
        po_Replace(o, i, PO(ok->obj));
        return 0;
    case ALPODICT_GetObjAndIndexForKey:
        if ((oki->index = po_KeyIndex(o, oki->key)) < 0)
        {
            oki->obj = NULL;
            return ALPOEF_NOTFOUND;
        }
        oki->obj = (PrefsObject *)o->u.con.items[oki->index];
        return 0;
    case ALPODICT_GetObjAtIndex:
        if (oi->index < 0 || oi->index >= o->u.con.count)
        {
            oi->obj = NULL;
            return ALPOEF_NOTFOUND;
        }
        oi->obj = (PrefsObject *)o->u.con.items[oi->index];
        return 0;
    case ALPODICT_ReplaceObjAtIndex:
        if (!oi->obj)
            return ALPOEF_MISSING_OBJECT;
        if (oi->index < 0 || oi->index >= o->u.con.count)
            return ALPOEF_NOTFOUND;
        po_Replace(o, oi->index, PO(oi->obj));
        return 0;
    case ALPODICT_GetObjAndKeyAtIndex:
        if (oki->index < 0 || oki->index >= o->u.con.count)
        {
            oki->obj = NULL;
            oki->key = NULL;
            return ALPOEF_NOTFOUND;
        }
        oki->obj = (PrefsObject *)o->u.con.items[oki->index];
        oki->key = o->u.con.keys[oki->index];
        return 0;
    case ALPODICT_InsertObjAtIndex:
        if (po_KeyIndex(o, oki->key) >= 0)
            return ALPOEF_DUPLICATE;
        return po_Insert(o, oki->index, PO(oki->obj), oki->key);
    case ALPODICT_GetKeyAndIndexForObj:
        if ((oki->index = po_ObjIndex(o, PO(oki->obj))) < 0)
        {
            oki->key = NULL;
            return ALPOEF_NOTFOUND;
        }
        oki->key = o->u.con.keys[oki->index];
        return 0;
    }
    return ALPOEF_UNKNOWN_METHOD;
}

/* The Alloc methods that take a parameter, per type. */
static struct PObj *po_AllocWith(ULONG type, struct TagItem *tag, ULONG *err)
{
    struct PObj *o;
    struct TagItem set;

    if (tag->ti_Tag != ALPO_AllocWithString && (tag->ti_Tag - ALPO_Dummy) / 10000 != 3)
        return NULL;

    if (!(o = po_Alloc(type)))
    {
        *err |= ALPOEF_MEMORY;
        return NULL;
    }

    if (tag->ti_Tag == ALPO_AllocWithString)
    {
        ULONG e = po_SetWithString(o, (CONST_STRPTR)tag->ti_Data);

        if (e)
        {
            *err |= e;
            po_Release((PrefsObject *)o);
            return NULL;
        }
        return o;
    }

    set.ti_Data = tag->ti_Data;
    switch (type)
    {
    case ALPOT_String:
        if (tag->ti_Tag == ALPOSTR_AllocSetString)
            set.ti_Tag = ALPOSTR_SetString;
        else if (tag->ti_Tag == ALPOSTR_AllocSetUniString)
            set.ti_Tag = ALPOSTR_SetUniString;
        else
            goto unknown;
        if ((*err |= po_StringMethod(o, &set)) == 0)
            return o;
        break;
    case ALPOT_Number:
        if (tag->ti_Tag == ALPONUM_AllocSetLong)
            set.ti_Tag = ALPONUM_SetLong;
        else if (tag->ti_Tag == ALPONUM_AllocSetBool)
            set.ti_Tag = ALPONUM_SetBool;
        else if (tag->ti_Tag == ALPONUM_AllocSetDouble)
            set.ti_Tag = ALPONUM_SetDouble;
        else
            goto unknown;
        if ((*err |= po_NumberMethod(o, &set)) == 0)
            return o;
        break;
    case ALPOT_Date:
        if (tag->ti_Tag == ALPODAT_AllocSetTimeVal)
            set.ti_Tag = ALPODAT_SetTimeVal;
        else if (tag->ti_Tag == ALPODAT_AllocSetCTime)
        {
            /* 1970 to 1978 is 2922 days */
            set.ti_Tag = ALPODAT_SetSecsSince1978;
            set.ti_Data = tag->ti_Data >= 2922 * 86400 ? tag->ti_Data - 2922 * 86400 : 0;
        }
        else
            goto unknown;
        if ((*err |= po_DateMethod(o, &set)) == 0)
            return o;
        break;
    case ALPOT_Binary:
        if (tag->ti_Tag == ALPOBIN_AllocWithSize)
        {
            if ((*err |= po_SetBinary(o, NULL, (LONG)tag->ti_Data)) == 0)
                return o;
            break;
        }
        if (tag->ti_Tag != ALPOBIN_AllocWithData)
            goto unknown;
        set.ti_Tag = ALPOBIN_SetData;
        if ((*err |= po_BinaryMethod(o, &set)) == 0)
            return o;
        break;
    default:
        goto unknown;
    }
    po_Release((PrefsObject *)o);
    return NULL;

unknown:
    *err |= ALPOEF_UNKNOWN_METHOD;
    po_Release((PrefsObject *)o);
    return NULL;
}

/*
 * Run the methods in tags. want is the type the access function is for,
 * or ALPOT_None for PrefsBaseObjectA(), which only knows the base methods.
 */
PrefsObject *po_Dispatch(ULONG want, PrefsObject *obj, ULONG *error, struct TagItem *tags)
{
    struct TagItem *tstate = tags, *tag;
    struct PObj *o = PO(obj);
    ULONG err = 0;

    while ((tag = NextTagItem(&tstate)))
    {
        ULONG e = 0;

        if (tag->ti_Tag < ALPO_Dummy || tag->ti_Tag > ALPO_Dummy + 49999)
        {
            err |= ALPOEF_UNKNOWN_METHOD;
            continue;
        }

        if (tag->ti_Tag == ALPO_Alloc || tag->ti_Tag == ALPO_AllocWithString
            || (tag->ti_Tag - ALPO_Dummy) / 10000 == 3)
        {
            struct PObj *n;

            if (want == ALPOT_None)
            {
                err |= ALPOEF_IMPOSSIBLE_OP;
                continue;
            }
            if (tag->ti_Tag == ALPO_Alloc)
            {
                if (!(n = po_Alloc(want)))
                    err |= ALPOEF_MEMORY;
            }
            else
                n = po_AllocWith(want, tag, &err);
            if (n)
                o = n;
            continue;
        }

        if (!o)
        {
            err |= ALPOEF_MISSING_OBJECT;
            continue;
        }
        if (want != ALPOT_None && o->po_Type != want)
        {
            err |= ALPOEF_WRONG_OBJECTTYPE;
            continue;
        }

        switch (tag->ti_Tag)
        {
        case ALPO_Release:
            if (o->po_Retain <= 1)
            {
                po_Release((PrefsObject *)o);
                o = NULL;
            }
            else
                o->po_Retain--;
            break;
        case ALPO_Copy:
            {
                struct PObj *c = PO(po_Copy((PrefsObject *)o));

                if (c)
                    o = c;
                else
                    e = ALPOEF_MEMORY;
            }
            break;
        case ALPO_Retain:
            o->po_Retain++;
            break;
        case ALPO_Identify:
            if (tag->ti_Data)
                *(ULONG *)tag->ti_Data = o->po_Type;
            break;
        case ALPO_GetRetainCount:
            if (tag->ti_Data)
                *(LONG *)tag->ti_Data = o->po_Retain;
            break;
        case ALPO_Serialize:
            e = tag->ti_Data ? xml_Serialize((PrefsObject *)o, (struct ALPOPutProcInfo *)tag->ti_Data)
                             : ALPOEF_ILLEGAL_PARAMETER;
            break;
        case ALPO_Deserialize:
            e = tag->ti_Data ? xml_Deserialize((PrefsObject *)o, (struct ALPOGetProcInfo *)tag->ti_Data)
                             : ALPOEF_ILLEGAL_PARAMETER;
            break;
        case ALPO_SetWithString:
            e = po_SetWithString(o, (CONST_STRPTR)tag->ti_Data);
            break;
        case ALPO_GetAsString:
            e = po_GetAsString(o, (struct ALPOString *)tag->ti_Data);
            break;
        default:
            switch (o->po_Type)
            {
            case ALPOT_String:      e = po_StringMethod(o, tag); break;
            case ALPOT_Number:      e = po_NumberMethod(o, tag); break;
            case ALPOT_Date:        e = po_DateMethod(o, tag); break;
            case ALPOT_Binary:      e = po_BinaryMethod(o, tag); break;
            case ALPOT_Array:       e = po_ArrayMethod(o, tag); break;
            case ALPOT_Dictionary:  e = po_DictMethod(o, tag); break;
            default:                e = ALPOEF_INTERNAL_ERROR; break;
            }
            break;
        }
        err |= e;
    }

    if (error)
        *error = err;
    return (PrefsObject *)o;
}
