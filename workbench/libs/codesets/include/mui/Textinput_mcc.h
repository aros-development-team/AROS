/*
** Textinput.mcc
** -------------
**
** General textual input MUI class
**
** (C) 1997-2001 Oliver Wagner <owagner@vapor.com>
** All Rights Reserved
**
*/

#ifndef TEXTINPUT_MCC_H
#define TEXTINPUT_MCC_H

#ifndef LIBRARIES_MUI_H
#include <libraries/mui.h>
#endif

/*
** Class name, object macros
*/

#define MUIC_Textinput "Textinput.mcc"
#define TextinputObject MUI_NewObject(MUIC_Textinput

#define MUIC_Textinputscroll "Textinputscroll.mcc"
#define TextinputscrollObject MUI_NewObject(MUIC_Textinputscroll


#define MCC_TI_TAGBASE ((TAG_USER)|((1307<<16)+0x712))
#define MCC_TI_ID(x) (MCC_TI_TAGBASE+(x))

#define MCC_Textinput_Version 29
#define MCC_Textinput_Revision 1

#ifdef __GNUC__
  #ifdef __PPC__
    #pragma pack(2)
  #endif
#elif defined(__VBCC__)
  #pragma amiga-align
#endif


/*
** Methods
*/

#define MUIM_Textinput_ExternalEdit MCC_TI_ID(1)        /* V1 */
#define MUIM_Textinputscroll_Inform MCC_TI_ID(2)        /* V1 (private) */
#define MUIM_Textinputmcp_GrabCols MCC_TI_ID(3)         /* V1 (private) */
#define MUIM_Textinput_Blink MCC_TI_ID(4)               /* V1 (private) */
#define MUIM_Textinput_SaveToFile MCC_TI_ID(5)          /* V1 */
#define MUIM_Textinput_LoadFromFile MCC_TI_ID(6)        /* V1 */
#define MUIM_Textinput_ExternalEditDone MCC_TI_ID(7)    /* V1 (private) */
#define MUIM_Textinput_DoRevert MCC_TI_ID(8)            /* V1 */
#define MUIM_Textinput_DoDelLine MCC_TI_ID(9)           /* V1 */
#define MUIM_Textinput_DoMarkStart MCC_TI_ID(10)        /* V1 */
#define MUIM_Textinput_DoMarkAll MCC_TI_ID(11)          /* V1 */
#define MUIM_Textinput_DoCut MCC_TI_ID(12)              /* V1 */
#define MUIM_Textinput_DoCopy MCC_TI_ID(13)             /* V1 */
#define MUIM_Textinput_DoPaste MCC_TI_ID(14)            /* V1 */
#define MUIM_Textinput_AppendText MCC_TI_ID(15)         /* V1 */
#define MUIM_Textinputmcp_LAct MCC_TI_ID(16)            /* V1 (private) */
#define MUIM_Textinputmcp_LCopy MCC_TI_ID(17)           /* V1 (private) */
#define MUIM_Textinputmcp_LAdd MCC_TI_ID(18)            /* V1 (private) */
#define MUIM_Textinput_DoToggleWordwrap MCC_TI_ID(19)   /* V1 */
#define MUIM_Textinput_Acknowledge MCC_TI_ID(20)        /* V1 */
#define MUIM_Textinput_TranslateEvent MCC_TI_ID(21)     /* V1 */
#define MUIM_Textinput_InsertText MCC_TI_ID(22)         /* V1 */
#define MUIM_Textinput_DoLeft MCC_TI_ID(23)             /* V1 */
#define MUIM_Textinput_DoRight MCC_TI_ID(24)            /* V1 */
#define MUIM_Textinput_DoUp MCC_TI_ID(25)               /* V1 */
#define MUIM_Textinput_DoDown MCC_TI_ID(26)             /* V1 */
#define MUIM_Textinput_DoLineStart MCC_TI_ID(27)        /* V1 */
#define MUIM_Textinput_DoLineEnd MCC_TI_ID(28)          /* V1 */
#define MUIM_Textinput_DoTop MCC_TI_ID(29)              /* V1 */
#define MUIM_Textinput_DoBottom MCC_TI_ID(30)           /* V1 */
#define MUIM_Textinput_DoPageUp MCC_TI_ID(31)           /* V1 */
#define MUIM_Textinput_DoPageDown MCC_TI_ID(32)         /* V1 */
#define MUIM_Textinput_DoPopup MCC_TI_ID(33)            /* V1 */
#define MUIM_Textinput_DoPrevWord MCC_TI_ID(34)         /* V1 */
#define MUIM_Textinput_DoNextWord MCC_TI_ID(35)         /* V1 */
#define MUIM_Textinput_DoDel MCC_TI_ID(36)              /* V1 */
#define MUIM_Textinput_DoDelEOL MCC_TI_ID(37)           /* V1 */
#define MUIM_Textinput_DoBS MCC_TI_ID(38)               /* V1 */
#define MUIM_Textinput_DoBSSOL MCC_TI_ID(39)            /* V1 */
#define MUIM_Textinput_DoubleClick MCC_TI_ID(42)        /* V1 */
#define MUIM_Textinput_DoBSWord MCC_TI_ID(43)           /* V1 */
#define MUIM_Textinput_DoDelWord MCC_TI_ID(44)          /* V1 */
#define MUIM_Textinput_DoInsertFile MCC_TI_ID(45)       /* V1 */
#define MUIM_Textinput_InsertFromFile MCC_TI_ID(46)     /* V1 */
#define MUIM_Textinput_HandleChar MCC_TI_ID(47)         /* V14 */
#define MUIM_Textinput_HandleURL MCC_TI_ID(48)          /* V16 */
#define MUIM_Textinput_HandleRexxSignal MCC_TI_ID(49)   /* V18 (Private) */
#define MUIM_Textinput_HandleMisspell MCC_TI_ID(50)     /* V18 (Private) */
#define MUIM_Textinput_DoToggleCase MCC_TI_ID(51)       /* V21 */
#define MUIM_Textinput_DoToggleCaseEOW MCC_TI_ID(52)    /* V21 */
#define MUIM_Textinput_DoIncrementDec MCC_TI_ID(53)     /* V21 */
#define MUIM_Textinput_DoDecrementDec MCC_TI_ID(54)     /* V21 */
#define MUIM_Textinputmcp_DefaultKeys MCC_TI_ID(55)     /* V21 (Private) */
#define MUIM_Textinput_DoUndo MCC_TI_ID(56)             /* V21 */
#define MUIM_Textinput_DoRedo MCC_TI_ID(57)             /* V21 */
#define MUIM_Textinput_DoTab MCC_TI_ID(58)              /* V22 */
#define MUIM_Textinput_DoNextGadget MCC_TI_ID(59)       /* V22 */
#define MUIM_Textinput_DoSetBookmark1 MCC_TI_ID(60)     /* V22 */
#define MUIM_Textinput_DoSetBookmark2 MCC_TI_ID(61)     /* V22 */
#define MUIM_Textinput_DoSetBookmark3 MCC_TI_ID(62)     /* V22 */
#define MUIM_Textinput_DoGotoBookmark1 MCC_TI_ID(63)    /* V22 */
#define MUIM_Textinput_DoGotoBookmark2 MCC_TI_ID(64)    /* V22 */
#define MUIM_Textinput_DoGotoBookmark3 MCC_TI_ID(65)    /* V22 */
#define MUIM_Textinput_DoCutLine MCC_TI_ID(66)          /* V22 */
#define MUIM_Textinput_DoCopyCut MCC_TI_ID(67)          /* V29 */

