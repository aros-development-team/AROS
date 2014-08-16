/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2014 TextEditor.mcc Open Source Team

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
#include <proto/exec.h>
#include <proto/intuition.h>

#include <libraries/mui.h>
#include "muiextra.h"

#include <mcc_common.h>

#include <mui/TextEditor_mcc.h>

#include <limits.h>

// if something in our configuration setup (keybindings, etc)
// has changed we can increase the config version so that TextEditor
// will popup a warning about and obsolete configuration.
#define CONFIG_VERSION 4

#define EOS       INT_MAX
#define EOC       INT_MAX

#define UNDERLINE 0x01
#define BOLD      0x02
#define ITALIC    0x04
#define COLOURED  0x08

#if defined(__amigaos4__)
#define AllocVecShared(size, flags)  AllocVecTags((size), AVT_Type, MEMF_SHARED, AVT_Lock, FALSE, ((flags)&MEMF_CLEAR) ? AVT_ClearWithValue : TAG_IGNORE, 0, TAG_DONE)
#else
#define AllocVecShared(size, flags)  AllocVec((size), (flags))
#endif

#define VERSION_IS_AT_LEAST(ver, rev, minver, minrev) (((ver) > (minver)) || ((ver) == (minver) && (rev) == (minrev)) || ((ver) == (minver) && (rev) > (minrev)))
#define LIB_VERSION_IS_AT_LEAST(lib, minver, minrev)  VERSION_IS_AT_LEAST(((struct Library *)(lib))->lib_Version, ((struct Library *)(lib))->lib_Revision, minver, minrev)

#if defined(__MORPHOS__)
#include <proto/exec.h>
#define IS_MORPHOS2 LIB_VERSION_IS_AT_LEAST(SysBase, 51, 0)
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

#ifndef MAX
#define MAX(a,b)          (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b)          (((a) < (b)) ? (a) : (b))
#endif
#ifndef MINMAX
#define MINMAX(min,x,max) (MAX((min),MIN((x),(max))))
#endif

enum EventType
{
  ET_NONE = 0,
  ET_PASTECHAR,
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
  LONG column;
  UWORD style;
};

struct LineColor
{
  LONG column;
  UWORD color;
};

struct LineNode
{
  STRPTR   Contents;      // Set this to the linecontents (allocated via the poolhandle)
  LONG     Length;        // The length of the line (including the '\n')
  LONG allocatedContents;
  struct LineStyle *Styles; // Set this to the styles used for this line (allocated via the poolhandle). The array is terminated by an (EOS,0) marker
  struct LineColor *Colors; // The colors to use (allocated via the poolhandle). The array is terminated by an (EOC,0) marker
  BOOL     Highlight;     // Set this to TRUE if you want the line to be highlighted
  UWORD    Flow;          // Use the MUIV_TextEditor_Flow_xxx values...
  UWORD    Separator;     // See definitions below
  BOOL     clearFlow;     // if the flow definition should be cleared on the next line
};

struct line_node
{
  struct MinNode node;        // standard Exec MinNode

  struct LineNode line;

  LONG visual;                // How many lines are this line wrapped over
  UWORD flags;                // Different flags...
};

struct bookmark
{
  struct line_node *line;
  LONG x;
};

struct marking
{
  BOOL enabled;                // Boolean that indicates wether block is on/off
  struct line_node *startline; // Line where blockings starts
  LONG startx;                 // X place of start
  struct line_node *stopline;  // Line where marking ends
  LONG stopx;                  // X place of stop
};

struct pos_info
{
  struct line_node *line; // Pointer to actual line
  LONG lines;             // Lines down
  LONG x;                 // Chars in
  LONG bytes;             // Lines in bytes
  LONG extra;             // Lines+1 in bytes
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
    LONG x;            // pasteblock
    LONG y;            // pasteblock
  } blk;

  LONG x;
  LONG y;
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
  LONG  ExportWrap;    // For your use only (reflects MUIA_TextEditor_ExportWrap)
  BOOL   Last;         // Set to TRUE if this is the last line
  APTR   data;         // pointer to the instance data of TextEditor.mcc (PRIVATE)
  BOOL   failure;      // something went wrong during the export
};

