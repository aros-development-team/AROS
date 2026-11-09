/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: PrefsObjects as XML, and prefs files in ENV: and ENVARC:.

    <?xml version="1.0" encoding="UTF-8"?>
    <pobjects version="1">
    <dict>
        <key>Width</key>
        <integer>640</integer>
        ...
    </dict>
    </pobjects>

    Strings hold 8-bit characters; in a UTF-8 stream each byte is one
    character, so any byte string reads back as it was written.
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <string.h>

#include "application_intern.h"
#include "application_po.h"

/* ---------------------------------------------------------------------- */
/* Writing                                                                 */

static void w_Ch(struct ALPOPutProcInfo *pi, LONG c)
{
    pi->putChProc(c, pi);
}

static void w_Str(struct ALPOPutProcInfo *pi, CONST_STRPTR s)
{
    while (*s)
        w_Ch(pi, (UBYTE)*s++);
}

static void w_Indent(struct ALPOPutProcInfo *pi)
{
    LONG i;

    for (i = 0; i < pi->tabDepth; i++)
        w_Ch(pi, '\t');
}

/* Text content: markup characters escaped, 8-bit characters as UTF-8. */
static void w_Text(struct ALPOPutProcInfo *pi, CONST_STRPTR s, ULONG len)
{
    ULONG i;

    for (i = 0; i < len; i++)
    {
        UBYTE c = s[i];

        switch (c)
        {
        case '<':   w_Str(pi, "&lt;"); break;
        case '>':   w_Str(pi, "&gt;"); break;
        case '&':   w_Str(pi, "&amp;"); break;
        default:
            if (c < 0x20 && c != '\t' && c != '\n' && c != '\r')
            {
                TEXT num[12];

                po_FormatLong(num, c);
                w_Str(pi, "&#");
                w_Str(pi, num);
                w_Ch(pi, ';');
            }
            else if (c >= 0x80 && pi->encoding == ALPOENC_UTF8)
            {
                w_Ch(pi, 0xC0 | (c >> 6));
                w_Ch(pi, 0x80 | (c & 0x3F));
            }
            else
                w_Ch(pi, c);
            break;
        }
    }
}

static void w_Element(struct ALPOPutProcInfo *pi, CONST_STRPTR name, CONST_STRPTR text, ULONG len)
{
    w_Indent(pi);
    w_Ch(pi, '<');
    w_Str(pi, name);
    w_Ch(pi, '>');
    w_Text(pi, text, len);
    w_Str(pi, "</");
    w_Str(pi, name);
    w_Str(pi, ">\n");
}

static void w_Base64(struct ALPOPutProcInfo *pi, CONST UBYTE *data, LONG size)
{
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    LONG i;

    for (i = 0; i < size; i += 3)
    {
        ULONG v = data[i] << 16;

        if (i + 1 < size)
            v |= data[i + 1] << 8;
        if (i + 2 < size)
            v |= data[i + 2];
        w_Ch(pi, b64[(v >> 18) & 63]);
        w_Ch(pi, b64[(v >> 12) & 63]);
        w_Ch(pi, i + 1 < size ? b64[(v >> 6) & 63] : '=');
        w_Ch(pi, i + 2 < size ? b64[v & 63] : '=');
    }
}