/*
** Messages
*/

struct MUIP_Textinput_ExternalEdit { ULONG MethodID; };
struct MUIP_Textinputscroll_Inform { ULONG MethodID; ULONG xo; ULONG yo; ULONG xs; ULONG ys; ULONG xv; ULONG yv; ULONG noedit; };
struct MUIP_Textinputmcp_GrabCols  { ULONG MethodID; ULONG notall; };
struct MUIP_Textinput_Blink { ULONG MethodID; };
struct MUIP_Textinput_SaveToFile { ULONG MethodID; STRPTR filename; };
struct MUIP_Textinput_LoadFromFile { ULONG MethodID; STRPTR filename; };
struct MUIP_Textinput_ExternalEditDone { ULONG MethodID; ULONG changed; };
struct MUIP_Textinput_DoRevert { ULONG MethodID; };
struct MUIP_Textinput_DoDelLine { ULONG MethodID; };
struct MUIP_Textinput_DoCutLine { ULONG MethodID; };
struct MUIP_Textinput_DoMarkStart { ULONG MethodID; };
struct MUIP_Textinput_DoMarkAll { ULONG MethodID; };
struct MUIP_Textinput_DoCut { ULONG MethodID; };
struct MUIP_Textinput_DoCopyCut { ULONG MethodID; };
struct MUIP_Textinput_DoCopy { ULONG MethodID; };
struct MUIP_Textinput_DoPaste { ULONG MethodID; };
struct MUIP_Textinput_AppendText { ULONG MethodID; STRPTR text; LONG len; };
struct MUIP_Textinputmcp_LAct { ULONG MethodID; ULONG which; };
struct MUIP_Textinputmcp_LCopy { ULONG MethodID; };
struct MUIP_Textinputmcp_LAdd { ULONG MethodID; };
struct MUIP_Textinput_DoToggleWordwrap { ULONG MethodID; };
struct MUIP_Textinput_Acknowledge { ULONG MethodID; STRPTR contents; };
struct MUIP_Textinput_TranslateEvent { ULONG MethodID; struct InputEvent *ie; STRPTR mappedstring; ULONG *mappedlen; };
struct MUIP_Textinput_InsertText { ULONG MethodID; STRPTR text; LONG len; };
struct MUIP_Textinput_DoLeft { ULONG MethodID; };
struct MUIP_Textinput_DoRight { ULONG MethodID; };
struct MUIP_Textinput_DoUp { ULONG MethodID; };
struct MUIP_Textinput_DoDown { ULONG MethodID; };
struct MUIP_Textinput_DoLineStart { ULONG MethodID; };
struct MUIP_Textinput_DoLineEnd { ULONG MethodID; };
struct MUIP_Textinput_DoTop { ULONG MethodID; };
struct MUIP_Textinput_DoBottom { ULONG MethodID; };
struct MUIP_Textinput_DoPageUp { ULONG MethodID; };
struct MUIP_Textinput_DoPageDown { ULONG MethodID; };
struct MUIP_Textinput_DoPopup { ULONG MethodID; };
struct MUIP_Textinput_DoPrevWord { ULONG MethodID; };
struct MUIP_Textinput_DoNextWord { ULONG MethodID; };
struct MUIP_Textinput_DoDel { ULONG MethodID; };
struct MUIP_Textinput_DoDelEOL { ULONG MethodID; };
struct MUIP_Textinput_DoBS { ULONG MethodID; };
struct MUIP_Textinput_DoBSSOL { ULONG MethodID; };
struct MUIP_Textinput_DoubleClick { ULONG MethodID; ULONG xp; ULONG yp; ULONG cnt; };
struct MUIP_Textinput_DoDelWord { ULONG MethodID; };
struct MUIP_Textinput_DoBSWord { ULONG MethodID; };
struct MUIP_Textinput_DoInsertFile { ULONG MethodID; };
struct MUIP_Textinput_InsertFromFile { ULONG MethodID; STRPTR filename; };
struct MUIP_Textinput_HandleChar { ULONG MethodID; ULONG ch; ULONG quiet; };
struct MUIP_Textinput_HandleURL { ULONG MethodID; STRPTR url; };
struct MUIP_Textinput_HandleRexxSignal { ULONG MethodID; };
struct MUIP_Textinput_HandleMisspell { ULONG MethodID; STRPTR word; STRPTR pos; STRPTR correction; };
struct MUIP_Textinput_DoToggleCase { ULONG MethodID; };
struct MUIP_Textinput_DoToggleCaseEOW { ULONG MethodID; };
struct MUIP_Textinput_DoIncrementDec { ULONG MethodID; };
struct MUIP_Textinput_DoDecrementDec { ULONG MethodID; };
struct MUIP_Textinputmcp_DefaultKeys { ULONG MethodID; };
struct MUIP_Textinput_DoUndo { ULONG MethodID; };
struct MUIP_Textinput_DoRedo { ULONG MethodID; };
struct MUIP_Textinput_DoTab { ULONG MethodID; };
struct MUIP_Textinput_DoNextGadget { ULONG MethodID; };
struct MUIP_Textinput_DoSetBookmark1 { ULONG MethodID; };
struct MUIP_Textinput_DoSetBookmark2 { ULONG MethodID; };
struct MUIP_Textinput_DoSetBookmark3 { ULONG MethodID; };
struct MUIP_Textinput_DoGotoBookmark1 { ULONG MethodID; };
struct MUIP_Textinput_DoGotoBookmark2 { ULONG MethodID; };
struct MUIP_Textinput_DoGotoBookmark3 { ULONG MethodID; };


