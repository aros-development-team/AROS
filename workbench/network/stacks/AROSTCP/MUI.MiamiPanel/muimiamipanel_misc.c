
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <mui/Urltext_mcc.h>

#include "muimiamipanel_intern.h"
#include "muimiamipanel_misc.h"
#include "muimiamipanel_locale.h"

/***********************************************************************/

#if !defined(__MORPHOS____) && !defined(__AROS__)
ULONG STDARGS
DoSuperNew(struct IClass *cl, Object *obj, ULONG tag1,...)
{
    return DoSuperMethod(cl, obj, OM_NEW, &tag1, NULL);
}
#endif

/***********************************************************************/

Object *
ovfixspace(void)
{
    return RectangleObject,
        MUIA_FixHeightTxt, "A",
    End;
}

/****************************************************************************/

Object *
ohfixspace(void)
{
    return RectangleObject,
        MUIA_FixWidthTxt, "A",
    End;
}

/****************************************************************************/

Object *
otextitem(void)
{
    return TextObject,
        MUIA_Text_Contents, "-",
        MUIA_Text_SetMax,   TRUE,
    End;
}

/****************************************************************************/

Object *
obartitle(ULONG id, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    return MUI_MakeObject(MUIO_BarTitle, __(id));
}

/****************************************************************************/

Object *
owspace(ULONG weight)
{
    return RectangleObject,
            MUIA_Weight, weight,
    End;
}

/****************************************************************************/

Object *
olabel(ULONG id, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    return Label(__(id));
}

/****************************************************************************/

Object *
olabel1(ULONG id, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    return Label1(__(id));
}

/****************************************************************************/

Object *
ollabel1(ULONG id, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    return LLabel1(__(id));
}

/***********************************************************************/

Object *
olabel2(ULONG id, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    return Label2(__(id));
}

/***********************************************************************/

Object *
obutton(ULONG label, ULONG help, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    register Object *obj;

    if (obj = MUI_MakeObject(MUIO_Button, __(label)))
        SetAttrs(obj, MUIA_CycleChain, TRUE,
                     MUIA_ShortHelp,  __(help),
                     TAG_DONE);

    return obj;
}

/***********************************************************************/

Object *
ourlText(UBYTE *url, UBYTE *text, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    return UrltextObject,
        MUIA_Urltext_Text,   text,
        MUIA_Urltext_Url,    url,
        MUIA_Urltext_SetMax, 0,
    End;
}

/***********************************************************************/

Object *
ocheck(ULONG key, ULONG help, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    register Object *obj;

    if (obj = MUI_MakeObject(MUIO_Checkmark, __(key)))
        SetAttrs(obj, MUIA_CycleChain, TRUE, MUIA_ShortHelp, __(help), TAG_DONE);

    return obj;
}

/***********************************************************************/

Object *
ocycle(ULONG key, UBYTE **entries, ULONG help, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    register Object *obj;

    if (obj = MUI_MakeObject(MUIO_Cycle, __(key), (ULONG)entries))
        SetAttrs(obj, MUIA_CycleChain, TRUE, MUIA_ShortHelp,__(help), TAG_DONE);

    return obj;
}

/***********************************************************************/

ULONG
openWindow(Object *app, Object *win, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    ULONG v;

    if (win)
    {
        set(win, MUIA_Window_Open, TRUE);
        get(win, MUIA_Window_Open, &v);
        if (!v) get(app, MUIA_Application_Iconified, &v);
    }
    else v = FALSE;

    if (!v) DisplayBeep(0);

    return v;
}

/***********************************************************************/

void
grouping(UBYTE *source, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    UBYTE          buf[256];
    register UBYTE *s, *d, c = MiamiPanelBaseIntern->mpb_groupSep[0];
    int            j;

    for (j = 3, s = source+strlen(source)-1, d = buf; s>=source; j--, d++, s--)
    {
        if (!j)
        {
            j = 3;
            *d++ = c;
        }

        *d = *s;
    }
    *d = 0;

    for (s = buf+strlen(buf)-1, d = source; s>=buf; d++, s--) *d = *s;
    *d = 0;
}

/***********************************************************************/

