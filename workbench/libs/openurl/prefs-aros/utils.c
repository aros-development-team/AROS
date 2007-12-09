/*
**  OpenURL - MUI preferences for openurl.library
**
**  Written by Troels Walsted Hansen <troels@thule.no>
**  Placed in the public domain.
**
**  Developed by:
**  - Alfonso Ranieri <alforan@tin.it>
**  - Stefan Kost <ensonic@sonicpulse.de>
**
**  Ported to OS4 by Alexandre Balaban <alexandre@balaban.name>
**
*/


#include "OpenURL.h"

#if defined(__amigaos4__)
#include <stdarg.h>
#endif

/**************************************************************************/

#define _KEY(k)  ((ULONG)(k)) ? MUIA_ControlChar : TAG_IGNORE, ((ULONG)(k)) ? (ULONG)getKeyChar(NULL,(ULONG)(k)) : 0
#define _HELP(h) ((ULONG)(h)) ? MUIA_ShortHelp   : TAG_IGNORE, ((ULONG)(h)) ? (ULONG)getString((ULONG)(h)) : 0

/***********************************************************************/

#if !defined(__MORPHOS__) && !defined(__amigaos4__)
ULONG STDARGS
DoSuperNew(struct IClass *cl,Object *obj,ULONG tag1,...)
{
    return DoSuperMethod(cl,obj,OM_NEW,&tag1,NULL);
}
#endif

/**************************************************************************/

#ifdef __amigaos4__
int stccpy(char *dst, const char *src, int m)
{
    char c;
    int  j = m;

    while((c = *src++) != '\0')
    {
        if (--j > 0)
            *dst++ = c;
        else
            break;
    }
    *dst = '\0';
    return m - j;
}
#endif /* __amigaos4__ */

/**************************************************************************/

#ifdef __amigaos4__
/**********************************************************
**
** The following function saves the variable name passed in
** 'varname' to the ENV(ARC) system so that the application
** can become AmiUpdate aware.
**
**********************************************************/
void SetAmiUpdateENVVariable( CONST_STRPTR varname )
{
  /* AmiUpdate support code */
  BPTR lock;
  APTR oldwin;

  /* obtain the lock to the home directory */
  if(( lock = GetProgramDir() ))
  {
    TEXT progpath[2048];
    TEXT varpath[1024] = "AppPaths";

    /*
    get a unique name for the lock,
    this call uses device names,
    as there can be multiple volumes
    with the same name on the system
    */

    if( DevNameFromLock( lock, progpath, sizeof(progpath), DN_FULLPATH ))
    {
      /* stop any "Insert volume..." type requesters */
      oldwin = SetProcWindow((APTR)-1);

      /*
      finally set the variable to the
      path the executable was run from
      don't forget to supply the variable
      name to suit your application
      */

      AddPart( varpath, varname, 1024);
      SetVar( varpath, progpath, -1, GVF_GLOBAL_ONLY|GVF_SAVE_VAR );

      /* turn requesters back on */
      SetProcWindow( oldwin );
    }
  }
}

#endif /* __amigaos4__ */

/**************************************************************************/
ULONG
xget(Object *obj,ULONG attribute)
{
    ULONG x;

    get(obj,attribute,&x);

    return x;
}

/**************************************************************************/

Object *
olabel(ULONG id)
{
    return Label((ULONG)getString(id));
}

/****************************************************************************/

Object *
ollabel(ULONG id)
{
    return LLabel((ULONG)getString(id));
}

/****************************************************************************/

Object *
ollabel1(ULONG id)
{
    return LLabel1((ULONG)getString(id));
}

/****************************************************************************/

Object *
olabel1(ULONG id)
{
    return Label1((ULONG)getString(id));
}

/***********************************************************************/

Object *
olabel2(ULONG id)
{
    return Label2((ULONG)getString(id));
}

/****************************************************************************/

Object *
oflabel(ULONG text)
{
    return FreeLabel((ULONG)getString(text));
}

/****************************************************************************/

Object *
obutton(ULONG text,ULONG help)
{
    Object *obj;

    if (obj = MUI_MakeObject(MUIO_Button,(ULONG)getString(text)))
        SetAttrs(obj,MUIA_CycleChain,TRUE,_HELP(help),TAG_DONE);

    return obj;
}

/***********************************************************************/

Object *
oibutton(ULONG spec,ULONG help)
{
    if (spec==IBT_Up) spec = (ULONG)"\33I[6:38]";
    else if (spec==IBT_Down) spec = (ULONG)"\33I[6:39]";
         else return NULL;

    return TextObject,
        _HELP(help),
        MUIA_CycleChain,    TRUE,
        MUIA_Font,          MUIV_Font_Button,
        MUIA_InputMode,     MUIV_InputMode_RelVerify,
        ButtonFrame,
        MUIA_Background,    MUII_ButtonBack,
        MUIA_Text_Contents, spec,
        MUIA_Text_PreParse, MUIX_C,
        MUIA_Text_SetMax,   TRUE,
    End;
}

/****************************************************************************/

