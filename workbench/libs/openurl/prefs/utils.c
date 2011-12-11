/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2009 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#include "openurl.h"

#include <SDI/SDI_stdarg.h>
#include "macros.h"

#include "debug.h"

/**************************************************************************/

#define _KEY(k)  ((ULONG)(k)) ? MUIA_ControlChar : TAG_IGNORE, ((ULONG)(k)) ? (ULONG)getKeyChar(NULL,(ULONG)(k)) : 0
#define _HELP(h) ((ULONG)(h)) ? MUIA_ShortHelp   : TAG_IGNORE, ((ULONG)(h)) ? (IPTR)getString((ULONG)(h)) : 0

/***********************************************************************/

// DoSuperNew()
// Calls parent NEW method within a subclass
#if !defined(__MORPHOS__)
#if !defined(__AROS__)
Object * VARARGS68K DoSuperNew(struct IClass *cl, Object *obj, ...)
{
  Object *rc;
  VA_LIST args;

  ENTER();

  VA_START(args, obj);
  rc = (Object *)DoSuperMethod(cl, obj, OM_NEW, VA_ARG(args, ULONG), NULL);
  VA_END(args);

  RETURN(rc);
  return rc;
}
#endif
#endif

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

Object *olabel(ULONG id)
{
    return Label((IPTR)getString(id));
}

/****************************************************************************/

Object *ollabel(ULONG id)
{
    return LLabel((IPTR)getString(id));
}

/****************************************************************************/

Object *ollabel1(ULONG id)
{
    return LLabel1((IPTR)getString(id));
}

/****************************************************************************/

Object *olabel1(ULONG id)
{
    return Label1((IPTR)getString(id));
}

/***********************************************************************/

Object *olabel2(ULONG id)
{
    return Label2((IPTR)getString(id));
}

/****************************************************************************/

Object *oflabel(ULONG text)
{
    return FreeLabel((IPTR)getString(text));
}

/****************************************************************************/

Object *obutton(ULONG text, ULONG help)
{
    Object *obj;

    if((obj = MUI_MakeObject(MUIO_Button, (IPTR)getString(text))) != NULL)
        SetAttrs(obj,MUIA_CycleChain,TRUE,_HELP(help),TAG_DONE);

    return obj;
}

/***********************************************************************/

Object *oibutton(ULONG spec, ULONG help)
{
    if (spec==IBT_Up) spec = (IPTR)"\33I[6:38]";
    else if (spec==IBT_Down) spec = (IPTR)"\33I[6:39]";
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

Object *otbutton(ULONG label, ULONG help)
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

Object *ocheckmark(ULONG key, ULONG help)
{
    Object *obj;

    if((obj = MUI_MakeObject(MUIO_Checkmark, (IPTR)getString(key))) != NULL)
        SetAttrs(obj,MUIA_CycleChain,TRUE,_HELP(help),TAG_DONE);

    return obj;
}

/****************************************************************************/

Object *opopbutton(ULONG img, ULONG help)
{
    Object *obj;

    if((obj = MUI_MakeObject(MUIO_PopButton, img)) != NULL)
        SetAttrs(obj,MUIA_CycleChain,TRUE,_HELP(help),TAG_DONE);

    return obj;
}

/****************************************************************************/

Object *ostring(ULONG maxlen, ULONG key, ULONG help)
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

Object *opopport(ULONG maxLen, ULONG key, ULONG help)
{
    return popportObject,
        _HELP(help),
        MUIA_Popport_Key, key,
        MUIA_Popport_Len, maxLen,
    End;
}

/***********************************************************************/

Object *opopph(CONST_STRPTR *syms, STRPTR *names, ULONG maxLen, ULONG key, ULONG asl, ULONG help)
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

ULONG openWindow(Object *app, Object *win)
{
    ULONG v;

    if (win)
    {
        set(win,MUIA_Window_Open,TRUE);
        v = xget(win, MUIA_Window_Open);
        if (!v)
          v = xget(app, MUIA_Application_Iconified);
    }
    else v = FALSE;

    if (!v) DisplayBeep(0);

    return v;
}

/***********************************************************************/

IPTR delEntry(Object *obj, APTR entry)
{
    APTR e = NULL;
    int  i;

    for (i = 0; ;i++)
    {
        DoMethod(obj,MUIM_List_GetEntry,i,(IPTR)&e);
        if (!e || e==entry) break;
    }

    if (e) DoMethod(obj,MUIM_List_Remove,i);

    return (IPTR)e;
}

/**************************************************************************/
