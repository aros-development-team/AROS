/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2009 by TextEditor.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc

 $Id$

***************************************************************************/

#ifndef TEXTEDITOR_MCC_PRIV_H
#define TEXTEDITOR_MCC_PRIV_H

#include <graphics/rastport.h>
#include <libraries/iffparse.h>
#include <proto/intuition.h>

#include <libraries/mui.h>
#include "muiextra.h"

#include <mcc_common.h>

#include <mui/TextEditor_mcc.h>

// if something in our configuration setup (keybindings, etc)
// has changed we can increase the config version so that TextEditor
// will popup a warning about and obsolete configuration.
#define CONFIG_VERSION 4

#define EOS        (unsigned short)-1
#define EOC        (unsigned short)0xffff

#define UNDERLINE 0x01
#define BOLD      0x02
#define ITALIC    0x04
#define COLOURED  0x08

// define memory flags not existing on older platforms
#ifndef MEMF_SHARED
#if defined(__MORPHOS__)
#define MEMF_SHARED MEMF_ANY
#else
#define MEMF_SHARED MEMF_PUBLIC
#endif
#endif

#if defined(__MORPHOS__)
#include <proto/exec.h>
#define IS_MORPHOS2 (((struct Library *)SysBase)->lib_Version >= 51)
#endif

// proper RAWKEY_ defines were first introduced in OS4 and MorphOS
// and unfortunately they are also a bit different, so lets
// prepare an alternate table for it
#if defined(__amigaos4__)
#include <proto/keymap.h>

#define RAWKEY_NUMLOCK    0x79

#elif defined(__MORPHOS__)
#include <devices/rawkeycodes.h>

#define RAWKEY_CRSRUP     RAWKEY_UP
#define RAWKEY_CRSRDOWN   RAWKEY_DOWN
#define RAWKEY_CRSRRIGHT  RAWKEY_RIGHT
#define RAWKEY_CRSRLEFT   RAWKEY_LEFT
#define RAWKEY_PRINTSCR   RAWKEY_PRTSCREEN
#define RAWKEY_BREAK      RAWKEY_PAUSE

#define RAWKEY_AUD_STOP       RAWKEY_CDTV_STOP
#define RAWKEY_AUD_PLAY_PAUSE RAWKEY_CDTV_PLAY
#define RAWKEY_AUD_PREV_TRACK RAWKEY_CDTV_PREV
#define RAWKEY_AUD_NEXT_TRACK RAWKEY_CDTV_NEXT
#define RAWKEY_AUD_SHUFFLE    RAWKEY_CDTV_REW
#define RAWKEY_AUD_REPEAT     RAWKEY_CDTV_FF

#else

#define RAWKEY_INSERT    0x47 /* Not on classic keyboards */
#define RAWKEY_PAGEUP    0x48 /* Not on classic keyboards */
#define RAWKEY_PAGEDOWN  0x49 /* Not on classic keyboards */
#define RAWKEY_F11       0x4B /* Not on classic keyboards */
#define RAWKEY_CRSRUP    0x4C
#define RAWKEY_CRSRDOWN  0x4D
#define RAWKEY_CRSRRIGHT 0x4E
#define RAWKEY_CRSRLEFT  0x4F
#define RAWKEY_F1        0x50
#define RAWKEY_F2        0x51
#define RAWKEY_F3        0x52
#define RAWKEY_F4        0x53
#define RAWKEY_F5        0x54
#define RAWKEY_F6        0x55
#define RAWKEY_F7        0x56
#define RAWKEY_F8        0x57
#define RAWKEY_F9        0x58
#define RAWKEY_F10       0x59
#define RAWKEY_HELP      0x5F
#define RAWKEY_SCRLOCK   0x6B /* Not on classic keyboards */
#define RAWKEY_PRINTSCR  0x6D /* Not on classic keyboards */
#define RAWKEY_BREAK     0x6E /* Not on classic keyboards */
#define RAWKEY_F12       0x6F /* Not on classic keyboards */
#define RAWKEY_HOME      0x70 /* Not on classic keyboards */
#define RAWKEY_END       0x71 /* Not on classic keyboards */