struct ImportMessage
{
  const char *Data;          // The first time the hook is called, then this will be either the value of MUIA_TextEditor_Contents, or the argument given to MUIM_TextEditor_Insert.
  struct LineNode *linenode; // Pointer to a linenode, which you should fill out
  APTR PoolHandle;           // A poolhandle, all allocations done for styles or contents must be made from this pool, and the size of the allocation must be stored in the first LONG
  LONG ImportWrap;           // For your use only (reflects MUIA_TextEditor_ImportWrap)
  ULONG ConvertTabs;         // do not convert to spaces when importing tabs (\t)
  LONG TabSize;              // if convert tabs to spaces we specify here how many spaces to use
};

struct Grow
{
  char *array;

  int itemSize;
  int itemCount;
  int maxItemCount;

  APTR pool;
};

struct InstData
{
  LONG    ypos;             // ypos of gadget
  LONG    fontheight;       // font height

  LONG    CPos_X;           // Cursor x pos.
  struct  line_node *actualline;    // The actual line...
  LONG    pixel_x;          // Pixel x-pos of cursor. (for up/down movement)
  UWORD   style;            // Current style (bold-italic-underline)
  UWORD   Flow;
  UWORD   Separator;

  LONG    visual_y;         // The line nr of the top line
  LONG    totallines;       // Total number of lines
  LONG    maxlines;         // max visual lines in gadget
  ULONG   flags;

  BOOL    cursor_shown;       // visibility of the cursor
  Object  *object;            // Pointer to the object itself
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
  struct  MinList    linelist;
  BOOL    shown;
  BOOL    update;

  struct  IFFHandle *iff;

  ULONG   StartSecs, StartMicros;
  UWORD   selectmode;

  Object  *slider;
  Object  *KeyUpFocus;
  Object  *PointerObj;

  LONG    Rows;     // The value of the rows tag
  LONG    Columns;  // The value of the columns tag

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
  BOOL    use_fixedfont;

  struct  TextFont  *normalfont;
  struct  TextFont  *fixedfont;

  UWORD           BlinkSpeed;
  LONG            CursorWidth;
  struct  Hook    *DoubleClickHook;
  struct  Hook    *ExportHook;
  struct  Hook    *ImportHook;
  LONG            ExportWrap;
  LONG            ImportWrap;
  BOOL            HasChanged;
  LONG            TabSize;        // number of spaces to use when Tab2Spaces is active
  LONG            GlobalTabSize;  // number of spaces as configured in MUI prefs
  LONG            TabSizePixels;  // number of pixels a Tab2Spaces conversion will consume
  BOOL            ConvertTabs;    // convert to spaces when TAB key is used. Otherwise insert \t
  LONG            WrapBorder;
  ULONG           WrapMode;
  BOOL            WrapWords;      // wrap at words boundaries rather than hard wrapping at each char
  struct UserAction *undoSteps;   // pointer to memory for the undo actions
  ULONG           maxUndoSteps;   // how many steps can be put into the undoBuffer
  ULONG           usedUndoSteps;  // how many steps in the undoBuffer have been used so far
  ULONG           nextUndoStep;   // index of the next undo/redo step
  BOOL            userUndoBufferSize;
  BOOL            TypeAndSpell;
  BOOL            inactiveCursor;
  BOOL            selectPointer;
  BOOL            activeSelectPointer;
  Object *        SuggestWindow;
  Object *        SuggestListview;
  BOOL            SuggestSpawn;
  BOOL            LookupSpawn;
  char            SuggestCmd[256];
  char            LookupCmd[256];
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

  char **Keywords;
};

// AllocBitMap.c
struct BitMap * SAVEDS ASM MUIG_AllocBitMap(REG(d0, LONG), REG(d1, LONG), REG(d2, LONG), REG(d3, LONG flags), REG(a0, struct BitMap *));
void SAVEDS ASM MUIG_FreeBitMap(REG(a0, struct BitMap *));

// BlockOperators.c
void MarkAllBlock(struct InstData *, struct marking *);
STRPTR GetBlock(struct InstData *, struct marking *);
void RedrawArea(struct InstData *, LONG, struct line_node *, LONG, struct line_node *);
void NiceBlock(struct marking *, struct marking *);
LONG CutBlock(struct InstData *, ULONG);
LONG CutBlock2(struct InstData *, ULONG, struct marking *);
void CheckBlock(struct InstData *, struct line_node *);

