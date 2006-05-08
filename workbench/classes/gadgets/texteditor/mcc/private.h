/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by TextEditor.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc

 $Id: private.h,v 1.16 2005/08/16 21:21:01 damato Exp $

***************************************************************************/

#ifndef TEXTEDITOR_MCC_PRIV_H
#define TEXTEDITOR_MCC_PRIV_H

#include <graphics/rastport.h>

#ifndef ClassAct
#include <libraries/mui.h>
#ifdef __AROS__
#define MUIA_Prop_Release             0x80429839
#else
#include "muiextra.h"
#endif
#else
#include <exec/semaphores.h>
#define MUIM_DrawBackground 0x804238ca
#define MUIM_GetConfigItem  0x80423edb
#define set(obj,attr,value) SetAttrs(obj,attr,value,TAG_DONE)
#endif

#include <mcc_common.h>

#include "TextEditor_mcc.h"
#include "Debug.h"

#define EOS        (unsigned short)-1

#define UNDERLINE 0x01
#define BOLD      0x02
#define ITALIC    0x04
#define COLOURED  0x08

// some own usefull MUI-style macros to check mouse positions in objects
#define _between(a,x,b) 					((x)>=(a) && (x)<=(b))
#define _isinobject(o,x,y) 				(_between(_mleft(o),(x),_mright (o)) && _between(_mtop(o) ,(y),_mbottom(o)))
#define _isinwholeobject(o,x,y) 	(_between(_left(o),(x),_right (o)) && _between(_top(o) ,(y),_bottom(o)))

// own common macros
#define Enabled(data)   ((data)->blockinfo.enabled && \
                         ((data)->blockinfo.startx != (data)->blockinfo.stopx || \
                          (data)->blockinfo.startline != (data)->blockinfo.stopline || \
                           data->selectmode == 3))

struct bookmark
{
  struct  line_node *line;
  UWORD   x;
};

struct marking
{
  BOOL    enabled;          /* Boolean that indicates wether block is on/off */
  struct  line_node *startline; /* Line where blockings starts */
  UWORD   startx;           /* X place of start */
  struct  line_node *stopline;  /* Line where marking ends */
  UWORD   stopx;            /* X place of stop */
};

struct pos_info
{
  struct  line_node *line;      // Pointer to actual line
  UWORD   lines;              // Lines down
  UWORD   x;                // Chars in
  UWORD   bytes;              // Lines in bytes
  UWORD   extra;              // Lines+1 in bytes
};

struct UserAction
{
  UWORD type;

  struct
  {
    UBYTE   character;     // deletechar
    UBYTE   style;
    UBYTE   flow;
    UBYTE   separator;
  } del;

  UBYTE   *clip;       // deleteblock

  struct
  {
    UWORD x, y;       // pasteblock
  } blk;

  UWORD x, y;
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

  struct  MsgPort   *clipport;
  struct  IOClipReq *clipboard;

  ULONG   StartSecs, StartMicros;
  UWORD   selectmode;

  Object  *slider;
  Object  *KeyUpFocus;

  ULONG   Rows, Columns;  // The value of the rows/columns tags

  APTR    RawkeyBindings;
  ULONG   blockqual;

  ULONG   textcolor;
  ULONG   backgroundcolor;
  ULONG   highlightcolor;
  ULONG   cursorcolor;
  ULONG   cursortextcolor;
  ULONG   markedcolor;
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
  struct  Hook      *DoubleClickHook;
  struct  Hook      *ExportHook;
  struct  Hook      *ImportHook;
  ULONG           ExportWrap;
  UWORD           ImportWrap;
  BOOL            HasChanged;
  BOOL            Smooth;
  UWORD           TabSize;
  ULONG           WrapBorder;
  APTR            undobuffer;
  APTR            undopointer;
  ULONG           undosize;
  BOOL            TypeAndSpell;
  APTR            SuggestWindow;
  APTR            SuggestListview;
  UWORD           SuggestSpawn;
  UWORD           LookupSpawn;
  STRPTR          SuggestCmd;
  STRPTR          LookupCmd;
  ULONG           clipcount;
  APTR            cliphandle;

  APTR            UpdateInfo;

  ULONG           HStart;

  UWORD           Pen;
  BOOL            NoNotify;
  ULONG           *colormap;

  struct  bookmark    bookmarks[4];

  #ifndef ClassAct
  struct  MUI_EventHandlerNode ehnode;
  struct  MUI_InputHandlerNode ihnode;
  struct  MUI_InputHandlerNode blinkhandler;
  #else
  struct  GadgetInfo      *GInfo;
  struct  SignalSemaphore semaphore;
  struct  Image         *Bevel;
  UWORD               BevelVert;
  UWORD               BevelHoriz;
  struct  TextAttr        *TextAttrPtr;
  #endif

  UBYTE   CtrlChar;
};