#define RAWKEY_AUD_STOP       0x72
#define RAWKEY_AUD_PLAY_PAUSE 0x73
#define RAWKEY_AUD_PREV_TRACK 0x74
#define RAWKEY_AUD_NEXT_TRACK 0x75
#define RAWKEY_AUD_SHUFFLE    0x76
#define RAWKEY_AUD_REPEAT     0x77

#define RAWKEY_NUMLOCK   0x79

#endif

// some own usefull MUI-style macros to check mouse positions in objects
#define _between(a,x,b)           ((x)>=(a) && (x)<=(b))
#define _isinobject(o,x,y)         (_between(_mleft(o),(x),_mright (o)) && _between(_mtop(o) ,(y),_mbottom(o)))
#define _isinwholeobject(o,x,y)   (_between(_left(o),(x),_right (o)) && _between(_top(o) ,(y),_bottom(o)))

// own common macros
#define Enabled(data)   ((data)->blockinfo.enabled == TRUE && \
                         ((data)->blockinfo.startx != (data)->blockinfo.stopx || \
                          (data)->blockinfo.startline != (data)->blockinfo.stopline || \
                          (data)->selectmode == 3))


// private/expermental attribute definitions
#define MUIA_TextEditor_HorizontalScroll  (TextEditor_Dummy + 0x2d)
#define MUIA_TextEditor_Prop_Release      (TextEditor_Dummy + 0x01)
#define MUIA_TextEditor_PopWindow_Open    (TextEditor_Dummy + 0x03)

// special flagging macros
#define setFlag(mask, flag)             (mask) |= (flag)               // set the flag "flag" in "mask"
#define clearFlag(mask, flag)           (mask) &= ~(flag)              // clear the flag "flag" in "mask"
#define maskFlag(mask, flag)            (mask) &= (flag)               // mask the variable "mask" with flags "flag" bitwise
#define isAnyFlagSet(mask, flag)        (((mask) & (flag)) != 0)       // return TRUE if at least one of the flags is set
#define isFlagSet(mask, flag)           (((mask) & (flag)) == (flag))  // return TRUE if the flag is set
#define isFlagClear(mask, flag)         (((mask) & (flag)) == 0)       // return TRUE if the flag is NOT set

enum EventType
{
  ET_PASTECHAR = 0,
  ET_BACKSPACECHAR,
  ET_DELETECHAR,
  ET_SPLITLINE,
  ET_MERGELINES,
  ET_BACKSPACEMERGE,
  ET_PASTEBLOCK,
  ET_DELETEBLOCK,
  ET_DELETEBLOCK_NOMOVE,
  ET_STYLEBOLD,
  ET_STYLEUNBOLD,
  ET_STYLEITALIC,
  ET_STYLEUNITALIC,
  ET_STYLEUNDERLINE,
  ET_STYLEUNUNDERLINE,
  ET_REPLACEBLOCK,
};

enum CursorState
{
  CS_OFF = 0,
  CS_INACTIVE,
  CS_ACTIVE,
};

struct LineStyle
{
  UWORD column;
  UWORD style;
};

struct LineColor
{
  UWORD column;
  UWORD color;
};

struct LineNode
{
  STRPTR   Contents;      // Set this to the linecontents (allocated via the poolhandle)
  ULONG    Length;        // The length of the line (including the '\n')
  ULONG allocatedContents;
  struct LineStyle *Styles; // Set this to the styles used for this line (allocated via the poolhandle). The array is terminated by an (EOS,0) marker
  ULONG allocatedStyles;
  ULONG usedStyles;
  struct LineColor *Colors; // The colors to use (allocated via the poolhandle). The array is terminated by an (EOC,0) marker
  ULONG allocatedColors;
  ULONG usedColors;
  BOOL     Highlight;     // Set this to TRUE if you want the line to be highlighted
  UWORD    Flow;          // Use the MUIV_TextEditor_Flow_xxx values...
  UWORD    Separator;     // See definitions below
  BOOL     clearFlow;     // if the flow definition should be cleared on the next line
};