Object *
otbutton(ULONG label,ULONG help)
{
    return TextObject,
        _KEY(label),
        _HELP(help),
        MUIA_CycleChain,    TRUE,
        MUIA_Font,          MUIV_Font_Button,
        MUIA_InputMode,     MUIV_InputMode_Toggle,
        ButtonFrame,
        MUIA_Background,    MUII_ButtonBack,
        MUIA_Text_Contents, getString(label),
        MUIA_Text_PreParse, MUIX_C,
        MUIA_Text_HiCharIdx, '_',
    End;
}

/****************************************************************************/

Object *
ocheckmark(ULONG key,ULONG help)
{
    Object *obj;

    if (obj = MUI_MakeObject(MUIO_Checkmark,(ULONG)getString(key)))
        SetAttrs(obj,MUIA_CycleChain,TRUE,_HELP(help),TAG_DONE);

    return obj;
}

/****************************************************************************/

Object *
opopbutton(ULONG img,ULONG help)
{
    Object *obj;

    if (obj = MUI_MakeObject(MUIO_PopButton,img))
        SetAttrs(obj,MUIA_CycleChain,TRUE,_HELP(help),TAG_DONE);

    return obj;
}

/****************************************************************************/

Object *
ostring(ULONG maxlen,ULONG key,ULONG help)
{
    return StringObject,
        _KEY(key),
        _HELP(help),
        MUIA_CycleChain,         TRUE,
        StringFrame,
        MUIA_String_AdvanceOnCR, TRUE,
        MUIA_String_MaxLen,      maxlen,
    End;
}

/***********************************************************************/

Object *
opopport(ULONG maxLen,ULONG key,ULONG help)
{
    return popportObject,
        _HELP(help),
        MUIA_Popport_Key, key,
        MUIA_Popport_Len, maxLen,
    End;
}

/***********************************************************************/

Object *
opopph(STRPTR *syms,STRPTR *names,ULONG maxLen,ULONG key,ULONG asl,ULONG help)
{
    return popphObject,
        _HELP(help),
        MUIA_Popph_Syms,     syms,
        MUIA_Popph_Names,    names,
        MUIA_Popph_MaxLen,   maxLen,
        MUIA_Popph_Key,      key,
        MUIA_Popph_Asl,      asl,
    End;
}

/***********************************************************************/

ULONG
openWindow(Object *app,Object *win)
{
    ULONG v;

    if (win)
    {
        set(win,MUIA_Window_Open,TRUE);
        get(win,MUIA_Window_Open,&v);
        if (!v) get(app,MUIA_Application_Iconified,&v);
    }
    else v = FALSE;

    if (!v) DisplayBeep(0);

    return v;
}

/***********************************************************************/

#ifndef __MORPHOS__
static ULONG fmtfunc = 0x16C04E75;

void STDARGS
msprintf(STRPTR to, STRPTR fmt,...)
{
    #if defined(__amigaos4__)
    va_list       va;
    va_startlinear(va,fmt);
    RawDoFmt(fmt,va_getlinearva(va,CONST APTR),(APTR)&fmtfunc,to);
    va_end(va);
    #else
    RawDoFmt(fmt,&fmt+1,(APTR)&fmtfunc,to);
    #endif
}
#endif

/**************************************************************************/

struct stream
{
    STRPTR  buf;
    int     size;
    int     counter;
    int     stop;
};

#ifdef __MORPHOS__
static void
msnprintfStuff(void)
{
    register struct stream *s = (struct stream *)REG_A3;
    register UBYTE         c  = (TEXT)REG_D0;
#elif defined(__AROS__)
AROS_UFH2S(void, msnprintfStuff,
AROS_UFHA(TEXT           , c, D0),
AROS_UFHA(struct stream *, s, A3))
{
    AROS_USERFUNC_INIT
#else
static void SAVEDS ASM
msnprintfStuff(REG(d0,TEXT c),REG(a3,struct stream *s))
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
#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif
}

#ifdef __MORPHOS__
static struct EmulLibEntry msnprintfStuffTrap = {TRAP_LIB,0,(void *)&msnprintfStuff};
#endif


int
#if !defined( __MORPHOS__ )
STDARGS
#endif
msnprintf(STRPTR buf,int size, STRPTR fmt,...)
{
    struct stream s;
    #ifdef __MORPHOS__
    va_list       va;
    va_start(va,fmt);
    #elif defined(__amigaos4__)
    va_list       va;
    va_startlinear(va,fmt);
    #endif

    s.buf     = buf;
    s.size    = size;
    s.counter = 0;
    s.stop    = 0;

    #ifdef __MORPHOS__
    RawDoFmt(fmt,va->overflow_arg_area,(APTR)&msnprintfStuffTrap,&s);
    va_end(va);
    #elif defined(__amigaos4__)
    RawDoFmt(fmt,va_getlinearva(va,CONST APTR),(APTR)msnprintfStuff,&s);
    va_end(va);
    #else
    RawDoFmt(fmt,&fmt+1,(APTR)msnprintfStuff,&s);
    #endif

    return s.counter-1;
}

/**************************************************************************/

ULONG
delEntry(Object *obj,APTR entry)
{
    APTR e;
    int  i;

    for (i = 0; ;i++)
    {
        DoMethod(obj,MUIM_List_GetEntry,i,(ULONG)&e);
        if (!e || e==entry) break;
    }

    if (e) DoMethod(obj,MUIM_List_Remove,i);

    return (ULONG)e;
}

/**************************************************************************/