struct BitMap * SAVEDS ASM MUIG_AllocBitMap(REG(d0, LONG), REG(d1, LONG), REG(d2, LONG), REG(d3, LONG flags), REG(a0, struct BitMap *));
VOID SAVEDS ASM MUIG_FreeBitMap(REG(a0, struct BitMap *));

void  RequestInput  (struct InstData *);
void  RejectInput   (struct InstData *);

void  ScrollIntoDisplay (struct InstData *);
long  CheckSep    (unsigned char, struct InstData *);
long  CheckSent   (unsigned char, struct InstData *);
void  NextLine    (struct InstData *);

unsigned long convert (unsigned long);
LONG  PrintLine   (LONG, struct line_node *, LONG, BOOL, struct InstData *);
void  ClearLine   (char *, LONG, LONG, struct InstData *);
void  ScrollUp    (LONG, LONG, struct InstData *);
void  ScrollDown    (LONG, LONG, struct InstData *);
void  SetCursor   (LONG, struct line_node *, long, struct InstData *);
LONG  LineCharsWidth (char *, struct InstData *);
ULONG FlowSpace   (UWORD, STRPTR, struct InstData *);
long  ExpandLine    (struct line_node *, LONG, struct InstData *);
long  CompressLine  (struct line_node *, struct InstData *);
void  OffsetToLines (LONG, struct line_node *, struct pos_info *, struct InstData *);
void  DumpText    (LONG, LONG, LONG, BOOL, struct InstData *);
long  Init_LineNode (struct line_node *, struct line_node *, char *, struct InstData *);
short VisualHeight  (struct line_node *, struct InstData *);
void  GetLine     (LONG, struct pos_info *, struct InstData *);
LONG  LineToVisual  (struct line_node *, struct InstData *);

LONG  PasteClip   (LONG x, struct line_node *line, struct InstData *);
long  SplitLine   (LONG x, struct line_node *, BOOL, struct UserAction *, struct InstData *);
long  MergeLines    (struct line_node *, struct InstData *);
long  RemoveChars   (LONG, struct line_node *, LONG, struct InstData *);
long  PasteChars    (LONG, struct line_node *, LONG, char *, struct UserAction *, struct InstData *);

void  SetBookmark       (UWORD, struct InstData *);
void  GotoBookmark      (UWORD, struct InstData *);

void  GoTop           (struct InstData *);
void  GoPreviousPage    (struct InstData *);
void  GoPreviousLine    (struct InstData *);
void  GoUp            (struct InstData *);

void  GoBottom        (struct InstData *);
void  GoNextPage        (struct InstData *);
void  GoNextLine        (struct InstData *);
void  GoDown          (struct InstData *);

void  GoNextWord        (struct InstData *);
void  GoEndOfLine       (struct InstData *);
void  GoNextSentence    (struct InstData *);
void  GoRight         (struct InstData *);

void  GoPreviousWord    (struct InstData *);
void  GoStartOfLine     (struct InstData *);
void  GoPreviousSentence  (struct InstData *);
void  GoLeft          (struct InstData *);

void  PosFromCursor     (short, short, struct InstData *);
void  MarkText        (LONG, struct line_node *, LONG, struct line_node *, struct InstData *);

VOID  RedrawArea        (UWORD, struct line_node *, UWORD, struct line_node *, struct InstData *);
void  NiceBlock       (struct marking *, struct marking *);
LONG  CutBlock        (struct InstData *, long, long, BOOL);

void  UpdateStyles      (struct InstData *);
LONG  GetStyle        (LONG, struct line_node *);
void  AddStyle        (struct marking *, unsigned short, long, struct InstData *);
void  AddStyleToLine      (LONG, struct line_node *, LONG, UWORD, struct InstData *);

void  *MyAllocPooled    (void *, unsigned long);
void  MyFreePooled      (void *, void *);

struct line_node  *AllocLine(struct InstData *data);
void FreeLine(struct line_node *line, struct InstData *data);

/* ------------ */

extern SAVEDS ASM ULONG _Dispatcher(REG(a0, struct IClass * cl), REG(a2, Object * obj), REG(a1, Msg msg));

void  InitConfig(Object *, struct InstData *);
void  FreeConfig(struct InstData *, struct MUI_RenderInfo *);

ULONG HandleARexx (struct InstData *, STRPTR command);

struct line_node *ImportText(char *, struct InstData *, struct Hook *, LONG);
void *ExportText(struct line_node *, struct Hook *, LONG);

struct  line_node *loadtext (void);
unsigned short  *CheckStyles      (char *);

LONG CutBlock2 (struct InstData *, long, long, struct marking *, BOOL);
char *GetBlock (struct marking *, struct InstData *);

long AddToUndoBuffer (long, char *, struct InstData *);
void ResetUndoBuffer (struct InstData *);
long Undo       (struct InstData *);
long Redo       (struct InstData *);