struct line_node
{
  struct line_node *next;     // Pointer to next line
  struct line_node *previous; // Pointer to previous line

  struct LineNode line;

  UWORD visual;               // How many lines are this line wrapped over
  UWORD flags;                // Different flags...
};

struct bookmark
{
  struct  line_node *line;
  UWORD   x;
};

struct marking
{
  BOOL    enabled;              // Boolean that indicates wether block is on/off
  struct  line_node *startline; // Line where blockings starts
  UWORD   startx;               // X place of start
  struct  line_node *stopline;  // Line where marking ends
  UWORD   stopx;                // X place of stop
};

struct pos_info
{
  struct  line_node *line;      // Pointer to actual line
  UWORD   lines;                // Lines down
  UWORD   x;                    // Chars in
  UWORD   bytes;                // Lines in bytes
  UWORD   extra;                // Lines+1 in bytes
};

struct UserAction
{
  enum EventType type;

  struct
  {
    UBYTE   character; // deletechar
    UBYTE   style;
    UBYTE   flow;
    UBYTE   separator;
    UBYTE   highlight;
  } del;

  STRPTR clip;         // deleteblock

  struct
  {
    UWORD x, y;        // pasteblock
  } blk;

  UWORD x, y;
};

struct ExportMessage
{
  APTR   UserData;     // This is set to what your hook returns (NULL the first time)
  STRPTR Contents;     // Pointer to the current line
  ULONG  Length;       // Length of Contents, including the '\n' character
  ULONG  SkipFront;    // amount of chars to skip at the front of the current line
  ULONG  SkipBack;     // amount of chars to skip at the back of the current line
  struct LineStyle *Styles;      // Pointer to array of words with style definition
  struct LineColor *Colors;      // pointer to array of words with color definitions
  BOOL   Highlight;    // is the current line highlighted?
  UWORD  Flow;         // Current lines textflow
  UWORD  Separator;    // Current line contains a separator bar? see below
  ULONG  ExportWrap;   // For your use only (reflects MUIA_TextEditor_ExportWrap)
  BOOL   Last;         // Set to TRUE if this is the last line
  APTR   data;         // pointer to the instance data of TextEditor.mcc (PRIVATE)
  BOOL   failure;      // something went wrong during the export
};

struct ImportMessage
{
  STRPTR  Data;               /* The first time the hook is called, then this will be either the value of MUIA_TextEditor_Contents, or the argument given to MUIM_TextEditor_Insert. */
  struct  LineNode *linenode; /* Pointer to a linenode, which you should fill out */
  APTR    PoolHandle;         /* A poolhandle, all allocations done for styles or contents must be made from this pool, and the size of the allocation must be stored in the first LONG */
  ULONG   ImportWrap;         /* For your use only (reflects MUIA_TextEditor_ImportWrap) */
};

struct InstData
{
  WORD    xpos;             // xpos of gadget
  WORD    ypos;             // ypos of gadget
  WORD    realypos;
  UWORD   height;           // font height
  UWORD   innerwidth;         // inner gadget width in pixels (-cursor)

  UWORD   CPos_X;           // Cursor x pos.
  struct  line_node *actualline;    // The actual line...
  WORD    pixel_x;          // Pixel x-pos of cursor. (for up/down movement)
  UWORD   style;            // Current style (bold-italic-underline)
  UWORD   Flow;
  UWORD   Separator;

  LONG    visual_y;         // The line nr of the top line
  LONG    totallines;         // Total number of lines
  LONG    maxlines;         // max visual lines in gadget
  ULONG   flags;

