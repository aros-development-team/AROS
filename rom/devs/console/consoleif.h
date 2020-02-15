#ifndef CONSOLEIF_H
#define CONSOLEIF_H
/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the console class
    Lang: english
*/

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif
#ifndef PROTO_INTUITION_H
#   include <proto/intuition.h>
#endif

enum
{
    A_Console_Window = TAG_USER + 1
};

enum
{
    M_Console_Write = TAG_USER + 1,
    M_Console_Scroll,
    M_Console_DoCommand,
    M_Console_Left,
    M_Console_Right,
    M_Console_Up,
    M_Console_Down,
    M_Console_GetDefaultParams,
    M_Console_RenderCursor,
    M_Console_UnRenderCursor,
    M_Console_ClearCell,
    M_Console_NewWindowSize,
    M_Console_HandleGadgets,
    M_Console_Copy,
    M_Console_Paste,
    M_Console_GetColorPen = M_Console_Paste + 5
};

struct P_Console_ScrollDown
{
    ULONG MethodID;
    WORD LinePos;               /* Lines including this one will be scrolled */

};

struct P_Console_ScrollUp
{
    ULONG MethodID;
    WORD LinePos;               /* Lines including this one will be scrolled */

};

struct P_Console_DoCommand
{
    ULONG MethodID;
    BYTE Command;               /* Erase in display, scroll, next line etc.. */
    UBYTE NumParams;
    IPTR *Params;               /* The command's parameters */
};

struct P_Console_Left
{
    ULONG MethodID;
    UWORD Num;
};
struct P_Console_Right
{
    ULONG MethodID;
    UWORD Num;
};
struct P_Console_Up
{
    ULONG MethodID;
    UWORD Num;
};

struct P_Console_Down
{
    ULONG MethodID;
    UWORD Num;
};

struct P_Console_RenderCursor
{
    ULONG MethodID;
};

struct P_Console_UnRenderCursor
{
    ULONG MethodID;
};

struct P_Console_ClearCell
{
    ULONG MethodID;
    WORD X;
    WORD Y;
};

struct P_Console_NewWindowSize
{
    ULONG MethodID;
};

struct P_Console_Copy
{
    ULONG MethodID;
};

struct P_Console_Paste
{
    ULONG MethodID;
};

struct P_Console_HandleGadgets
{
    ULONG MethodID;
    struct InputEvent *Event;
};

struct P_Console_GetDefaultParams
{
    ULONG MethodID;
    BYTE Command;
    IPTR *Params;
};

struct P_Console_GetColorPen
{
    ULONG MethodID;
    UWORD ColorIdx;
    UBYTE *PenPtr;
};

#define Console_DoCommand(o, cmd, numparams, params)	\
({							\
	struct P_Console_DoCommand p;			\
	p.MethodID = M_Console_DoCommand;		\
	p.Command = cmd;				\
	p.NumParams = numparams;			\
	p.Params = (IPTR *)params;			\
	DoMethodA((o), (Msg)&p);			\
})

#define Console_Left(o, num)		\
({					\
    struct P_Console_Left p;		\
    p.MethodID	= M_Console_Left;	\
    p.Num	= num;			\
    DoMethodA((o), (Msg)&p);		\
})


#define Console_Right(o, num)		\
({					\
    struct P_Console_Right p;		\
    p.MethodID	= M_Console_Right;	\
    p.Num	= num;			\
    DoMethodA((o), (Msg)&p);		\
})

#define Console_Up(o, num)		\
({					\
    struct P_Console_Up p;		\
    p.MethodID	= M_Console_Up;		\
    p.Num	= num;			\
    DoMethodA((o), (Msg)&p);		\
})

#define Console_Down(o, num)		\
({					\
    struct P_Console_Down p;		\
    p.MethodID	= M_Console_Down;	\
    p.Num	= num;			\
    DoMethodA((o), (Msg)&p);		\
})

#define Console_GetDefaultParams(o, cmd, params)	\
({							\
    struct P_Console_GetDefaultParams p;		\
    p.MethodID	= M_Console_GetDefaultParams;		\
    p.Command	= cmd;					\
    p.Params	= params;				\
    DoMethodA((o), (Msg)&p);				\
})


#define Console_GetColorPen(o, coloridx, penptr)	\
({							\
    struct P_Console_GetColorPen p;		        \
    p.MethodID	= M_Console_GetColorPen;		\
    p.ColorIdx = coloridx;		                \
    p.PenPtr = penptr;		                        \
    DoMethodA((o), (Msg)&p);				\
})


#define Console_RenderCursor(o)			\
({						\
    struct P_Console_RenderCursor p;		\
    p.MethodID	= M_Console_RenderCursor;	\
    DoMethodA((o), (Msg)&p);			\
})

#define Console_UnRenderCursor(o)		\
({						\
    struct P_Console_UnRenderCursor p;		\
    p.MethodID	= M_Console_UnRenderCursor;	\
    DoMethodA((o), (Msg)&p);			\
})

#define Console_ClearCell(o, x, y)		\
({						\
    struct P_Console_ClearCell p;		\
    p.MethodID	= M_Console_ClearCell;		\
    p.X		= x;				\
    p.Y		= y;				\
    DoMethodA((o), (Msg)&p);			\
})

#define Console_NewWindowSize(o)		\
({						\
    struct P_Console_NewWindowSize p;		\
    p.MethodID	= M_Console_NewWindowSize;	\
    DoMethodA((o), (Msg)&p);			\
})

#define Console_HandleGadgets(o, e)	       	\
({						\
    struct P_Console_HandleGadgets p;		\
    p.MethodID	= M_Console_HandleGadgets;	\
    p.Event = e;	      			\
    DoMethodA((o), (Msg)&p);			\
})

#define Console_Copy(o)	       	\
({						\
  struct P_Console_Copy p;			\
  p.MethodID	= M_Console_Copy;		\
  DoMethodA((o), (Msg)&p);			\
 })

#define Console_Paste(o)	       	\
({						\
  struct P_Console_Paste p;			\
  p.MethodID	= M_Console_Paste;		\
  DoMethodA((o), (Msg)&p);			\
})


#endif /* CONSOLEIF_H */