ULONG ClearText   (struct InstData *);
ULONG ToggleCursor  (struct InstData *);
ULONG InputTrigger  (struct IClass *, struct InstData *);
ULONG InsertText    (struct InstData *, STRPTR, BOOL);
void  FreeTextMem   (struct line_node *, struct InstData *);
void  ResetDisplay  (struct InstData *);

void CheckWord    (struct InstData *);
void SuggestWord    (struct InstData *);
void *SuggestWindow (struct InstData *);

void AddClipping    (struct InstData *);
void RemoveClipping (struct InstData *);

ULONG Get(struct IClass *, Object *, struct opGet *);
ULONG Set(struct IClass *, Object *, struct opSet *);
ULONG HandleInput(struct IClass *, Object *, struct MUIP_HandleEvent *);

void Key_Backspace  (struct InstData *);
void Key_Delete   (struct InstData *);
void Key_Return   (struct InstData *);
void Key_Tab      (struct InstData *);
void Key_Clear      (struct InstData *);
void Key_Cut      (struct InstData *);
void Key_Copy     (struct InstData *);
void Key_Paste      (struct InstData *);
void Key_DelLine    (struct InstData *);
void Key_ToUpper    (struct InstData *);
void Key_ToLower    (struct InstData *);

unsigned short LineNr (struct line_node *, struct InstData *);
struct line_node *LineNode (unsigned short, struct InstData *);

UWORD GetColor      (UWORD, struct line_node *);
VOID  AddColor      (struct marking *, UWORD, struct InstData *);

ULONG OM_MarkText   (struct MUIP_TextEditor_MarkText *, struct InstData *);
ULONG OM_BlockInfo  (struct MUIP_TextEditor_BlockInfo *, struct InstData *);
ULONG OM_Search   (struct MUIP_TextEditor_Search *, struct InstData *);
ULONG OM_Replace    (Object *obj, struct MUIP_TextEditor_Replace *msg, struct InstData *data);

extern struct Hook ImPlainHook;
extern struct Hook ImEMailHook;
extern struct Hook ImMIMEHook;
extern struct Hook ImMIMEQuoteHook;

extern struct Hook ExportHookPlain;
extern struct Hook ExportHookEMail;

  enum
  {
    mUp, mDown, mLeft, mRight, mPreviousPage, mNextPage,
    mStartOfLine, mEndOfLine, mTop, mBottom, mPreviousWord,
    mNextWord, mPreviousLine, mNextLine, mPreviousSentence,
    mNextSentence, kSuggestWord, kBackspace, kDelete, kReturn,
    kTab, kCut, kCopy, kPaste, kUndo, kRedo,
    kDelBOL, kDelEOL, kDelBOW, kDelEOW,
    kNextGadget, kGotoBookmark1, kGotoBookmark2, kGotoBookmark3,
    kSetBookmark1, kSetBookmark2, kSetBookmark3, kDelLine,
    mKey_LAST
  };

  enum
  {
    pastechar = 0,
    backspacechar,
    deletechar,
    splitline,
    mergelines,
    backspacemerge,
    pasteblock,
    deleteblock,
    deleteblock_nomove,
    stylebold,
    styleunbold,
    styleitalic,
    styleunitalic,
    styleunderline,
    styleununderline
  };

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

#include "amiga-align.h"

  struct te_key
  {
    UWORD code;
    ULONG qual;
    UWORD act;
  };

  struct keybindings
  {
    struct  te_key  keydata;
  };

#include "default-align.h"

  struct line_node
  {
    struct  line_node *next;        /* Pointer to next line */
    struct  line_node *previous;    /* Pointer to previous line */

		struct LineNode line;

    UWORD   visual;             /* How many lines are this line wrapped over */
//  UWORD   flags;              /* Different flags... */
  };

  enum
  {
    FLG_HScroll     = 1L << 0,
    FLG_NumLock     = 1L << 1,
    FLG_ReadOnly    = 1L << 2,
    FLG_FastCursor  = 1L << 3,
    FLG_CheckWords  = 1L << 4,
    FLG_InsertMode  = 1L << 5,
    FLG_Quiet       = 1L << 6,
    FLG_PopWindow   = 1L << 7,
    FLG_UndoLost    = 1L << 8,
    FLG_Draw        = 1L << 9,
    FLG_InVGrp      = 1L << 10,
    FLG_Ghosted     = 1L << 11,
    FLG_OwnBkgn     = 1L << 12,
    FLG_FreezeCrsr  = 1L << 13,
    FLG_Active      = 1L << 14,
    FLG_OwnFrame    = 1L << 15,
    FLG_ARexxMark   = 1L << 16,
    FLG_FirstInit   = 1L << 17,
    FLG_AutoClip    = 1L << 18,
    FLG_Activated   = 1L << 19, // the gadget was activated by MUIM_GoActive()

    FLG_NumberOf
  };


#define  MUIM_TextEditor_InputTrigger     0xad000101
#define  MUIM_TextEditor_ToggleCursor     0xad000102

#endif /* TEXTEDITOR_MCC_PRIV_H */