ULONG xml_Serialize(PrefsObject *obj, struct ALPOPutProcInfo *pi)
{
    struct PObj *o = PO(obj);
    TEXT buf[40];
    LONG i;

    if (!o || !pi->putChProc)
        return ALPOEF_ILLEGAL_PARAMETER;

    switch (o->po_Type)
    {
    case ALPOT_String:
        w_Element(pi, "string", o->u.str.s, o->u.str.len);
        break;

    case ALPOT_Number:
        if (o->u.num.type == ALPONUMT_Bool)
            w_Element(pi, "bool", o->u.num.l ? "TRUE" : "FALSE", o->u.num.l ? 4 : 5);
        else if (o->u.num.type == ALPONUMT_Double)
        {
            po_FormatDouble(buf, o->u.num.d);
            w_Element(pi, "real", buf, strlen(buf));
        }
        else
        {
            po_FormatLong(buf, o->u.num.l);
            w_Element(pi, "integer", buf, strlen(buf));
        }
        break;

    case ALPOT_Date:
        po_FormatDate(buf, o->u.date.secs);
        w_Element(pi, "date", buf, strlen(buf));
        break;

    case ALPOT_Binary:
        w_Indent(pi);
        w_Str(pi, "<data>");
        w_Base64(pi, o->u.bin.data, o->u.bin.size);
        w_Str(pi, "</data>\n");
        break;

    case ALPOT_Array:
    case ALPOT_Dictionary:
        {
            CONST_STRPTR name = o->po_Type == ALPOT_Array ? "array" : "dict";

            w_Indent(pi);
            w_Ch(pi, '<');
            w_Str(pi, name);
            w_Str(pi, ">\n");
            pi->tabDepth++;
            for (i = 0; i < o->u.con.count; i++)
            {
                if (o->u.con.keys)
                    w_Element(pi, "key", o->u.con.keys[i], strlen(o->u.con.keys[i]));
                xml_Serialize((PrefsObject *)o->u.con.items[i], pi);
            }
            pi->tabDepth--;
            w_Indent(pi);
            w_Str(pi, "</");
            w_Str(pi, name);
            w_Str(pi, ">\n");
        }
        break;

    default:
        return ALPOEF_WRONG_OBJECT;
    }
    return 0;
}