// flags for CutBlock() and CutBlock2()
#define CUTF_CLIPBOARD (1<<0)
#define CUTF_CUT       (1<<1)
#define CUTF_UPDATE    (1<<2)

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
UWORD GetColor(LONG, struct line_node *);
void AddColor(struct InstData *, struct marking *, UWORD);

// Dispatcher.c
void ResetDisplay(struct InstData *);
void RequestInput(struct InstData *);
void RejectInput(struct InstData *);

// EditorStuff.c
BOOL PasteClip(struct InstData *, LONG , struct line_node *);
BOOL SplitLine(struct InstData *, LONG , struct line_node *, BOOL, struct UserAction *);
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

// Grow.c
void InitGrow(struct Grow *grow, APTR pool, int itemSize);
void FreeGrow(struct Grow *grow);
void AddToGrow(struct Grow *grow, void *newItem);
void RemoveFromGrow(struct Grow *grow);

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
BOOL ImportText(struct InstData *, const char *, struct Hook *, LONG, struct MinList *);
BOOL ReimportText(struct IClass *, Object *);

// InitConfig.c
void InitConfig(struct IClass *, Object *);
void FreeConfig(struct IClass *, Object *);

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
void FreeTextMem(struct InstData *, struct MinList *);
BOOL Init_LineNode(struct InstData *, struct line_node *, CONST_STRPTR);
BOOL ExpandLine(struct InstData *, struct line_node *, LONG);
BOOL CompressLine(struct InstData *, struct line_node *);
void InsertLines(struct MinList *lines, struct line_node *after);
LONG LineCharsWidth(struct InstData *, CONST_STRPTR);
ULONG VisualHeight(struct InstData *, struct line_node *);
void OffsetToLines(struct InstData *, LONG, struct line_node *, struct pos_info *);
LONG LineNr(struct InstData *, struct line_node *);
struct line_node *LineNode(struct InstData *, LONG);
void ScrollUpDown(struct InstData *);
void SetCursor(struct InstData *, LONG, struct line_node *, BOOL);
void DumpText(struct InstData *, LONG, LONG, LONG, BOOL);
void GetLine(struct InstData *, LONG, struct pos_info *);
LONG LineToVisual(struct InstData *, struct line_node *);
LONG CountLines(struct InstData *, struct MinList *);

// Navigation.c
void SetBookmark(struct InstData *, ULONG);
void GotoBookmark(struct InstData *, ULONG);
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
LONG FlowSpace(struct InstData *, UWORD, STRPTR);
void PosFromCursor(struct InstData *, LONG, LONG);

// Pointer.c
void SetupSelectPointer(struct InstData *data);
void CleanupSelectPointer(struct InstData *data);
void ShowSelectPointer(struct InstData *data, Object *obj);
void HideSelectPointer(struct InstData *data, Object *obj);

// PrintLineWithStyles.c
ULONG ConvertStyle(UWORD);
LONG PrintLine(struct InstData *, LONG, struct line_node *, LONG, BOOL);
ULONG ConvertPen(struct InstData *, UWORD, BOOL);
void DrawSeparator(struct InstData *, struct RastPort *, LONG, LONG, LONG, LONG);

// Search.c
IPTR mSearch(struct IClass *, Object *, struct MUIP_TextEditor_Search *);
IPTR mReplace(struct IClass *, Object *, struct MUIP_TextEditor_Replace *);

// SetBlock.c
IPTR mSetBlock(struct InstData *, struct MUIP_TextEditor_SetBlock *msg);

// SpellChecker.c
void SpellCheckWord(struct InstData *);
void SuggestWord(struct InstData *);
Object *SuggestWindow(struct InstData *);
void ParseKeywords(struct InstData *data, const char *keywords);
void FreeKeywords(struct InstData *data);
void CheckSingleWordAgainstKeywords(struct InstData *data, const char *word);
void KeywordCheck(struct InstData *);