  UWORD   cursor_shown;       // Width of stored cursor
  Object  *object;          // Pointer to the object itself
  struct  BitMap *doublebuffer; // Doublebuffer for line-printing
  BOOL    mousemove;
  UWORD   smooth_wait;        // Counter to see if smooth scroll is happening
  BOOL    scrollaction;       // If scrolling takes place
  WORD    scr_direction;      // Scroll direction
  struct  RastPort  doublerp; // Doublebuffer rastport
  struct  RastPort  copyrp;
  struct  RastPort  tmprp;    // temporary rastport (for TextFit/TextLength checks)
  struct  marking   blockinfo;
  struct  Rectangle CursorPosition;
  struct  RastPort  *rport;
  struct  TextFont  *font;
  APTR    mypool;
  struct  Locale    *mylocale;
  struct  line_node *firstline;
  BOOL    shown;
  BOOL    update;

  struct  IFFHandle *iff;

  ULONG   StartSecs, StartMicros;
  UWORD   selectmode;

  Object  *slider;
  Object  *KeyUpFocus;
  Object  *PointerObj;

  ULONG   Rows, Columns;  // The value of the rows/columns tags

  struct te_key *RawkeyBindings;
  ULONG   blockqual;

  ULONG   textcolor;
  ULONG   backgroundcolor;
  ULONG   highlightcolor;
  ULONG   cursorcolor;
  ULONG   cursortextcolor;
  ULONG   markedcolor;
  ULONG   inactivecolor;
  ULONG   separatorshine;
  ULONG   separatorshadow;
  ULONG   allocatedpens;

  STRPTR  background;
  BOOL    fastbackground;
  BOOL    use_fixedfont;

  struct  TextFont  *normalfont;
  struct  TextFont  *fixedfont;

  UWORD           BlinkSpeed;
  UWORD           CursorWidth;
  struct  Hook    *DoubleClickHook;
  struct  Hook    *ExportHook;
  struct  Hook    *ImportHook;
  ULONG           ExportWrap;
  UWORD           ImportWrap;
  BOOL            HasChanged;
  UWORD           TabSize;
  ULONG           WrapBorder;
  ULONG           WrapMode;
  struct UserAction *undoSteps;   // pointer to memory for the undo actions
  ULONG           maxUndoSteps;   // how many steps can be put into the undoBuffer
  ULONG           usedUndoSteps;  // how many steps in the undoBuffer have been used so far
  ULONG           nextUndoStep;   // index of the next undo/redo step
  BOOL            userUndoBufferSize;
  BOOL            TypeAndSpell;
  BOOL            inactiveCursor;
  BOOL            selectPointer;
  BOOL            activeSelectPointer;
  APTR            SuggestWindow;
  APTR            SuggestListview;
  UWORD           SuggestSpawn;
  UWORD           LookupSpawn;
  const char *    SuggestCmd;
  const char *    LookupCmd;
  ULONG           clipcount;
  APTR            cliphandle;

  APTR            UpdateInfo;

  ULONG           HStart;

  UWORD           Pen;
  BOOL            NoNotify;
  ULONG           *colormap;

  struct  bookmark    bookmarks[4];

  struct  MUI_EventHandlerNode ehnode;
  struct  MUI_InputHandlerNode ihnode;
  struct  MUI_InputHandlerNode blinkhandler;

  UBYTE   CtrlChar;

  enum CursorState currentCursorState;
};

// AllocBitMap.c
struct BitMap * SAVEDS ASM MUIG_AllocBitMap(REG(d0, LONG), REG(d1, LONG), REG(d2, LONG), REG(d3, LONG flags), REG(a0, struct BitMap *));
void SAVEDS ASM MUIG_FreeBitMap(REG(a0, struct BitMap *));

// AllocFunctions.c
#if defined(__amigaos4__)
#define SHARED_MEMFLAG          MEMF_SHARED
#else
#define SHARED_MEMFLAG          MEMF_ANY
#endif