#if !defined(__AROS__)
#ifdef __MORPHOS__
void
sprintf(UBYTE *to, UBYTE *fmt,...)
{
  va_list args;

  va_start(args, fmt);
  VNewRawDoFmt(fmt, (APTR)0, to, args);
  va_end(args);
}
#else
static UWORD fmtfunc[] = { 0x16c0, 0x4e75 };

void
sprintf(UBYTE *to, UBYTE *fmt,...)
{
    RawDoFmt(fmt, &fmt+1, (APTR)fmtfunc, to);
}
#endif
#endif /* !__AROS__ */

/***********************************************************************/

#if !defined(__AROS__)
#ifdef __MORPHOS__
static void
snprintfStuff(void)
{
    register struct stream *s = (struct stream *)REG_A3;
    register UBYTE         c  = (UBYTE)REG_D0;
#else
static void ASM
snprintfStuff(REG(d0, UBYTE c), REG(a3, struct stream *s))
{
#endif

    if (!s->stop)
    {
        if (++s->counter>=s->size)
        {
            *(s->buf) = 0;
            s->stop   = 1;
        }
        else *(s->buf++) = c;
    }
}

#ifdef __MORPHOS__
static struct EmulLibEntry snprintfStuffTrap = {TRAP_LIB, 0, (void *)&snprintfStuff};
#endif


int STDARGS
snprintf(UBYTE *buf, int size, UBYTE *fmt,...)
{
    struct stream s;
    #ifdef __MORPHOS__
    va_list       va;

    va_start(va,fmt);
    #endif

    s.buf     = buf;
    s.size    = size;
    s.counter = 0;
    s.stop    = 0;

    #ifdef __MORPHOS__
    RawDoFmt(fmt, va->overflow_arg_area, (APTR)&snprintfStuffTrap, &s);
    va_end(va);
    #else
    RawDoFmt(fmt, &fmt+1, (APTR)snprintfStuff, &s);
    #endif

    return s.counter-1;
}
#endif

/****************************************************************************/
/*
** The following functions are used
** to handle the per interface scale
*/

ULONG
IDToValue(Tag tag, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    return GetTagData(tag, 0, scales);
}
/****************************************************************************/

Tag
valueToID(ULONG val)
{
    register struct TagItem *t;

    for (t = scales; (t->ti_Tag!=TAG_DONE) && (t->ti_Data!=val); t++);

    return t->ti_Tag;
}

/****************************************************************************/

struct ifnode *
createIFNode(struct MPS_Prefs *prefs,UBYTE *name,ULONG scale, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    register struct ifnode *ifnode;

    ObtainSemaphore(&MiamiPanelBaseIntern->mpb_memSem);

    if (ifnode = AllocPooled(MiamiPanelBaseIntern->mpb_pool, sizeof(struct ifnode)))
    {
        stccpy(ifnode->name,name, sizeof(ifnode->name));
        ifnode->scale = scale;

        AddHead((struct List *)&prefs->iflist, (struct Node *)ifnode);
    }

    ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_memSem);

    return ifnode;
}

/****************************************************************************/

struct ifnode *
findIFNode(struct MPS_Prefs *prefs,UBYTE *name)
{
    register struct ifnode *ifnode;

    for (ifnode = (struct ifnode *)prefs->iflist.mlh_Head; ifnode->link.mln_Succ; ifnode = (struct ifnode *)ifnode->link.mln_Succ)
        if (!strcmp(ifnode->name,name)) break;

    return (ifnode->link.mln_Succ) ? ifnode : NULL;
}

/****************************************************************************/

void
freeIFList(struct MPS_Prefs *prefs, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    register struct Node *node;

    ObtainSemaphore(&MiamiPanelBaseIntern->mpb_memSem);

    while (node = RemHead((struct List *)&prefs->iflist))
        FreePooled(MiamiPanelBaseIntern->mpb_pool, node, sizeof(struct ifnode));

    ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_memSem);
}

/****************************************************************************/

void
moveMinList(struct MinList *to,struct MinList *from, struct MiamiPanelBase_intern *MiamiPanelBaseIntern)
{
    register struct Node *node;

    while (node = RemTail((struct List *)from)) AddTail((struct List *)to, node);
}

/****************************************************************************/

#ifdef __MORPHOS__
void *
memcpy(void *to,const void *from,size_t len)
{
    CopyMem((APTR)from, (APTR)to, len);

    return to;
}
#endif

/****************************************************************************/