/*
** Attributes
*/

#define MUIA_Textinput_Multiline MCC_TI_ID(100)             /* V1 i.g BOOL */
#define MUIA_Textinput_MaxLen MCC_TI_ID(101)                /* V1 i.g ULONG */
#define MUIA_Textinput_MaxLines MCC_TI_ID(102)              /* V1 i.g ULONG */
#define MUIA_Textinput_AutoExpand MCC_TI_ID(103)            /* V1 isg BOOL */
#define MUIA_Textinput_Contents MCC_TI_ID(104)              /* V1 isg STRPTR */
#define MUIA_Textinput_LeftOffset MCC_TI_ID(105)            /* V1 .sg ULONG (private) */
#define MUIA_Textinput_TopOffset MCC_TI_ID(106)             /* V1 .sg ULONG (private) */
#define MUIA_Textinput_TSCO MCC_TI_ID(107)                  /* V1 .sg Object (private) */
#define MUIA_Textinput_Blinkrate MCC_TI_ID(108)             /* V1 isg ULONG */
#define MUIA_Textinput_Cursorstyle MCC_TI_ID(109)           /* V1 isg ULONG */
#define MUIA_Textinput_AdvanceOnCR MCC_TI_ID(110)           /* V1 isg BOOL */
#define MUIA_Textinput_TmpExtension MCC_TI_ID(111)          /* V1 isg STRPTR */
#define MUIA_Textinput_Quiet MCC_TI_ID(112)                 /* V1 .sg BOOL */
#define MUIA_Textinput_Acknowledge MCC_TI_ID(113)           /* V1 ..g STRPTR */
#define MUIA_Textinput_Integer MCC_TI_ID(114)               /* V1 isg ULONG */
#define MUIA_Textinput_MinVersion MCC_TI_ID(115)            /* V1 i.. ULONG */
#define MUIA_Textinput_DefKeys MCC_TI_ID(116)               /* V1 ..g APTR (private) */
#define MUIA_Textinput_DefaultPopup MCC_TI_ID(117)          /* V1 i.. BOOL */
#define MUIA_Textinput_WordWrap MCC_TI_ID(118)              /* V1 isg ULONG */
#define MUIA_Textinput_IsNumeric MCC_TI_ID(119)             /* V1 isg BOOL */
#define MUIA_Textinput_MinVal MCC_TI_ID(120)                /* V1 isg ULONG */
#define MUIA_Textinput_MaxVal MCC_TI_ID(121)                /* V1 isg ULONG */
#define MUIA_Textinput_AcceptChars MCC_TI_ID(122)           /* V1 isg STRPTR */
#define MUIA_Textinput_RejectChars MCC_TI_ID(123)           /* V1 isg STRPTR */
#define MUIA_Textinput_Changed MCC_TI_ID(124)               /* V1 ..g BOOL */
#define MUIA_Textinput_AttachedList MCC_TI_ID(125)          /* V1 isg Object */
#define MUIA_Textinput_RemainActive MCC_TI_ID(126)          /* V1 isg BOOL */
#define MUIA_Textinput_CursorPos MCC_TI_ID(127)             /* V1 .sg ULONG */
#define MUIA_Textinput_Secret MCC_TI_ID(128)                /* V1 isg BOOL */
#define MUIA_Textinput_Lines MCC_TI_ID(129)                 /* V1 ..g ULONG */
#define MUIA_Textinput_Editable MCC_TI_ID(130)              /* V1 isg BOOL */
#define MUIA_Textinputscroll_UseWinBorder MCC_TI_ID(131)    /* V1 i.. BOOL */
#define MUIA_Textinput_IsOld MCC_TI_ID(132)                 /* V1 isg BOOL */
#define MUIA_Textinput_MarkStart MCC_TI_ID(133)             /* V13 isg ULONG */
#define MUIA_Textinput_MarkEnd MCC_TI_ID(134)               /* V13 isg ULONG */
#define MUIA_Textinputscroll_VertScrollerOnly MCC_TI_ID(135)/* V14 i.. BOOL */
#define MUIA_Textinput_NoInput MCC_TI_ID(136)               /* V15 i.g BOOL */
#define MUIA_Textinput_SetMin MCC_TI_ID(137)                /* V15 isg BOOL */
#define MUIA_Textinput_SetMax MCC_TI_ID(138)                /* V15 isg BOOL */
#define MUIA_Textinput_SetVMax MCC_TI_ID(139)               /* V15 isg BOOL */
#define MUIA_Textinput_Styles MCC_TI_ID(140)                /* V15 isg ULONG */
#define MUIA_Textinput_PreParse MCC_TI_ID(141)              /* V18 isg STRPTR */
#define MUIA_Textinput_Format MCC_TI_ID(142)                /* V19 i.g ULONG */
#define MUIA_Textinput_SetVMin MCC_TI_ID(143)               /* V20 isg BOOL */
#define MUIA_Textinput_HandleURLHook MCC_TI_ID(144)         /* V22 isg struct Hook * */
#define MUIA_Textinput_Tabs MCC_TI_ID(145)                  /* V22 isg ULONG */
#define MUIA_Textinput_TabLen MCC_TI_ID(146)                /* V22 isg ULONG */
#define MUIA_Textinput_Bookmark1 MCC_TI_ID(147)             /* V22 isg ULONG */
#define MUIA_Textinput_Bookmark2 MCC_TI_ID(148)             /* V22 isg ULONG */
#define MUIA_Textinput_Bookmark3 MCC_TI_ID(149)             /* V22 isg ULONG */
#define MUIA_Textinput_CursorSize MCC_TI_ID(150)            /* V22 isg ULONG */
#define MUIA_Textinput_TopLine MCC_TI_ID(151)               /* V22 isg ULONG */
#define MUIA_Textinput_Font MCC_TI_ID(152)                  /* V23 isg ULONG */
#define MUIA_Textinput_SuggestParse MCC_TI_ID(153)          /* V24 isg ULONG */
#define MUIA_Textinput_ProhibitParse MCC_TI_ID(154)         /* V24 isg ULONG */
#define MUIA_Textinput_NoCopy MCC_TI_ID(155)                /* V26 isg ULONG */
#define MUIA_Textinput_MinimumWidth MCC_TI_ID(156)          /* V26 i.g ULONG */
#define MUIA_Textinput_ResetMarkOnCursor MCC_TI_ID(157)     /* V29 isg BOOL */
#define MUIA_Textinput_NoExtraSpacing MCC_TI_ID(158)        /* V29 isg BOOL */