struct line_node *AllocLine(struct InstData *);
void FreeLine(struct InstData *, struct line_node *);
#if !defined(__amigaos4__) && !defined(__MORPHOS__) && !defined(__AROS__)
APTR AllocVecPooled(APTR, ULONG);
void FreeVecPooled(APTR, APTR);
#endif

// BlockOperators.c
STRPTR GetBlock(struct InstData *, struct marking *);
void RedrawArea(struct InstData *, UWORD, struct line_node *, UWORD, struct line_node *);
void NiceBlock(struct marking *, struct marking *);
LONG CutBlock(struct InstData *, BOOL, BOOL, BOOL);
LONG CutBlock2(struct InstData *, BOOL, BOOL, BOOL, struct marking *);

// CaseConversion.c
void Key_ToUpper(struct InstData *);
void Key_ToLower(struct InstData *);

// ClipboardServer.c
BOOL StartClipboardServer(void);
void ShutdownClipboardServer(void);
IPTR ClientStartSession(ULONG mode);
void ClientEndSession(IPTR session);
void ClientWriteChars(IPTR session, struct line_node *line, LONG start, LONG length);
void ClientWriteLine(IPTR session, struct line_node *line);
LONG ClientReadLine(IPTR session, struct line_node **line, ULONG *cset);

// ColorOperators.c
UWORD GetColor(UWORD, struct line_node *);
void AddColor(struct InstData *, struct marking *, UWORD);

// Dispatcher.c
void ResetDisplay(struct InstData *);
void RequestInput(struct InstData *);
void RejectInput(struct InstData *);

// EditorStuff.c
BOOL PasteClip(struct InstData *, LONG x, struct line_node *line);
BOOL SplitLine(struct InstData *, LONG x, struct line_node *, BOOL, struct UserAction *);
BOOL MergeLines(struct InstData *, struct line_node *);
BOOL RemoveChars(struct InstData *, LONG, struct line_node *, LONG);
BOOL PasteChars(struct InstData *, LONG, struct line_node *, LONG, const char *, struct UserAction *);

// ExportBlock.c
IPTR mExportBlock(struct IClass *, Object *, struct MUIP_TextEditor_ExportBlock *);

// ExportText.c
IPTR mExportText(struct IClass *, Object *, struct MUIP_TextEditor_ExportText *);

// GetSetAttrs.c
IPTR mGet(struct IClass *, Object *, struct opGet *);
IPTR mSet(struct IClass *, Object *, struct opSet *);

// HandleARexx.c
IPTR mHandleARexx(struct IClass *, Object *, struct MUIP_TextEditor_ARexxCmd *);

// HandleInput.c
IPTR mHandleInput(struct IClass *, Object *, struct MUIP_HandleEvent *);
void Key_Backspace(struct InstData *);
void Key_Delete(struct InstData *);
void Key_Return(struct InstData *);
void Key_Tab(struct InstData *);
void Key_Clear(struct InstData *);
void Key_Cut(struct InstData *);
void Key_Copy(struct InstData *);
void Key_Paste(struct InstData *);
void Key_DelLine(struct InstData *);
void ScrollIntoDisplay(struct InstData *);
void MarkText(struct InstData *, LONG, struct line_node *, LONG, struct line_node *);

// ImportText.c
struct line_node *ImportText(struct InstData *, char *, struct Hook *, LONG);

// InitConfig.c
void InitConfig(struct InstData *, Object *);
void FreeConfig(struct InstData *, struct MUI_RenderInfo *);

