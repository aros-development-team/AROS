#ifndef URLTEXT_MCC_H
#define URLTEXT_MCC_H

/*
**  $VER: Urltext_private.h 18.9 (7.5.2003)
**  Includes Release 18.9
**
**  Urltext.mcc
**  Active Url MUI class
**
**  (C) 2000-2003 Alfonso Ranieri <alforan@tin.it>
**  All Rights Reserved
**
*/

#ifndef LIBRARIES_MUI_H
#include <libraries/mui.h>
#endif

#ifdef __GNUC__
   #ifdef __PPC__
    #pragma pack(2)
   #endif
#elif defined(__VBCC__)
   #pragma amiga-align
#endif

/***********************************************************************/

#define MUIC_Urltext       "Urltext.mcc"
#define UrltextObject      MUI_NewObject(MUIC_Urltext

/***********************************************************************/

#define MUISN_Alfie     0xFEC9
#define TAG_MUI_Alfie   (TAG_USER|(MUISN_Alfie<<16))

/***********************************************************************/

/* attributes - Scheme is: [ISGN] */
enum
{
    MUIA_Urltext_Base = (int)(TAG_MUI_Alfie+200),

    MUIA_Urltext_MouseOutPen,       /* [IS..] (struct MUI_PenSpec *) PRIVATE!           */
    MUIA_Urltext_MouseOverPen,      /* [IS..] (struct MUI_PenSpec *) PRIVATE!           */
    MUIA_Urltext_VisitedPen,        /* [IS..] (struct MUI_PenSpec *) PRIVATE!           */
    MUIA_Urltext_MouseOver,         /* [.S.N] (BOOL)                 PRIVATE!           */
    MUIA_Urltext_PUnderline,        /* [.S..] (BOOL)                 PRIVATE!           */
    MUIA_Urltext_PDoVisitedPen,     /* [.S..] (BOOL)                 PRIVATE!           */
    MUIA_Urltext_PFallBack,         /* [.S..] (BOOL)                 PRIVATE!           */

    MUIA_Urltext_Url,               /* [ISGN] (STRPTR)                                  */
    MUIA_Urltext_Text,              /* [ISGN] (STRPTR)                                  */
    MUIA_Urltext_Active,            /* [..G.] (BOOL)                                    */
    MUIA_Urltext_Visited,           /* [..GN] (BOOL)                                    */
    MUIA_Urltext_Underline,         /* [I...] (BOOL)                                    */
    MUIA_Urltext_FallBack,          /* [I...] (BOOL)                                    */
    MUIA_Urltext_DoVisitedPen,      /* [I...] (BOOL)                                    */
    MUIA_Urltext_SetMax,            /* [I...] (BOOL)                                    */
    MUIA_Urltext_DoOpenURL,         /* [I...] (BOOL)                                    */
    MUIA_Urltext_NoMenu,            /* [I...] (BOOL)                                    */

    MUIA_Urltext_Font,              /* PRIVATE!                                         */
    MUIA_Urltext_Version,           /* PRIVATE!                                         */

    MUIA_Urltext_NoOpenURLPrefs,    /* [I...] (BOOL)                                    */
};

/***********************************************************************/

/* methods */
enum
{
    MUIAM_Urltext_Base = (int)(TAG_MUI_Alfie+200),
    MUIM_Urltext_OpenURL,
    MUIM_Urltext_Copy,
    MUIM_Urltext_OpenURLPrefs,
    MUIM_Urltext_AddCM,
};

/***********************************************************************/

struct MUIP_Urltext_OpenURL
{
    ULONG   MethodID;
    ULONG   flags;
};

enum
{
    MUIV_Urltext_OpenURL_CheckOver = 1<<0,
};

struct MUIP_Urltext_Copy
{
    ULONG   MethodID;
    ULONG   unit;
};

/***********************************************************************/
/*
** Urltext defaults
*/

#define DEFAULT_MOUSEOUT_PEN    ((APTR)"m6")
#define DEFAULT_MOUSEOVER_PEN   ((APTR)"m0")
#define DEFAULT_VISITED_PEN     ((APTR)"m7")
#define DEFAULT_UNDERLINE       TRUE
#define DEFAULT_FALLBACK        TRUE
#define DEFAULT_DOVISITEDPEN    TRUE
#define DEFAULT_FONT            NULL
#define DEFAULT_SETMAX          TRUE
#define DEFAULT_DOOPENURL       TRUE

/***********************************************************************/
/*
** alfie's prefs
*/

#define ALFIE               MAKE_ID('a','l','f','i')

#define ALFIE_MOUSEOUT_PEN  ((APTR)"r00000000,404B404B,FFFFFFFF")
#define ALFIE_MOUSEOVER_PEN ((APTR)"rE0E0E0E0,FFFFFFFF,00000000")
#define ALFIE_VISITED_PEN   ((APTR)"rAF3BAF3B,2A2A2A2A,FFFFFFFF")
#define ALFIE_UNDERLINE     TRUE
#define ALFIE_FALLBACK      TRUE
#define ALFIE_DOVISITEDPEN  TRUE
#define ALFIE_SETMAX        TRUE

/****************************************************************************/

#ifdef __GNUC__
   #ifdef __PPC__
    #pragma pack()
   #endif
#elif defined(__VBCC__)
   #pragma default-align
#endif

#endif /* URLTEXT_MCC_H */