static void w_Document(PrefsObject *dict, struct ALPOPutProcInfo *pi, ULONG *err)
{
    LONG depth = pi->tabDepth;

    w_Str(pi, pi->encoding == ALPOENC_UTF8
              ? "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
              : "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
    w_Str(pi, "<pobjects version=\"1\">\n");
    *err |= xml_Serialize(dict, pi);
    w_Str(pi, "</pobjects>\n");
    pi->tabDepth = depth;
}

/* ---------------------------------------------------------------------- */
/* Reading                                                                 */

static LONG r_Ch(struct ALPOGetProcInfo *pi)
{
    return pi->getChProc(pi);
}

static void r_UnCh(struct ALPOGetProcInfo *pi, LONG c)
{
    if (c >= 0)
        pi->unGetChProc(c, pi);
}

static LONG r_SkipSpace(struct ALPOGetProcInfo *pi)
{
    LONG c;

    do
        c = r_Ch(pi);
    while (c == ' ' || c == '\t' || c == '\n' || c == '\r');
    return c;
}

/* Skip to just after 'end', a short terminator like "?>" or "-->": the
   last strlen(end) characters read are compared, so "--->" ends too. */
static BOOL r_SkipPast(struct ALPOGetProcInfo *pi, CONST_STRPTR end)
{
    ULONG len = strlen(end), n = 0;
    TEXT last[8];
    LONG c;

    if (len == 0 || len > sizeof(last))
        return FALSE;
    while ((c = r_Ch(pi)) >= 0)
    {
        if (n < len)
            last[n++] = c;
        else
        {
            memmove(last, last + 1, len - 1);
            last[len - 1] = c;
        }
        if (n == len && memcmp(last, end, len) == 0)
            return TRUE;
    }
    return FALSE;
}

/*
 * Read the next tag into buf: "name" for an open tag, "/name" for a close
 * tag, "name/" for an empty element. Declarations, comments and attributes
 * are skipped. Text before the tag must be blank.
 */
static ULONG r_Tag(struct ALPOGetProcInfo *pi, STRPTR buf)
{
    LONG c, n;

    for (;;)
    {
        c = r_SkipSpace(pi);
        if (c != '<')
            return c < 0 ? ALPOEF_READTAG : ALPOEF_XMLERROR;

        c = r_Ch(pi);
        if (c == '?')
        {
            if (!r_SkipPast(pi, "?>"))
                return ALPOEF_XMLERROR;
            continue;
        }
        if (c == '!')
        {
            LONG c2 = r_Ch(pi);

            if (c2 == '-')
            {
                if (r_Ch(pi) != '-' || !r_SkipPast(pi, "-->"))
                    return ALPOEF_XMLERROR;
            }
            else if (!r_SkipPast(pi, ">"))
                return ALPOEF_XMLERROR;
            continue;
        }
        break;
    }

    n = 0;
    if (c == '/')
    {
        buf[n++] = '/';
        c = r_Ch(pi);
    }
    while (c >= 0 && c != '>' && c != '/' && c != ' ' && c != '\t' && c != '\n' && c != '\r')
    {
        if (n < TAGBUFFER_SIZE - 2)
            buf[n++] = c;
        c = r_Ch(pi);
    }
    /* attributes */
    while (c >= 0 && c != '>' && c != '/')
    {
        if (c == '"' || c == '\'')
        {
            LONG q = c;

            while ((c = r_Ch(pi)) >= 0 && c != q)
                ;
        }
        c = r_Ch(pi);
    }
    if (c == '/')
    {
        buf[n++] = '/';
        c = r_Ch(pi);
    }
    buf[n] = '\0';
    if (c != '>' || n == 0)
        return ALPOEF_XMLERROR;
    return 0;
}

struct TextBuf
{
    STRPTR tb_Buf;
    ULONG  tb_Len, tb_Size;
};

static BOOL tb_Add(struct TextBuf *tb, UBYTE c)
{
    if (tb->tb_Len + 1 >= tb->tb_Size)
    {
        ULONG size = tb->tb_Size ? tb->tb_Size * 2 : 64;
        STRPTR n = AllocVec(size, MEMF_ANY);

        if (!n)
            return FALSE;
        if (tb->tb_Len)
            CopyMem(tb->tb_Buf, n, tb->tb_Len);
        FreeVec(tb->tb_Buf);
        tb->tb_Buf = n;
        tb->tb_Size = size;
    }
    tb->tb_Buf[tb->tb_Len++] = c;
    tb->tb_Buf[tb->tb_Len] = '\0';
    return TRUE;
}

/* A character above 255 cannot be held in an 8-bit string. */
static BOOL tb_AddCode(struct TextBuf *tb, ULONG code)
{
    return tb_Add(tb, code < 256 ? code : '?');
}

static ULONG r_Entity(struct ALPOGetProcInfo *pi, struct TextBuf *tb)
{
    TEXT name[12];
    LONG c, n = 0;
    ULONG code = 0;

    while ((c = r_Ch(pi)) >= 0 && c != ';' && n < (LONG)sizeof(name) - 1)
        name[n++] = c;
    name[n] = '\0';
    if (c != ';')
        return ALPOEF_XMLERROR;

    if (strcmp(name, "lt") == 0)            code = '<';
    else if (strcmp(name, "gt") == 0)       code = '>';
    else if (strcmp(name, "amp") == 0)      code = '&';
    else if (strcmp(name, "quot") == 0)     code = '"';
    else if (strcmp(name, "apos") == 0)     code = '\'';
    else if (name[0] == '#')
    {
        STRPTR p = name + 1;
        BOOL hex = (*p == 'x' || *p == 'X');

        if (hex)
            p++;
        if (!*p)
            return ALPOEF_XMLERROR;
        for (; *p; p++)
        {
            LONG d;

            if (*p >= '0' && *p <= '9')
                d = *p - '0';
            else if (hex && (*p | 0x20) >= 'a' && (*p | 0x20) <= 'f')
                d = (*p | 0x20) - 'a' + 10;
            else
                return ALPOEF_XMLERROR;
            code = code * (hex ? 16 : 10) + d;
        }
    }
    else
        return ALPOEF_XMLERROR;

    return tb_AddCode(tb, code) ? 0 : ALPOEF_MEMORY;
}

/* Text up to the next '<', entities and UTF-8 decoded. */
static ULONG r_Text(struct ALPOGetProcInfo *pi, struct TextBuf *tb)
{
    LONG c;

    tb->tb_Len = 0;
    if (!tb_Add(tb, 0))
        return ALPOEF_MEMORY;
    tb->tb_Len = 0;

    while ((c = r_Ch(pi)) >= 0 && c != '<')
    {
        ULONG err = 0;

        if (c == '&')
            err = r_Entity(pi, tb);
        else if (c >= 0xC0 && pi->encoding == ALPOENC_UTF8)
        {
            ULONG code, more;
            LONG c2;

            if (c >= 0xF0)      { code = c & 0x07; more = 3; }
            else if (c >= 0xE0) { code = c & 0x0F; more = 2; }
            else                { code = c & 0x1F; more = 1; }
            while (more--)
            {
                if ((c2 = r_Ch(pi)) < 0 || (c2 & 0xC0) != 0x80)
                    return ALPOEF_READDATA;
                code = (code << 6) | (c2 & 0x3F);
            }
            err = tb_AddCode(tb, code) ? 0 : ALPOEF_MEMORY;
        }
        else
            err = tb_Add(tb, c) ? 0 : ALPOEF_MEMORY;
        if (err)
            return err;
    }
    if (c < 0)
        return ALPOEF_READDATA;
    r_UnCh(pi, c);
    return 0;
}

static ULONG r_CloseTag(struct ALPOGetProcInfo *pi, CONST_STRPTR name)
{
    ULONG err = r_Tag(pi, pi->key_otag);

    if (err)
        return err;
    if (pi->key_otag[0] != '/' || strcmp(pi->key_otag + 1, name) != 0)
        return ALPOEF_WRONG_CTAG;
    return 0;
}

static ULONG xml_TypeOf(CONST_STRPTR tag)
{
    TEXT name[16];
    ULONG len = strlen(tag);

    if (len >= sizeof(name))
        return ALPOT_None;
    strcpy(name, tag);
    if (len && name[len - 1] == '/')
        name[len - 1] = '\0';

    if (strcmp(name, "string") == 0)
        return ALPOT_String;
    if (strcmp(name, "integer") == 0 || strcmp(name, "real") == 0 || strcmp(name, "bool") == 0
        || strcmp(name, "true") == 0 || strcmp(name, "false") == 0)
        return ALPOT_Number;
    if (strcmp(name, "date") == 0)
        return ALPOT_Date;
    if (strcmp(name, "data") == 0)
        return ALPOT_Binary;
    if (strcmp(name, "array") == 0)
        return ALPOT_Array;
    if (strcmp(name, "dict") == 0)
        return ALPOT_Dictionary;
    return ALPOT_None;
}

static LONG b64_Value(UBYTE c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static ULONG r_Base64(struct PObj *o, struct TextBuf *tb)
{
    UBYTE *data;
    ULONG i, n = 0, bits = 0, acc = 0;

    if (!(data = AllocVec(tb->tb_Len / 4 * 3 + 3, MEMF_ANY | MEMF_CLEAR)))
        return ALPOEF_MEMORY;
    for (i = 0; i < tb->tb_Len; i++)
    {
        LONG v = b64_Value(tb->tb_Buf[i]);

        if (v < 0)
            continue;
        acc = (acc << 6) | v;
        bits += 6;
        if (bits >= 8)
        {
            bits -= 8;
            data[n++] = (acc >> bits) & 0xFF;
        }
    }
    FreeVec(o->u.bin.data);
    o->u.bin.data = data;
    o->u.bin.size = n;
    return 0;
}

/*
 * The open tag has been read, into pi->key_otag. Fill obj from the content
 * and read the close tag.
 */
/* Containers inside containers, from the object being read: a file from
   ENV: must not be able to run the reader out of stack. */
#define XML_MAXDEPTH 32

static ULONG xml_DeserializeDepth(PrefsObject *obj, struct ALPOGetProcInfo *pi, ULONG depth)
{
    struct PObj *o = PO(obj);
    struct TextBuf tb = { NULL, 0, 0 };
    TEXT tag[16];
    ULONG len, err = 0;
    BOOL empty;

    if (!o || !pi->getChProc || !pi->unGetChProc || !pi->key_otag)
        return ALPOEF_ILLEGAL_PARAMETER;
    if (depth > XML_MAXDEPTH)
        return ALPOEF_XMLERROR | ALPOEF_DESERIALIZATION;

    len = strlen(pi->key_otag);
    if (xml_TypeOf(pi->key_otag) != o->po_Type || len >= sizeof(tag))
        return ALPOEF_WRONG_OTAG;
    strcpy(tag, pi->key_otag);
    empty = (tag[len - 1] == '/');
    if (empty)
        tag[len - 1] = '\0';

    if (o->po_Type == ALPOT_Array || o->po_Type == ALPOT_Dictionary)
    {
        STRPTR key = NULL;

        while (!empty)
        {
            struct PObj *item;
            ULONG type;

            if ((err = r_Tag(pi, pi->key_otag)))
                break;
            if (pi->key_otag[0] == '/')
            {
                if (strcmp(pi->key_otag + 1, tag) != 0)
                    err = ALPOEF_WRONG_CTAG;
                break;
            }
            if (o->po_Type == ALPOT_Dictionary)
            {
                if (strcmp(pi->key_otag, "key") != 0)
                {
                    err = ALPOEF_WRONG_OTAG;
                    break;
                }
                if ((err = r_Text(pi, &tb)) || (err = r_CloseTag(pi, "key")))
                    break;
                FreeVec(key);
                if (!(key = app_StrDup(tb.tb_Buf)))
                {
                    err = ALPOEF_MEMORY;
                    break;
                }
                if ((err = r_Tag(pi, pi->key_otag)))
                    break;
            }
            if ((type = xml_TypeOf(pi->key_otag)) == ALPOT_None)
            {
                err = ALPOEF_WRONG_OTAG;
                break;
            }
            if (!(item = PO(po_New(type))))
            {
                err = ALPOEF_MEMORY;
                break;
            }
            if ((err = xml_DeserializeDepth((PrefsObject *)item, pi, depth + 1)))
            {
                po_Release((PrefsObject *)item);
                break;
            }
            if (o->po_Type == ALPOT_Dictionary)
            {
                if (!po_DictSet((PrefsObject *)o, key, (PrefsObject *)item))
                    err = ALPOEF_MEMORY;
            }
            else if (!po_ArrayInsert((PrefsObject *)o, (PrefsObject *)item, -1))
                err = ALPOEF_MEMORY;
            if (err)
            {
                po_Release((PrefsObject *)item);
                break;
            }
        }
        FreeVec(key);
        FreeVec(tb.tb_Buf);
        return err ? (err | ALPOEF_DESERIALIZATION) : 0;
    }

    if (!empty)
    {
        if ((err = r_Text(pi, &tb)) || (err = r_CloseTag(pi, tag)))
        {
            FreeVec(tb.tb_Buf);
            return err | ALPOEF_DESERIALIZATION;
        }
    }
    else
    {
        if (!tb_Add(&tb, 0))
            return ALPOEF_MEMORY;
        tb.tb_Len = 0;
    }

    switch (o->po_Type)
    {
    case ALPOT_String:
        FreeVec(o->u.str.s);
        o->u.str.s = tb.tb_Buf;
        o->u.str.len = tb.tb_Len;
        tb.tb_Buf = NULL;
        break;

    case ALPOT_Number:
        if (strcmp(tag, "true") == 0 || strcmp(tag, "false") == 0)
        {
            o->u.num.type = ALPONUMT_Bool;
            o->u.num.l = (tag[0] == 't');
        }
        else if (strcmp(tag, "bool") == 0)
        {
            CONST_STRPTR s = tb.tb_Buf;

            while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
                s++;
            o->u.num.type = ALPONUMT_Bool;
            o->u.num.l = (Strnicmp(s, "TRUE", 4) == 0 || Strnicmp(s, "YES", 3) == 0 || *s == '1');
        }
        else
        {
            LONG l;
            double d;
            BOOL isd;

            if (!po_ParseNumber(tb.tb_Buf, &l, &d, &isd))
                err = ALPOEF_READDATA;
            else if (tag[0] == 'r')
            {
                o->u.num.type = ALPONUMT_Double;
                o->u.num.d = d;
            }
            else
            {
                o->u.num.type = ALPONUMT_Long;
                o->u.num.l = l;
            }
        }
        break;

    case ALPOT_Date:
        if (!po_ParseDate(tb.tb_Buf, &o->u.date.secs))
            err = ALPOEF_READDATA;
        o->u.date.micro = 0;
        break;

    case ALPOT_Binary:
        err = r_Base64(o, &tb);
        break;
    }

    FreeVec(tb.tb_Buf);
    return err ? (err | ALPOEF_DESERIALIZATION) : 0;
}

ULONG xml_Deserialize(PrefsObject *obj, struct ALPOGetProcInfo *pi)
{
    return xml_DeserializeDepth(obj, pi, 0);
}

/* Read up to the first object's open tag, past a <pobjects> wrapper. */
ULONG xml_Begin(struct ALPOGetProcInfo *pi)
{
    if (!pi || !pi->getChProc || !pi->unGetChProc || !pi->key_otag)
        return ALPOT_None;
    if (r_Tag(pi, pi->key_otag))
        return ALPOT_None;
    if (strcmp(pi->key_otag, "pobjects") == 0 && r_Tag(pi, pi->key_otag))
        return ALPOT_None;
    return xml_TypeOf(pi->key_otag);
}

static ULONG xml_ReadDict(PrefsObject *dict, struct ALPOGetProcInfo *pi)
{
    PrefsObject *tmp;
    ULONG err;

    if (xml_Begin(pi) != ALPOT_Dictionary)
        return ALPOEF_DESERIALIZATION | ALPOEF_WRONG_OTAG;
    if (!(tmp = po_NewDict()))
        return ALPOEF_MEMORY;
    if (!(err = xml_Deserialize(tmp, pi)))
        po_MoveContents(dict, tmp);
    po_Release(tmp);
    return err;
}

/* ---------------------------------------------------------------------- */
/* Files                                                                   */

struct FileIO
{
    BPTR  fio_File;
    LONG  fio_Back;     /* pushed back character, or -1 */
    BOOL  fio_Error;
};

static LONG fio_Get(struct ALPOGetProcInfo *pi)
{
    struct FileIO *fio = pi->handlePtr;
    LONG c;

    if (fio->fio_Back >= 0)
    {
        c = fio->fio_Back;
        fio->fio_Back = -1;
        return c;
    }
    c = FGetC(fio->fio_File);
    return c < 0 ? -1 : c;
}

static LONG fio_UnGet(LONG c, struct ALPOGetProcInfo *pi)
{
    ((struct FileIO *)pi->handlePtr)->fio_Back = c;
    return c;
}

static void fio_Put(LONG c, struct ALPOPutProcInfo *pi)
{
    struct FileIO *fio = pi->handlePtr;

    if (FPutC(fio->fio_File, c) < 0)
        fio->fio_Error = TRUE;
}

ULONG xml_ReadFile(PrefsObject *dict, CONST_STRPTR path)
{
    struct FileIO fio = { 0, -1, FALSE };
    struct ALPOGetProcInfo pi;
    TEXT otag[TAGBUFFER_SIZE];
    ULONG err;

    if (!(fio.fio_File = Open(path, MODE_OLDFILE)))
    {
        /* a reset between xml_WriteFile()'s delete and rename left only
           the new copy */
        TEXT tmp[APP_PATHMAX];

        if (strlen(path) + 5 > sizeof(tmp))
            return ALPOEF_FILEERROR;
        strcpy(tmp, path);
        strcat(tmp, ".new");
        if (!(fio.fio_File = Open(tmp, MODE_OLDFILE)))
            return ALPOEF_FILEERROR;
    }
    memset(&pi, 0, sizeof(pi));
    pi.getChProc = fio_Get;
    pi.unGetChProc = fio_UnGet;
    pi.handlePtr = &fio;
    pi.key_otag = otag;
    pi.encoding = ALPOENC_UTF8;
    err = xml_ReadDict(dict, &pi);
    Close(fio.fio_File);
    return err;
}

/* Make the drawers above a file, as for ENV:Sys/x.xml on a fresh RAM: */
static void xml_MakeDrawers(CONST_STRPTR path)
{
    TEXT buf[APP_PATHMAX];
    STRPTR p;

    if (strlen(path) >= sizeof(buf))
        return;
    strcpy(buf, path);
    for (p = buf; *p; p++)
    {
        if (*p == '/')
        {
            BPTR lock;

            *p = '\0';
            if ((lock = Lock(buf, SHARED_LOCK)) || (lock = CreateDir(buf)))
                UnLock(lock);
            *p = '/';
        }
    }
}

ULONG xml_WriteFile(PrefsObject *dict, CONST_STRPTR path)
{
    struct FileIO fio = { 0, -1, FALSE };
    struct ALPOPutProcInfo pi;
    TEXT tmp[APP_PATHMAX];
    ULONG err = 0;

    if (strlen(path) + 5 > sizeof(tmp))
        return ALPOEF_ILLEGAL_PARAMETER;
    strcpy(tmp, path);
    strcat(tmp, ".new");

    xml_MakeDrawers(path);
    if (!(fio.fio_File = Open(tmp, MODE_NEWFILE)))
        return ALPOEF_FILEERROR;
    memset(&pi, 0, sizeof(pi));
    pi.putChProc = fio_Put;
    pi.handlePtr = &fio;
    pi.encoding = ALPOENC_UTF8;
    w_Document(dict, &pi, &err);
    if (!Close(fio.fio_File) || fio.fio_Error)
        err |= ALPOEF_FILEERROR;
    if (err)
        DeleteFile(tmp);
    else
    {
        DeleteFile(path);
        if (!Rename(tmp, path))
            err |= ALPOEF_FILEERROR;
    }
    return err;
}

static BOOL prefs_Path(STRPTR buf, CONST_STRPTR dir, CONST_STRPTR name)
{
    if (strlen(dir) + strlen(name) >= APP_PATHMAX)
        return FALSE;
    strcpy(buf, dir);
    strcat(buf, name);
    return TRUE;
}

/* name is below ENV:/ENVARC:, or a full path when it has a ':' */
ULONG prefs_Read(struct ApplicationBase *AppBase, PrefsObject *dict, CONST_STRPTR name,
                 BOOL env, BOOL envarc)
{
    TEXT path[APP_PATHMAX];
    ULONG err = ALPOEF_FILEERROR;

    (void)AppBase;
    if (po_Type(dict) != ALPOT_Dictionary)
        return ALPOEF_WRONG_OBJECTTYPE;
    po_Clear(dict);
    if (!name || !app_IsProcess())
        return ALPOEF_ILLEGAL_PARAMETER;

    if (strchr(name, ':'))
        return xml_ReadFile(dict, name);
    if (env && prefs_Path(path, "ENV:", name))
        err = xml_ReadFile(dict, path);
    if (err && envarc && prefs_Path(path, "ENVARC:", name))
        err = xml_ReadFile(dict, path);
    return err;
}

ULONG prefs_Write(struct ApplicationBase *AppBase, PrefsObject *dict, CONST_STRPTR name,
                  BOOL env, BOOL envarc)
{
    TEXT path[APP_PATHMAX];
    ULONG err = 0;

    (void)AppBase;
    if (po_Type(dict) != ALPOT_Dictionary)
        return ALPOEF_WRONG_OBJECTTYPE;
    if (!name || !app_IsProcess())
        return ALPOEF_ILLEGAL_PARAMETER;

    if (strchr(name, ':'))
        return xml_WriteFile(dict, name);
    if (env)
        err |= prefs_Path(path, "ENV:", name) ? xml_WriteFile(dict, path) : ALPOEF_ILLEGAL_PARAMETER;
    if (envarc)
        err |= prefs_Path(path, "ENVARC:", name) ? xml_WriteFile(dict, path) : ALPOEF_ILLEGAL_PARAMETER;
    return err;
}

/* ---------------------------------------------------------------------- */
/* Streams given by the caller                                             */

ULONG xml_ReadStream(PrefsObject *dict, struct ALPOGetProcInfo *pi)
{
    TEXT otag[TAGBUFFER_SIZE];
    STRPTR saved = pi->key_otag;
    ULONG err;

    if (po_Type(dict) != ALPOT_Dictionary)
        return ALPOEF_WRONG_OBJECTTYPE;
    po_Clear(dict);
    if (!saved)
        pi->key_otag = otag;
    err = xml_ReadDict(dict, pi);
    pi->key_otag = saved;
    return err;
}

ULONG xml_WriteStream(PrefsObject *dict, struct ALPOPutProcInfo *pi)
{
    ULONG err = 0;

    if (po_Type(dict) != ALPOT_Dictionary)
        return ALPOEF_WRONG_OBJECTTYPE;
    if (!pi->putChProc)
        return ALPOEF_ILLEGAL_PARAMETER;
    w_Document(dict, pi, &err);
    return err;
}