// Methods.c
IPTR mMarkText(struct InstData *, struct MUIP_TextEditor_MarkText *);
IPTR mBlockInfo(struct InstData *, struct MUIP_TextEditor_BlockInfo *);
IPTR mQueryKeyAction(struct IClass *, Object *, struct MUIP_TextEditor_QueryKeyAction *);
IPTR mClearText(struct IClass *, Object *, Msg);
IPTR mToggleCursor(struct IClass *, Object *, Msg);
IPTR mInputTrigger(struct IClass *, Object *, Msg);
ULONG InsertText(struct InstData *, STRPTR, BOOL);

// MixedFunctions.c
void AddClipping(struct InstData *);
void RemoveClipping(struct InstData *);
void FreeTextMem(struct InstData *, struct line_node *);
BOOL Init_LineNode(struct InstData *, struct line_node *, struct line_node *, CONST_STRPTR);
BOOL ExpandLine(struct InstData *, struct line_node *, LONG);
BOOL CompressLine(struct InstData *, struct line_node *);
LONG LineCharsWidth(struct InstData *, CONST_STRPTR);
ULONG VisualHeight(struct InstData *, struct line_node *);
void OffsetToLines(struct InstData *, LONG, struct line_node *, struct pos_info *);
LONG LineNr(struct InstData *, struct line_node *);
struct line_node *LineNode(struct InstData *, LONG);
void ScrollUp(struct InstData *, LONG, LONG);
void ScrollDown(struct InstData *, LONG, LONG);
void SetCursor(struct InstData *, LONG, struct line_node *, BOOL);
void DumpText(struct InstData *, LONG, LONG, LONG, BOOL);
void GetLine(struct InstData *, LONG, struct pos_info *);
LONG LineToVisual(struct InstData *, struct line_node *);

// Navigation.c
void SetBookmark(struct InstData *, UWORD);
void GotoBookmark(struct InstData *, UWORD);
void GoTop(struct InstData *);
void GoPreviousPage (struct InstData *);
void GoPreviousLine(struct InstData *);
void GoUp(struct InstData *);
void GoBottom(struct InstData *);
void GoNextPage(struct InstData *);
void GoNextLine(struct InstData *);
void GoDown(struct InstData *);
void GoNextWord(struct InstData *);
void GoEndOfLine(struct InstData *);
void GoNextSentence(struct InstData *);
void GoRight(struct InstData *);
void GoPreviousWord(struct InstData *);
void GoStartOfLine(struct InstData *);
void GoPreviousSentence(struct InstData *);
void GoLeft(struct InstData *);
BOOL CheckSep(struct InstData *, char);
BOOL CheckSent(struct InstData *, char);
void NextLine(struct InstData *);
ULONG FlowSpace(struct InstData *, UWORD, STRPTR);
void PosFromCursor(struct InstData *, WORD, WORD);

// Pointer.c
void SetupSelectPointer(struct InstData *data);
void CleanupSelectPointer(struct InstData *data);
void ShowSelectPointer(struct InstData *data, Object *obj);
void HideSelectPointer(struct InstData *data, Object *obj);

// PrintLineWithStyles.c
ULONG convert(UWORD);
LONG PrintLine(struct InstData *, LONG, struct line_node *, LONG, BOOL);
ULONG ConvertPen(struct InstData *, UWORD, BOOL);
void DrawSeparator(struct InstData *, struct RastPort *, WORD, WORD, WORD, WORD);

// Search.c
IPTR mSearch(struct IClass *, Object *, struct MUIP_TextEditor_Search *);
IPTR mReplace(struct IClass *, Object *, struct MUIP_TextEditor_Replace *);

// SetBlock.c
IPTR mSetBlock(struct InstData *, struct MUIP_TextEditor_SetBlock *msg);

// SpellChecker.c
void CheckWord(struct InstData *);
void SuggestWord(struct InstData *);
Object *SuggestWindow(struct InstData *);

// StyleOperators.c
void UpdateStyles(struct InstData *);
UWORD GetStyle(LONG, struct line_node *);
void AddStyle(struct InstData *, struct marking *, UWORD, BOOL);
void AddStyleToLine(struct InstData *, LONG, struct line_node *, LONG, UWORD);