// StyleOperators.c
void UpdateStyles(struct InstData *);
UWORD GetStyle(LONG, struct line_node *);
void AddStyle(struct InstData *, struct marking *, UWORD, BOOL);
void AddStyleToLine(struct InstData *, LONG, struct line_node *, LONG, UWORD);

// UndoFunctions.c
BOOL AddToUndoBuffer(struct InstData *, enum EventType, void *);
void ResetUndoBuffer(struct InstData *);
void ResizeUndoBuffer(struct InstData *, ULONG);
void FreeUndoBuffer(struct InstData *);
BOOL Undo(struct InstData *);
BOOL Redo(struct InstData *);

// NewGfx.c
LONG TextLengthNew(struct RastPort *rp, const char *string, ULONG count, LONG tabSizePixels);
ULONG TextFitNew(struct RastPort *rp, const char *string, ULONG strLen, struct TextExtent *textExtent, const struct TextExtent *constrainingExtent, LONG strDirection, LONG constrainingBitWidth, LONG constrainingBitHeight, LONG tabSizePixels);
void TextNew(struct RastPort *rp, const char *string, ULONG count, LONG tabSizePixels);

#if !defined(__amigaos4__) && !defined(__MORPHOS__) && !defined(__AROS__)
// AllocVecPooled.c
APTR AllocVecPooled(APTR, ULONG);
// FreeVecPooled.c
void FreeVecPooled(APTR, APTR);
#endif

#if !defined(GetHead)
// GetHead.c
struct Node *GetHead(struct List *);
#endif
#if !defined(GetPred)
// GetPred.c
struct Node *GetPred(struct Node *);
#endif
#if !defined(GetSucc)
// GetSucc.c
struct Node *GetSucc(struct Node *);
#endif
#if !defined(GetTail)
// GetTail.c
struct Node *GetTail(struct List *);
#endif
#if !defined(MoveList)
// MoveList.c
void MoveList(struct List *to, struct List *from);
#endif

// define some convenience functions for accessing the list of lines
// to avoid constant type casting within the normal source
#define GetFirstLine(lines)     (struct line_node *)(GetHead((struct List *)(lines)))
#define GetLastLine(lines)      (struct line_node *)(GetTail((struct List *)(lines)))
#define GetNextLine(line)       (struct line_node *)(GetSucc((struct Node *)(line)))
#define GetPrevLine(line)       (struct line_node *)(GetPred((struct Node *)(line)))
#define HasNextLine(line)       (GetSucc((struct Node *)(line)) != NULL)
#define HasPrevLine(line)       (GetPred((struct Node *)(line)) != NULL)
#define AddLine(lines, line)    AddTail((struct List *)(lines), (struct Node *)(line))
#define InsertLine(line, after) Insert(NULL, (struct Node *)(line), (struct Node *)(after))
#define RemLine(line)           Remove((struct Node *)(line))
#define RemFirstLine(lines)     (struct line_node *)RemHead((struct List *)(lines))
#define RemLastLine(lines)      (struct line_node *)RemTail((struct List *)(lines))
#define MoveLines(to, from)     MoveList((struct List *)(to), (struct List *)(from))
#define InitLines(lines)        NewList((struct List *)(lines))
#define ContainsLines(lines)    (IsListEmpty((struct List *)(lines)) == FALSE)

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
  FLG_PasteStyles    = 1L << 21, // respect styles when pasting text
  FLG_PasteColors    = 1L << 22, // respect colors when pasting text
  FLG_ForcedTabSize  = 1L << 23, // override the user defined TAB size
  FLG_MUI4           = 1L << 31, // running under MUI4

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

#define ARRAY_SIZE(x)         (sizeof(x[0]) ? sizeof(x)/sizeof(x[0]) : 0)

#ifndef MUIKEY_CUT
#define MUIKEY_CUT 22
#endif

#ifndef MUIKEY_COPY
#define MUIKEY_COPY 23
#endif

#ifndef MUIKEY_PASTE
#define MUIKEY_PASTE 24
#endif

#ifndef MUIKEY_UNDO
#define MUIKEY_UNDO 25
#endif

#ifndef MUIKEY_REDO
#define MUIKEY_REDO 26
#endif

#endif /* TEXTEDITOR_MCC_PRIV_H */