/*
** Special values
*/

#define MUIV_Textinput_ParseB_URL      0
#define MUIV_Textinput_ParseB_Misspell 1
#define MUIV_Textinput_ParseF_URL      (1<<MUIV_Textinput_ParseB_URL)
#define MUIV_Textinput_ParseF_Misspell (1<<MUIV_Textinput_ParseB_Misspell)

#define MUIV_Textinput_Tabs_Ignore  0
#define MUIV_Textinput_Tabs_Spaces  1
#define MUIV_Textinput_Tabs_Disk    2

#define MUIV_Textinput_NoMark ((ULONG)~0)

#define MUIV_Textinput_Styles_None  0
#define MUIV_Textinput_Styles_MUI   1
#define MUIV_Textinput_Styles_IRC   2
#define MUIV_Textinput_Styles_Email 3
#define MUIV_Textinput_Styles_HTML  4

#define MUIV_Textinput_Format_Left      0
#define MUIV_Textinput_Format_Center    1
#define MUIV_Textinput_Format_Centre    1
#define MUIV_Textinput_Format_Right     2

#define MUIV_Textinput_Font_Normal 0
#define MUIV_Textinput_Font_Fixed  1

#ifdef __GNUC__
  #ifdef __PPC__
    #pragma pack()
  #endif
#elif defined(__VBCC__)
  #pragma default-align
#endif

#endif