// UndoFunctions.c
BOOL AddToUndoBuffer(struct InstData *, enum EventType, void *);
void ResetUndoBuffer(struct InstData *);
void ResizeUndoBuffer(struct InstData *, ULONG);
BOOL Undo(struct InstData *);
BOOL Redo(struct InstData *);

#if defined(DEBUG)
void DumpLine(struct line_node *line);
#else
#define DumpLine(line) ((void)0)
#endif

extern struct Hook ImPlainHook;
extern struct Hook ImEMailHook;
extern struct Hook ImMIMEHook;
extern struct Hook ImMIMEQuoteHook;

extern struct Hook ExportHookPlain;
extern struct Hook ExportHookEMail;
extern struct Hook ExportHookNoStyle;

struct UpdateData
{
  UWORD type;
  UWORD x;
  struct line_node *line;
  UWORD length;
  STRPTR characters;
};

#define IEQUALIFIER_SHIFT   0x0200
#define IEQUALIFIER_ALT     0x0400
#define IEQUALIFIER_COMMAND 0x0800

struct KeyAction
{
  BOOL  vanilla;
  UWORD key;
  ULONG qualifier;
  UWORD action;
};

enum
{
  FLG_HScroll        = 1L << 0,
  FLG_NumLock        = 1L << 1,
  FLG_ReadOnly       = 1L << 2,
  FLG_FastCursor     = 1L << 3,
  FLG_CheckWords     = 1L << 4,
  FLG_InsertMode     = 1L << 5,
  FLG_Quiet          = 1L << 6,
  FLG_PopWindow      = 1L << 7,
  FLG_UndoLost       = 1L << 8,
  FLG_Draw           = 1L << 9,
  FLG_InVGrp         = 1L << 10,
  FLG_Ghosted        = 1L << 11,
  FLG_OwnBackground  = 1L << 12,
  FLG_FreezeCrsr     = 1L << 13,
  FLG_Active         = 1L << 14,
  FLG_OwnFrame       = 1L << 15,
  FLG_ARexxMark      = 1L << 16,
  FLG_FirstInit      = 1L << 17,
  FLG_AutoClip       = 1L << 18,
  FLG_Activated      = 1L << 19, // the gadget was activated by MUIM_GoActive()
  FLG_ActiveOnClick  = 1L << 20, // should the gadget activated on click

  FLG_NumberOf
};

#define  MUIM_TextEditor_InputTrigger     0xad000101
#define  MUIM_TextEditor_ToggleCursor     0xad000102

// for iffparse clipboard management
#define ID_FTXT    MAKE_ID('F','T','X','T')
#define ID_CHRS    MAKE_ID('C','H','R','S')
#define ID_FLOW    MAKE_ID('F','L','O','W')
#define ID_HIGH    MAKE_ID('H','I','G','H')
#define ID_SBAR    MAKE_ID('S','B','A','R')
#define ID_COLS    MAKE_ID('C','O','L','S')
#define ID_STYL    MAKE_ID('S','T','Y','L')
#define ID_CSET    MAKE_ID('C','S','E','T')

#include "amiga-align.h"
struct te_key
{
  UWORD code;
  ULONG qual;
  UWORD act;
};
#include "default-align.h"

extern const struct te_key default_keybindings[];

/// xget()
//  Gets an attribute value from a MUI object
ULONG xget(Object *obj, const IPTR attr);
#if defined(__GNUC__)
  // please note that we do not evaluate the return value of GetAttr()
  // as some attributes (e.g. MUIA_Selected) always return FALSE, even
  // when they are supported by the object. But setting b=0 right before
  // the GetAttr() should catch the case when attr doesn't exist at all
  #define xget(OBJ, ATTR) ({IPTR b=0; GetAttr(ATTR, OBJ, &b); b;})
#endif
///

#endif /* TEXTEDITOR_MCC_PRIV_H */
