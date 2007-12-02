/*
**  openurl.library - universal URL display and browser
**  launcher library
**
**  Written by Troels Walsted Hansen <troels@thule.no>
**  Placed in the public domain.
**
**  Developed by:
**  - Alfonso Ranieri <alforan@tin.it>
**  - Stefan Kost <ensonic@sonicpulse.de>
*/


#include "lib.h"
#include <dos/dostags.h>
#include "openurl.library_rev.h"
#include <exec/execbase.h>

#ifdef __AROS__
// MAXRMARG doesn't exist in AROS's headers
// The value 15 comes from the AOS3.9 SDK
#define MAXRMARG (15)
#endif

/**************************************************************************/

#ifdef __AROS__
AROS_LH2(ULONG, URL_OpenA, 
    AROS_LHA(STRPTR, URL, A0),
    AROS_LHA(struct TagItem *, attrs, A1),
    struct Library *, library, 5, Openurl
)
{
    AROS_LIBFUNC_INIT
#else
ULONG LIBCALL
URL_OpenA(REG(a0,UBYTE *URL),REG(a1,struct TagItem *attrs))
{
#endif
    struct List portList;
    UBYTE       buf[256], *fullURL = NULL, *pubScreenName;
    ULONG       res, show, toFront, newWindow, launch, httpPrepend = FALSE;

    NEWLIST(&portList);
    ObtainSemaphore(&lib_prefsSem);


    /* parse arguments */
    pubScreenName = (UBYTE *)GetTagData(URL_PubScreenName,(ULONG)"Workbench",attrs);
    show          = GetTagData(URL_Show,lib_prefs->up_DefShow,attrs);
    toFront       = GetTagData(URL_BringToFront,lib_prefs->up_DefBringToFront,attrs);
    newWindow     = GetTagData(URL_NewWindow,lib_prefs->up_DefNewWindow,attrs);
    launch        = GetTagData(URL_Launch,lib_prefs->up_DefLaunch,attrs);

    /* make a copy of the global list of named ports */
    Forbid();
    res = copyList(&portList,&SysBase->PortList,sizeof(struct Node));
    Permit();
    if (!res) goto done;

    /* prepend "http://" if URL has no method */
    if (lib_prefs->up_Flags & UPF_PREPENDHTTP)
    {
        UBYTE *colon;

        colon = strchr(URL,':');

        if (!colon) httpPrepend = TRUE;
        else
        {
            UBYTE *p;

            for (p = URL; p<colon; p++)
            {
                if (!isalnum(*p) && (*p!='+') && (*p!='-'))
                {
                    httpPrepend = TRUE;
                    break;
                }
            }
        }
    }

    if (httpPrepend)
    {
        ULONG len = strlen(URL)+8;

        if (len>sizeof(buf))
        {
            if (!(fullURL = allocVecPooled(strlen(URL)+8))) goto done;
        }
        else fullURL = buf;

        msprintf(fullURL,"http://%s",(ULONG)URL);
    }
    else fullURL = URL;

    /* Be case insensitive - Piru */
    if ((lib_prefs->up_Flags & UPF_DOMAILTO) && !Strnicmp(URL,"mailto:",7))
        res = sendToMailer(fullURL,&portList,show,toFront,launch,pubScreenName);
    else
        if ((lib_prefs->up_Flags & UPF_DOFTP) && !Strnicmp(URL,"ftp://",6))
            res = sendToFTP(fullURL,&portList,show,toFront,newWindow,launch,pubScreenName);
        else res = sendToBrowser(fullURL,&portList,show,toFront,newWindow,launch,pubScreenName);

done:
    ReleaseSemaphore(&lib_prefsSem);
    freeList(&portList,sizeof(struct Node));
    if (httpPrepend && fullURL && fullURL!=buf) freeVecPooled(fullURL);

    return res;

#ifdef __AROS__
    AROS_LIBFUNC_EXIT
#endif
}

/**************************************************************************/

#ifdef __AROS__
AROS_LH1(struct URL_Prefs *, URL_GetPrefsA, 
    AROS_LHA(struct TagItem *, attrs, A0),
    struct Library *, library, 12, Openurl
)
{
    AROS_LIBFUNC_INIT
#else
struct URL_Prefs * LIBCALL
URL_GetPrefsA(REG(a0,struct TagItem *attrs))
{
#endif
    struct URL_Prefs *p;
    ULONG            mode;

    mode = GetTagData(URL_GetPrefs_Mode,URL_GetPrefs_Mode_Env,attrs);

    if (mode==URL_GetPrefs_Mode_Default)
    {
        if (!(p = allocPooled(sizeof(struct URL_Prefs)))) return NULL;
        setDefaultPrefs(p);

        return p;
    }

    if ((mode==URL_GetPrefs_Mode_Env) || (mode==URL_GetPrefs_Mode_Envarc))
    {
        mode = (mode==URL_GetPrefs_Mode_Env) ? LOADPREFS_ENV : LOADPREFS_ENVARC;

        if (!(p = allocPooled(sizeof(struct URL_Prefs))))
            return NULL;

        if (loadPrefs(p,mode))
            return p;

        if (GetTagData(URL_GetPrefs_FallBack,TRUE,attrs))
        {

            if ((mode==LOADPREFS_ENV) && loadPrefs(p,LOADPREFS_ENVARC))
                return p;

            setDefaultPrefs(p);

            return p;
        }

        URL_FreePrefsA(p,NULL);

        return NULL;
    }

    ObtainSemaphoreShared(&lib_prefsSem);
    p = copyPrefs(lib_prefs);
    ReleaseSemaphore(&lib_prefsSem);

    return p;
#ifdef __AROS__
    AROS_LIBFUNC_EXIT
#endif
}

/**************************************************************************/

#ifdef __AROS__
AROS_LH0(struct URL_Prefs *, URL_OldGetPrefs, 
    struct Library *, library, 6, Openurl
)
{
    AROS_LIBFUNC_INIT
#else
struct URL_Prefs * LIBCALL
URL_OldGetPrefs(void)
{
#endif
    return URL_GetPrefsA(NULL);
#ifdef __AROS__
    AROS_LIBFUNC_EXIT
#endif
}

/**************************************************************************/

#ifdef __AROS__
AROS_LH2(VOID, URL_FreePrefsA, 
    AROS_LHA(struct URL_Prefs *, p, A0),
    AROS_LHA(struct TagItem *, attrs, A1),
    struct Library *, library, 13, Openurl
)
{
    AROS_LIBFUNC_INIT
#else
void LIBCALL
URL_FreePrefsA(REG(a0,struct URL_Prefs *p),REG(a1,struct TagItem *attrs))
{
#endif
    if (p)
    {
        freeList((struct List *)&p->up_BrowserList,sizeof(struct URL_BrowserNode));
        freeList((struct List *)&p->up_MailerList,sizeof(struct URL_MailerNode));
        freeList((struct List *)&p->up_FTPList,sizeof(struct URL_FTPNode));
        freePooled(p,sizeof(*p));
    }
#ifdef __AROS__
    AROS_LIBFUNC_EXIT
#endif
}

/**************************************************************************/

#ifdef __AROS__
AROS_LH1(VOID, URL_OldFreePrefs, 
    AROS_LHA(struct URL_Prefs *, p, A0),
    struct Library *, library, 7, Openurl
)
{
    AROS_LIBFUNC_INIT
#else
void LIBCALL
URL_OldFreePrefs(REG(a0,struct URL_Prefs *p))
{
#endif
    URL_FreePrefsA(p,NULL);
#ifdef __AROS__
    AROS_LIBFUNC_EXIT
#endif
}

/**************************************************************************/

#ifdef __AROS__
AROS_LH2(ULONG, URL_SetPrefsA, 
    AROS_LHA(struct URL_Prefs *, p, A0),
    AROS_LHA(struct TagItem *, attrs, A1),
    struct Library *, library, 14, Openurl
)
{
    AROS_LIBFUNC_INIT
#else
ULONG LIBCALL
URL_SetPrefsA(REG(a0,struct URL_Prefs *p),REG(a1,struct TagItem *attrs))
{
#endif
    ULONG res = FALSE;

    if (p->up_Version==PREFS_VERSION)
    {
        struct URL_Prefs *newp;

        ObtainSemaphore(&lib_prefsSem);

        if ((newp = copyPrefs(p)))
        {
            newp->up_Version = PREFS_VERSION;
            newp->up_Flags &= ~UPF_ISDEFAULTS;

            URL_FreePrefsA(lib_prefs,NULL);
            lib_prefs = newp;

            if ((res = savePrefs(DEF_ENV,lib_prefs)))
            {
                if (GetTagData(URL_SetPrefs_Save,FALSE,attrs))
                {
                    res = savePrefs(DEF_ENVARC,lib_prefs);
                }
            }
        }

        ReleaseSemaphore(&lib_prefsSem);
    }

    return res;
#ifdef __AROS__
    AROS_LIBFUNC_EXIT
#endif
}

/**************************************************************************/

#ifdef __AROS__
AROS_LH2(ULONG, URL_OldSetPrefs, 
    AROS_LHA(struct URL_Prefs *, p, A0),
    AROS_LHA(BOOL, save, D0),
    struct Library *, library, 8, Openurl
)
{
    AROS_LIBFUNC_INIT
#else
ULONG LIBCALL
URL_OldSetPrefs(REG(a0,struct URL_Prefs *p),REG(d0,ULONG save))
{
#endif
    struct TagItem stags[] = { {URL_SetPrefs_Save,0} , {TAG_DONE} };

    stags[0].ti_Data = save;

    return URL_SetPrefsA(p,stags);
#ifdef __AROS__
    AROS_LIBFUNC_EXIT
#endif
}

/**************************************************************************/

#ifdef __AROS__
AROS_LH0(struct URL_Prefs *, URL_OldGetDefaultPrefs, 
    struct Library *, library, 9, Openurl
)
{
    AROS_LIBFUNC_INIT
#else
struct URL_Prefs * LIBCALL
URL_OldGetDefaultPrefs(void)
{
#endif
    struct TagItem gtags[] = { {URL_GetPrefs_Mode,URL_GetPrefs_Mode_Default} , {TAG_DONE} };

    return URL_GetPrefsA(gtags);
#ifdef __AROS__
    AROS_LIBFUNC_EXIT
#endif
}

/**************************************************************************/

#ifdef __AROS__
AROS_LH1(ULONG, URL_LaunchPrefsAppA, 
    AROS_LHA(struct TagItem *, tags, A0),
    struct Library *, library, 15, Openurl
)
{
    AROS_LIBFUNC_INIT
#else
ULONG LIBCALL
URL_LaunchPrefsAppA(REG(a0,struct TagItem *attrs))
{
#endif
    BPTR in;

    if ((in  = Open("NIL:",MODE_OLDFILE)))
    {
        BPTR out;

        if ((out = Open("NIL:",MODE_OLDFILE)))
        {
            UBYTE      name[256];
            struct TagItem stags[] = {{SYS_Input,       0},
                                      {SYS_Output,      0},
                                      {NP_StackSize,    16000},
                                      {SYS_Asynch,      TRUE},
                                      #ifdef __MORPHOS__
                                      {NP_PPCStackSize, 32000},
                                      #endif
                                      {TAG_DONE}};

            if (GetVar("OpenURL_Prefs_Path",name,sizeof(name),GVF_GLOBAL_ONLY)<=0)
                strcpy(name,"Sys:Prefs/OpenURL");

            stags[0].ti_Data = (ULONG)in;
            stags[1].ti_Data = (ULONG)out;
            SystemTagList(name,stags);

            return TRUE;
        }

        Close(in);
    }

    return FALSE;
#ifdef __AROS__
    AROS_LIBFUNC_EXIT
#endif
}

/**************************************************************************/

#ifdef __AROS__
AROS_LH0(ULONG, URL_OldLaunchPrefsApp, 
    struct Library *, library, 10, Openurl
)
{
    AROS_LIBFUNC_INIT
#else
ULONG LIBCALL
URL_OldLaunchPrefsApp(void)
{
#endif
    return URL_LaunchPrefsAppA(NULL);
#ifdef __AROS__
    AROS_LIBFUNC_EXIT
#endif
}

/**************************************************************************/

#ifdef __AROS__
AROS_LH2(ULONG, URL_GetAttr, 
    AROS_LHA(ULONG, attr, D0),
    AROS_LHA(ULONG *, storage, A0),
    struct Library *, library, 16, Openurl
)
{
    AROS_LIBFUNC_INIT
#else
ULONG LIBCALL
URL_GetAttr(REG(d0,ULONG attr),REG(a0,ULONG *storage))
{
#endif
    switch (attr)
    {
        case URL_GetAttr_Version:          *storage = VERSION;        return TRUE;
        case URL_GetAttr_Revision:         *storage = REVISION;       return TRUE;
        case URL_GetAttr_VerString:        *storage = (ULONG)PRGNAME; return TRUE;

        case URL_GetAttr_PrefsVer:         *storage = PREFS_VERSION;  return TRUE;

        case URL_GetAttr_HandlerVersion:   *storage = 0;              return TRUE;
        case URL_GetAttr_HandlerRevision:  *storage = 0;              return TRUE;
        case URL_GetAttr_HandlerVerString: *storage = (ULONG)"";      return TRUE;

        default: return FALSE;
    }
#ifdef __AROS__
    AROS_LIBFUNC_EXIT
#endif
}

/**************************************************************************/

#ifdef __MORPHOS__
LONG dispatch(void)
{
    struct RexxMsg *msg = (struct RexxMsg *)REG_A0;
#elif defined(__AROS__)
// FIXME: implement me correctly
LONG dispatch(void)
{
    struct RexxMsg *msg = 0;
#else
LONG ASM SAVEDS dispatch(REG(a0,struct RexxMsg *msg),REG(a1,UBYTE **resPtr))
{
#endif

    UBYTE  *fun = msg->rm_Args[0];
    ULONG  res, na = msg->rm_Action & RXARGMASK;

    if (!stricmp(fun,"OPENURL"))
    {
        if (na<1) return 17;
        else
        {
            struct TagItem tags[MAXRMARG+1];
            UBYTE          *url;
            int            i, j;

            for (i = na, j = 0, url = NULL; i>0; i--)
            {
                UBYTE *arg = msg->rm_Args[i];
                Tag   tag;

                if (!arg || !*arg) continue;

                if (!stricmp(arg,"SHOW") || !stricmp(arg,"NOSHOW")) tag = URL_Show;
                else if (!stricmp(arg,"TOFRONT") || !stricmp(arg,"NOTOFRONT")) tag  = URL_BringToFront;
                     else if (!stricmp(arg,"NEWWIN") || !stricmp(arg,"NONEWWIN")) tag = URL_NewWindow;
                          else if (!stricmp(arg,"LAUNCH") || !stricmp(arg,"NOLAUNCH")) tag  = URL_Launch;
                               else
                               {
                                   url = arg;
                                   continue;
                               }

                tags[j].ti_Tag  = tag;
                tags[j++].ti_Data = strnicmp(arg,"NO",2);
            }

            tags[j].ti_Tag = TAG_END;

            res = url && URL_OpenA(url,tags);
        }
    }
    else
    {
        if (!stricmp(fun,"OPENURLPREFS"))
        {
            if (na!=0) return 17;

            res = URL_LaunchPrefsAppA(NULL);
        }
        else return 1;
    }

#ifdef __MORPHOS__
    return (REG_A0 = (ULONG)CreateArgstring(res ? "1" : "0",1)) ? 0 : 3;
#elif defined(__AROS__)
    return 0; // FIXME: implement me correctly
#else
    return (*resPtr = CreateArgstring(res ? "1" : "0",1)) ? 0 : 3;
#endif
}

/**************************************************************************/

#ifdef __AROS__
AROS_LH1(VOID, DoFunction, 
    AROS_LHA(STRPTR, rxmsg, A0),
    struct Library *, library, 11, Openurl
)
{
    AROS_LIBFUNC_INIT
// FIXME: implement my correctly
    AROS_LIBFUNC_EXIT
}
#endif

/**************************************************************************/
