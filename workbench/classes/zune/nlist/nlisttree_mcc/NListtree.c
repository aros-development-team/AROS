/***************************************************************************

 NListtree.mcc - New Listtree MUI Custom Class
 Copyright (C) 1999-2001 by Carsten Scholling
 Copyright (C) 2001-2013 by NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

/****** NListtree.mcc/--background-- *****************************************
*
* There are two possible entry-types in a NListtree class list:
* Leaves and nodes. Leaves are simple entries which have no special
* features except they are holding some data. Nodes are almost
* the same type, holding data too, but having a list attached
* where you can simply add other entries which can be again leaves
* or nodes.
*
* Every node is structured as follows:
*
*   struct MUI_NListtree_TreeNode {
*
*     struct  MinNode tsn_Node;
*     STRPTR  tn_Name;
*     UWORD tn_Flags;
*     APTR  tn_User;
*   };
*
* It contains a name field tn_Name, flags tn_Flags and a pointer to
* user data tn_User.
*
* The tn_Flags field can hold the following flags:
*
*   TNF_LIST    The node contains a list where other nodes
*           can be inserted.
*
*   TNF_OPEN    The list node is open, sub nodes are displayed.
*
*   TNF_FROZEN    The node doesn't react on doubleclick or
*           open/close by the user.
*
*   TNF_NOSIGN    The indicator of list nodes isn't shown.
*
*   TNF_SELECTED  The entry is currently selected.
*
* These flags, except TNF_SELECTED, can be used in
* MUIM_NListtree_Insert at creation time. They will be passed to
* the newly created entry. Also you can do a quick check about the
* state and kind of each entry. But - NEVER EVER - modify any flag
* yourself or NListtree will crash. Be warned!
*
* *********************************************************************
*   THE ABOVE STRUCT IS READ-ONLY!! NEVER CHANGE ANY ENTRY OF THIS
*   STRUCTURE DIRECTLY NOR THINK ABOUT THE CONTENTS OF ANY PRIVATE
*           FIELD OR YOU WILL DIE IN HELL!
* *********************************************************************
*
*
* You can create very complex tree structures. NListtree only uses
* one list which holds all information needed and has no extra
* display list like other list tree classes ;-)
*
* The tree nodes can be inserted and removed, sorted, moved, exchanged,
* renamed  or multi  selected.  To sort you can also  drag&drop  them.
* Modifications can be made in relation to the whole tree, to only one
* level, to a sub-tree or to only one tree node.
*
* The user can control the listtree by the MUI keys, this means a node
* is opened with "Right" and closed with "Left". Check your MUI prefs
* for the specified keys.
*
* You can define which of the columns will react on double-clicking.
* The node toggles its status from open or closed and vice versa.
*
*
* Drag&Drop capabilities:
*
* If you set MUIA_NList_DragSortable to TRUE, the list tree will
* become active for Drag&Drop. This means you can drag and drop
* entries on the same list tree again. While dragging an indicator
* shows where to drop.
*
*   Drag a  Drop on   Result
*
*   leaf  leaf    Exchange leaves.
*   node  leaf    Nothing happens.
*   entry closed node Move entry, the compare hook is used.
*   entry open node Move entry to defined position.
*
* You can not drop an entry on itself, nor can you drop an opened node on
* any of its members.
*
* To exchange data with other objects, you have to create your own
* subclass of NListtree class and react on the drag methods.
*
*
* Author: Carsten Scholling (c)1999-2000  email: cs@aphaso.de
*
******************************************************************************
*
*/

/*
**  Includes
*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <graphics/gfxmacros.h>
#undef GetOutlinePen

#include <libraries/gadtools.h>
#include <clib/alib_protos.h>
#include <clib/macros.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/exec.h>

#include "private.h"

#include "NListtree.h"
#include "version.h"
#include "muiextra.h"

#define DATA_BUF_SIZE          4096

/*********************************************************************************************/

// stack definition (where is this used!?)
LONG __stack = 16384;

/*
**  Small helpful macros...
*/
#define DIFF(a,b)       (MAX((a),(b))-MIN((a),(b)))
#define CLN(x)          ((struct MUI_NListtree_ListNode *)(x))
#define CTN(x)          ((struct MUI_NListtree_TreeNode *)(x))

/*
**  Type definition for compare function.
*/

void qsort2(struct MUI_NListtree_TreeNode **table, ULONG entries, struct NListtree_Data *data);

/*
**  Some prototypes.
*/
ULONG MultiTestFunc( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn, LONG seltype, LONG selflags );

struct TreeImage_Data
{
  Object  *obj;
  LONG  spec;
};

#if !defined(__MORPHOS__)
#ifdef __AROS__
static __attribute__ ((noinline)) Object * VARARGS68K DoSuperNew(struct IClass *cl, Object *obj, Tag tag1, ...)
{
    AROS_SLOWSTACKTAGS_PRE_AS(tag1, Object *)
    retval = (Object *)DoSuperMethod(cl, obj, OM_NEW, AROS_SLOWSTACKTAGS_ARG(tag1), NULL);
    AROS_SLOWSTACKTAGS_POST
}
#else
static Object * VARARGS68K DoSuperNew(struct IClass *cl, Object *obj, ...)
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
#endif // __AROS__
#endif // !__MORPHOS__

/*****************************************************************************\
*******************************************************************************
**
**  Graphic and drawing functions.
**
*******************************************************************************
\*****************************************************************************/

/*
**  Set A/B-Pen and DrawMode depending on OS version.
*/
INLINE VOID MySetABPenDrMd( struct RastPort *rp, LONG apen, LONG bpen, UBYTE drmd )
{
  if ( LIBVER( GfxBase ) >= 39 )
    SetABPenDrMd( rp, apen, bpen, drmd );
  else
  {
    SetAPen( rp, apen );
    SetBPen( rp, bpen );
    SetDrMd( rp, drmd );
  }
}


/*
**  Draw a simple line from l/t to r/b. All lines are pure horiz. or vert.!
*/
INLINE VOID DrawLine( struct RastPort *rp, WORD l, WORD t, WORD r, WORD b )
{
  Move( rp, l, t );
  Draw( rp, r, b );
}


/*
**  Draw windows line l/t to r/b. All
**  lines are pure horiz. or vert.!
*/
INLINE VOID DrawLineDotted( struct RastPort *rp, WORD l, WORD t, WORD r, WORD b )
{
  if ( l == r )
  {
    while( t <= b )
    {
      WritePixel( rp, l, t );
      t += 2;
    }
  }
  else
  {
    while( l <= r )
    {
      WritePixel( rp, l, t );
      l += 2;
    }
  }
}



/*
**  Draw a vertical bar.
*/
INLINE VOID DrawTreeVertBar( struct TreeImage_Data *data, struct MyImage *im, WORD l, WORD t, UNUSED WORD r, WORD b )
{
  struct RastPort *rp = (struct RastPort *)_rp( data->obj );
  UWORD m = l + ( im->nltdata->MaxImageWidth - 1 ) / 2;

  ENTER();

  switch(im->nltdata->LineType)
  {
    case MUICFGV_NListtree_LineType_Disabled:
      // nothing
    break;

    case MUICFGV_NListtree_LineType_Normal:
    {
      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Line] ) );
      DrawLine( rp, m, t, m, b );
    }
    break;

    case MUICFGV_NListtree_LineType_Dotted:
    {
      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Line] ) );
      DrawLineDotted( rp, m, t, m, b );
    }
    break;

    case MUICFGV_NListtree_LineType_Shadow:
    {
      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Shadow] ) );
      DrawLine( rp, m+1, t, m+1, b );
      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Line] ) );
      DrawLine( rp, m, t, m, b );
    }
    break;

    case MUICFGV_NListtree_LineType_Glow:
    {
      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Shadow] ) );
      DrawLine( rp, m + 2, t, m + 2, b );

      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Line] ) );
      DrawLine( rp, m, t, m, b );

      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Glow] ) );
      DrawLine( rp, m - 1, t, m - 1, b );
      DrawLine( rp, m+1, t, m+1, b );
    }
    break;
  }

  LEAVE();
}


/*
**  Draw a vertical T bar.
*/
INLINE VOID DrawTreeVertBarT( struct TreeImage_Data *data, struct MyImage *im, WORD l, WORD t, WORD r, WORD b )
{
  struct RastPort *rp = (struct RastPort *)_rp( data->obj );
  UWORD m = l + ( im->nltdata->MaxImageWidth - 1 ) / 2;
  UWORD h = t + ( b - t ) / 2;

  ENTER();

  switch(im->nltdata->LineType)
  {
    case MUICFGV_NListtree_LineType_Disabled:
      // nothing
    break;

    case MUICFGV_NListtree_LineType_Normal:
    {
      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Line] ) );
      DrawLine( rp, m, t, m, b );
      DrawLine( rp, m, h, r, h );
    }
    break;

    case MUICFGV_NListtree_LineType_Dotted:
    {
      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Line] ) );
      DrawLineDotted( rp, m, t, m, b );
      DrawLineDotted( rp, m, h, m + (im->nltdata->MaxImageWidth-1)/2 + im->nltdata->IndentWidth, h );
    }
    break;

    case MUICFGV_NListtree_LineType_Shadow:
    {
      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Shadow] ) );
      DrawLine( rp, m+1, t, m+1, b );
      DrawLine( rp, m+1, h + 1, r, h + 1 );

      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Line] ) );
      DrawLine( rp, m, t, m, b );
      DrawLine( rp, m, h, r, h );
    }
    break;

    case MUICFGV_NListtree_LineType_Glow:
    {
      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Shadow] ) );
      DrawLine( rp, m + 2, t,   m + 2,  b );
      DrawLine( rp, m + 3, h + 2, r,  h + 2 );

      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Glow] ) );
      DrawLine( rp, m - 1, t, m - 1, b );
      DrawLine( rp, m+1, t, m+1, b );

      DrawLine( rp, m+1, h - 1, r, h - 1 );
      DrawLine( rp, m+1, h + 1, r, h + 1 );

      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Line] ) );
      DrawLine( rp, m, t, m, b );
      DrawLine( rp, m, h, r, h );
    }
    break;
  }

  LEAVE();
}



/*
**  Draw a vertical end bar.
*/
INLINE VOID DrawTreeVertBarEnd( struct TreeImage_Data *data, struct MyImage *im, WORD l, WORD t, WORD r, WORD b )
{
  struct RastPort *rp = (struct RastPort *)_rp( data->obj );
  UWORD m = l + ( im->nltdata->MaxImageWidth - 1 ) / 2;
  UWORD h = t + ( b - t ) / 2;

  ENTER();

  switch(im->nltdata->LineType)
  {
    case MUICFGV_NListtree_LineType_Disabled:
      // nothing
    break;

    case MUICFGV_NListtree_LineType_Normal:
    {
      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Line] ) );
      DrawLine( rp, m, t, m, h );
      DrawLine( rp, m, h, r, h );
    }
    break;

    case MUICFGV_NListtree_LineType_Dotted:
    {
      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Line] ) );
      DrawLineDotted( rp, m, t, m, h );
      DrawLineDotted( rp, m, h, m + (im->nltdata->MaxImageWidth-1)/2 + im->nltdata->IndentWidth, h );
    }
    break;

    case MUICFGV_NListtree_LineType_Shadow:
    {
      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Shadow] ) );
      DrawLine( rp, m+1, t,   m+1,  h );
      DrawLine( rp, m+1, h + 1, r,  h + 1 );

      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Line] ) );
      DrawLine( rp, m, t, m,  h );
      DrawLine( rp, m, h, r,    h );
    }
    break;

    case MUICFGV_NListtree_LineType_Glow:
    {
      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Shadow] ) );
      DrawLine( rp, m + 2, t, m + 2,  h - 2 );
      DrawLine( rp, m, h + 2, r,  h + 2 );

      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Glow] ) );
      DrawLine( rp, m - 1, t, m - 1,  h );
      DrawLine( rp, m+1, h - 1, r, h - 1 );

      DrawLine( rp, m+1, t, m+1,  h );
      DrawLine( rp, m - 1, h + 1, r, h + 1 );

      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Line] ) );
      DrawLine( rp, m , t, m, h );
      DrawLine( rp, m, h, r, h );
    }
    break;
  }

  LEAVE();
}


/*
**  Draw a horizontal bar.
*/
INLINE VOID DrawTreeHorBar( struct TreeImage_Data *data, struct MyImage *im, WORD l, WORD t, WORD r, WORD b )
{
  struct RastPort *rp = (struct RastPort *)_rp( data->obj );
  UWORD h = t + ( b - t ) / 2;

  ENTER();

  switch(im->nltdata->LineType)
  {
    case MUICFGV_NListtree_LineType_Disabled:
      // nothing
    break;

    case MUICFGV_NListtree_LineType_Normal:
    {
      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Line] ) );
      DrawLine( rp, l-1, h, r+1, h );
    }
    break;

    case MUICFGV_NListtree_LineType_Dotted:
    {
      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Line] ) );
      DrawLineDotted( rp, l-1, h, r+1, h );
    }
    break;

    case MUICFGV_NListtree_LineType_Shadow:
    {
      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Shadow] ) );
      DrawLine( rp, l-1, h + 1, r+1, h + 1 );

      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Line] ) );
      DrawLine( rp, l-1, h, r+1, h );
    }
    break;

    case MUICFGV_NListtree_LineType_Glow:
    {
      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Shadow] ) );
      DrawLine( rp, l-1, h + 2, r, h + 2 );

      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Glow] ) );
      DrawLine( rp, l-1, h - 1, r, h - 1 );
      DrawLine( rp, l-1, h + 1, r, h + 1 );

      SetAPen( rp, MUIPEN( im->nltdata->Pen[PEN_Line] ) );
      DrawLine( rp, l-1, h, r, h );
    }
    break;
  }

  LEAVE();
}



/*****************************************************************************\
*******************************************************************************
**
**  Tree image class functions.
**
*******************************************************************************
\*****************************************************************************/

/*
**  Draw function for special tree image class.
*/
IPTR TreeImage_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
  struct TreeImage_Data *data = INST_DATA( cl, obj );

  DoSuperMethodA( cl, obj, (Msg)msg);

  if(isFlagSet(msg->flags, MADF_DRAWOBJECT) || isFlagSet(msg->flags, MADF_DRAWUPDATE))
  {
    struct MyImage *im;
    WORD l, t, r, b;

    D(DBF_DRAW, "DRAW SPEC: %ld - %ld/%ld/%ld/%ld", data->spec, _defwidth(obj), _defheight(obj), _minwidth(obj), _minheight(obj));
    D(DBF_DRAW, "%ld/%ld/%ld/%ld", _left(obj), _top(obj), _right(obj), _bottom(obj));

    im = (struct MyImage *)xget( obj, MUIA_UserData );

    l = _left( obj );
    t = _top( obj );
    r = _right( obj );
    b = _bottom( obj );

    SetDrMd( _rp( obj ), JAM1 );

    switch( data->spec )
    {
      case SPEC_Vert:
        DrawTreeVertBar( data, im, l, t, r, b );
        break;

      case SPEC_VertT:
        DrawTreeVertBarT( data, im, l, t, r, b );
        break;

      case SPEC_VertEnd:
        DrawTreeVertBarEnd( data, im, l, t, r, b );
        break;

      case SPEC_Hor:
        DrawTreeHorBar( data, im, l, t, r, b );
        break;

      default:
        break;
    }
  }

  msg->flags = 0;

  return( 0 );
}


/*
**  Special tree image class creation.
*/
IPTR TreeImage_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
  struct TreeImage_Data *data;

  if ( !( obj = (Object *)DoSuperMethodA( cl, obj, (Msg)msg) ) )
    return( 0 );

  ENTER();

  data = INST_DATA( cl, obj );

  data->obj = obj;
  data->spec = 0;

  RETURN(obj);
  return (IPTR)obj;
}


/*
**  Attribute setting function for special tree image class.
*/
ULONG TreeImage_Set(struct IClass *cl, Object *obj, Msg msg)
{
  struct TreeImage_Data *data = INST_DATA( cl, obj );
  struct TagItem *tags, *tag;

  for(tags = ((struct opSet *)msg)->ops_AttrList; (tag = (struct TagItem *)NextTagItem((APTR)&tags));)
  {
    switch( tag->ti_Tag )
    {
      case MUIA_TI_Spec:
        data->spec = tag->ti_Data;
        break;
    }
  }

  D(DBF_IMAGES, "SET SPEC: %ld", data->spec);

  return( 0 );
}


/*
**  Dispatcher for the special tree image class.
*/

DISPATCHER(TreeImage_Dispatcher)
{
  switch( msg->MethodID )
  {
    case OM_NEW:      return( TreeImage_New(      cl, obj, (APTR)msg) );
    case OM_SET:      return( TreeImage_Set(      cl, obj, (APTR)msg) );
    case MUIM_Draw:   return( TreeImage_Draw(     cl, obj, (APTR)msg) );
  }

  return( DoSuperMethodA( cl, obj, msg) );
}

/*
**  Dispatcher for the special tree image class no. 2.
**  Get the maximum width of images.
*/
DISPATCHER(NodeImage_Dispatcher)
{
  if ( msg->MethodID == MUIM_Show )
  {
    struct  MyImage *im;
    UWORD w, h;

    im = (struct MyImage *)xget( obj, MUIA_UserData );

    w = _defwidth( obj );
    h = _defheight( obj );

    if ( im->nltdata->MaxImageWidth < w )
      im->nltdata->MaxImageWidth = w;

    if ( im->nltdata->MaxImageHeight < h )
    {
      im->nltdata->MaxImageHeight = h;

      if ( xget( obj, MUIA_NList_LineHeight ) < h )
        nnset( im->nltdata->Obj, MUIA_NList_MinLineHeight, h );
    }

    D(DBF_IMAGES, "=====> DefWidth: %ld, DefHeight: %ld, MaxWidth: %ld", w, h, im->nltdata->MaxImageWidth);
  }

  return( DoSuperMethodA( cl, obj, (Msg)msg) );
}

/*****************************************************************************\
*******************************************************************************
**
**  Helpful handling functions.
**
*******************************************************************************
\*****************************************************************************/

/* MorphOS, OS4 and AROS support Alloc/FreeVecPooled internally */
#if defined(__amigaos3__)

#ifdef MYDEBUG
ULONG totalmem = 0;
#endif

/*
**  Allocate pooled memory.
*/
INLINE APTR AllocVecPooled( APTR mempool, ULONG size )
{
  ULONG *mem;

  if((mem = (ULONG *)AllocPooled(mempool, size + 4)))
  {
#ifdef MYDEBUG
    totalmem += size;
    D(DBF_MEMORY, "0x%08lx = AllocPooled( mempool, %ld ) ==> total: %ld", mem, size + 4, totalmem);
#endif

    *mem++  = size;
  }

  return( (APTR)mem );
}


/*
**  Free previously allocated pool memory.
*/
INLINE VOID FreeVecPooled( APTR mempool, APTR mem )
{
  ULONG *m = (ULONG *)mem;

#ifdef MYDEBUG
  totalmem -= m[-1];
  D(DBF_MEMORY, "           FreePooled( mempool, 0x%08lx, %ld ) ==> total: %ld", &m[-1], m[-1] + 4, totalmem);
#endif

  FreePooled( mempool, &m[-1], m[-1] + 4 );
}

#endif /* !__MORPHOS__ && !__amigaos4__ */

#if !defined(__amigaos4__) && !defined(__AROS__)
struct Node *GetSucc(struct Node *node)
{
  struct Node *result = NULL;

  if(node != NULL && node->ln_Succ != NULL && node->ln_Succ->ln_Succ != NULL)
    result = node->ln_Succ;

  return result;
}

struct Node *GetPred(struct Node *node)
{
  struct Node *result = NULL;

  if(node != NULL && node->ln_Pred != NULL && node->ln_Pred->ln_Pred != NULL)
    result = node->ln_Pred;

  return result;
}

struct Node *GetHead(struct List *list)
{
  struct Node *result = NULL;

  if(list != NULL && IsListEmpty(list) == FALSE)
    result = list->lh_Head;

  return result;
}

struct Node *GetTail(struct List *list)
{
  struct Node *result = NULL;

  if(list != NULL && IsListEmpty(list) == FALSE)
    result = list->lh_TailPred;

  return result;
}
#endif // !__amigaos4__ && !__AROS__

/*****************************************************************************\
*******************************************************************************
**
**  Helpful object related functions.
**
*******************************************************************************
\*****************************************************************************/

#if 0 /* unused */
INLINE ULONG MyCallHookA(struct Hook *hook, struct NListtree_Data *data, struct NListtree_HookMessage *msg)
{
  return(CallHookPkt(hook, data->Obj, msg));
}
#endif

#ifdef __AROS__
#define MyCallHook(hook, data, ...) \
 ({ IPTR __args[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) }; \
    CallHookPkt(hook, data->Obj, __args); })
#else
static IPTR STDARGS VARARGS68K MyCallHook(struct Hook *hook, struct NListtree_Data *data, ...)
{
  IPTR ret;
  VA_LIST va;

  VA_START(va, data);
  ret = CallHookPkt(hook, data->Obj, VA_ARG(va, APTR));
  VA_END(va);

  return ret;
}
#endif

/*
**  Release a pen.
*/
VOID RelPen( struct MUI_RenderInfo *mri, LONG *pen )
{
  if ( *pen != -1 )
  {
    /*
    **  Only valid between Setup/Cleanup!
    */
    if ( mri )
    {
      MUI_ReleasePen( mri, *pen );
    }

    *pen = -1;
  }
}


/*
**  Relase pen if obtained and obtain a new pen.
*/
VOID ObtPen( struct MUI_RenderInfo *mri, LONG *pen, struct MUI_PenSpec *ps )
{
  /*
  **  Only valid between Setup/Cleanup!
  */
  if ( mri )
  {
    RelPen( mri, pen );
    *pen = MUI_ObtainPen( mri, ps, 0 );
  }
}


/*
**  Fully clear and dispose an image for NList use.
*/
VOID DisposeImage( struct NListtree_Data *data, ULONG nr )
{
  if ( data->Image[nr].ListImage )
  {
    DoMethod( data->Obj, MUIM_NList_DeleteImage, data->Image[nr].ListImage );
    data->Image[nr].ListImage = NULL;

    if ( data->Image[nr].Image )
    {
      MUI_DisposeObject( data->Image[nr].Image );
      data->Image[nr].Image = NULL;
    }
  }
}


/*
**  Fully set up an image for NList use.
*/
VOID SetupImage( struct NListtree_Data *data, struct MUI_ImageSpec *is, ULONG nr )
{
  ENTER();

  if ( is )
  {
    Object *rim;

    data->Image[nr].nltdata = data;
    data->MaxImageWidth = 0;
    data->MaxImageHeight = 0;

    if ( nr == IMAGE_Folder )
    {
      rim = MUI_NewObject( MUIC_Image, MUIA_Image_Spec, is, MUIA_UserData, &data->Image[nr], TAG_DONE );
    }
    else
    {
      rim = NewObject( data->CL_NodeImage->mcc_Class, NULL, MUIA_Image_Spec, is,
                                                            MUIA_Frame,       MUIV_Frame_None,
                                                            MUIA_InnerBottom, 0,
                                                            MUIA_InnerLeft,   0,
                                                            MUIA_InnerRight,  0,
                                                            MUIA_InnerTop,    0,
                                                            MUIA_Weight,      0,
                                                            MUIA_UserData,    &data->Image[nr], TAG_DONE );
    }

    if ( rim )
    {
      data->Image[nr].Image = rim;
      data->Image[nr].ListImage = (Object *)DoMethod( data->Obj, MUIM_NList_CreateImage, rim, 0 );

      LEAVE();
      return;
    }
  }

  LEAVE();
}


/*
**  Activate Active/DoubleClick notification.
*/
INLINE VOID ActivateNotify( struct NListtree_Data *data )
{
  D(DBF_NOTIFY, "Activenotify: %lx",data);
  if(isFlagClear(data->Flags, NLTF_ACTIVENOTIFY))
  {
    DoMethod(data->Obj, MUIM_Notify, MUIA_NList_Active, MUIV_EveryTime,
      data->Obj, 2, MUIM_NListtree_GetListActive, 0);

    DoMethod(data->Obj, MUIM_Notify, MUIA_NList_DoubleClick, MUIV_EveryTime,
      data->Obj, 2, MUIM_NListtree_GetDoubleClick, MUIV_TriggerValue);

    SET_FLAG(data->Flags, NLTF_ACTIVENOTIFY);
  }
}


/*
**  Deactivate Active/DoubleClick notification.
*/
INLINE VOID DeactivateNotify( struct NListtree_Data *data )
{
  D(DBF_NOTIFY, "Deactivenotify: %lx",data);
  if(isFlagSet(data->Flags, NLTF_ACTIVENOTIFY))
  {
    DoMethod(data->Obj, MUIM_KillNotifyObj, MUIA_NList_DoubleClick, data->Obj);
    DoMethod(data->Obj, MUIM_KillNotifyObj, MUIA_NList_Active, data->Obj);

    if(isFlagClear(data->Flags, NLTF_SAFE_NOTIFIES))
    {
	  // removing MUI notifies must be considered unsafe
	  // The solution is to trigger an artifical notification to let
	  // the object's superclass Notify.mui finally remove the killed
	  // notifies from above before
	  // NOTE: MUIA_NList_ListCompatibility is considered obsolete and
	  // currently obeyed during OM_NEW only. Hence it is safe to use
	  // this attribute for the trigger.
	  set(data->Obj, MUIA_NList_ListCompatibility, 0);
	}

    CLEAR_FLAG(data->Flags, NLTF_ACTIVENOTIFY);
  }
}



/*
#define MakeNotify( data, attr, val ) if ( !( data->Flags & NLTF_QUIET ) ) SetAttrs( data->Obj, MUIA_Group_Forward, FALSE, MUIA_NListtree_OnlyTrigger, TRUE, attr, val, TAG_DONE )
#define MakeSet( data, attr, val )    if ( !( data->Flags & NLTF_QUIET ) ) set( data->Obj, attr, val )
*/


INLINE VOID MakeNotify( struct NListtree_Data *data, ULONG tag, APTR val )
{
  if(isFlagClear(data->Flags, NLTF_QUIET))
  {
    SetAttrs( data->Obj, MUIA_Group_Forward, FALSE, MUIA_NListtree_OnlyTrigger, TRUE, tag, val, TAG_DONE );
  }
}

INLINE VOID MakeSet( struct NListtree_Data *data, ULONG tag, APTR val )
{
  if(isFlagClear(data->Flags, NLTF_QUIET))
  {
    set( data->Obj, tag, val );
  }
  else
  {
    SET_FLAG(data->Flags, NLTF_SETACTIVE);
  }
}


/*****************************************************************************\
*******************************************************************************
**
**  List and table handling functions.
**
*******************************************************************************
\*****************************************************************************/

/*
**  Create a table.
*/
struct MUI_NListtree_TreeNode **NLCreateTable( struct NListtree_Data *data, struct Table *table, LONG numentrieswanted )
{
  struct MUI_NListtree_TreeNode **newtable;
  LONG oldtablesize;

  /*
  **  Don't allocate less than one entry.
  */
  if ( numentrieswanted < 1 )
    numentrieswanted = 1;

  /*
  **  Round to next multiple of 64.
  */
  oldtablesize  = table->tb_Size;
  table->tb_Size  = ( numentrieswanted + 63 ) & ~63;

  /*
  **  Allocate the table.
  */
  if((newtable = (struct MUI_NListtree_TreeNode **)AllocVecPooled(data->TreePool, sizeof(struct MUI_NListtree_TreeNode *) * table->tb_Size)))
  {
    /*
    **  If there is an old table given, copy it to the new table.
    */
    if ( table->tb_Table )
    {
      CopyMem( table->tb_Table, newtable, sizeof( struct MUI_NListtree_TreeNode * ) * oldtablesize );
      FreeVecPooled( data->TreePool, table->tb_Table );
    }
    else
      oldtablesize = 0;

    /*
    **  Clear all the unused entries.
    */
    memset( &newtable[oldtablesize], 0, ( table->tb_Size - oldtablesize ) * sizeof( struct MUI_NListtree_TreeNode * ) );
    table->tb_Table = newtable;
  }

  return( newtable );
}


/*
**  Add an entry to a table.
*/
BOOL NLAddToTable( struct NListtree_Data *data, struct Table *table, struct MUI_NListtree_TreeNode *newentry )
{
  /*
  **  If the table is filled `to the brim', expand it.
  */
  if ( ( table->tb_Entries + 1 ) > table->tb_Size )
  {
    /*
    **  Allocate another list table.
    */
    if ( !NLCreateTable( data, table, table->tb_Entries + 1 ) )
      return( FALSE );
  }

  /*
  **  Add it to the end of the table.
  */
  table->tb_Table[table->tb_Entries++] = newentry;

  return( TRUE );
}


/*
**  Find an entry in a table.
*/
LONG NLFindInTable( struct Table *table, struct MUI_NListtree_TreeNode *entry )
{
  LONG i;

  for( i = 0; i < table->tb_Entries; i++ )
  {
    if ( table->tb_Table[i] == entry )
    {
      return( i );
    }
  }

  return( -1 );
}



/*
**  Remove an entry from a table.
*/
VOID NLRemoveFromTable( struct NListtree_Data *data, struct Table *table, struct MUI_NListtree_TreeNode *entry )
{
  LONG i;

  if ( ( i = NLFindInTable( table, entry ) ) != -1 )
  {
    if ( --table->tb_Entries == 0 )
    {
      FreeVecPooled( data->TreePool, table->tb_Table );
      table->tb_Table   = NULL;
      table->tb_Size    = 0;
      table->tb_Current = -2;
    }
    else
    {
      LONG j;

      /*
      **  Remove the entry from the table.
      */
      for( j = i; j <= table->tb_Entries; j++ )
        table->tb_Table[j] = table->tb_Table[j + 1];

      /*
      **  Clear the last entry and shrink the table.
      */
      table->tb_Table[table->tb_Entries] = NULL;

      if ( table->tb_Current >= i )
        table->tb_Current--;
    }
  }
}



/*****************************************************************************\
*******************************************************************************
**
**  Tree-List handling functions.
**
*******************************************************************************
\*****************************************************************************/

/*
**  Do a refresh, but only if not frozen. If so, set refresh flag.
*/
INLINE VOID DoRefresh( struct NListtree_Data *data )
{
  if(isFlagSet(data->Flags, NLTF_QUIET))
  {
    SET_FLAG(data->Flags, NLTF_REFRESH);
  }
  else
  {
    DoMethod( data->Obj, MUIM_NList_Redraw, MUIV_NList_Redraw_All );
  }
}


/*
**  Do only one quiet.
*/
INLINE ULONG DoQuiet( struct NListtree_Data *data, BOOL quiet )
{
  if ( quiet )
  {
    SET_FLAG(data->Flags, NLTF_QUIET);

    if ( ++data->QuietCounter == 1 )
      nnset( data->Obj, MUIA_NList_Quiet, TRUE );
  }
  else
  {
    if ( --data->QuietCounter == 0 )
    {
      nnset( data->Obj, MUIA_NList_Quiet, FALSE );
      CLEAR_FLAG(data->Flags, NLTF_QUIET);

      if(isFlagSet(data->Flags, NLTF_SETACTIVE))
      {
        set( data->Obj, MUIA_NListtree_Active, data->ActiveNode );
        CLEAR_FLAG(data->Flags, NLTF_SETACTIVE);
      }
    }
  }

  return( data->QuietCounter );
}


/*
**  Get the parent node to a given.
*/
INLINE struct MUI_NListtree_TreeNode *GetParent( struct MUI_NListtree_TreeNode *tn )
{
  return( tn ? tn->tn_Parent : NULL );
}


/*
**  Get the parent node to a given, but NOT the root list.
*/
struct MUI_NListtree_TreeNode *GetParentNotRoot( struct MUI_NListtree_TreeNode *tn )
{
  if((tn = GetParent(tn)))
  {
    if ( GetParent( tn ) )
    {
      return( tn );
    }
  }

  return( NULL );
}


/*
**  Check, if a given node is a child of another.
*/
struct MUI_NListtree_TreeNode *IsXChildOfY( struct MUI_NListtree_TreeNode *x, struct MUI_NListtree_TreeNode *y )
{
  do
  {
    if ( y == x )
    {
      return( y );
    }
  }
  while((y = GetParentNotRoot(y)));

  return( NULL );
}


/*
**  Check, if a given node is a child of a list member.
*/
struct MUI_NListtree_TreeNode *IsXChildOfListMemberNotSelf( struct MUI_NListtree_TreeNode *entry, struct Table *table )
{
  LONG i;

  for( i = 0; i < table->tb_Entries; i++ )
  {
    if ( entry != table->tb_Table[i] )
    {
      if ( IsXChildOfY( table->tb_Table[i], entry ) )
      {
        return( table->tb_Table[i] );
      }
    }
  }

  return( NULL );
}


struct MUI_NListtree_TreeNode *IsXChildOfListMember( struct MUI_NListtree_TreeNode *entry, struct Table *table )
{
  LONG i;

  for( i = 0; i < table->tb_Entries; i++ )
  {
    if ( IsXChildOfY( table->tb_Table[i], entry ) )
    {
      return( table->tb_Table[i] );
    }
  }

  return( NULL );
}



/*****************************************************************************\
*******************************************************************************
**
**  Helpful tree handling functions.
**
*******************************************************************************
\*****************************************************************************/

INLINE VOID RemoveNListEntry( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn, ULONG pos )
{
  D(DBF_LISTTREE,  "Removed entry '%s' from pos %ld.", tn->tn_Name, tn->tn_NListEntry ? tn->tn_NListEntry->entpos : 999999);

  DoMethod( data->Obj, MUIM_NList_Remove, pos );
  tn->tn_NListEntry = NULL;
}


INLINE VOID InsertNListEntry( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn, ULONG pos )
{
  struct MUI_NList_GetEntryInfo ei;

  DoMethod( data->Obj, MUIM_NList_InsertSingle, tn, pos );

  if(isFlagSet(data->Flags, NLTF_NLIST_DIRECT_ENTRY_SUPPORT))
  {
    ei.pos = -3; ei.line = -2;
    DoMethod( data->Obj, MUIM_NList_GetEntryInfo, &ei );
    tn->tn_NListEntry = (struct NListEntry *)ei.entry;

    D(DBF_LISTTREE, "Inserted entry '%s' at pos %ld.", tn->tn_Name, tn->tn_NListEntry->entpos);
  }
}

INLINE VOID ReplaceNListEntry( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn, ULONG pos )
{
  struct MUI_NList_GetEntryInfo ei;

  DoMethod(data->Obj, MUIM_NList_ReplaceSingle, tn, pos, NOWRAP, ALIGN_LEFT);

  if(isFlagSet(data->Flags, NLTF_NLIST_DIRECT_ENTRY_SUPPORT))
  {
    ei.pos = -3; ei.line = -2;
    DoMethod( data->Obj, MUIM_NList_GetEntryInfo, &ei );
    tn->tn_NListEntry = (struct NListEntry *)ei.entry;

    D(DBF_LISTTREE, "Replaced entry '%s' at pos %ld.", tn->tn_Name, tn->tn_NListEntry->entpos);
  }
}


/*
**  Iterate through all entries recursive and get specified entry by pos.
*/
struct MUI_NListtree_TreeNode *GetEntryByTotalPos( struct MUI_NListtree_ListNode *ln, LONG pos, LONG *cpos, ULONG flags )
{
  struct MUI_NListtree_TreeNode *tn = CTN( (APTR)&ln->ln_List );

  if(isFlagSet(ln->ln_Flags, TNF_LIST))
  {
    if(isFlagClear(flags, MUIV_NListtree_GetEntry_Flag_Visible) || isFlagSet(ln->ln_Flags, TNF_OPEN))
    {
      while((tn = CTN(GetSucc((struct Node *)tn))))
      {
        struct MUI_NListtree_TreeNode *rettn;

        if ( *cpos == pos )
          return( tn );

        if(isFlagClear(flags, MUIV_NListtree_GetEntry_Flag_Visible) || isFlagSet(tn->tn_IFlags, TNIF_VISIBLE))
          *cpos += 1;

        if(isFlagClear(flags, MUIV_NListtree_GetEntry_Flag_SameLevel))
        {
          if((rettn = GetEntryByTotalPos(CLN(tn), pos, cpos, flags)))
            return( rettn );
        }
      }
    }
  }

  return( NULL );
}


/*
**  Iterate through all entries recursive and get the pos of the specified entry.
*/
BOOL GetEntryPos( struct MUI_NListtree_ListNode *ln, struct MUI_NListtree_TreeNode *stn, LONG *pos )
{
  struct MUI_NListtree_TreeNode *tn = CTN( (APTR)&ln->ln_List );

  while((tn = CTN(GetSucc((struct Node *)tn))))
  {
    if ( stn == tn )
      return( TRUE );

    *pos += 1;

    if(isFlagSet(tn->tn_Flags, TNF_LIST))
    {
      if ( GetEntryPos( CLN( tn ), stn, pos ) )
        return( TRUE );
    }
  }

  return( FALSE );
}


/*************************************************************************
 Return the visual position (i.e. the position within the plain list)
 of the given TreeNode
**************************************************************************/
static LONG GetVisualPos(struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn)
{
  LONG pos = -1;

  ENTER();

  if(tn != CTN((APTR)&data->RootList) && tn != NULL)
  {
    if(isFlagSet(data->Flags, NLTF_NLIST_DIRECT_ENTRY_SUPPORT))
    {
      // respect visible nodes only
      if(isFlagSet(tn->tn_IFlags, TNIF_VISIBLE) && tn->tn_NListEntry != NULL)
        pos = (int)tn->tn_NListEntry->entpos;
    }

    // respect visible nodes only
    if(pos == -1 && isFlagSet(tn->tn_IFlags, TNIF_VISIBLE))
    {
      // result will be MUIV_NList_GetPos_End (-1) if not found.
      pos = MUIV_NList_GetPos_Start;
      DoMethod(data->Obj, MUIM_NList_GetPos, tn, &pos);
    }
  }

  RETURN(pos);
  return pos;
}

/*************************************************************************
 Count the number of visual entries in a tree node (excluding the
 node itself). A invisible has also 0 entries. Returns also 0 if the
 entry is no list.
**************************************************************************/
static LONG GetVisualEntries( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn)
{
  LONG entries = 0;

  ENTER();

  /* Easy case */
  if(tn == CTN((APTR)&data->RootList))
    entries = xget(data->Obj, MUIA_NList_Entries);
  else if(isFlagSet(tn->tn_Flags, TNF_LIST) && isFlagSet(tn->tn_Flags, TNF_OPEN) && isFlagSet(tn->tn_IFlags, TNIF_VISIBLE))
  {
    struct MUI_NListtree_TreeNode *tn2;

    tn2 = CTN(GetSucc((struct Node *)tn));
    if(tn2 != NULL && isFlagSet(tn2->tn_IFlags, TNIF_VISIBLE))
    {
      return GetVisualPos(data,tn2) - GetVisualPos(data,tn) - 1;
    }
    else
    {
      /* Number of entries is number of the entries directly in this
      ** list and the number of the entries within the open nodes
       */

      struct Table *tb = &CLN( tn )->ln_Table;
      int i;

      entries = tb->tb_Entries;

      for( i = 0; i < tb->tb_Entries; i++ )
      {
        entries += GetVisualEntries(data, tb->tb_Table[i]);
      }
    }
  }

  RETURN(entries);
  return entries;
}

/*
**  Count the number of visual entries in a tree node until a maximum of max.
*/
static BOOL GetVisualEntriesMax( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn, LONG pos, LONG *ent, LONG max )
{
  BOOL success = FALSE;

  ENTER();

  if(isFlagSet(tn->tn_Flags, TNF_OPEN))
  {
    struct Table *tb = &CLN( tn )->ln_Table;
    LONG i;

    *ent += tb->tb_Entries;

    for( i = 0; ( i < tb->tb_Entries ) && ( *ent <= max ); i++ )
    {
      if(GetVisualEntriesMax(data, tb->tb_Table[i], pos, ent, max))
      {
        success = TRUE;
        break;
      }
    }
  }

  RETURN(success);
  return success;
}


/*************************************************************************
 Determine the visual position a node would have, if inserted after
 prevtn inside ln
**************************************************************************/
static LONG GetVisualInsertPos( struct NListtree_Data *data, struct MUI_NListtree_ListNode *ln, struct MUI_NListtree_TreeNode *prevtn )
{
  LONG ent;

  ENTER();

  if ((SIPTR)prevtn == INSERT_POS_HEAD)
    ent = GetVisualPos(data, CTN(ln)) + 1;
  else if ( (SIPTR)prevtn == INSERT_POS_TAIL )
    ent = GetVisualPos(data, CTN(ln)) + GetVisualEntries(data, CTN(ln)) + 1;
  else
    ent = GetVisualPos(data, CTN(prevtn)) + GetVisualEntries(data,CTN(prevtn)) + 1;

  RETURN(ent);
  return ent;
}

struct MUI_NListtree_TreeNode *TreeNodeSelectAdd( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn )
{
  NLAddToTable( data, &data->SelectedTable, tn );

  D(DBF_LISTTREE, "Node 0x%08lx - %s selected (%ld nodes selected), Current: %ld", tn, tn->tn_Name, data->SelectedTable.tb_Entries, data->SelectedTable.tb_Current);

  return( tn );
}


VOID TreeNodeSelectRemove( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn )
{
  NLRemoveFromTable( data, &data->SelectedTable, tn );

  D(DBF_LISTTREE, "Node 0x%08lx - %s DEselected (%ld nodes selected), Current: %ld", tn, tn->tn_Name, data->SelectedTable.tb_Entries, data->SelectedTable.tb_Current);
}


/*
**  Change the selection state of one entry depending on supplied info.
*/
LONG TreeNodeSelectOne( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn, LONG seltype, LONG selflags, LONG *state )
{
  BOOL doselect = TRUE;
  LONG st, wassel = tn->tn_Flags;

  if ( seltype == MUIV_NListtree_Select_Toggle )
  {
    if(isFlagSet(tn->tn_Flags, TNF_SELECTED))
      st = MUIV_NListtree_Select_Off;
    else
      st = MUIV_NListtree_Select_On;
  }
  else
    st = seltype;

  if(isFlagClear(selflags, MUIV_NListtree_Select_Flag_Force))
  {
    doselect = MultiTestFunc( data, tn, st, selflags );
  }

  if ( doselect )
  {
    if(isFlagSet(tn->tn_IFlags, TNIF_VISIBLE))
    {
      if( st == MUIV_NListtree_Select_On && isFlagClear(wassel, TNF_SELECTED))
        DoMethod( data->Obj, MUIM_NList_Select, GetVisualPos( data, tn ), MUIV_NList_Select_On, NULL );

      else if(st == MUIV_NListtree_Select_Off && isFlagSet(wassel, TNF_SELECTED))
        DoMethod( data->Obj, MUIM_NList_Select, GetVisualPos( data, tn ), MUIV_NList_Select_Off, NULL );
    }
    else
    {
      switch ( st )
      {
        case MUIV_NListtree_Select_On:
          SET_FLAG(tn->tn_Flags, TNF_SELECTED);
          break;

        case MUIV_NListtree_Select_Off:
          CLEAR_FLAG(tn->tn_Flags, TNF_SELECTED);
          break;
      }
    }

    if ( state )
      *state = st;
  }

  return( st );
}


/*
**  Iterate through all entries recursive and let change flags (selected).
*/
VOID TreeNodeSelectAll( struct NListtree_Data *data, struct MUI_NListtree_ListNode *ln, LONG seltype, LONG selflags, LONG *state )
{
  LONG i;
  struct Table *tb = &ln->ln_Table;

  for( i = 0; i < tb->tb_Entries; i++ )
  {
    TreeNodeSelectOne( data, tb->tb_Table[i], seltype, selflags, state );

    if(isFlagSet(tb->tb_Table[i]->tn_Flags, TNF_LIST))
    {
      TreeNodeSelectAll( data, CLN( tb->tb_Table[i] ), seltype, selflags, state );
    }
  }
}



/*
**  Iterate through all entries recursive and let change flags (selected) of visible entries.
*/
VOID TreeNodeSelectVisible( struct NListtree_Data *data, struct MUI_NListtree_ListNode *ln, LONG seltype, LONG selflags, LONG *state )
{
  LONG i;
  struct Table *tb = &ln->ln_Table;

  for( i = 0; i < tb->tb_Entries; i++ )
  {
    if(isFlagSet(tb->tb_Table[i]->tn_IFlags, TNIF_VISIBLE))
    {
      TreeNodeSelectOne( data, tb->tb_Table[i], seltype, selflags, state );

      if(isFlagSet(tb->tb_Table[i]->tn_Flags, TNF_LIST))
      {
        TreeNodeSelectAll( data, CLN( tb->tb_Table[i] ), seltype, selflags, state );
      }
    }
  }
}



/*
**  Change the selection state depending on supplied info.
*/
LONG TreeNodeSelect( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn, LONG seltype, LONG selflags, LONG *state )
{
  if(isFlagSet(data->Flags, NLTF_AUTOSELECT_CHILDS))
  {
    TreeNodeSelectAll( data, CLN( tn ), seltype, selflags, state );
  }
  else
  {
    TreeNodeSelectOne( data, tn, seltype, selflags, state );
  }

  return( state ? *state : 0 );
}




struct MUI_NListtree_TreeNode *GetInsertNodeSorted( struct NListtree_Data *data, struct MUI_NListtree_ListNode *li, struct MUI_NListtree_TreeNode *tn )
{
  struct MUI_NListtree_TreeNode *in, *in2 = NULL;

  if((in = CTN(GetHead((struct List *)&li->ln_List))))
  {
    while((LONG)DoMethod(data->Obj, MUIM_NListtree_Compare, in, tn, 0) < 0)
    {
      in2 = in;

      if ( !( in = CTN(GetSucc( (struct Node *)in ) ) ) )
      {
        break;
      }
    }
  }

  if ( !in2 )
  {
    in2 = CTN( INSERT_POS_TAIL );
  }

  return( in2 );
}



VOID RemoveTreeVisible( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn, LONG pos )
{
  if(isFlagSet(tn->tn_Flags, TNF_LIST) && ( tn = CTN( GetHead( (struct List *)&(CLN( tn ))->ln_List ) ) ) )
  {
    do
    {
      if(isFlagSet(tn->tn_IFlags, TNIF_VISIBLE))
      {
        D(DBF_LISTTREE, "Visible removing node \"%s\" at pos %ld ( %s | %s )", tn->tn_Name, pos, isFlagSet(tn->tn_Flags, TNF_SELECTED) ? "SEL" : "", isFlagSet(data->Flags, NLTF_QUALIFIER_LSHIFT) ? "LSHIFT" : "");

        if(isFlagSet(tn->tn_Flags, TNF_SELECTED))
        {
          D(DBF_LISTTREE, "Unselecting node \"%s\" at pos %ld ( %s )", tn->tn_Name, pos, isFlagSet(data->Flags, NLTF_QUALIFIER_LSHIFT) ? "LSHIFT" : "");

          DoMethod( data->Obj, MUIM_NList_Select, pos, MUIV_NList_Select_Off, NULL );

          if(isFlagSet(data->Flags, NLTF_QUALIFIER_LSHIFT))
            SET_FLAG(tn->tn_Flags, TNF_SELECTED);
        }

        CLEAR_FLAG(tn->tn_IFlags, TNIF_VISIBLE);
        RemoveNListEntry( data, tn, pos );
      }

      if(isFlagSet(tn->tn_Flags, TNF_OPEN))
      {
        RemoveTreeVisible( data, tn, pos );
      }
    }
    while((tn = CTN(GetSucc((struct Node *)tn))));
  }
}


VOID ReplaceTreeVisibleSort(struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn, LONG *pos)
{
  /*
  **  Determine that the node holds a list
  **  and the node is not open.
  */
  if(isFlagSet(tn->tn_Flags, TNF_LIST) && isFlagSet(tn->tn_Flags, TNF_OPEN))
  {
    if((tn = CTN(GetHead((struct List *)&(CLN(tn))->ln_List))))
    {
      do
      {
        ReplaceNListEntry(data, tn, *pos);

        if(isFlagSet(tn->tn_Flags, TNF_SELECTED))
        {
          DoMethod(data->Obj, MUIM_NList_Select, *pos, MUIV_NList_Select_On, NULL);
        }

        if(*pos >= 0) *pos += 1;

        if(isFlagSet(tn->tn_Flags, TNF_OPEN))
        {
          ReplaceTreeVisibleSort(data, tn, pos);
        }
      }
      while((tn = CTN(GetSucc((struct Node *)tn))));
    }
  }
}


VOID RemoveTreeNodeVisible( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn, LONG *xpos )
{
  LONG pos;

  if(isFlagSet(tn->tn_IFlags, TNIF_VISIBLE))
  {
    pos = GetVisualPos( data, tn );

    D(DBF_LISTTREE, "Visible removing node \"%s\" at pos %ld ( %s | %s )", tn->tn_Name, pos, isFlagSet(tn->tn_Flags, TNF_SELECTED) ? "SEL" : "", isFlagSet(data->Flags, NLTF_QUALIFIER_LSHIFT) ? "LSHIFT" : "");

    if(isFlagSet(tn->tn_Flags, TNF_SELECTED))
    {
      D(DBF_LISTTREE, "Unselecting node \"%s\" at pos %ld ( %s )", tn->tn_Name, pos, isFlagSet(data->Flags, NLTF_QUALIFIER_LSHIFT) ? "LSHIFT" : "");

      DoMethod( data->Obj, MUIM_NList_Select, pos, MUIV_NList_Select_Off, NULL );

      if(isFlagSet(data->Flags, NLTF_QUALIFIER_LSHIFT))
        SET_FLAG(tn->tn_Flags, TNF_SELECTED);
    }

    CLEAR_FLAG(tn->tn_IFlags, TNIF_VISIBLE);
    RemoveNListEntry( data, tn, pos );

    if ( xpos )
      *xpos = pos;

    if(isFlagSet(tn->tn_Flags, TNF_OPEN))
    {
      RemoveTreeVisible( data, tn, pos );
    }

    if ( pos > 0 )  DoMethod( data->Obj, MUIM_NList_Redraw, pos - 1 );
  }
}


VOID InsertTreeVisible( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn, LONG *pos )
{
  /*
  **  Determine that the node holds a list
  **  and the node is not open.
  */
  if(isFlagSet(tn->tn_Flags, TNF_LIST))
  {
    if((tn = CTN(GetHead((struct List *)&(CLN(tn))->ln_List))))
    {
      do
      {
        if(isFlagClear(tn->tn_IFlags, TNIF_VISIBLE))
        {
          D(DBF_LISTTREE, "Visible inserting  node \"%s\" at pos %ld ( %s | %s )", tn->tn_Name, pos, isFlagSet(tn->tn_Flags, TNF_SELECTED) ? "SEL" : "", isFlagSet(data->Flags, NLTF_QUALIFIER_LSHIFT) ? "LSHIFT" : "");

          SET_FLAG(tn->tn_IFlags, TNIF_VISIBLE);
          InsertNListEntry( data, tn, *pos );

          if(isFlagSet(tn->tn_Flags, TNF_SELECTED))
          {
            D(DBF_LISTTREE, "  Selecting node \"%s\" at pos %ld ( %s )", tn->tn_Name, pos, isFlagSet(data->Flags, NLTF_QUALIFIER_LSHIFT) ? "LSHIFT" : "");

            DoMethod( data->Obj, MUIM_NList_Select, *pos, MUIV_NList_Select_On, NULL );
          }
        }

        if ( *pos >= 0 ) *pos += 1;

        if(isFlagSet(tn->tn_Flags, TNF_OPEN))
        {
          InsertTreeVisible( data, tn, pos );
        }
      }
      while((tn = CTN(GetSucc((struct Node *)tn))));
    }
  }
}


LONG InsertTreeNodeVisible( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn, struct MUI_NListtree_ListNode *ln, struct MUI_NListtree_TreeNode *prevtn )
{
  LONG pos = 0, temppos;

  if (isFlagSet(ln->ln_Flags, TNF_OPEN) && ((pos = GetVisualInsertPos( data, ln, prevtn)) != -1))
  {
    SET_FLAG(tn->tn_IFlags, TNIF_VISIBLE);

    D(DBF_LISTTREE, "Visible inserting node 0x%lx into list 0x%lx after 0x%lx at pos %ld", tn, ln, prevtn, pos);

    InsertNListEntry( data, tn, pos );

    if(isFlagSet(tn->tn_Flags, TNF_SELECTED))
    {
      D(DBF_LISTTREE, "  Selecting node \"%s\" at pos %ld ( %s )", tn->tn_Name, pos, isFlagSet(data->Flags, NLTF_QUALIFIER_LSHIFT) ? "LSHIFT" : "");

      DoMethod( data->Obj, MUIM_NList_Select, pos, MUIV_NList_Select_On, NULL );
    }

    temppos = pos + 1;

    if(isFlagSet(tn->tn_Flags, TNF_OPEN))
    {
      InsertTreeVisible( data, tn, &temppos );
    }

    if ( pos > 0 )
      DoMethod( data->Obj, MUIM_NList_Redraw, pos - 1 );
  }
  else
  {
    CLEAR_FLAG(tn->tn_IFlags, TNIF_VISIBLE);
  }

  return( pos );
}



VOID ShowTree(struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn)
{
  if(isFlagSet(tn->tn_Flags, TNF_OPEN))
  {
    LONG first = xget(data->Obj, MUIA_NList_First);
    LONG visible = xget(data->Obj, MUIA_NList_Visible);
    LONG max;
    LONG pos;
    LONG a;
    LONG b;
    LONG treeentries = 0;
    LONG jmp;

    pos = GetVisualPos(data, tn);
    GetVisualEntriesMax(data, tn, pos, &treeentries, visible);

    a = pos - first;
    b = visible - a - 1;

    max = a + b;
    jmp = pos + MIN(max, treeentries);

    D(DBF_LISTTREE, "first: %ld, visible: %ld, pos: %ld, treeentries: %ld, a: %ld, b: %ld, max: %ld, jmp: %ld",
      first, visible, pos, treeentries, a, b, max, jmp);

    DoMethod(data->Obj, MUIM_NList_Jump, jmp);
  }
}



/*
**
*/
VOID ExpandTree( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn )
{
  /*
  **  Determine that the node holds a list
  */
  if(isFlagSet(tn->tn_Flags, TNF_LIST))
  {
    LONG i;

    struct Table *tb = &CLN( tn )->ln_Table;

    for( i = 0; i < tb->tb_Entries; i++ )
    {
      if(isFlagSet(tb->tb_Table[i]->tn_Flags, TNF_LIST) && isFlagClear(tb->tb_Table[i]->tn_Flags, TNF_OPEN))
      {
        if ( data->OpenHook )
        {
          MyCallHook( data->OpenHook, data, MUIA_NListtree_OpenHook, tb->tb_Table[i] );
        }

        D(DBF_LISTTREE, "Expanding node \"%s\"", tb->tb_Table[i]->tn_Name);

        SET_FLAG(tb->tb_Table[i]->tn_Flags, TNF_OPEN);
      }

      CLEAR_FLAG(tb->tb_Table[i]->tn_IFlags, TNIF_VISIBLE);

      ExpandTree( data, tb->tb_Table[i] );
    }
  }
}


VOID OpenTreeNode( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn )
{
  LONG entries, pos, spos=0;

  if(isFlagClear(tn->tn_Flags, TNF_OPEN))
  {
    SET_FLAG(tn->tn_Flags, TNF_OPEN);

    if ( data->OpenHook )
    {
      MyCallHook( data->OpenHook, data, MUIA_NListtree_OpenHook, tn );
    }

    if(isFlagSet(tn->tn_IFlags, TNIF_VISIBLE))
    {
      spos = pos = GetVisualPos( data, tn );
      entries = xget( data->Obj, MUIA_NList_Entries );

      pos++;

      if ( pos == entries )
      {
        pos = MUIV_NList_Insert_Bottom;
      }

      DeactivateNotify( data );

      /*
      if ( tb->tb_Entries > ( data->NumEntries / 4 ) )
      {
        i = MUIV_NList_Insert_Bottom;

        DoMethod( data->Obj, MUIM_NList_Clear, 0 );

        CLEAR_FLAG(tn->tn_IFlags, TNIF_VISIBLE);
        InsertTreeVisible( data, CTN( &data->RootList ), &i );
      }
      else
      */
      {
        InsertTreeVisible( data, tn, &pos );
      }

      ActivateNotify( data );

      /* sba: the active note could be changed, but the notify calling was disabled */
      DoMethod(data->Obj, MUIM_NListtree_GetListActive, 0);
    }

    DoMethod( data->Obj, MUIM_NList_Redraw, spos );
  }
}


VOID OpenTreeNodeExpand( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn )
{
  LONG entries, pos;

  pos = GetVisualPos( data, tn );
  entries = xget( data->Obj, MUIA_NList_Entries );

  pos++;

  if ( pos == entries )
  {
    pos = MUIV_NList_Insert_Bottom;
  }

  if(isFlagClear(tn->tn_Flags, TNF_OPEN))
  {
    if ( data->OpenHook )
    {
      MyCallHook( data->OpenHook, data, MUIA_NListtree_OpenHook, tn );
    }

    D(DBF_LISTTREE, "Expanding node \"%s\"", tn->tn_Name);

    SET_FLAG(tn->tn_Flags, TNF_OPEN);

    ExpandTree( data, tn );
    InsertTreeVisible( data, tn, &pos );
  }
}


/*
**
*/
VOID OpenTreeNodeListExpand( struct NListtree_Data *data, struct MUI_NListtree_ListNode *ln )
{
  LONG i;
  struct Table *tb = &ln->ln_Table;

  if ( ln == &data->RootList )
  {
    i = MUIV_NList_Insert_Bottom;

    DoMethod( data->Obj, MUIM_NList_Clear, 0 );

    ExpandTree( data, CTN( ln ) );
    InsertTreeVisible( data, CTN( ln ), &i );
  }
/*
  else if ( tb->tb_Entries > (LONG)( data->NumEntries / 4 ) )
  {
    i = MUIV_NList_Insert_Bottom;

    DoMethod( data->Obj, MUIM_NList_Clear, 0 );

    ExpandTree( data, CTN( ln ) );
    InsertTreeVisible( data, CTN( &data->RootList ), &i );
  }
*/
  else
  {
    DoMethod(data->Obj, MUIM_NList_Redraw, GetVisualPos(data, tb->tb_Table[0]));

    for( i = 0; i < tb->tb_Entries; i++ )
    {
      OpenTreeNodeExpand( data, tb->tb_Table[i] );
    }
  }
}



/*
**
*/
VOID CollapseTree( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn, BOOL clearvisible )
{
  /*
  **  Determine that the node holds a list
  */
  if(isFlagSet(tn->tn_Flags, TNF_LIST))
  {
    LONG i;

    struct Table *tb = &CLN( tn )->ln_Table;

    for( i = 0; i < tb->tb_Entries; i++ )
    {
      if(isFlagSet(tb->tb_Table[i]->tn_Flags, TNF_OPEN))
      {
        if ( data->CloseHook )
        {
          MyCallHook( data->CloseHook, data, MUIA_NListtree_CloseHook, tb->tb_Table[i] );
        }

        D(DBF_LISTTREE, "Collapsing node \"%s\"", tb->tb_Table[i]->tn_Name);

        CLEAR_FLAG(tb->tb_Table[i]->tn_Flags, TNF_OPEN);
      }

      if ( clearvisible )
        CLEAR_FLAG(tb->tb_Table[i]->tn_IFlags, TNIF_VISIBLE);

      CollapseTree( data, tb->tb_Table[i], clearvisible );
    }
  }
}




VOID CloseTreeNode( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn )
{
  LONG pos;

  if(isFlagSet(tn->tn_Flags, TNF_OPEN))
  {
    pos = GetVisualPos( data, tn ) + 1;

    if ( data->CloseHook )
    {
      MyCallHook( data->CloseHook, data, MUIA_NListtree_CloseHook, tn );
    }

    CLEAR_FLAG(tn->tn_Flags, TNF_OPEN);

    DeactivateNotify( data );

    /*
    if ( tb->tb_Entries > ( data->NumEntries / 4 ) )
    {
      i = MUIV_NList_Insert_Bottom;

      DoMethod( data->Obj, MUIM_NList_Clear, 0 );

      CLEAR_FLAG(tn->tn_IFlags, TNIF_VISIBLE);
      InsertTreeVisible( data, CTN( &data->RootList ), &i );
    }
    else
    */
    {
      RemoveTreeVisible( data, tn, pos );
    }

    ActivateNotify( data );

    /* sba: the active note could be changed, but the notify calling was disabled */
    DoMethod(data->Obj, MUIM_NListtree_GetListActive, 0);

    DoMethod( data->Obj, MUIM_NList_Redraw, pos - 1 );
  }
}


VOID CloseTreeNodeCollapse( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn )
{
  LONG pos;

  pos = GetVisualPos( data, tn ) + 1;

  if(isFlagSet(tn->tn_Flags, TNF_OPEN))
  {
    if ( data->CloseHook )
    {
      MyCallHook( data->CloseHook, data, MUIA_NListtree_CloseHook, tn );
    }

    D(DBF_LISTTREE, "Collapsing node \"%s\"", tn->tn_Name);

    CLEAR_FLAG(tn->tn_Flags, TNF_OPEN);
  }

  RemoveTreeVisible( data, tn, pos );
  CollapseTree( data, tn, FALSE );

  DoMethod( data->Obj, MUIM_NList_Redraw, pos - 1 );
}


/*
**
*/
VOID CloseTreeNodeListCollapse( struct NListtree_Data *data, struct MUI_NListtree_ListNode *ln )
{
  LONG i;
  struct Table *tb = &ln->ln_Table;

  if ( ln == &data->RootList )
  {
    i = MUIV_NList_Insert_Bottom;

    DoMethod( data->Obj, MUIM_NList_Clear, 0 );

    CollapseTree( data, CTN( ln ), TRUE );
    InsertTreeVisible( data, CTN( ln ), &i );
  }
/*
  else if(tb->tb_Entries > (LONG)(data->NumEntries / 4))
  {
    i = MUIV_NList_Insert_Bottom;

    DoMethod( data->Obj, MUIM_NList_Clear, 0 );

    CollapseTree( data, CTN( ln ), TRUE );
    InsertTreeVisible( data, CTN( &data->RootList ), &i );
  }
*/
  else
  {
    for( i = 0; i < tb->tb_Entries; i++ )
    {
      CloseTreeNodeCollapse( data, tb->tb_Table[i] );
    }
  }
}



VOID ActivateTreeNode(struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn)
{
  struct MUI_NListtree_TreeNode *tn2 = tn;
  ULONG jmppos;
  LONG newact;

  if(data->ActiveNode != NULL)
  {
    switch(data->AutoVisible)
    {
      case MUIV_NListtree_AutoVisible_Off:
      {
        // nothing to do
      }
      break;

      case MUIV_NListtree_AutoVisible_Normal:
      {
        if(isFlagSet(tn2->tn_IFlags, TNIF_VISIBLE))
        {
          jmppos = GetVisualPos(data, tn2);
          DoMethod(data->Obj, MUIM_NList_Jump, jmppos);
        }
      }
      break;

      case MUIV_NListtree_AutoVisible_FirstOpen:
      {
        while(isFlagClear(tn2->tn_IFlags, TNIF_VISIBLE))
          tn2 = tn2->tn_Parent;

        jmppos = GetVisualPos(data, tn2);
        DoMethod(data->Obj, MUIM_NList_Jump, jmppos);
      }
      break;

      case MUIV_NListtree_AutoVisible_Expand:
      {
        while((tn2 = tn2->tn_Parent) != NULL)
          OpenTreeNode(data, tn2);

        jmppos = GetVisualPos(data, tn2);
        DoMethod(data->Obj, MUIM_NList_Jump, jmppos);
      }
      break;
    }
  }

  if(tn != NULL)
    newact = GetVisualPos(data, tn);
  else
    newact = MUIV_NList_Active_Off;

  /* sba: set the new entry only if it really changes, otherwise we lose some
     MUIA_NList_Active notifications */

  D(DBF_LISTTREE, "ActivateTreeNode: %ld %ld",newact,xget(data->Obj,MUIA_NList_Active));
  if(newact != (LONG)xget(data->Obj,MUIA_NList_Active))
    set(data->Obj, MUIA_NList_Active, newact);
}


VOID RemoveNode1( struct NListtree_Data *data, UNUSED struct MUI_NListtree_ListNode *li, struct MUI_NListtree_TreeNode *tn, LONG pos )
{
  /*
  **  If deleted entry is active, then activate the next/previous node.
  */
  if ( data->TempActiveNode == tn )
  {
    /*
    **  Make sure that deleting the last entry in a
    **  list node causes the NEW last entry to be active!
    */
    if ( !( data->TempActiveNode = CTN( GetSucc( (struct Node *)&tn->tn_Node ) ) ) )
      if ( !( data->TempActiveNode = CTN( GetPred( (struct Node *)&tn->tn_Node ) ) ) )
        data->TempActiveNode = GetParentNotRoot( tn );

    D(DBF_LISTTREE, "Would set active node to: %s - 0x%08lx", data->TempActiveNode ? data->TempActiveNode->tn_Name : (STRPTR)"NULL", data->TempActiveNode);
  }

  D(DBF_LISTTREE, "Removing node: %s - 0x%08lx, pos: %ld", tn->tn_Name, tn, pos);

  /*
  **  Prevent calculating positions with practical
  **  deleted entries.
  */
  if(isFlagSet(tn->tn_IFlags, TNIF_VISIBLE))
  {
    CLEAR_FLAG(tn->tn_IFlags, TNIF_VISIBLE);

    RemoveNListEntry( data, tn, pos );
  }

  /*
  **  Remove the node from the selected list.
  */
  if(isFlagSet(tn->tn_Flags, TNF_SELECTED))
  {
    TreeNodeSelectRemove( data, tn );
  }
}

VOID RemoveNode2( struct NListtree_Data *data, struct MUI_NListtree_ListNode *li, struct MUI_NListtree_TreeNode *tn )
{
  Remove( (struct Node *)&tn->tn_Node );
  NLRemoveFromTable( data, &li->ln_Table, tn );

  /*
  **  Call internal/user supplied destruct function.
  */
  DoMethod(data->Obj, MUIM_NListtree_Destruct, tn->tn_Name, tn->tn_User, data->TreePool, 0);

  /*
  **  Free all allocated memory.
  */
  if(isFlagSet(tn->tn_IFlags, TNIF_ALLOCATED))
  {
    if ( tn->tn_Name )
      FreeVecPooled( data->TreePool, tn->tn_Name );
  }

  if(isFlagSet(tn->tn_Flags, TNF_LIST))
  {
    if ( ( (struct MUI_NListtree_ListNode *)tn )->ln_Table.tb_Table )
      FreeVecPooled( data->TreePool, ( (struct MUI_NListtree_ListNode *)tn )->ln_Table.tb_Table );

    ( (struct MUI_NListtree_ListNode *)tn )->ln_Table.tb_Table = NULL;
  }

  FreeVecPooled( data->TreePool, tn );

  /*
  **  Subtract one from the global number of entries.
  */
  data->NumEntries--;
}



VOID RemoveChildNodes( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn, LONG pos )
{
  struct MUI_NListtree_ListNode *ln = CLN( tn );

  if(isFlagSet(tn->tn_Flags, TNF_LIST))
  {
    while((tn = CTN(GetHead((struct List *)&ln->ln_List))))
    {
      D(DBF_LISTTREE, "Removing node \"%s\", pos %ld", tn->tn_Name, pos);

      RemoveChildNodes( data, tn, pos + 1 );
      RemoveNode1( data, CLN( GetParent( tn ) ), tn, pos );
      RemoveNode2( data, CLN( GetParent( tn ) ), tn );
    }
  }
}



VOID RemoveNodes( struct NListtree_Data *data, struct MUI_NListtree_ListNode *li, struct MUI_NListtree_TreeNode *tn, LONG pos )
{
  RemoveChildNodes( data, tn, pos + 1 );
  RemoveNode1( data, li, tn, pos );
  RemoveNode2( data, li, tn );

  if ( pos > 0 )  DoMethod( data->Obj, MUIM_NList_Redraw, pos - 1 );
}



VOID QuickRemoveNodes( struct NListtree_Data *data, struct MUI_NListtree_ListNode *li )
{
  struct MUI_NListtree_ListNode *ln = li;

  if(isFlagSet(li->ln_Flags, TNF_LIST))
  {
    while((li = CLN(RemHead((struct List *)&ln->ln_List))))
    {
      if(isFlagSet(li->ln_Flags, TNF_LIST))
        QuickRemoveNodes( data, li );

      if(isFlagSet(li->ln_Flags, TNF_SELECTED))
      {
        TreeNodeSelectRemove( data, CTN( li ) );
      }

      /*
      **  Call internal/user supplied destruct function.
      */
      DoMethod(data->Obj, MUIM_NListtree_Destruct, li->ln_Name, li->ln_User, data->TreePool, 0);
    }
  }
}


struct MUI_NListtree_TreeNode *FindTreeNodeByName( struct NListtree_Data *data, struct MUI_NListtree_ListNode *ln, STRPTR name, UWORD flags )
{
  struct MUI_NListtree_TreeNode *tn2, *tn;

  if(isFlagSet(flags, MUIV_NListtree_FindName_Flag_StartNode))
    tn = CTN( ln );
  else
    tn = CTN( GetHead( (struct List *)&ln->ln_List ) );


  if ( tn )
  {
    do
    {
      if(isFlagSet(tn->tn_IFlags, TNIF_VISIBLE) || isFlagClear(flags, MUIV_NListtree_FindName_Flag_Visible))
      {
        if(isFlagSet(tn->tn_Flags, TNF_SELECTED) || isFlagClear(flags, MUIV_NListtree_FindName_Flag_Selected))
        {
          if ( !( (LONG)MyCallHook( data->FindNameHook, data, MUIA_NListtree_FindNameHook, name, tn->tn_Name, tn->tn_User, flags ) ) )
          {
            return( tn );
          }
        }
      }

      if(isFlagSet(tn->tn_Flags, TNF_LIST) && isFlagClear(flags, MUIV_NListtree_FindName_Flag_SameLevel))
      {
        if((tn2 = FindTreeNodeByName(data, CLN(tn), name, (flags & ~MUIV_NListtree_FindName_Flag_StartNode))))
        {
          return( tn2 );
        }
      }
    }
    while((tn = CTN(GetSucc((struct Node *)&tn->tn_Node))));
  }

  return( NULL );
}



struct MUI_NListtree_TreeNode *FindTreeNodeByNameRev( struct NListtree_Data *data, struct MUI_NListtree_ListNode *ln, STRPTR name, UWORD flags )
{
  struct MUI_NListtree_TreeNode *tn;

  do
  {
    if(isFlagSet(flags, MUIV_NListtree_FindName_Flag_StartNode))
      tn = CTN( ln );
    else
      tn = CTN( GetTail( (struct List *)&ln->ln_List ) );

    if ( tn )
    {
      do
      {
        if(isFlagSet(tn->tn_IFlags, TNIF_VISIBLE) || isFlagClear(flags, MUIV_NListtree_FindName_Flag_Visible))
        {
          if(isFlagSet(tn->tn_Flags, TNF_SELECTED) || isFlagClear(flags, MUIV_NListtree_FindName_Flag_Selected))
          {
            if ( !( (LONG)MyCallHook( data->FindNameHook, data, MUIA_NListtree_FindNameHook, name, tn->tn_Name, tn->tn_User, flags ) ) )
            {
              return( tn );
            }
          }
        }
      }
      while((tn = CTN(GetPred((struct Node *)&tn->tn_Node))));

      if(isFlagClear(flags, MUIV_NListtree_FindName_Flag_SameLevel))
      {
        ln = CLN( GetParent( CTN( ln ) ) );
        SET_FLAG(flags, MUIV_NListtree_FindName_Flag_StartNode);
      }
      else
        ln = NULL;
    }
  }
  while ( ln );

  return( NULL );
}



struct MUI_NListtree_TreeNode *FindTreeNodeByUserData( struct NListtree_Data *data, struct MUI_NListtree_ListNode *ln, STRPTR userdata, UWORD flags )
{
  struct MUI_NListtree_TreeNode *tn2, *tn;

  if(isFlagSet(flags, MUIV_NListtree_FindUserData_Flag_StartNode))
    tn = CTN( ln );
  else
    tn = CTN( GetHead( (struct List *)&ln->ln_List ) );


  if ( tn )
  {
    do
    {
      if(isFlagSet(tn->tn_IFlags, TNIF_VISIBLE) || isFlagClear(flags, MUIV_NListtree_FindUserData_Flag_Visible))
      {
        if(isFlagSet(tn->tn_Flags, TNF_SELECTED) || isFlagClear(flags, MUIV_NListtree_FindUserData_Flag_Selected))
        {
          if ( !( (LONG)MyCallHook( data->FindUserDataHook, data, MUIA_NListtree_FindUserDataHook, userdata, tn->tn_User, tn->tn_Name, flags ) ) )
          {
            return( tn );
          }
        }
      }

      if(isFlagSet(tn->tn_Flags, TNF_LIST) && isFlagClear(flags, MUIV_NListtree_FindUserData_Flag_SameLevel))
      {
        if((tn2 = FindTreeNodeByUserData(data, CLN(tn), userdata, (flags & ~MUIV_NListtree_FindUserData_Flag_StartNode))))
        {
          return( tn2 );
        }
      }
    }
    while((tn = CTN(GetSucc((struct Node *)&tn->tn_Node))));
  }

  return( NULL );
}



struct MUI_NListtree_TreeNode *FindTreeNodeByUserDataRev( struct NListtree_Data *data, struct MUI_NListtree_ListNode *ln, STRPTR userdata, UWORD flags )
{
  struct MUI_NListtree_TreeNode *tn;

  do
  {
    if(isFlagSet(flags, MUIV_NListtree_FindUserData_Flag_StartNode))
      tn = CTN( ln );
    else
      tn = CTN( GetTail( (struct List *)&ln->ln_List ) );

    if ( tn )
    {
      do
      {
        if(isFlagSet(tn->tn_IFlags, TNIF_VISIBLE) || isFlagClear(flags, MUIV_NListtree_FindUserData_Flag_Visible))
        {
          if(isFlagSet(tn->tn_Flags, TNF_SELECTED) || isFlagClear(flags, MUIV_NListtree_FindUserData_Flag_Selected))
          {
            if ( !( (LONG)MyCallHook( data->FindUserDataHook, data, MUIA_NListtree_FindUserDataHook, userdata, tn->tn_User, tn->tn_Name, flags ) ) )
            {
              return( tn );
            }
          }
        }
      }
      while((tn = CTN(GetPred((struct Node *)&tn->tn_Node))));

      if(isFlagClear(flags, MUIV_NListtree_FindUserData_Flag_SameLevel))
      {
        ln = CLN( GetParent( CTN( ln ) ) );
        SET_FLAG(flags, MUIV_NListtree_FindUserData_Flag_StartNode);
      }
      else
        ln = NULL;
    }
  }
  while ( ln );

  return( NULL );
}



VOID InsertTreeNode( struct NListtree_Data *data, struct MUI_NListtree_ListNode *ln, struct MUI_NListtree_TreeNode *tn1, struct MUI_NListtree_TreeNode *tn2 )
{
  /*
  **  Give the entry a parent.
  */
  tn1->tn_Parent = CTN( ln );

  /*
  **  Insert entry into list.
  */
  if ( (IPTR)tn2 == (IPTR)INSERT_POS_HEAD )
  {
    AddHead( (struct List *)&ln->ln_List, (struct Node *)&tn1->tn_Node );
  }
  else if ( (IPTR)tn2 == (IPTR)INSERT_POS_TAIL )
  {
    AddTail( (struct List *)&ln->ln_List, (struct Node *)&tn1->tn_Node );
  }
  else
  {
    Insert( (struct List *)&ln->ln_List, (struct Node *)&tn1->tn_Node, (struct Node *)&tn2->tn_Node );
  }

  InsertTreeNodeVisible( data, tn1, ln, tn2 );
  NLAddToTable( data, &ln->ln_Table, tn1 );
}



struct MUI_NListtree_TreeNode *DuplicateNode( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *nodetodup )
{
  struct MUI_NListtree_ListNode *new;
  APTR user = NULL;

  if((new = CLN(AllocVecPooled( data->TreePool, isFlagSet(nodetodup->tn_Flags, TNF_LIST) ? sizeof(struct MUI_NListtree_ListNode ) : sizeof(struct MUI_NListtree_TreeNode)))))
  {
    if(isFlagSet(nodetodup->tn_Flags, TNF_LIST))
      NewList( (struct List *)&new->ln_List );

    /*
    **  Should we duplicate the supplied node name?
    */
    if(isFlagSet(data->Flags, NLTF_DUPNODENAMES))
    {
      int len = strlen( nodetodup->tn_Name ) + 1;

      new->ln_Name = (STRPTR)AllocVecPooled( data->TreePool, len );
      strlcpy( new->ln_Name, nodetodup->tn_Name, len );
      SET_FLAG(new->ln_IFlags, TNIF_ALLOCATED);
    }
    else
      new->ln_Name = nodetodup->tn_Name;

    /*
    **  Create new user dats
    */
    user = (APTR)DoMethod(data->Obj, MUIM_NListtree_Construct, nodetodup->tn_Name, nodetodup->tn_User, data->TreePool, MUIV_NListtree_ConstructHook_Flag_AutoCreate);

    if ( !new->ln_Name || !user )
    {
      /*
      **  Free all previously allocated memory if
      **  something failed before.
      */
      if(new->ln_Name != NULL && isFlagSet(new->ln_IFlags, TNIF_ALLOCATED))
        FreeVecPooled( data->TreePool, new->ln_Name );

      DoMethod(data->Obj, MUIM_NListtree_Destruct, new->ln_Name, new->ln_User, data->TreePool, 0);

      FreeVecPooled( data->TreePool, new );
      new = NULL;
    }
    else
    {
      new->ln_User  = user;
      new->ln_Flags = ( nodetodup->tn_Flags & ~TNF_SELECTED );

      /*
      **  Add one to the global number of entries
      */
      data->NumEntries++;
    }
  }

  return(CTN(new));
}



struct MUI_NListtree_TreeNode *CreateParentStructure( struct NListtree_Data *data, ULONG method, struct MUI_NListtree_ListNode **destlist, struct MUI_NListtree_TreeNode **destentry, struct MUI_NListtree_TreeNode *nodetodup, UWORD cnt )
{
  struct MUI_NListtree_ListNode *prevnode, *newlist;

  if((prevnode = CLN(GetParentNotRoot(nodetodup))))
    *destlist = CLN( CreateParentStructure( data, method, destlist, destentry, CTN(prevnode), cnt + 1 ) );

  D(DBF_LISTTREE, "Adding node 0x%08lx - %s to node 0x%08lx - %s", nodetodup, nodetodup->tn_Name, *destlist, (*destlist)->ln_Name);

  /*
  **  Allocate memory for the new entry and
  **  initialize list structure for use.
  */
  if ( ( method == MUIM_NListtree_Copy ) || cnt )
  {
    newlist = CLN( DuplicateNode( data, nodetodup ) );
  }
  else
  {
    newlist = CLN(nodetodup);
  }

  if ( newlist )
  {
    /*
    **  Add the new created entry to the specified list.
    */
    if ( cnt )
    {
      /*
      **  Insert entry into list.
      */
      InsertTreeNode( data, *destlist, CTN(newlist), *destentry );
      *destentry = CTN( INSERT_POS_TAIL );
      if(isFlagSet(nodetodup->tn_IFlags, TNIF_VISIBLE))
        SET_FLAG(newlist->ln_IFlags, TNIF_VISIBLE);
      else
        CLEAR_FLAG(newlist->ln_IFlags, TNIF_VISIBLE);
    }
  }

  return( CTN( newlist ) );
}



struct MUI_NListtree_TreeNode *CreateChildStructure( struct NListtree_Data *data, struct MUI_NListtree_ListNode *listnode, struct MUI_NListtree_ListNode *nodetodup, struct MUI_NListtree_ListNode *orignode, ULONG cnt )
{
  struct MUI_NListtree_ListNode *new;

  if ( cnt > 0 )
    new = CLN(DuplicateNode( data, CTN(nodetodup)));
  else
    new = nodetodup;

  if ( new )
  {
    /*
    **  Insert entry into list.
    */
    InsertTreeNode( data, listnode, CTN(new), CTN( INSERT_POS_TAIL ) );

    if(isFlagSet(orignode->ln_Flags, TNF_LIST))
    {
      if ( !IsListEmpty( (struct List *)&orignode->ln_List ) )
      {
        struct MUI_NListtree_ListNode *ln = CLN( &orignode->ln_List );
        LONG i;

        for ( i = 0; i < orignode->ln_Table.tb_Entries; i++ )
        {
          ln = CLN( GetSucc( (struct Node *)ln ) );

          CreateChildStructure( data, new, ln, ln, cnt + 1 );
        }
      }
    }
  }

  return(CTN(new));
}



/*****************************************************************************\
*******************************************************************************
**
**  Graphical and render functions.
**
*******************************************************************************
\*****************************************************************************/

static void InsertTreeImages( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn, struct MUI_NListtree_TreeNode *otn, UWORD cnt )
{
  struct MUI_NListtree_TreeNode *gp;

  if ( tn )
  {
    LONG x1 = -1, x2 = 0;

    // we do a recursive call in case we find a
    // parent node for the current one.
    if((gp = GetParent(tn)))
      InsertTreeImages( data, gp, otn, cnt + 1 );

    x2 = data->MaxImageWidth;

    if ( GetSucc( (struct Node *)&tn->tn_Node ) )
    {
      if(cnt == 0)
      {
        if(isFlagClear(tn->tn_Flags, TNF_LIST))
        {
          x1 = SPEC_VertT;
        }
      }
      else
      {
        x1 = SPEC_Vert;
      }
    }
    else
    {
      if(cnt == 0)
      {
        if(isFlagClear(tn->tn_Flags, TNF_LIST))
        {
          x1 = SPEC_VertEnd;
        }
      }
      else if ( gp )
      {
        x1 = SPEC_Space;
      }
    }

    if ( ( x1 != -1 ) && x2 )
    {
      if ( x1 == SPEC_Space )
        otn->tn_Space += x2 + data->IndentWidth;

      if ( otn->tn_Space > 0 )
      {
        snprintf(data->buf, DATA_BUF_SIZE, "%s\033O[%lx;%x;%d,%d]", data->buf, (unsigned long)data->Image[IMAGE_Tree].ListImage, (unsigned int)MUIA_TI_Spec, (int)SPEC_Space, (int)otn->tn_Space );

        otn->tn_ImagePos += otn->tn_Space;
        otn->tn_Space = 0;
      }

      if ( x1 != SPEC_Space )
      {
        x2 += data->IndentWidth;

        /*
        **  Should we draw the root tree? No? Just use space.
        */
        if(isFlagSet(data->Flags, NLTF_NO_ROOT_TREE) && gp->tn_Parent == NULL)
        {
          snprintf(data->buf, DATA_BUF_SIZE, "%s\033O[%lx;%x;%d,%d]", data->buf, (unsigned long)data->Image[IMAGE_Tree].ListImage, (unsigned int)MUIA_TI_Spec, (int)SPEC_Space, (int)x2 );

          otn->tn_ImagePos += x2;
        }
        else
        {
          // ensures proper text alignment with subnodes in
          // case the user selected an additional indentwidth
          if(data->IndentWidth > 0)
            x2 += 2;

          snprintf(data->buf, DATA_BUF_SIZE, "%s\033O[%lx;%x;%d,%d]", data->buf, (unsigned long)data->Image[IMAGE_Tree].ListImage, (unsigned int)MUIA_TI_Spec, (int)x1, (int)x2 );

          otn->tn_ImagePos += x2;
        }
      }
    }
  }
}

static void InsertImage( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *otn )
{
  LONG x1 = -1;

  InsertTreeImages( data, otn, otn, 0 );

  if((isFlagSet(otn->tn_Flags, TNF_LIST) && isFlagClear(otn->tn_Flags, TNF_NOSIGN)) && ( !IsListEmpty( (struct List *)&(CLN( otn ))->ln_List ) || isFlagClear(data->Flags, NLTF_EMPTYNODES)))
  {
    if(isFlagSet(otn->tn_Flags, TNF_OPEN))
    {
      x1 = IMAGE_Open;
    }
    else
    {
      x1 = IMAGE_Closed;
    }

    snprintf(data->buf, DATA_BUF_SIZE, "%s\033O[%lx]", data->buf, (unsigned long)data->Image[x1].ListImage );

    x1 = SPEC_Hor;

    // add some indent width
    if(data->IndentWidth > 0)
      snprintf(data->buf, DATA_BUF_SIZE, "%s\033O[%lx;%x;%d,%d]", data->buf, (unsigned long)data->Image[IMAGE_Tree].ListImage, (unsigned int)MUIA_TI_Spec, (int)x1, (unsigned int)data->IndentWidth);

    if(data->UseFolderImage == TRUE)
    {
      snprintf(data->buf, DATA_BUF_SIZE, "%s\033O[%lx]\033O[%lx;%x;%d,%d]", data->buf, (unsigned long)data->Image[IMAGE_Folder].ListImage, (unsigned long)data->Image[IMAGE_Tree].ListImage, (unsigned int)MUIA_TI_Spec, (unsigned int)SPEC_Space, 3);
    }
  }
}


static void DrawImages( struct MUI_NListtree_TreeNode *otn, struct MUI_NListtree_TreeNode *tn, struct NListtree_Data *data, UWORD cnt )
{
  if ( tn )
  {
    struct MUI_NListtree_TreeNode *gp;

    if((gp = GetParent(tn)))
      DrawImages( otn, gp, data, cnt + 1 );

    if(cnt == 0)
      InsertImage( data, otn );
  }
}



/*****************************************************************************\
*******************************************************************************
**
**  General internal class related functions.
**
*******************************************************************************
\*****************************************************************************/

ULONG MultiTestFunc( struct NListtree_Data *data, struct MUI_NListtree_TreeNode *tn, LONG seltype, LONG selflags )
{
  LONG currtype = isFlagSet(tn->tn_Flags, TNF_SELECTED) ? MUIV_NListtree_Select_On : MUIV_NListtree_Select_Off;
  BOOL doselect = TRUE;

  if(isFlagSet(data->Flags, NLTF_DRAGDROP))
    return( FALSE );

  if(isFlagSet(data->Flags, NLTF_NLIST_NO_SCM_SUPPORT))
    return( FALSE );

  if ( currtype != seltype )
  {
    if ( data->MultiTestHook )
    {
      doselect = (BOOL)MyCallHook( data->MultiTestHook, data, MUIA_NListtree_MultiTestHook, tn, seltype, selflags, currtype );
    }

    if ( doselect )
    {
      doselect = DoMethod( data->Obj, MUIM_NListtree_MultiTest, tn, seltype, selflags, currtype );
    }
  }


  /*
  if ( doselect )
    D(DBF_ALWAYS, "MULTITEST: 0x%08lx - %s", tn, tn->tn_Name);
  */

  return( doselect );
}

HOOKPROTONH(NList_MultiTestFunc, ULONG, Object *obj, struct MUI_NListtree_TreeNode *tn)
{
  struct NListtree_Data *data;
  ULONG ret = FALSE;

  data = (struct NListtree_Data *)xget( obj, MUIA_UserData );

  if(isFlagSet(data->Flags, NLTF_SELECT_METHOD))
    return( TRUE );

  if ( data->MultiSelect != MUIV_NListtree_MultiSelect_None )
  {
    if ( ( data->MultiSelect == MUIV_NListtree_MultiSelect_Default ) ||
      ( data->MultiSelect == MUIV_NListtree_MultiSelect_Always ) ||
      ( ( data->MultiSelect == MUIV_NListtree_MultiSelect_Shifted ) && isFlagSet(data->Flags, NLTF_QUALIFIER_LSHIFT)))
    {
      ret = MultiTestFunc( data, tn, MUIV_NListtree_Select_On, 0 );
    }
  }

  return( ret );
}
MakeStaticHook(NList_MultiTestHook, NList_MultiTestFunc);

HOOKPROTONHNO(_ConstructFunc, APTR, struct MUIP_NListtree_ConstructMessage *msg)
{
  APTR retdata;

  /*
  **  Allocate memory for the user field and
  **  copy the supplied string to the new
  **  allocated piece of memory.
  */
  if ( msg->UserData )
  {
    int len = strlen((STRPTR)msg->UserData ) + 1;

    if((retdata = AllocVecPooled(msg->MemPool, len)))
    {
      strlcpy( (STRPTR)retdata, (STRPTR)msg->UserData, len );

      D(DBF_LISTTREE, "Internal CostructHook ==> Data: '%s'", (STRPTR)msg->UserData);

      return( retdata );
    }
  }

  return( NULL );
}
MakeStaticHook(_ConstructHook, _ConstructFunc);

HOOKPROTONHNO(_DestructFunc, ULONG, struct MUIP_NListtree_DestructMessage *msg)
{
  D(DBF_LISTTREE, "Internal DestructHook ==> Data: '%s'", (STRPTR)msg->UserData);

  /*
  **  Free the string memory.
  */
  if ( msg->UserData )
  {
    FreeVecPooled( msg->MemPool, msg->UserData );
  }

  return( 0 );
}
MakeStaticHook(_DestructHook, _DestructFunc);

/*
**
**  The internal compare hook functions.
**
*/
HOOKPROTONHNONP(_CompareFunc_Head, LONG) //struct MUIP_NListtree_CompareMessage *msg)
{
  return( -1 );
}
MakeStaticHook(_CompareHook_Head, _CompareFunc_Head);

HOOKPROTONHNONP(_CompareFunc_Tail, LONG) //struct MUIP_NListtree_CompareMessage *msg)
{
  return( 1 );
}
MakeStaticHook(_CompareHook_Tail, _CompareFunc_Tail);

HOOKPROTONHNO(_CompareFunc_LeavesTop, LONG, struct MUIP_NListtree_CompareMessage *msg)
{
  if(isFlagSet(msg->TreeNode1->tn_Flags, TNF_LIST) && isFlagSet(msg->TreeNode2->tn_Flags, TNF_LIST))
  {
    return( Stricmp( msg->TreeNode1->tn_Name, msg->TreeNode2->tn_Name ) );
  }
  else if(isFlagSet(msg->TreeNode1->tn_Flags, TNF_LIST))
  {
    return( 1 );
  }
  else if(isFlagSet(msg->TreeNode2->tn_Flags, TNF_LIST))
  {
    return( -1 );
  }
  else
  {
    return( Stricmp( msg->TreeNode1->tn_Name, msg->TreeNode2->tn_Name ) );
  }
}
MakeStaticHook(_CompareHook_LeavesTop, _CompareFunc_LeavesTop);

HOOKPROTONHNO(_CompareFunc_LeavesBottom, LONG, struct MUIP_NListtree_CompareMessage *msg)
{
  if(isFlagSet(msg->TreeNode1->tn_Flags, TNF_LIST) && isFlagSet(msg->TreeNode2->tn_Flags, TNF_LIST))
  {
    return( Stricmp( msg->TreeNode1->tn_Name, msg->TreeNode2->tn_Name ) );
  }
  else if(isFlagSet(msg->TreeNode1->tn_Flags, TNF_LIST))
  {
    return( -1 );
  }
  else if(isFlagSet(msg->TreeNode2->tn_Flags, TNF_LIST))
  {
    return( 1 );
  }
  else
  {
    return( Stricmp( msg->TreeNode1->tn_Name, msg->TreeNode2->tn_Name ) );
  }
}
MakeStaticHook(_CompareHook_LeavesBottom, _CompareFunc_LeavesBottom);

HOOKPROTONHNO(_CompareFunc_LeavesMixed, LONG, struct MUIP_NListtree_CompareMessage *msg)
{
  return( Stricmp( msg->TreeNode1->tn_Name, msg->TreeNode2->tn_Name ) );
}
MakeStaticHook(_CompareHook_LeavesMixed, _CompareFunc_LeavesMixed);

VOID SortList( struct MUI_NListtree_ListNode *ln, struct NListtree_Data *data )
{
  if ( ln->ln_Table.tb_Entries > 1 )
  {
    struct MUI_NListtree_TreeNode **localtable;
    LONG i;

    // let's start the quicksort algorithm to sort our entries.
    qsort2(ln->ln_Table.tb_Table, ln->ln_Table.tb_Entries, data);

    localtable = ln->ln_Table.tb_Table;

    NewList( (struct List *)&ln->ln_List );

    for( i = 0; i < ln->ln_Table.tb_Entries; i++ )
    {
      AddTail( (struct List *)&ln->ln_List, (struct Node *)localtable[i] );
    }
  }
}


struct MUI_NListtree_ListNode *ListNode_Sort( struct MUI_NListtree_ListNode *ln, struct NListtree_Data *data, ULONG flags )
{
  if(isFlagSet(flags, MUIV_NListtree_Sort_Flag_RecursiveAll) || isFlagSet(flags, MUIV_NListtree_Sort_Flag_RecursiveOpen))
  {
    struct MUI_NListtree_ListNode *ln2 = CLN( &ln->ln_List );

    while((ln2 = CLN(GetSucc((struct Node *)ln2))))
    {
      if(isFlagSet(ln->ln_Flags, TNF_LIST))
      {
        if(isFlagSet(ln->ln_Flags, TNF_OPEN) || isFlagSet(flags, MUIV_NListtree_Sort_Flag_RecursiveAll))
          ListNode_Sort( ln2, data, flags );
      }
    }
  }

  if(isFlagSet(ln->ln_Flags, TNF_LIST))
    SortList( ln, data );

  return( ln );
}

HOOKPROTONHNO(_FindNameFunc_CaseSensitive, LONG, struct MUIP_NListtree_FindNameMessage *msg)
{
  return( strcmp( msg->Name, msg->NodeName ) );
}
MakeStaticHook(_FindNameHook_CaseSensitive, _FindNameFunc_CaseSensitive);

HOOKPROTONHNO(_FindNameFunc_CaseInsensitive, LONG, struct MUIP_NListtree_FindNameMessage *msg)
{
  return( Stricmp( msg->Name, msg->NodeName ) );
}
MakeStaticHook(_FindNameHook_CaseInsensitive, _FindNameFunc_CaseInsensitive);

HOOKPROTONHNO(_FindNameFunc_Part, LONG, struct MUIP_NListtree_FindNameMessage *msg)
{
  return( strncmp( msg->Name, msg->NodeName, strlen( msg->Name ) ) );
}
MakeStaticHook(_FindNameHook_Part, _FindNameFunc_Part);

HOOKPROTONHNO(_FindNameFunc_PartCaseInsensitive, LONG, struct MUIP_NListtree_FindNameMessage *msg)
{
  return( Strnicmp( msg->Name, msg->NodeName, strlen( msg->Name ) ) );
}
MakeStaticHook(_FindNameHook_PartCaseInsensitive, _FindNameFunc_PartCaseInsensitive);

HOOKPROTONHNO(_FindNameFunc_PointerCompare, LONG, struct MUIP_NListtree_FindNameMessage *msg)
{
  return( (LONG)( (SIPTR)msg->Name - (SIPTR)msg->NodeName ) );
}
MakeStaticHook(_FindNameHook_PointerCompare, _FindNameFunc_PointerCompare);

HOOKPROTONHNO(_FindUserDataFunc_CaseSensitive, LONG, struct MUIP_NListtree_FindUserDataMessage *msg)
{
  return( strcmp( (STRPTR)msg->User, (STRPTR)msg->UserData ) );
}
MakeStaticHook(_FindUserDataHook_CaseSensitive, _FindUserDataFunc_CaseSensitive);

HOOKPROTONHNO(_FindUserDataFunc_CaseInsensitive, LONG, struct MUIP_NListtree_FindUserDataMessage *msg)
{
  return( Stricmp( (STRPTR)msg->User, (STRPTR)msg->UserData ) );
}
MakeStaticHook(_FindUserDataHook_CaseInsensitive, _FindUserDataFunc_CaseInsensitive);

HOOKPROTONHNO(_FindUserDataFunc_Part, LONG, struct MUIP_NListtree_FindUserDataMessage *msg)
{
  return( strncmp( (STRPTR)msg->User, (STRPTR)msg->UserData, strlen( (STRPTR)msg->User ) ) );
}
MakeStaticHook(_FindUserDataHook_Part, _FindUserDataFunc_Part);

HOOKPROTONHNO(_FindUserDataFunc_PartCaseInsensitive, LONG, struct MUIP_NListtree_FindUserDataMessage *msg)
{
  return( Strnicmp( (STRPTR)msg->User, (STRPTR)msg->UserData, strlen( (STRPTR)msg->User ) ) );
}
MakeStaticHook(_FindUserDataHook_PartCaseInsensitive, _FindUserDataFunc_PartCaseInsensitive);

HOOKPROTONHNO(_FindUserDataFunc_PointerCompare, LONG, struct MUIP_NListtree_FindUserDataMessage *msg)
{
  return( (LONG)( (SIPTR)msg->User - (SIPTR)msg->UserData ) );
}
MakeStaticHook(_FindUserDataHook_PointerCompare, _FindUserDataFunc_PointerCompare);


/*****************************************************************************\
*******************************************************************************
**
**  General internal class related help functions.
**
*******************************************************************************
\*****************************************************************************/

/****** NListtree.mcc/MUIA_NListtree_Active **********************************
*
*   NAME
*
* MUIA_NListtree_Active -- [.SG], struct MUI_NListtree_TreeNode *
*
*
*   SPECIAL VALUES
*
*   MUIV_NListtree_Active_Off
*   MUIV_NListtree_Active_Parent
*   MUIV_NListtree_Active_First
*   MUIV_NListtree_Active_FirstVisible
*   MUIV_NListtree_Active_LastVisible
*
*
*   FUNCTION
*
* Setting this attribute will move the cursor to the defined tree node
* if it is visible. If the node is in an opened tree the listview is
* scrolling into the visible area. Setting MUIV_NListtree_Active_Off will
* vanish the cursor.
*
* MUIV_NListtree_Active_First/FirstVisible/LastVisible are special values
* for activating the lists first or the top/bottom visible entry.
*
* See MUIA_NListtree_AutoVisible for special activation features.
*
* If this attribute is read it returns the active tree node. The result
* is MUIV_NListtree_Active_Off if there is no active entry.
*
*
*   NOTIFICATIONS
*
* You can create a notification on MUIA_NListtree_Active. The
* TriggerValue is the active tree node.
*
*
*   SEE ALSO
*
* MUIA_NListtree_AutoVisible, MUIA_NList_First, MUIA_NList_Visible,
* MUIA_NListtree_ActiveList
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_ActiveList ******************************
*
*   NAME
*
* MUIA_NListtree_ActiveList -- [..G], struct MUI_NListtree_TreeNode *
*
*
*   SPECIAL VALUES
*
*   MUIV_NListtree_ActiveList_Off
*
*
*   FUNCTION
*
* If this attribute is read it returns the active list node. The
* active list node is always the parent of the active entry.
* The result is MUIV_NListtree_ActiveList_Off if there is no
* active list (when there is no active entry).
*
*
*   NOTIFICATIONS
*
* You can create notifications on MUIA_NListtree_ActiveList. The
* TriggerValue is the active list node.
*
*
*   SEE ALSO
*
* MUIA_NListtree_Active
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_AutoVisible *****************************
*
*   NAME
*
* MUIA_NListtree_AutoVisible -- [ISG], struct MUI_NListtree_TreeNode *
*
*
*   SPECIAL VALUES
*
*   MUIV_NListtree_AutoVisible_Off
*   MUIV_NListtree_AutoVisible_Normal
*   MUIV_NListtree_AutoVisible_FirstOpen
*   MUIV_NListtree_AutoVisible_Expand
*
*
*   FUNCTION
*
* Set this to make your list automatically jump to the active
* entry.
*
*   MUIV_NListtree_AutoVisible_Off:
*     The display does NOT scroll the active entry into the
*     visible area.
*
*   MUIV_NListtree_AutoVisible_Normal:
*     This will scroll the active entry into the visible area
*     if it is visible (entry is a member of an open node).
*     This is the default.
*
*   MUIV_NListtree_AutoVisible_FirstOpen:
*     Nodes are not opened, but the first open parent node of
*     the active entry is scrolled into the visible area if the
*     active entry is not visible.
*
*   MUIV_NListtree_AutoVisible_Expand:
*     All parent nodes are opened until the first open node is
*     reached and the active entry will be scrolled into the
*     visible area.
*
*
*   NOTIFICATIONS
*
*
*   SEE ALSO
*
* MUIA_NListtree_Active, MUIA_NList_AutoVisible
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_CloseHook *******************************
*
*   NAME
*
* MUIA_NListtree_CloseHook -- [IS.], struct Hook *
*
*
*   SPECIAL VALUES
*
*
*   FUNCTION
*
* The close hook is called after a list node is closed, then you can
* change the list.
*
* The close hook will be called with the hook in A0, the object in A2
* and a MUIP_NListtree_CloseMessage struct in A1 (see nlisttree_mcc.h).
*
* To remove the hook set this to NULL.
*
*
*   NOTIFICATION
*
*
*   SEE ALSO
*
* MUIA_NListtree_Open, MUIA_NListtree_CloseHook
*
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_CompareHook *****************************
*
*   NAME
*
* MUIA_NListtree_CompareHook -- [IS.], struct Hook *
*
*
*   SPECIAL VALUES
*
*   MUIV_NListtree_CompareHook_Head
*   MUIV_NListtree_CompareHook_Tail
*   MUIV_NListtree_CompareHook_LeavesTop
*   MUIV_NListtree_CompareHook_LeavesMixed
*   MUIV_NListtree_CompareHook_LeavesBottom
*
*
*   FUNCTION
*
* Set this attribute to your own hook if you want to sort the entries in
* the list tree by your own way.
*
* When you sort your list or parts of your list via MUIM_NListtree_Sort,
* using the insert method with MUIV_NListtree_Insert_Sort or dropping an
* entry on a closed node, this compare hook is called.
*
* There are some builtin compare hooks available, called:
*
*   MUIV_NListtree_CompareHook_Head
*     Any entry is inserted at head of the list.
*
*   MUIV_NListtree_CompareHook_Tail
*     Any entry is inserted at tail of the list.
*
*   MUIV_NListtree_CompareHook_LeavesTop
*     Leaves are inserted at top of the list, nodes at bottom. They are
*     alphabetically sorted.
*
*   MUIV_NListtree_CompareHook_LeavesMixed
*     The entries are only alphabetically sorted.
*
*   MUIV_NListtree_CompareHook_LeavesBottom
*     Leaves are inserted at bottom of the list, nodes at top. They are
*     alphabetically sorted. This is default.
*
* The hook will be called with the hook in A0, the object in A2 and
* a MUIP_NListtree_CompareMessage struct in A1 (see nlisttree_mcc.h). You
* should return something like:
*
*   <0  (TreeNode1 <  TreeNode2)
*    0  (TreeNode1 == TreeNode2)
*   >0  (TreeNode1 >  TreeNode2)
*
*
*   NOTIFICATION
*
*
*   SEE ALSO
*
* MUIA_NListtree_Insert, MUIM_DragDrop,
* MUIA_NList_CompareHook
*
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_ConstructHook ***************************
*
*   NAME
*
* MUIA_NListtree_ConstructHook -- [IS.], struct Hook *
*
*
*   SPECIAL VALUES
*
*   MUIV_NListtree_ConstructHook_String
*
*   MUIV_NListtree_ConstructHook_Flag_AutoCreate
*     If using the KeepStructure feature in MUIM_NListtree_Move or
*     MUIM_NListtree_Copy, this flag will be set when calling your
*     construct hook. Then you can react if your hook is not simply
*     allocating memory.
*
*
*   FUNCTION
*
* The construct hook is called whenever you add an entry to your
* listtree. The pointer isn't inserted directly, the construct hook is
* called and its result code is added.
*
* When an entry shall be removed the corresponding destruct hook is
* called.
*
* The construct hook will be called with the hook in A0, the object in
* A2 and a MUIP_NListtree_ConstructMessage struct in A1 (see
* nlisttree_mcc.h).
* The message holds a standard kick 3.x memory pool pointer. If you want,
* you can use the exec or amiga.lib functions for allocating memory
* within this pool, but this is only an option.
*
* If the construct hook returns NULL, nothing will be added to the list.
*
* There is a builtin construct hook available called
* MUIV_NListtree_ConstructHook_String. This expects that the field
* 'tn_User' in the treenode is a string pointer (STRPTR), which's
* string is copied.
* Of course you have to use MUIV_NListtree_DestructHook_String in
* this case!
*
* To remove the hook set this to NULL.
*
* NEVER pass a NULL pointer when you have specified the internal string
* construct/destruct hooks or NListtree will die!
*
*
*   NOTIFICATION
*
*
*   SEE ALSO
*
* MUIA_NList_ConstructHook, MUIA_NListtree_DestructHook,
* MUIA_NListtree_DisplayHook
*
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_DestructHook ****************************
*
*   NAME
*
* MUIA_NListtree_DestructHook -- [IS.], struct Hook *
*
*
*   SPECIAL VALUES
*
*   MUIV_NListtree_DestructHook_String
*
*
*   FUNCTION
*
* Set up a destruct hook for your listtree. The destruct hook is called
* whenevere you remove an entry from the listtree. Here you can free memory
* which was allocated by the construct hook before.
*
* The destruct hook will be called with the hook in A0, the object
* in A2 and a MUIP_NListtree_DestructMessage struct in A1 (see
* nlisttree_mcc.h).
* The message holds a standard kick 3.x memory pool pointer. You must
* use this pool when you have used it inside the construct hook to
* allocate pooled memory.
*
* There is a builtin destruct hook available called
* MUIV_NListtree_DestructHook_String. This expects that the 'User' data
* in the treenode is a string and you have used
* MUIV_NListtree_ConstructHook_String in the construct hook!
*
* To remove the hook set this to NULL.
*
*
*   NOTIFICATION
*
*
*   SEE ALSO
*
* MUIA_NList_ConstructHook, MUIA_NListtree_ConstructHook,
* MUIA_NListtree_DisplayHook
*
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_DisplayHook *****************************
*
*   NAME
*
* MUIA_NListtree_DisplayHook -- [IS.],
*
*
*   SPECIAL VALUES
*
*
*   FUNCTION
*
* You have to supply a display hook to specify what should be shown in
* the listview, otherwise only the name of the nodes is displayed.
*
* The display hook will be called with the hook in A0, the object in
* A2 and a MUIP_NListtree_DisplayMessage struct in A1 (see nlisttree_mcc.h).
*
* The structure holds a pointer to a string array containing as many
* entries as your listtree may have columns. You have to fill this
* array with the strings you want to display. Check out that the array
* pointer of the tree column is set to NULL, if the normal name of the
* node should appear.
* You can set a preparse string in Preparse for the corresponding col
* element. Using it you'll be able to avoid copying the string in a
* buffer to add something in the beginning of the col string.
*
* The display hook also gets the position of the current entry as
* additional parameter. It is stored in the longword preceding the col
* array (don't forget it's a LONG).
*
* You can set the array pointer of the tree column to a string, which is
* diplayed instead of the node name. You can use this to mark nodes.
*
* See MUIA_NList_Format for details about column handling.
*
* To remove the hook and use the internal default display hook set this
* to NULL.
*
*
*   NOTIFICATION
*
*
*   SEE ALSO
*
* MUIA_NList_Format, MUIA_Text_Contents
*
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_CopyToClipHook **************************
*
*   NAME
*
* MUIA_NListtree_CopyToClipHook -- [IS.],
*
*
*   SPECIAL VALUES
*
*   MUIV_NListtree_CopyToClipHook_Default
*
*
*   FUNCTION
*
*   This thing works near like MUIA_NListtree_DisplayHook, but is
*   called when the NListtree object want to make a clipboard copy.
*
*   You can return only one string pointer. If you return NULL,
*   nothing will be copied. If you return -1, the entry will be
*   handled as a normal string and the name field is used.
*
*   The builtin hook skips all ESC sequences and adds a tab char
*   between columns.
*
*
*   NOTIFICATION
*
*
*   SEE ALSO
*
* MUIM_NListtree_CopyToClip
*
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_DoubleClick *****************************
*
*   NAME
*
* MUIA_NListtree_DoubleClick -- [ISG], ULONG
*
*
*   SPECIAL VALUES
*
*   MUIV_NListtree_DoubleClick_Off
*   MUIV_NListtree_DoubleClick_All
*   MUIV_NListtree_DoubleClick_Tree
*   MUIV_NListtree_DoubleClick_NoTrigger
*
*
*   FUNCTION
*
* A doubleclick opens a node if it was closed, it is closed if the node
* was open. You have to set the column which should do this.
*
* Normally only the column number is set here, but there are special
* values:
*
*   MUIV_NListtree_DoubleClick_Off:
*     A doubleclick is not handled.
*
*   MUIV_NListtree_DoubleClick_All:
*     All columns react on doubleclick.
*
*   MUIV_NListtree_DoubleClick_Tree
*     Only a doubleclick on the defined tree column is recognized.
*
*   MUIV_NListtree_DoubleClick_NoTrigger:
*     A doubleclick is not handled and not triggered!
*
*
*   NOTIFICATION
*
* The TriggerValue of the notification is the tree node you have double-
* clicked, you can GetAttr() MUIA_NListtree_DoubleClick for the column
* number. The struct 'MUI_NListtree_TreeNode *' is used for trigger.
*
* The notification is done on leaves and on node columns, which are not
* set in MUIA_NListtree_DoubleClick.
*
*   SEE ALSO
*
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_DragDropSort ****************************
*
*   NAME
*
* MUIA_NListtree_DragDropSort -- [IS.], BOOL
*
*
*   SPECIAL VALUES
*
*
*   FUNCTION
*
* Setting this attribute to FALSE will disable the ability to sort the
* list tree by drag & drop. Defaults to TRUE.
*
*
*   NOTIFICATION
*
*
*   SEE ALSO
*
*
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_DupNodeName *****************************
*
*   NAME
*
* MUIA_NListtree_DupNodeName -- [IS.], BOOL
*
*
*   SPECIAL VALUES
*
*
*   FUNCTION
*
* If this attribute is set to FALSE the names of the node will not be
* duplicated, only the string pointers are used. Be careful the strings
* have to be valid everytime.
*
*
*   NOTIFICATION
*
*
*   SEE ALSO
*
*
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_EmptyNodes ******************************
*
*   NAME
*
* MUIA_NListtree_EmptyNodes -- [IS.], BOOL
*
*
*   SPECIAL VALUES
*
*
*   FUNCTION
*
* Setting this attribute to TRUE will display all empty nodes as leaves,
* this means no list indicator is shown. Nevertheless the entry is
* handled like a node.
*
*
*   NOTIFICATION
*
*
*   SEE ALSO
*
*
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_FindNameHook ****************************
*
*   NAME
*
* MUIA_NListtree_FindNameHook -- [IS.],
*
*
*   SPECIAL VALUES
*
*   MUIV_NListtree_FindNameHook_CaseSensitive
*     Search for the complete string, case sensitive.
*
*   MUIV_NListtree_FindNameHook_CaseInsensitive
*     Search for the complete string, case insensitive.
*
*   MUIV_NListtree_FindNameHook_Part
*     Search for the first part of the string, case sensitive.
*
*   MUIV_NListtree_FindNameHook_PartCaseInsensitive
*     Search for the first part of the string, case insensitive.
*
*   MUIV_NListtree_FindNameHook_PointerCompare
*     Do only a pointer comparision. Note, that this is in fact
*     a pointer subtraction to fit into the rules. It returns
*     the difference of the two fields if no match.
*
*   FUNCTION
*
* You can install a FindName hook to specify your own search
* criteria.
*
* The find name hook will be called with the hook in A0, the object in
* A2 and a MUIP_NListtree_FindNameMessage struct in A1
* (see nlisttree_mcc.h). It should return (ret != 0) for entries which
* are not matching the pattern and a value of 0 if a match.
*
* The find name message structure holds a pointer to a string
* containing the name to search for and pointers to the name- and user-
* field of the node which is currently processed.
*
* The MUIV_NListtree_FindNameHook_CaseSensitive will be used as default.
*
*   NOTIFICATION
*
*
*   SEE ALSO
*
* MUIM_NListtree_FindName, MUIM_NListtree_FindUserData,
* MUIA_NListtree_FindUserDataHook
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_FindUserDataHook ************************
*
*   NAME
*
* MUIA_NListtree_FindUserDataHook -- [IS.],
*
*
*   SPECIAL VALUES
*
*   MUIV_NListtree_FindUserDataHook_CaseSensitive
*     Search for the complete string, case sensitive.
*
*   MUIV_NListtree_FindUserDataHook_CaseInsensitive
*     Search for the complete string, case insensitive.
*
*   MUIV_NListtree_FindUserDataHook_Part
*     Search for the first part of the string, case sensitive.
*
*   MUIV_NListtree_FindUserDataHook_PartCaseInsensitive
*     Search for the first part of the string, case insensitive.
*
*   MUIV_NListtree_FindUserDataHook_PointerCompare
*     Do only a pointer comparision. Note, that this is in fact
*     a pointer subtraction to fit into the rules. It returns
*     the difference of the two user fields if no match.
*
*
*   FUNCTION
*
* You can install a FindUserData hook to specify your own search
* criteria.
*
* The find user data hook will be called with the hook in A0, the object
* in A2 and a MUIP_NListtree_FindUserDataMessage struct in A1
* (see nlisttree_mcc.h). It should return (ret != 0) for entries which
* are not matching the pattern and a value of 0 if a match.
*
* The find user data message structure holds a pointer to a string
* containing the data to search for and pointers to the user- and name-
* field of the node which is currently processed.
*
* MUIV_NListtree_FindUserDataHook_CaseSensitive will be used as
* default.
*
*   NOTIFICATION
*
*
*   SEE ALSO
*
* MUIM_NListtree_FindName, MUIM_NListtree_FindUserData,
* MUIA_NListtree_FindNameHook
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_Format **********************************
*
*   NAME
*
* MUIA_NListtree_Format -- [IS.], STRPTR
*
*
*   SPECIAL VALUES
*
*
*   FUNCTION
*
* Same as MUIA_NList_Format, but one column is reserved for the tree
* indicators and the names of the nodes.
*
* For further detailed information see MUIA_NList_Format!
*
*
*   NOTIFICATION
*
*
*   SEE ALSO
*
* MUIA_NList_Format, MUIA_NListtree_DisplayHook,
* MUIA_Text_Contents
*
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_MultiSelect *****************************
*
*   NAME
*
* MUIA_NListtree_MultiSelect -- [I..],
*
*
*   SPECIAL VALUES
*
*   MUIV_NListtree_MultiSelect_None
*   MUIV_NListtree_MultiSelect_Default
*   MUIV_NListtree_MultiSelect_Shifted
*   MUIV_NListtree_MultiSelect_Always
*
*
*   FUNCTION
*
* Four possibilities exist for a listviews multi select
* capabilities:
*
*   MUIV_NListtree_MultiSelect_None:
*     The list tree cannot multiselect at all.
*
*   MUIV_NListtree_MultiSelect_Default:
*     The multi select type (with or without shift)
*     depends on the users preferences setting.
*
*   MUIV_NListtree_MultiSelect_Shifted:
*     Overrides the users prefs, multi selecting only
*     together with shift key.
*
*   MUIV_NListtree_MultiSelect_Always:
*     Overrides the users prefs, multi selecting
*     without shift key.
*
*
*   NOTIFICATION
*
* NOTES
*
*   SEE ALSO
*
* MUIA_NListtree_MultiTestHook, MUIM_NListtree_MultiSelect
*
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_MultiTestHook ***************************
*
*   NAME
*
* MUIA_NListtree_MultiTestHook -- [IS.], struct Hook *
*
*
*   SPECIAL VALUES
*
*
*   FUNCTION
*
* If you plan to have a multi selecting list tree but not
* all of your entries are actually multi selectable, you
* can supply a MUIA_NListtree_MultiTestHook.
*
* The multi test hook will be called with the hook in A0, the object
* in A2 and a MUIP_NListtree_MultiTestMessage struct in A1 (see
* nlisttree_mcc.h) and should return TRUE if the entry is multi
* selectable, FALSE otherwise.
*
* To remove the hook set this to NULL.
*
*
*   NOTIFICATION
*
*
*   SEE ALSO
*
* MUIA_NListtree_ConstructHook, MUIA_NListtree_DestructHook
*
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_OpenHook ********************************
*
*   NAME
*
* MUIA_NListtree_OpenHook -- [IS.], struct Hook *
*
*
*   SPECIAL VALUES
*
*
*   FUNCTION
*
* The open hook is called whenever a list node will be opened, so you
* can change the list before the node is open.
*
* The open hook will be called with the hook in A0, the object in A2
* and a MUIP_NListtree_OpenMessage struct in A1 (see nlisttree_mcc.h).
*
* To remove the hook set this to NULL.
*
*
*   NOTIFICATION
*
*
*   SEE ALSO
*
* MUIA_NListtree_Open, MUIA_NListtree_CloseHook
*
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_Quiet ***********************************
*
*   NAME
*
* MUIA_NListtree_Quiet -- [.S.], QUIET
*
*
*   SPECIAL VALUES
*
*
*   FUNCTION
*
* If you add/remove lots of entries to/from a listtree, this will cause
* lots of screen action and slow down the operation. Setting
* MUIA_NListtree_Quiet to TRUE will temporarily prevent the listtree from
* being refreshed, this refresh will take place only once when you set
* it back to FALSE again.
*
* MUIA_NListtree_Quiet holds a nesting count to avoid trouble with
* multiple setting/unsetting this attribute. You are encoraged to
* always use TRUE/FALSE pairs here or you will went in trouble.
*
* DO NOT USE MUIA_NList_Quiet here!
*
*
*   NOTIFICATION
*
*
*   SEE ALSO
*
* MUIM_NListtree_Insert, MUIM_NListtree_Remove
*
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_Title ***********************************
*
*   NAME
*
* MUIA_NListtree_Title -- [IS.], BOOL
*
*
*   SPECIAL VALUES
*
*
*   FUNCTION
*
* Specify a title for the current listtree.
*
* For detailed information see MUIA_NList_Title!
*
*
*   NOTIFICATION
*
*
* BUGS
*
* The title should not be a string as for single column listviews. This
* attribute can only be set to TRUE or FALSE.
*
*
*   SEE ALSO
*
*
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_TreeColumn ******************************
*
*   NAME
*
* MUIA_NListtree_TreeColumn -- [ISG], ULONG
*
*
*   SPECIAL VALUES
*
*   FUNCTION
*
* Specify the column of the list tree, the node indicator and the name
* of the node are displayed in.
*
*
*   NOTIFICATION
*
*
*   SEE ALSO
*
* MUIA_NListtree_DisplayHook, MUIA_NListtree_Format
*
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_ShowTree ********************************
*
*   NAME
*
* MUIA_NListtree_ShowTree -- [ISG], ULONG
*
*
*   SPECIAL VALUES
*
*   FUNCTION
*
* Specify FALSE here if you want the whole tree to be disappear.
* Defaults to TRUE;
*
*   NOTIFICATION
*
*
*   SEE ALSO
*
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_DropType ********************************
*
*   NAME
*
* MUIA_NListtree_DropType -- [..G], ULONG
*
*
*   SPECIAL VALUES
*
*   MUIV_NListtree_DropType_None
*   MUIV_NListtree_DropType_Above
*   MUIV_NListtree_DropType_Below
*   MUIV_NListtree_DropType_Onto
*
*
*   FUNCTION
*
* After a successfull drop operation, this value holds the position
* relative to the value of MUIA_NListtree_DropTarget/DropTargetPos.
*
*
*   NOTIFICATION
*
*
*   SEE ALSO
*
* MUIA_NListtree_DropTarget, MUIA_NListtree_DropTargetPos
*
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_DropTarget ******************************
*
*   NAME
*
* MUIA_NListtree_DropTarget -- [..G], ULONG
*
*
*   SPECIAL VALUES
*
*   FUNCTION
*
* After a successfull drop operation, this value holds the entry where
* the entry was dropped. The relative position (above etc.) can be
* obtained by reading the attribute MUIA_NListtree_DropType.
*
*
*   NOTIFICATION
*
*
*   SEE ALSO
*
* MUIA_NListtree_DropTargetPos, MUIA_NListtree_DropType
*
*
******************************************************************************
*
*/

/****** NListtree.mcc/MUIA_NListtree_DropTargetPos ***************************
*
*   NAME
*
* MUIA_NListtree_DropTargetPos -- [..G], ULONG
*
*
*   SPECIAL VALUES
*
*   FUNCTION
*
* After a successfull drop operation, this value holds the integer
* position of the entry where the dragged entry was dropped. The
* entry itself can be obtained by reading MUIA_NListtree_DropTarget,
* the relative position (above etc.) can be obtained by reading the
* attribute MUIA_NListtree_DropType.
*
*
*   NOTIFICATION
*
*
*   SEE ALSO
*
* MUIA_NListtree_DropTarget, MUIA_NListtree_DropType
*
*
******************************************************************************
*
*/

static VOID SetAttributes( struct NListtree_Data *data, struct opSet *msg, BOOL initial )
{
  struct MUI_NListtree_ListNode *actlist;
  struct MUI_NListtree_TreeNode *actnode;
  struct TagItem *tags, *tag;
  struct Hook *orgHook = NULL;
  BOOL intfunc, onlytrigger = FALSE;

  for(tags = msg->ops_AttrList; (tag = NextTagItem((APTR)&tags)); )
  {
    intfunc = TRUE;

    switch( tag->ti_Tag )
    {
      /*
      **  Configuration part.
      */
      case MUIA_NListtree_IsMCP:
        SET_FLAG(data->Flags, NLTF_ISMCP);
      break;

      case MUICFG_NListtree_ImageSpecClosed:
      {
        DisposeImage( data, IMAGE_Closed );
        SetupImage( data, (struct MUI_ImageSpec *)tag->ti_Data, IMAGE_Closed );
        DoRefresh( data );

        D(DBF_GETSET, "SET MUICFG_NListtree_ImageSpecClosed: '%s'", (STRPTR)tag->ti_Data);
      }
      break;

      case MUICFG_NListtree_ImageSpecOpen:
      {
        DisposeImage( data, IMAGE_Open );
        SetupImage( data, (struct MUI_ImageSpec *)tag->ti_Data, IMAGE_Open );
        DoRefresh( data );

        D(DBF_GETSET, "SET MUICFG_NListtree_ImageSpecOpen: '%s'", (STRPTR)tag->ti_Data);
      }
      break;

      case MUICFG_NListtree_ImageSpecFolder:
      {
        DisposeImage( data, IMAGE_Folder );
        SetupImage( data, (struct MUI_ImageSpec *)tag->ti_Data, IMAGE_Folder );
        DoRefresh( data );

        D(DBF_GETSET, "SET MUICFG_NListtree_ImageSpecFolder: '%s'", (STRPTR)tag->ti_Data);
      }
      break;

      case MUICFG_NListtree_PenSpecLines:
      {
        if( data->MRI )
        {
          ObtPen( data->MRI, &data->Pen[PEN_Line], (struct MUI_PenSpec *)tag->ti_Data );
          DoRefresh( data );

          D(DBF_GETSET, "SET MUICFG_NListtree_PenSpecLines: %s", (STRPTR)tag->ti_Data);
        }
      }
      break;

      case MUICFG_NListtree_PenSpecShadow:
      {
        if( data->MRI )
        {
          ObtPen( data->MRI, &data->Pen[PEN_Shadow], (struct MUI_PenSpec *)tag->ti_Data );
          DoRefresh( data );

          D(DBF_GETSET, "SET MUICFG_NListtree_PenSpecShadow: %s", (STRPTR)tag->ti_Data);
        }
      }
      break;

      case MUICFG_NListtree_PenSpecGlow:
      {
        if( data->MRI )
        {
          ObtPen( data->MRI, &data->Pen[PEN_Glow], (struct MUI_PenSpec *)tag->ti_Data );
          DoRefresh( data );

          D(DBF_GETSET, "SET MUICFG_NListtree_PenSpecGlow: %s", (STRPTR)tag->ti_Data);
        }
      }
      break;

      case MUICFG_NListtree_IndentWidth:
      {
        data->IndentWidth = (BYTE)tag->ti_Data;
        DoRefresh( data );

        D(DBF_GETSET, "SET MUICFG_NListtree_IndentWidth: %ld", tag->ti_Data);
      }
      break;

      case MUICFG_NListtree_LineType:
      {
        data->LineType = (ULONG)tag->ti_Data;
        DoRefresh( data );

        D(DBF_GETSET, "SET MUICFG_NListtree_LineType: %ld", tag->ti_Data);
      }
      break;

      case MUICFG_NListtree_UseFolderImage:
      {
        data->UseFolderImage = (BOOL)tag->ti_Data;
        DoRefresh(data);

        D(DBF_GETSET, "SET MUICFG_NListtree_UseFolderImage: %ld", tag->ti_Data);
      }
      break;

      case MUICFG_NListtree_OpenAutoScroll:
      {
        if ( (BOOL)tag->ti_Data )
          SET_FLAG(data->Flags, NLTF_OPENAUTOSCROLL);
        else
          CLEAR_FLAG(data->Flags, NLTF_OPENAUTOSCROLL);

        D(DBF_GETSET, "SET MUICFG_NListtree_OpenAutoScroll: %ld", tag->ti_Data);
      }
      break;

      /*
      **  General part.
      */
      case MUIA_NListtree_OnlyTrigger:
      {
        onlytrigger = TRUE;
        D(DBF_GETSET, "SET MUIA_NListtree_OnlyTrigger");
      }
      break;

      case MUIA_NList_Active:
      {
        D(DBF_GETSET, "SET MUIA_NList_Active  %ld%s",tag->ti_Data,isFlagSet(data->Flags, NLTF_ACTIVENOTIFY)?" notify activation set":"no notify set");
      }
      break;

      case MUIA_NListtree_Active:
      {
        D(DBF_GETSET, "SET MUIA_NListtree_Active");
        if((tag->ti_Data == (ULONG)MUIV_NListtree_Active_Off) ||
           (tag->ti_Data == (IPTR)&data->RootList) ||
           ((tag->ti_Data == (ULONG)MUIV_NListtree_Active_Parent) &&
            (data->ActiveNode == (ULONG)MUIV_NListtree_Active_Off))
          )
        {
          actnode = MUIV_NListtree_Active_Off;
          actlist = &data->RootList;
        }
        else
        {
          struct MUI_NListtree_TreeNode *tn;

          /*
          **  Parent of the active node.
          */
          if(tag->ti_Data == (ULONG)MUIV_NListtree_Active_Parent)
          {
            tag->ti_Data = (IPTR)GetParent( data->ActiveNode );
          }

          /*
          **  First list entry (visible or not).
          */
          if(tag->ti_Data == (ULONG)MUIV_NListtree_Active_First)
            tag->ti_Data = (IPTR)GetHead( (struct List *)&data->RootList.ln_List );

          /*
          **  First visible entry.
          */
          if(tag->ti_Data == (ULONG)MUIV_NListtree_Active_FirstVisible)
          {
            LONG firstvisible;

            firstvisible = xget( data->Obj, MUIA_NList_First );
            DoMethod( data->Obj, MUIM_NList_GetEntry, firstvisible, &tn );

            tag->ti_Data = (IPTR)tn;
          }

          /*
          **  Last visible entry.
          */
          if(tag->ti_Data == (ULONG)MUIV_NListtree_Active_LastVisible)
          {
            LONG lastvisible;

            lastvisible = xget( data->Obj, MUIA_NList_First ) + xget( data->Obj, MUIA_NList_Visible ) - 1;
            DoMethod( data->Obj, MUIM_NList_GetEntry, lastvisible, &tn );

            tag->ti_Data = (IPTR)tn;
          }

          if ( tag->ti_Data != (IPTR)&data->RootList )
          {
            actnode = CTN( tag->ti_Data );
            actlist = CLN( GetParent( (CTN( tag->ti_Data) ) ) );
          } else
          {
            /* Do nothing */
            return;
          }
        }

        /*
        **  With actnode and actlist we avoid ActiveNode ping-pong...
        */
        if(!initial && (actnode != data->ActiveNode || isFlagSet(data->Flags, NLTF_SETACTIVE)))
        {
          data->ActiveNode  = actnode;
          data->ActiveNodeNum = xget( data->Obj, MUIA_NList_Active );
        }

        if (!initial)
        {
          /*
          **  For notification only.
          */
          if (data->ActiveList != actlist)
          {
            data->ActiveList = actlist;
            MakeNotify( data, MUIA_NListtree_ActiveList, actlist );
            }
        }


        if ( !onlytrigger )
        {
          ActivateTreeNode( data, actnode );
          DoMethod( data->Obj, MUIM_NListtree_Active, data->ActiveNodeNum, data->ActiveNode );

          D(DBF_GETSET, "SET MUIA_NListtree_Active: %s - 0x%08lx - list: 0x%08lx  rootlist: 0x%lx", actnode ? actnode->tn_Name : (STRPTR)"NULL", actnode, data->ActiveList, &data->RootList);
        }
        else
        {
          D(DBF_GETSET, "SET MUIA_NListtree_Active: TRIGGER: %s - 0x%08lx", actnode ? actnode->tn_Name : (STRPTR)"NULL", actnode);
        }
      }
      break;

      /*
      **  Dummy for notification.
      */
      case MUIA_NListtree_ActiveList:
        D(DBF_GETSET, "SET MUIA_NListtree_ActiveList (dummy)");
      break;

      case MUIA_NListtree_AutoVisible:
      {
        data->AutoVisible = (IPTR)tag->ti_Data;

        D(DBF_GETSET, "SET MUIA_NListtree_AutoVisible: %ld", tag->ti_Data);
      }
      break;

      case MUIA_NListtree_CloseHook:
      {
        data->CloseHook       = (struct Hook *)tag->ti_Data;

        D(DBF_GETSET, "SET MUIA_NListtree_CloseHook: 0x%08lx", tag->ti_Data);
      }
      break;

      case MUIA_NListtree_CompareHook:
      {
        switch( tag->ti_Data )
        {
          case MUIV_NListtree_CompareHook_Head:
          {
            orgHook = &_CompareHook_Head;
            D(DBF_GETSET, "SET MUIA_NListtree_CompareHook: MUIV_NListtree_CompareHook_Head");
          }
          break;

          case MUIV_NListtree_CompareHook_Tail:
          {
            orgHook = &_CompareHook_Tail;
            D(DBF_GETSET, "SET MUIA_NListtree_CompareHook: MUIV_NListtree_CompareHook_Tail");
          }
          break;

          case MUIV_NListtree_CompareHook_LeavesTop:
          {
            orgHook = &_CompareHook_LeavesTop;
            D(DBF_GETSET, "SET MUIA_NListtree_CompareHook: MUIV_NListtree_CompareHook_LeavesTop");
          }
          break;

          case MUIV_NListtree_CompareHook_LeavesMixed:
          {
            orgHook = &_CompareHook_LeavesMixed;
            D(DBF_GETSET, "SET MUIA_NListtree_CompareHook: MUIV_NListtree_CompareHook_LeavesMixed");
          }
          break;

          case MUIV_NListtree_CompareHook_LeavesBottom:
          {
            orgHook = &_CompareHook_LeavesBottom;
            D(DBF_GETSET, "SET MUIA_NListtree_CompareHook: MUIV_NListtree_CompareHook_LeavesBottom");
          }
          break;

          default:
          {
            intfunc = FALSE;

            data->CompareHook = (struct Hook*)tag->ti_Data;

            D(DBF_GETSET, "SET MUIA_NListtree_CompareHook: 0x%08lx", tag->ti_Data);
          }
          break;
        }

        if(intfunc)
        {
          /* The flags is quiet sensless because nobody asks for it expect in OM_NEW (which should be rewritten anywhy) */
          SET_FLAG(data->Flags, NLTF_INT_COMPAREHOOK);
          InitHook(&data->IntCompareHook, *orgHook, data);
          data->CompareHook = &data->IntCompareHook;
        }
        else
          CLEAR_FLAG(data->Flags, NLTF_INT_COMPAREHOOK);
      }
      break;

      case MUIA_NListtree_ConstructHook:
      {
        /*
        **  If old hook is internal, remove complete
        **  hook with all memory allocated.
        */
        if ( data->ConstructHook )
        {
          if ( data->ConstructHook->h_Entry == (HOOKFUNC)_ConstructFunc )
          {
            FreeVecPooled( data->MemoryPool, data->ConstructHook );
            data->ConstructHook = NULL;
          }
        }

        if(tag->ti_Data == (ULONG)MUIV_NListtree_ConstructHook_String)
        {
          D(DBF_GETSET, "SET MUIA_NListtree_ConstructHook: MUIV_NListtree_ConstructHook_String");

          if((data->ConstructHook = AllocVecPooled(data->MemoryPool, sizeof(struct Hook))))
          {
            SET_FLAG(data->Flags, NLTF_INT_CONSTRDESTRHOOK);

            InitHook(data->ConstructHook, _ConstructHook, data);
          }
          else data->Error = TRUE;
        }
        else
        {
          data->ConstructHook = (struct Hook *)tag->ti_Data;

          D(DBF_GETSET, "SET MUIA_NListtree_ConstructHook: 0x%08lx", tag->ti_Data);
        }
      }
      break;

      case MUIA_NListtree_DestructHook:
      {
        /*
        **  If old hook is internal, remove complete
        **  hook with all memory allocated.
        */
        if ( data->DestructHook )
        {
          if ( data->DestructHook->h_Entry == (HOOKFUNC)_DestructFunc )
          {
            FreeVecPooled( data->MemoryPool, data->DestructHook );
            data->DestructHook = NULL;
          }
        }

        if(tag->ti_Data == (ULONG)MUIV_NListtree_DestructHook_String)
        {
          D(DBF_GETSET, "SET MUIA_NListtree_DestructHook: MUIV_NListtree_DestructHook_String");

          if((data->DestructHook = AllocVecPooled(data->MemoryPool, sizeof(struct Hook))))
          {
            SET_FLAG(data->Flags, NLTF_INT_CONSTRDESTRHOOK);

            InitHook(data->DestructHook, _DestructHook, data);
          }
          else data->Error = TRUE;
        }
        else
        {
          data->DestructHook      = (struct Hook *)tag->ti_Data;

          D(DBF_GETSET, "SET MUIA_NListtree_DestructHook: 0x%08lx", tag->ti_Data);
        }
      }
      break;

      case MUIA_NListtree_DisplayHook:
      {
        if(tag->ti_Data != (ULONG)MUIV_NListtree_DisplayHook_Default)
        {
          data->DisplayHook     = (struct Hook *)tag->ti_Data;
        }
        else
          data->DisplayHook = NULL;

        D(DBF_GETSET, "SET MUIA_NListtree_DisplayHook: 0x%08lx", tag->ti_Data);
      }
      break;

      case MUIA_NListtree_CopyToClipHook:
      {
        if ( tag->ti_Data == MUIV_NListtree_CopyToClipHook_Default )
        {
          D(DBF_GETSET, "SET MUIA_NListtree_CopyToClipHook: MUIV_NListtree_CopyToClipHook_Default");

          data->CopyToClipHook = NULL;
        }
        else
        {
          data->CopyToClipHook      = (struct Hook *)tag->ti_Data;

          D(DBF_GETSET, "SET MUIA_NListtree_CopyToClipHook: 0x%08lx", tag->ti_Data);
        }
      }
      break;

      case MUIA_NListtree_DoubleClick:
      {
        if ( !onlytrigger )
        {
          data->DoubleClick = (BYTE)tag->ti_Data;
          D(DBF_GETSET, "SET MUIA_NListtree_DoubleClick: 0x%08lx", tag->ti_Data);
        }
      }
      break;

      case MUIA_NListtree_DragDropSort:
      {
        if ( (BOOL)tag->ti_Data )
          SET_FLAG(data->Flags, NLTF_DRAGDROPSORT);
        else
          CLEAR_FLAG(data->Flags, NLTF_DRAGDROPSORT);

        D(DBF_GETSET, "SET MUIA_NListtree_DragDropSort: %ld", tag->ti_Data);
      }
      break;

      case MUIA_NListtree_DupNodeName:
      {
        if ( (BOOL)tag->ti_Data )
          SET_FLAG(data->Flags, NLTF_DUPNODENAMES);
        else
          CLEAR_FLAG(data->Flags, NLTF_DUPNODENAMES);

        D(DBF_GETSET, "SET MUIA_NListtree_DupNodeName: %ld", tag->ti_Data);
      }
      break;

      case MUIA_NListtree_EmptyNodes:
      {
        if ( (BOOL)tag->ti_Data )
          SET_FLAG(data->Flags, NLTF_EMPTYNODES);
        else
          CLEAR_FLAG(data->Flags, NLTF_EMPTYNODES);

        if ( !initial )
          DoRefresh( data );

        D(DBF_GETSET, "SET MUIA_NListtree_EmptyNodes: %ld", tag->ti_Data);
      }
      break;

      case MUIA_NListtree_FindNameHook:
      {
        /*
        **  If old hook is internal, remove complete
        **  hook with all memory allocated.
        */
        if ( data->FindNameHook )
        {
          FreeVecPooled( data->MemoryPool, data->FindNameHook );
          data->FindNameHook = NULL;
        }

        data->FindNameHook = AllocVecPooled( data->MemoryPool, sizeof( struct Hook ) );

        switch( tag->ti_Data )
        {
          case MUIV_NListtree_FindNameHook_CaseSensitive:
          {
            orgHook = &_FindNameHook_CaseSensitive;
            D(DBF_GETSET, "SET MUIA_NListtree_FindNameHook: MUIV_NListtree_FindNameHook_CaseSensitive");
          }
          break;

          case MUIV_NListtree_FindNameHook_CaseInsensitive:
          {
            orgHook = &_FindNameHook_CaseInsensitive;
            D(DBF_GETSET, "SET MUIA_NListtree_FindNameHook: MUIV_NListtree_FindNameHook_CaseInsensitive");
          }
          break;

          case MUIV_NListtree_FindNameHook_Part:
          {
            orgHook = &_FindNameHook_Part;
            D(DBF_GETSET, "SET MUIA_NListtree_FindNameHook: MUIV_NListtree_FindNameHook_Part");
          }
          break;

          case MUIV_NListtree_FindNameHook_PartCaseInsensitive:
          {
            orgHook = &_FindNameHook_PartCaseInsensitive;
            D(DBF_GETSET, "SET MUIA_NListtree_FindNameHook: MUIV_NListtree_FindNameHook_PartCaseInsensitive");
          }
          break;

          case MUIV_NListtree_FindNameHook_PointerCompare:
          {
            orgHook = &_FindNameHook_PointerCompare;
            D(DBF_GETSET, "SET MUIA_NListtree_FindNameHook: MUIV_NListtree_FindNameHook_PointerCompare");
          }
          break;

          default:
          {
            intfunc = FALSE;

            CopyMem( (APTR)tag->ti_Data, data->FindNameHook, sizeof( struct Hook ) );

            D(DBF_GETSET, "SET MUIA_NListtree_FindNameHook: 0x%08lx", tag->ti_Data);
          }
          break;
        }

        if(intfunc)
        {
          InitHook(data->FindNameHook, *orgHook, data);
        }
      }
      break;

      case MUIA_NListtree_FindUserDataHook:
      {
        /*
        **  If old hook is internal, remove complete
        **  hook with all memory allocated.
        */
        if ( data->FindUserDataHook )
        {
          FreeVecPooled( data->MemoryPool, data->FindUserDataHook );
          data->FindUserDataHook = NULL;
        }

        data->FindUserDataHook = AllocVecPooled( data->MemoryPool, sizeof( struct Hook ) );

        switch( tag->ti_Data )
        {
          case MUIV_NListtree_FindUserDataHook_CaseSensitive:
          {
            orgHook = &_FindUserDataHook_CaseSensitive;
            D(DBF_GETSET, "SET MUIA_NListtree_FindUserDataHook: MUIV_NListtree_FindUserDataHook_CaseSensitive");
          }
          break;

          case MUIV_NListtree_FindUserDataHook_CaseInsensitive:
          {
            orgHook = &_FindUserDataHook_CaseInsensitive;
            D(DBF_GETSET, "SET MUIA_NListtree_FindUserDataHook: MUIV_NListtree_FindUserDataHook_CaseInsensitive");
          }
          break;

          case MUIV_NListtree_FindUserDataHook_Part:
          {
            orgHook = &_FindUserDataHook_Part;
            D(DBF_GETSET, "SET MUIA_NListtree_FindUserDataHook: MUIV_NListtree_FindUserDataHook_Part");
          }
          break;

          case MUIV_NListtree_FindUserDataHook_PartCaseInsensitive:
          {
            orgHook = &_FindUserDataHook_PartCaseInsensitive;
            D(DBF_GETSET, "SET MUIA_NListtree_FindUserDataHook: MUIV_NListtree_FindUserDataHook_PartCaseInsensitive");
          }
          break;

          case MUIV_NListtree_FindUserDataHook_PointerCompare:
          {
            orgHook = &_FindUserDataHook_PointerCompare;
            D(DBF_GETSET, "SET MUIA_NListtree_FindUserDataHook: MUIV_NListtree_FindUserDataHook_PointerCompare");
          }
          break;

          default:
          {
            intfunc = FALSE;

            CopyMem( (APTR)tag->ti_Data, data->FindUserDataHook, sizeof( struct Hook ) );

            D(DBF_GETSET, "SET MUIA_NListtree_FindUserDataHook: 0x%08lx", tag->ti_Data);
          }
          break;
        }

        if(intfunc)
        {
          InitHook(data->FindUserDataHook, *orgHook, data);
        }
      }
      break;

      case MUIA_NListtree_Format:
      {
        int len;

        /*
        **  If old data there, remove it.
        */
        if ( data->Format )
        {
          FreeVecPooled( data->MemoryPool, data->Format );
          data->Format = NULL;
        }

        /*
        **  Save raw list format.
        */
        len = strlen((STRPTR)tag->ti_Data) + 1;

        if((data->Format = AllocVecPooled( data->MemoryPool, len)))
        {
          strlcpy( data->Format, (STRPTR)tag->ti_Data, len );

          if ( !initial )
            nnset( data->Obj, MUIA_NList_Format, data->Format );

          D(DBF_GETSET, "SET MUIA_NListtree_Format: %s", (STRPTR)tag->ti_Data);
        }
      }
      break;

      case MUIA_NListtree_MultiSelect:
      {
        data->MultiSelect = (UBYTE)( tag->ti_Data & 0x00ff );

        /*
        if ( tag->ti_Data & MUIV_NListtree_MultiSelect_Flag_AutoSelectChilds )
          SET_FLAG(data->Flags, NLTF_AUTOSELECT_CHILDS);
        */

        D(DBF_GETSET, "SET MUIA_NListtree_MultiSelect: 0x%08lx", tag->ti_Data);
      }
      break;

      case MUIA_NListtree_MultiTestHook:
      {
        data->MultiTestHook     = (struct Hook *)tag->ti_Data;

        D(DBF_GETSET, "SET MUIA_NListtree_MultiTestHook: 0x%08lx", tag->ti_Data);
      }
      break;

      case MUIA_NListtree_OpenHook:
      {
        data->OpenHook        = (struct Hook *)tag->ti_Data;

        D(DBF_GETSET, "SET MUIA_NListtree_OpenHook: 0x%08lx", tag->ti_Data);
      }
      break;

      case MUIA_NListtree_Quiet:
      {
        if ( !DoQuiet( data, (BOOL)tag->ti_Data ) )
        {
          if(isFlagSet(data->Flags, NLTF_REFRESH))
          {
            if(isFlagSet(data->Flags, NLTF_ISMCP))
              DoRefresh( data );

            CLEAR_FLAG(data->Flags, NLTF_REFRESH);
          }
        }

        D(DBF_GETSET, "SET MUIA_NListtree_Quiet: %ld - Counter = %ld", tag->ti_Data, data->QuietCounter);
      }
      break;

      case MUICFG_NListtree_RememberStatus:
      {
        if ( (BOOL)tag->ti_Data )
          SET_FLAG(data->Flags, NLTF_REMEMBER_STATUS);
        else
          CLEAR_FLAG(data->Flags, NLTF_REMEMBER_STATUS);

        D(DBF_GETSET, "SET MUICFG_NListtree_RememberStatus: %ld", tag->ti_Data);
      }
      break;

      case MUIA_NListtree_Title:
      {
        if ( tag->ti_Data )
          SET_FLAG(data->Flags, NLTF_TITLE);
        else
          CLEAR_FLAG(data->Flags, NLTF_TITLE);

        if ( !initial )
          nnset( data->Obj, MUIA_NList_Title, tag->ti_Data );

        D(DBF_GETSET, "SET MUIA_NListtree_Title: %ld", tag->ti_Data);
      }
      break;

      case MUIA_NListtree_TreeColumn:
      {
        data->TreeColumn = (UWORD)( tag->ti_Data & 0x00ff );

        if ( !initial )
          DoRefresh( data );

        D(DBF_GETSET, "SET MUIA_NListtree_TreeColumn: 0x%08lx", tag->ti_Data);
      }
      break;

      case MUIA_NListtree_ShowTree:
      {
        if(tag->ti_Data == (ULONG)MUIV_NListtree_ShowTree_Toggle)
        {
          if(isFlagSet(data->Flags, NLTF_NO_TREE))
            CLEAR_FLAG(data->Flags, NLTF_NO_TREE);
          else
            SET_FLAG(data->Flags, NLTF_NO_TREE);
        }
        else if ( !tag->ti_Data )
          SET_FLAG(data->Flags, NLTF_NO_TREE);
        else
          CLEAR_FLAG(data->Flags, NLTF_NO_TREE);

        if ( !initial )
          DoRefresh( data );

        D(DBF_GETSET, "SET MUIA_NListtree_ShowTree: 0x%08lx", tag->ti_Data);
      }
      break;

      case MUIA_NListtree_NoRootTree:
      {
        /*
        **  data->MRI is set in _Setup(). This makes sure, this tag is only
        **  used in the initial configuration.
        */
        if ( tag->ti_Data && !data->MRI )
        {
          SET_FLAG(data->Flags, NLTF_NO_ROOT_TREE);
        }
      }
      break;

      default:
        D(DBF_GETSET, "SET attribute 0x%08lx to 0x%08lx", tag->ti_Tag, tag->ti_Data);
      break;
    }
  }
}


static BOOL GetAttributes( struct NListtree_Data *data, Msg msg)
{
  IPTR *store = ( (struct opGet *)msg)->opg_Storage;

  *store = 0;

  switch( ( (struct opGet *)msg)->opg_AttrID )
  {
    case MUIA_Version:
      *store = LIB_VERSION;
      return( TRUE );

    case MUIA_Revision:
      *store = LIB_REVISION;
      return( TRUE );

    case MUIA_NListtree_Active:

      *store = (IPTR)data->ActiveNode;
      D(DBF_GETSET, "GET MUIA_NListtree_Active: 0x%08lx", *store);
      return( TRUE );

    case MUIA_NListtree_ActiveList:

      if ( data->ActiveList == &data->RootList )
        *store = (IPTR)MUIV_NListtree_ActiveList_Off;
      else
        *store = (IPTR)data->ActiveList;
      D(DBF_GETSET, "GET MUIA_NListtree_ActiveList: 0x%08lx",*store);
      return( TRUE );

    case MUIA_NListtree_DoubleClick:

      *store = (IPTR)data->LDClickColumn;
      D(DBF_GETSET, "GET MUIA_NListtree_DoubleClick: 0x%08lx", *store);
      return( TRUE );

    case MUIA_NListtree_TreeColumn:

      *store = (IPTR)data->TreeColumn;
      D(DBF_GETSET, "GET MUIA_NListtree_TreeColumn: 0x%08lx", *store);
      return( TRUE );

    case MUIA_NListtree_ShowTree:

      *store = (IPTR)isFlagClear(data->Flags, NLTF_NO_TREE);
      D(DBF_GETSET, "GET MUIA_NListtree_ShowTree: 0x%08lx", *store);
      return( TRUE );

    case MUIA_NListtree_AutoVisible:

      *store = (IPTR)data->AutoVisible;
      D(DBF_GETSET, "GET MUIA_NListtree_AutoVisible: 0x%08lx", *store);
      return( TRUE );

    case MUIA_NListtree_DropType:

      *store = (IPTR)data->DropType;
      return( TRUE );

    case MUIA_NListtree_DropTarget:

      *store = (IPTR)data->DropTarget;
      return( TRUE );

    case MUIA_NListtree_DropTargetPos:

      *store = (IPTR)data->DropTargetPos;
      return( TRUE );

    case MUIA_NListtree_SelectChange:

      D(DBF_GETSET, "GET MUIA_NListtree_SelectChange");
      return( TRUE );

    /*
    **  We ignore these tags ;-)
    */
    case MUIA_NList_CompareHook:
    case MUIA_NList_ConstructHook:
    case MUIA_NList_DestructHook:
    case MUIA_NList_DisplayHook:
    case MUIA_NList_DragSortable:
    case MUIA_NList_DropMark:
    case MUIA_NList_Format:
    case MUIA_NList_MinLineHeight:
    case MUIA_NList_MultiTestHook:
    case MUIA_NList_Pool:
    case MUIA_NList_PoolPuddleSize:
    case MUIA_NList_PoolThreshSize:
    case MUIA_NList_ShowDropMarks:
    case MUIA_NList_SourceArray:
    case MUIA_NList_Title:
      return( FALSE );
      break;

    default:
      D(DBF_GETSET, "GET 0x%08lx", ( (struct opGet *)msg)->opg_AttrID);
      break;
  }

  return( FALSE );
}



/*****************************************************************************\
*******************************************************************************
**
**  Standard class related functions.
**
*******************************************************************************
\*****************************************************************************/

IPTR _New(struct IClass *cl, Object *obj, struct opSet *msg)
{
  struct NListtree_Data ld;

  memset(&ld, 0, sizeof(struct NListtree_Data));

  /*
  **  Create new memory pool.
  */
  #if defined(__amigaos4__)
  ld.MemoryPool = AllocSysObjectTags(ASOT_MEMPOOL, ASOPOOL_MFlags, MEMF_SHARED|MEMF_CLEAR,
                                                   ASOPOOL_Puddle, 16384,
                                                   ASOPOOL_Threshold, 8192,
                                                   ASOPOOL_Name, "NListtree.mcc pool",
                                                   ASOPOOL_LockMem, FALSE,
                                                   TAG_DONE);
  #else
  ld.MemoryPool = CreatePool(MEMF_CLEAR, 16384, 8192);
  #endif
  if(ld.MemoryPool != NULL)
  {
    #if defined(__amigaos4__)
    ld.TreePool = AllocSysObjectTags(ASOT_MEMPOOL, ASOPOOL_MFlags, MEMF_SHARED|MEMF_CLEAR,
                                                   ASOPOOL_Puddle, 16384,
                                                   ASOPOOL_Threshold, 4096,
                                                   ASOPOOL_Name, "NListtree.mcc tree pool",
                                                   ASOPOOL_LockMem, FALSE,
                                                   TAG_DONE);
    #else
    ld.TreePool = CreatePool(MEMF_CLEAR, 16384, 4096);
    #endif
    if(ld.TreePool != NULL)
    {
      ld.Format   = NULL;
      ld.ActiveNode = CTN( MUIV_NListtree_Active_Off );
      ld.DoubleClick  = MUIV_NListtree_DoubleClick_All;


      /*
      **  Default values!!!
      */
      SET_FLAG(ld.Flags, NLTF_DUPNODENAMES);
      SET_FLAG(ld.Flags, NLTF_OPENAUTOSCROLL);
      SET_FLAG(ld.Flags, NLTF_DRAGDROPSORT);
      ld.AutoVisible  = MUIV_NListtree_AutoVisible_Normal;

      /* data will be set below because we don't have the correct address yet...somebody really should rewrite this */
      InitHook(&ld.IntCompareHook, _CompareHook_LeavesBottom, NULL);

      SET_FLAG(ld.Flags, NLTF_INT_COMPAREHOOK);

      SetAttributes( &ld, msg, TRUE );

      /*
      **  If all hooks successfully installed, set up
      **  the class for use.
      */
      /*
      **  Finally create the superclass.
      */
      obj = (Object *)DoSuperNew( cl, obj,
        //$$$ MUIA_Listview_Input
        MUIA_NList_Input,       TRUE,
        //$$$ MUIA_Listview_MultiSelect
        MUIA_NList_MultiSelect,     ld.MultiSelect,
        ( ld.MultiSelect != 0 )     ? MUIA_NList_MultiTestHook : TAG_IGNORE,  &NList_MultiTestHook,
        //$$$ MUIA_Listview_DragType
        MUIA_NList_DragType,        isFlagSet(ld.Flags, NLTF_DRAGDROPSORT) ? MUIV_NList_DragType_Default : MUIV_NList_DragType_None,
        MUIA_NList_DragSortable,    isFlagSet(ld.Flags, NLTF_DRAGDROPSORT) ? TRUE : FALSE,
        MUIA_NList_ShowDropMarks,   isFlagSet(ld.Flags, NLTF_DRAGDROPSORT) ? TRUE : FALSE,
        //$$$ Remove
        MUIA_NList_EntryValueDependent, TRUE,
        isFlagSet(ld.Flags, NLTF_TITLE) ? MUIA_NList_Title  : TAG_IGNORE, TRUE,
        ld.Format           ? MUIA_NList_Format : TAG_IGNORE, ld.Format,
        //$$$ Not implemented
        MUIA_ContextMenu,       MUIV_NList_ContextMenu_Always,
        TAG_MORE, msg->ops_AttrList );

      if ( obj )
      {
        struct NListtree_Data *data = INST_DATA(cl, obj);
        struct Task *mytask;
        const char *taskname;
        IPTR ver=0, rev=0;

        CopyMem( &ld, data, sizeof( struct NListtree_Data ) );

        data->IntCompareHook.h_Data = data; /* now we have the correct address of data */

        if(isFlagSet(data->Flags, NLTF_INT_COMPAREHOOK))
          data->CompareHook = &data->IntCompareHook; /* and now we also have the correct address of the hook */

        DoSuperMethod(cl, obj, OM_GET, MUIA_Version, &ver);
        DoSuperMethod(cl, obj, OM_GET, MUIA_Revision, &rev);

        D(DBF_ALWAYS, "NList version: %ld.%ld", ver, rev);

        mytask = FindTask( NULL );

        if ( mytask->tc_Node.ln_Name )
          taskname = mytask->tc_Node.ln_Name;
        else
          taskname = "MUI Application";

        if ( ( ver < 20 ) || ( ( ver == 20 ) && ( rev < 130 ) ) )
        {
          struct EasyStruct es;

          memset(&es,0,sizeof(es));
          es.es_StructSize = sizeof(struct EasyStruct);
          es.es_Title      = (STRPTR)"Update information...";
          es.es_TextFormat = (STRPTR)"NListtree.mcc has detected that your version of\n"
                                     "NList.mcc which is used by task `%s'\n"
                                     "is outdated (V%ld.%ld). Please update at least to\n"
                                     "version 20.130, which is available at\n\n"
                                     "http://www.sf.net/projects/nlist-classes\n\n"
                                     "NListtree will terminate now to avoid problems...\n";
          es.es_GadgetFormat = (STRPTR)"Terminate";

          EasyRequest( NULL, &es, NULL, (IPTR)taskname, ver, rev );

          //CoerceMethod( cl, obj, OM_DISPOSE );
          return( 0 );
        }

        SET_FLAG(data->Flags, NLTF_NLIST_DIRECT_ENTRY_SUPPORT);

        /*
        **  Save instance data in private NList data field.
        */
        set( obj, MUIA_UserData, data );

        /*
        **  Instance data is now valid. Here we set
        **  up the default hooks.
        */
        if(!data->FindNameHook)
        {
          data->FindNameHook = AllocVecPooled( data->MemoryPool, sizeof( struct Hook ) );

          InitHook(data->FindNameHook, _FindNameHook_CaseInsensitive, data);
        }

        if ( !data->FindUserDataHook )
        {
          data->FindUserDataHook = AllocVecPooled( data->MemoryPool, sizeof( struct Hook ) );

          InitHook(data->FindUserDataHook, _FindUserDataHook_CaseInsensitive, data);
        }

        data->buf = AllocVecPooled( data->MemoryPool, DATA_BUF_SIZE );
        data->Obj = obj;

        /*
        **  Here we initialize the root list and setting
        **  some important values to it.
        */
        NewList( (struct List *)&data->RootList.ln_List );

        data->RootList.ln_Parent  = NULL;
        SET_FLAG(data->RootList.ln_Flags, TNF_LIST);
        SET_FLAG(data->RootList.ln_Flags, TNF_OPEN);
        SET_FLAG(data->RootList.ln_IFlags, TNIF_VISIBLE);
        SET_FLAG(data->RootList.ln_IFlags, TNIF_ROOT);

        // Initialize Selected-Table.
        data->SelectedTable.tb_Current = -2;

        // check if removing MUI notifies is safe
        #if defined(__amigaos3__) || defined(__amigaos4__)
        if(LIB_VERSION_IS_AT_LEAST(MUIMasterBase, 20, 5824))
        {
          // MUI4 for AmigaOS is safe for V20.5824+
          SET_FLAG(data->Flags, NLTF_SAFE_NOTIFIES);
        }
        else if(LIB_VERSION_IS_AT_LEAST(MUIMasterBase, 20, 2346) && LIBREV(MUIMasterBase) < 5000)
        {
          // MUI3.9 for AmigaOS is safe for V20.2346+
          SET_FLAG(data->Flags, NLTF_SAFE_NOTIFIES);
        }
        else
        {
          // MUI 3.8 and older version of MUI 3.9 or MUI4 are definitely unsafe
          CLEAR_FLAG(data->Flags, NLTF_SAFE_NOTIFIES);
        }
        #else
        // MorphOS and AROS must be considered unsafe unless someone from the
        // MorphOS/AROS team confirms that removing notifies in nested OM_SET
        // calls is safe.
        CLEAR_FLAG(data->Flags, NLTF_SAFE_NOTIFIES);
        #endif

        // Setup spacial image class and tree image.
        if((data->CL_TreeImage = MUI_CreateCustomClass(NULL, (STRPTR)MUIC_Area, NULL, sizeof(struct TreeImage_Data ), ENTRY(TreeImage_Dispatcher))))
        {
          if((data->CL_NodeImage = MUI_CreateCustomClass(NULL, (STRPTR)MUIC_Image, NULL, sizeof(struct TreeImage_Data), ENTRY(NodeImage_Dispatcher))))
          {
            ActivateNotify( data );

            return( (IPTR)obj );
          }
        }
      }
    }
  }

  CoerceMethod( cl, obj, OM_DISPOSE );

  return( 0 );
}


IPTR _Dispose(struct IClass *cl, Object *obj, Msg msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  ULONG ret;
  struct MUI_CustomClass *im1, *im2;
  APTR mempool, treepool;

  ENTER();

  /*
  **  Clear complete list without setting a new active entry.
  */
  DoMethod( obj, MUIM_NListtree_Clear, NULL, 0 );

  im1 = data->CL_TreeImage;
  im2 = data->CL_NodeImage;
  mempool = data->MemoryPool;
  treepool = data->TreePool;

  ret = DoSuperMethodA(cl, obj, msg);

  if(treepool != NULL)
  {
    #if defined(__amigaos4__)
    FreeSysObject(ASOT_MEMPOOL, treepool);
    #else
    DeletePool(treepool);
    #endif
  }

  if(mempool != NULL)
  {
    #if defined(__amigaos4__)
    FreeSysObject(ASOT_MEMPOOL, mempool);
    #else
    DeletePool(mempool);
    #endif
  }

  /*
  **  Delete special image classes.
  */
  if ( im1 )
    MUI_DeleteCustomClass( im1 );

  if ( im2 )
    MUI_DeleteCustomClass( im2 );

  RETURN(ret);
  return( ret );
}



IPTR _Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);

  SetAttributes(data, msg, FALSE);

  return DoSuperMethodA(cl, obj, (Msg)msg);
}


IPTR _Get(struct IClass *cl, Object *obj, Msg msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);

  if(GetAttributes(data, msg) == TRUE)
    return TRUE;

  return DoSuperMethodA(cl, obj, msg);
}


IPTR _Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  Object *pdobj, *idobj;
  SIPTR d;
  BOOL x;

  ENTER();

  if(!(DoSuperMethodA(cl, obj, (Msg)msg)))
  {
    RETURN(FALSE);
    return FALSE;
  }

  D(DBF_SETUP, "Before: cl_SubclassCount = %ld, cl_ObjectCount = %ld", cl->cl_SubclassCount, cl->cl_ObjectCount);

  /*
  **  Obtain pens (render info is valid after Setup()!
  */
  data->MRI = msg->RenderInfo;


  /*
  **  These values are used for drawing the lines.
  */
  data->Image[IMAGE_Tree].nltdata = data;
  data->Image[IMAGE_Tree].Image = NewObject( data->CL_TreeImage->mcc_Class, NULL, MUIA_FillArea,    FALSE,
                                                                                  MUIA_Frame,       MUIV_Frame_None,
                                                                                  MUIA_UserData,    &data->Image[IMAGE_Tree],
                                                                                  MUIA_InnerBottom, 0,
                                                                                  MUIA_InnerLeft,   0,
                                                                                  MUIA_InnerRight,  0,
                                                                                  MUIA_InnerTop,    0,
                                                                                  MUIA_Weight,      0,
                                                                                  TAG_DONE);
  data->Image[IMAGE_Tree].ListImage = (Object *)DoMethod( obj, MUIM_NList_CreateImage, data->Image[IMAGE_Tree].Image, 0L );

  data->compositingActive = 0;
  #if defined(__amigaos4__)
  // check whether compositing is enabled on our screen
  GetScreenAttrs(_screen(obj), SA_Compositing, &data->compositingActive, TAG_DONE);
  #endif

  /*
  **  Get and set image config.
  */
  if(isFlagClear(data->Flags, NLTF_ISMCP))
  {
    /*
    **  Set up temporary objects.
    */
    pdobj = MUI_NewObject( MUIC_Pendisplay, TAG_DONE );
    idobj = MUI_NewObject( MUIC_Imagedisplay, TAG_DONE );


    if ( DoMethod( obj, MUIM_GetConfigItem, MUICFG_NListtree_ImageSpecClosed, &d ) )
    {
      SetupImage( data, (struct MUI_ImageSpec *)d, IMAGE_Closed );

      D(DBF_SETUP, "C-Closed node image: '%s'", (STRPTR)d);
    }
    else if ( idobj )
    {
      set( idobj, MUIA_Imagedisplay_Spec, MUICFGV_NListtree_ImageSpecClosed_Default );
      d = xget( idobj, MUIA_Imagedisplay_Spec );

      SetupImage( data, (struct MUI_ImageSpec *)d, IMAGE_Closed );

      D(DBF_SETUP, "Closed node image: '%s'", (STRPTR)d);
    }


    if ( DoMethod( obj, MUIM_GetConfigItem, MUICFG_NListtree_ImageSpecOpen, &d ) )
    {
      SetupImage( data, (struct MUI_ImageSpec *)d, IMAGE_Open );

      D(DBF_SETUP, "C-Open node image: '%s'", (STRPTR)d);
    }
    else if ( idobj )
    {
      set( idobj, MUIA_Imagedisplay_Spec, MUICFGV_NListtree_ImageSpecOpen_Default );
      d = xget( idobj, MUIA_Imagedisplay_Spec );

      SetupImage( data, (struct MUI_ImageSpec *)d, IMAGE_Open );

      D(DBF_SETUP, "Open node image: '%s'", (STRPTR)d);
    }


    if ( DoMethod( obj, MUIM_GetConfigItem, MUICFG_NListtree_ImageSpecFolder, &d ) )
    {
      SetupImage( data, (struct MUI_ImageSpec *)d, IMAGE_Folder );

      D(DBF_SETUP, "C-Folder image: '%s'", (STRPTR)d);
    }
    else if ( idobj )
    {
      set( idobj, MUIA_Imagedisplay_Spec, MUICFGV_NListtree_ImageSpecFolder_Default );
      d = xget( idobj, MUIA_Imagedisplay_Spec );

      SetupImage( data, (struct MUI_ImageSpec *)d, IMAGE_Folder );

      D(DBF_SETUP, "Folder image: '%s'", (STRPTR)d);
    }

    /*
    **  Get and set line, shadow and draw pens
    */
    if ( DoMethod( obj, MUIM_GetConfigItem, MUICFG_NListtree_PenSpecLines, &d ) )
      ObtPen( data->MRI, &data->Pen[PEN_Line], (struct MUI_PenSpec *)d );
    else if( pdobj )
    {
      DoMethod( pdobj, MUIM_Pendisplay_SetMUIPen, MUICFGV_NListtree_PenSpecLines_Default );
      ObtPen( data->MRI, &data->Pen[PEN_Line], (struct MUI_PenSpec *)xget( pdobj, MUIA_Pendisplay_Spec ) );
    }


    if ( DoMethod( obj, MUIM_GetConfigItem, MUICFG_NListtree_PenSpecShadow, &d ) )
      ObtPen( data->MRI, &data->Pen[PEN_Shadow], (struct MUI_PenSpec *)d );
    else if( pdobj )
    {
      DoMethod( pdobj, MUIM_Pendisplay_SetMUIPen, MUICFGV_NListtree_PenSpecShadow_Default );
      ObtPen( data->MRI, &data->Pen[PEN_Shadow], (struct MUI_PenSpec *)xget( pdobj, MUIA_Pendisplay_Spec ) );
    }


    if ( DoMethod( obj, MUIM_GetConfigItem, MUICFG_NListtree_PenSpecGlow, &d ) )
      ObtPen( data->MRI, &data->Pen[PEN_Glow], (struct MUI_PenSpec *)d );
    else if( pdobj )
    {
      DoMethod( pdobj, MUIM_Pendisplay_SetMUIPen, MUICFGV_NListtree_PenSpecGlow_Default );
      ObtPen( data->MRI, &data->Pen[PEN_Glow], (struct MUI_PenSpec *)xget( pdobj, MUIA_Pendisplay_Spec ) );
    }


    /*
    **  Set values
    */
    if ( DoMethod( obj, MUIM_GetConfigItem, MUICFG_NListtree_LineType, &d ) )
      data->LineType = atoi( (STRPTR)d );
    else
      data->LineType = MUICFGV_NListtree_LineType_Default;


    if ( DoMethod( obj, MUIM_GetConfigItem, MUICFG_NListtree_IndentWidth, &d ) )
      data->IndentWidth = atoi( (STRPTR)d );
    else
      data->IndentWidth = MUICFGV_NListtree_IndentWidth_Default;

    if ( DoMethod( obj, MUIM_GetConfigItem, MUICFG_NListtree_RememberStatus, &d ) )
      x = atoi( (STRPTR)d );
    else
      x = MUICFGV_NListtree_RememberStatus_Default;

    if ( x )
      SET_FLAG(data->Flags, NLTF_REMEMBER_STATUS);
    else
      CLEAR_FLAG(data->Flags, NLTF_REMEMBER_STATUS);


    if ( DoMethod( obj, MUIM_GetConfigItem, MUICFG_NListtree_OpenAutoScroll, &d ) )
      x = atoi( (STRPTR)d );
    else
      x = MUICFGV_NListtree_OpenAutoScroll_Default;

    if ( x )
      SET_FLAG(data->Flags, NLTF_OPENAUTOSCROLL);
    else
      CLEAR_FLAG(data->Flags, NLTF_OPENAUTOSCROLL);

    if ( DoMethod( obj, MUIM_GetConfigItem, MUICFG_NListtree_UseFolderImage, &d ) )
      data->UseFolderImage = atoi( (STRPTR)d );
    else
      data->UseFolderImage = MUICFGV_NListtree_UseFolderImage_Default;


    /*
    **  Dispose temporary created objects.
    */
    if( pdobj )
      MUI_DisposeObject( pdobj );

    if ( idobj )
      MUI_DisposeObject( idobj );
  }


  D(DBF_SETUP, "After: cl_SubclassCount = %ld, cl_ObjectCount = %ld", cl->cl_SubclassCount, cl->cl_ObjectCount);

  DoRefresh( data );

  RETURN(TRUE);
  return TRUE;
}


IPTR _Cleanup(struct IClass *cl, Object *obj, Msg msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  UBYTE i;

  D(DBF_SETUP, "Before: cl_SubclassCount = %ld, cl_ObjectCount = %ld", cl->cl_SubclassCount, cl->cl_ObjectCount);

  /*
  **  Dispose images.
  */
  for( i = 0; i < 4; i++ )
    DisposeImage( data, i );

  RelPen( data->MRI, &data->Pen[PEN_Line] );
  RelPen( data->MRI, &data->Pen[PEN_Shadow] );
  RelPen( data->MRI, &data->Pen[PEN_Glow] );

  data->MRI = NULL;

  D(DBF_SETUP, "After: cl_SubclassCount = %ld, cl_ObjectCount = %ld", cl->cl_SubclassCount, cl->cl_ObjectCount);

  return( DoSuperMethodA( cl, obj, msg) );
}


IPTR _Show(struct IClass *cl, Object *obj, Msg msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);

  if (!DoSuperMethodA( cl, obj, (Msg)msg))
    return 0;

  data->EHNode.ehn_Priority = 2;
  data->EHNode.ehn_Flags    = 0;
  data->EHNode.ehn_Object   = obj;
  data->EHNode.ehn_Class    = cl;
  data->EHNode.ehn_Events   = IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY;
  DoMethod( _win( obj ), MUIM_Window_AddEventHandler, &data->EHNode );

  return 1;
}


IPTR _Hide(struct IClass *cl, Object *obj, Msg msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);

  DoMethod( _win( obj ), MUIM_Window_RemEventHandler, &data->EHNode );

  return DoSuperMethodA(cl, obj, msg);
}


IPTR _AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
  DoSuperMethodA( cl, obj, (Msg)msg);

  msg->MinMaxInfo->MinWidth  += 20;
  msg->MinMaxInfo->DefWidth  += 50;
  msg->MinMaxInfo->MaxWidth  += MUI_MAXMAX;

  msg->MinMaxInfo->MinHeight += 20;
  msg->MinMaxInfo->DefHeight += 50;
  msg->MinMaxInfo->MaxHeight += MUI_MAXMAX;

  return( 0 );
}



IPTR _Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
  return( DoSuperMethodA( cl, obj, (Msg)msg) );
}


IPTR _HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
  #define _between( a, x, b )   ( ( x ) >= ( a ) && ( x ) <= ( b ) )
  #define _isinobject( x, y )   ( _between( _mleft( obj ), ( x ), _mright( obj ) ) && _between( _mtop( obj ), ( y ), _mbottom( obj ) ) )
  struct NListtree_Data *data = INST_DATA(cl, obj);
  IPTR ret = 0;

  if ( msg->muikey )
  {
    struct MUI_NListtree_TreeNode *tn;

    switch ( msg->muikey )
    {
      case MUIKEY_LEFT:
      {
        BOOL changed = FALSE;

        if ( data->ActiveNode )
        {
          if(isFlagSet(data->ActiveNode->tn_Flags, TNF_LIST) && isFlagSet(data->ActiveNode->tn_Flags, TNF_OPEN))
          {
            DoMethod( obj, MUIM_NListtree_Close, MUIV_NListtree_Close_ListNode_Active, MUIV_NListtree_Close_TreeNode_Active, 0 );
            changed = TRUE;
          }
          else if ( (IPTR)GetParent( data->ActiveNode ) != (IPTR)&data->RootList )
          {
            set( obj, MUIA_NListtree_Active, GetParent( data->ActiveNode ) );
            changed = TRUE;
          }
        }
        else
        {
          set( obj, MUIA_NListtree_Active, MUIV_NListtree_Active_LastVisible );
          changed = TRUE;
        }

        if(changed)
          ret = MUI_EventHandlerRC_Eat;
      }
      break;

      case MUIKEY_RIGHT:
      {
        BOOL changed = FALSE;

        if ( data->ActiveNode )
        {
          if(isFlagSet(data->ActiveNode->tn_Flags, TNF_LIST))
          {
            if(isFlagClear(data->ActiveNode->tn_Flags, TNF_OPEN))
            {
              DoMethod( obj, MUIM_NListtree_Open, MUIV_NListtree_Open_ListNode_Active, MUIV_NListtree_Open_TreeNode_Active, 0 );
              changed = TRUE;
            }
            else if((tn = CTN(DoMethod(obj, MUIM_NListtree_GetEntry, data->ActiveNode, MUIV_NListtree_GetEntry_Position_Head, 0))))
            {
              if(xget(obj, MUIA_NListtree_Active) != (IPTR)tn)
              {
                set( obj, MUIA_NListtree_Active, tn );
                changed = TRUE;
              }
            }
          }
        }
        else
        {
          set( obj, MUIA_NListtree_Active, MUIV_NListtree_Active_FirstVisible );
          changed = TRUE;
        }

        if(changed)
          ret = MUI_EventHandlerRC_Eat;
      }
      break;

      case MUIKEY_UP:
        break;

      case MUIKEY_DOWN:
        break;

      case MUIKEY_TOGGLE:
        break;
    }
  }

  if ( msg->imsg)
  {
    WORD mx, my;


    /*
    **  We make this here, because the first act
    **  in the list tree may be selection.
    */
    if(isFlagSet(msg->imsg->Qualifier, IEQUALIFIER_LSHIFT))
    {
      SET_FLAG(data->Flags, NLTF_QUALIFIER_LSHIFT);
    }
    else
    {
      CLEAR_FLAG(data->Flags, NLTF_QUALIFIER_LSHIFT);
    }

    if(isFlagSet(msg->imsg->Qualifier, IEQUALIFIER_CONTROL))
    {
      SET_FLAG(data->Flags, NLTF_QUALIFIER_CONTROL);
    }
    else
    {
      CLEAR_FLAG(data->Flags, NLTF_QUALIFIER_CONTROL);
    }

    mx = msg->imsg->MouseX;
    my = msg->imsg->MouseY;

    switch ( msg->imsg->Class )
    {
      case IDCMP_MOUSEBUTTONS:
      {
        D(DBF_INPUT, "mousebutton");
        if ( _isinobject( mx, my ) )
        {
          struct MUI_NListtree_TreeNode *tn;
          struct MUI_NList_TestPos_Result tpres;

          D(DBF_INPUT, "inobject");

          switch( msg->imsg->Code )
          {
            case SELECTDOWN:

              /*
              **  Get pointer to the selected node.
              **  Set active node and list in global
              **  data structure.
              */
              DoMethod( obj, MUIM_NList_TestPos, mx, my, &tpres );

              if ( tpres.entry != -1 )
              {
                /*
                **  When user changed the column positions,
                **  we use this to avoid trouble.
                */
                //$$$ Remove is List used
                tpres.column = DoMethod( obj, MUIM_NList_ColumnToCol, tpres.column );

                DoMethod( obj, MUIM_NList_GetEntry, tpres.entry, &tn );

                /*
                **  Only react if node is not frozen.
                */
                if(isFlagClear(tn->tn_Flags, TNF_FROZEN))
                {
                  STATIC UWORD lastclickentry = 0;

                  /*
                  **  If node is a list entry, open or close the node
                  **  by simple click on the image depending on the
                  **  current state.
                  */
                  if(isFlagSet(tn->tn_Flags, TNF_LIST) &&
                     tpres.column == data->TreeColumn &&
                     tpres.xoffset >= tn->tn_ImagePos &&
                     tpres.xoffset <= tn->tn_ImagePos + data->MaxImageWidth)
                  {
                    /*
                    **  Do not start dragging if user clicked on arrow-image.
                    */
                    //SET_FLAG(data->Flags, NLTF_OVER_ARROW);
                    //D(bug( "SETTING OVER_ARROW FLAG\n" ) );

                    data->OpenCloseEntry = tn;

                    /*
                    **  NList should do nothing in case of arrow-click
                    **  with LSHIFT qualifier..
                    */
                    if(isFlagSet(data->Flags, NLTF_QUALIFIER_LSHIFT))
                      ret = MUI_EventHandlerRC_Eat;
                  }

                  /*
                  **  We get the click time out of the intuimessage
                  **  If double click, reset time values, if not, set values to current time.
                  */
                  if(DoubleClick(data->LDClickTimeSecs, data->LDClickTimeMicros, msg->imsg->Seconds, msg->imsg->Micros) &&
                     (tpres.entry == lastclickentry))
                  {
                    /*
                    **  Do not start dragging if user double clicked on an entry.
                    */
                    //SET_FLAG(data->Flags, NLTF_OVER_ARROW);
                    //D(bug( "SETTING OVER_ARROW FLAG\n" ) );

                    /*
                    **  Double clicked!! Check what we have to do...
                    */
                    if ( ( data->DoubleClick != MUIV_NListtree_DoubleClick_Off ) && ( data->DoubleClick != MUIV_NListtree_DoubleClick_NoTrigger ) )
                    {
                      //D(bug( "DoubleClick: %ld, TreeColumn: %ld, tpres.column: %ld\n", data->DoubleClick, data->TreeColumn, tpres.column ) );

                      if ( ( ( data->DoubleClick == MUIV_NListtree_DoubleClick_Tree ) && ( tpres.column == data->TreeColumn ) ) ||
                        ( data->DoubleClick == MUIV_NListtree_DoubleClick_All ) )
                      {
                        data->OpenCloseEntry = tn;

                        data->LDClickEntry      = tpres.entry;
                        data->LDClickColumn     = tpres.column;

                        //D(bug( "Double click detected on node 0x%08lx\n", tn ) );
                      }
                    }
                    else
                    {
                      data->LDClickEntry      = -1;
                      data->LDClickColumn     = -1;
                    }

                    data->LDClickTimeSecs   = 0;
                    data->LDClickTimeMicros = 0;
                  }
                  else
                  {
                    data->LDClickTimeSecs   = msg->imsg->Seconds;
                    data->LDClickTimeMicros = msg->imsg->Micros;
                  }

                  lastclickentry = tpres.entry;
                }
              }
              break;


            case SELECTUP:

              /*
              **  Get pointer to the selected node.
              **  Set active node and list in global
              **  data structure.
              */
              DoMethod( obj, MUIM_NList_TestPos, mx, my, &tpres );

              if ( tpres.entry != -1 )
              {
                /*
                **  When user changed the column positions,
                **  we use this to avoid trouble.
                */
                //$$$ Remove
                tpres.column = DoMethod( obj, MUIM_NList_ColumnToCol, tpres.column );

                DoMethod( obj, MUIM_NList_GetEntry, tpres.entry, &tn );

                D(DBF_INPUT, "Clicked on xoffset = %ld, col: %ld, impos = %ld, arrowwidth = %ld", tpres.xoffset, tpres.column, tn->tn_ImagePos, data->MaxImageWidth);

                if ( data->OpenCloseEntry )
                {
                  if(isFlagSet(data->OpenCloseEntry->tn_Flags, TNF_OPEN))
                    DoMethod( obj, MUIM_NListtree_Close, GetParent( data->OpenCloseEntry ), data->OpenCloseEntry, 0 );
                  else
                    DoMethod( obj, MUIM_NListtree_Open, GetParent( data->OpenCloseEntry ), data->OpenCloseEntry, 0 );

                  data->OpenCloseEntry = NULL;
                }
              }

              /*
              **  Delete over arrow flag when releasing mouse button.
              */
              //CLEAR_FLAG(data->Flags, NLTF_OVER_ARROW);
              //D(bug( "RESETTING OVER_ARROW FLAG\n" ) );
              break;


            /*
                        case MIDDLEDOWN:
            {
              struct MUI_NList_TestPos_Result tpres;
              struct TimeVal tv;

              DoMethod( obj, MUIM_NList_TestPos, mx, my, &tpres );

              if ( tpres.entry != -1 )
              {
                struct MUI_NListtree_TreeNode *tn;

                DoMethod( obj, MUIM_NList_GetEntry, tpres.entry, &tn );

                CurrentTime( &tv.Seconds, &tv.Microseconds );

                if ( DoubleClick( data->MDClickTime.Seconds, data->MDClickTime.Microseconds, tv.Seconds, tv.Microseconds ) &&
                  ( tpres.entry == data->MDClickEntry ) )
                {
                  data->MDClickTime.Seconds = 0;
                  data->MDClickTime.Microseconds  = 0;
                  data->MDClickEntry      = -1;
                  data->MDClickColumn     = -1;
                }
                else
                {
                  data->MDClickTime.Seconds = tv.Seconds;
                  data->MDClickTime.Microseconds  = tv.Microseconds;
                  data->MDClickEntry      = tpres.entry;
                  data->MDClickColumn     = tpres.column;
                }
              }
            }
              break;

            case MIDDLEUP:
              break;
            */
          }
        }
      }
      break;

      case IDCMP_MOUSEMOVE:
      {
        if ( _isinobject( mx, my ) )
        {
        }
      }
      break;
    }
  }

  #if defined(__amigaos3__)
  // with MUI 3.8 the superclass does not get this method, hence we must forward it ourself.
  if(ret != MUI_EventHandlerRC_Eat && MUIMasterBase != NULL && MUIMasterBase->lib_Version <= 19)
    ret = DoSuperMethodA(cl, obj, (Msg)msg);
  #endif

  return( ret );
}




IPTR _DragNDrop_DragQuery(struct IClass *cl, Object *obj, Msg msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct MUIP_DragQuery *d = (struct MUIP_DragQuery *)msg;
  ULONG result;

  ENTER();

  if(d->obj == obj)
  {
    //if(isFlagSet(data->Flags, NLTF_DRAGDROPSORT) && isFlagClear(data->Flags, NLTF_OVER_ARROW))
    if(isFlagSet(data->Flags, NLTF_DRAGDROPSORT))
    {
      RETURN(MUIV_DragQuery_Accept);
      return MUIV_DragQuery_Accept;
    }
    else
    {
      RETURN(MUIV_DragQuery_Refuse);
      return MUIV_DragQuery_Refuse;
    }
  }

  result = DoSuperMethodA(cl, obj, msg);

  RETURN(result);
  return result;
}


IPTR _DragNDrop_DragBegin(struct IClass *cl, Object *obj, Msg msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  IPTR result;

  ENTER();

  /*
  **  If active entry is the only selected one, use it
  **  for drag&drop.
  */
  /*
  if ( !data->SelectedTable.tb_Entries )
  {
    data->TempSelected = TreeNodeSelectAdd( data, data->ActiveNode );
  }
  */

  SET_FLAG(data->Flags, NLTF_DRAGDROP);
  data->OldDropTarget = NULL;

  result = DoSuperMethodA(cl, obj, msg);

  RETURN(result);
  return result;
}


#ifndef _between
#define _between(a, x, b) ((x) >= (a) && (x) <= (b))
#endif
#ifndef _isinobject
#define _isinobject(x, y) (_between(_mleft(obj), (x), _mright(obj)) && _between(_mtop(obj), (y), _mbottom(obj)))
#endif
IPTR _DragNDrop_DragReport(struct IClass *cl, Object *obj, Msg msg)
{
  struct MUIP_DragReport *d = (struct MUIP_DragReport *)msg;
  IPTR result;

  ENTER();

  if(_isinobject(d->x, d->y))
    result = DoSuperMethodA(cl, obj, msg);
  else
    result = MUIV_DragReport_Abort;

  RETURN(result);
  return result;
}


IPTR _DragNDrop_DropType(struct IClass *cl, Object *obj, struct MUIP_NList_DropType *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);

  ENTER();

  DoMethod(obj, MUIM_NList_GetEntry, *msg->pos, &data->DropTarget);

  data->DropTargetPos = *msg->pos;

  if(IsXChildOfListMember(data->DropTarget, &data->SelectedTable) == NULL)
  {
    if(msg->mousey <= msg->miny + 2)
    {
      *msg->type = MUIV_NListtree_DropType_Above;
    }
    else if(msg->mousey >= msg->maxy - 2)
    {
      *msg->type = MUIV_NListtree_DropType_Below;
      *msg->pos -= 1;
    }
    else
    {
      *msg->type = MUIV_NListtree_DropType_Onto;
    }
  }
  else
  {
    *msg->type = MUIV_NListtree_DropType_None;
  }

  if(*msg->type == MUIV_NListtree_DropType_Onto && isFlagSet(data->DropTarget->tn_Flags, TNF_LIST))
    data->DropType = MUIV_NListtree_DropType_Sorted;
  else
    data->DropType = *msg->type;

  DoMethod(obj, MUIM_NListtree_DropType, &data->DropTargetPos, &data->DropType, msg->minx, msg->maxx, msg->miny, msg->maxy, msg->mousex, msg->mousey);

  if(*msg->type == MUIV_NListtree_DropType_Below && data->DropType != MUIV_NListtree_DropType_Below)
    *msg->pos += 1;

  if(data->DropType == MUIV_NListtree_DropType_Sorted)
    *msg->type = MUIV_NListtree_DropType_Onto;
  else
    *msg->type = data->DropType;

  D(DBF_DRAGDROP, "OPos: %ld, OType: %ld, Pos: %ld, Type: %ld", *msg->pos, *msg->type, data->DropTargetPos, data->DropType);

  if(isFlagSet(data->DropTarget->tn_Flags, TNF_LIST) && isFlagClear(data->DropTarget->tn_Flags, TNF_OPEN) && *msg->type == MUIV_NListtree_DropType_Onto)
  {
    // Make sure the screen is currently not locked because of the current DnD action.
    // Opening the node now might lead to a deadlock, because the redraw might require
    // another lock. However, OS4's MUI is able to perform lockless DnD actions on true
    // color screens.
    if(data->compositingActive != 0 && _screen(obj)->LayerInfo.Lock.ss_Owner == NULL)
    {
      // the current possible drop target is a closed list node
      if(data->OldDropTarget != data->DropTarget)
      {
        D(DBF_DRAGDROP, "drop target changed, old='%s', new='%s'", data->OldDropTarget != NULL ? data->OldDropTarget->tn_Name : (STRPTR)"NULL", data->DropTarget ? data->DropTarget->tn_Name : (STRPTR)"NULL");
        // the possible drop target node has changed
        // remember the current time
        CurrentTime(&data->OpenDropListSecs, &data->OpenDropListMicros);
      }
      else
      {
        // no target change and we can drop something on the node
        // now check if the desired delay has passed
        ULONG secs;
        ULONG micros;
        ULONG diffSecs;

        // get the current system time
        CurrentTime(&secs, &micros);
        // calculate the difference to the last time
        diffSecs = secs - data->OpenDropListSecs;
        if(micros < data->OpenDropListMicros)
        {
          diffSecs--;
        }

        // open the node if the user held the mouse for at least one second
        // over the closed node
        if(diffSecs >= 1)
        {
          D(DBF_DRAGDROP, "drop target same since one second '%s'", data->DropTarget ? data->DropTarget->tn_Name : (STRPTR)"NULL");
          // the time has passed, now open the list node
          DoMethod(obj, MUIM_NListtree_Open, GetParent(data->DropTarget), data->DropTarget, 0);
          // reset the start time
          data->OpenDropListSecs = secs;
          data->OpenDropListMicros = micros;
        }
      }
    }
  }

  data->OldDropTarget = data->DropTarget;

  LEAVE();
  return 0;
}


IPTR _DragNDrop_NDropDraw(struct IClass *cl, Object *obj, struct MUIP_NListtree_DropDraw *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct RastPort *rp = _rp( obj );
  ULONG apen, bpen;
  UWORD *pens = _pens( obj );

  /*
  D(DBF_DRAGDROP, "OPos: %ld, OType: %ld, Pos: %ld, Type: %ld", msg->pos, msg->type,
    data->DropTargetPos, data->DropType);
  */

  apen = GetAPen( rp );
  bpen = GetBPen( rp );

  MySetABPenDrMd( rp, pens[MPEN_SHINE], pens[MPEN_SHADOW], JAM1 );
  SetDrPt( rp, 0xF0F0 );

  switch ( data->DropType )
  {
    case MUIV_NListtree_DropType_Above:

      Move( rp, msg->MinX, msg->MinY );
      Draw( rp, msg->MaxX, msg->MinY );

      MySetABPenDrMd( rp, MUIPEN( data->Pen[PEN_Shadow] ), MUIPEN( data->Pen[PEN_Shadow] ), JAM1 );
      SetDrPt( rp, 0x0F0F );

      Move( rp, msg->MinX, msg->MinY );
      Draw( rp, msg->MaxX, msg->MinY );

      break;

    case MUIV_NListtree_DropType_Below:
      Move( rp, msg->MinX, msg->MaxY );
      Draw( rp, msg->MaxX, msg->MaxY );

      Move( rp, msg->MinX, msg->MaxY );
      Draw( rp, msg->MaxX, msg->MaxY );

      MySetABPenDrMd( rp, MUIPEN( data->Pen[PEN_Shadow] ), MUIPEN( data->Pen[PEN_Shadow] ), JAM1 );
      SetDrPt( rp, 0x0F0F );

      Move( rp, msg->MinX, msg->MaxY );
      Draw( rp, msg->MaxX, msg->MaxY );

      Move( rp, msg->MinX, msg->MaxY );
      Draw( rp, msg->MaxX, msg->MaxY );
      break;

    case MUIV_NListtree_DropType_Onto:
    case MUIV_NListtree_DropType_Sorted:
      Move( rp, msg->MinX, msg->MinY + 1 );
      Draw( rp, msg->MaxX, msg->MinY + 1 );

      Move( rp, msg->MinX, msg->MaxY - 1 );
      Draw( rp, msg->MaxX, msg->MaxY - 1 );

      MySetABPenDrMd( rp, MUIPEN( data->Pen[PEN_Shadow] ), MUIPEN( data->Pen[PEN_Shadow] ), JAM1 );
      SetDrPt( rp, 0x0F0F );

      Move( rp, msg->MinX, msg->MinY + 1 );
      Draw( rp, msg->MaxX, msg->MinY + 1 );

      Move( rp, msg->MinX, msg->MaxY - 1 );
      Draw( rp, msg->MaxX, msg->MaxY - 1 );
      break;
  }

  SetDrPt( rp, (UWORD)~0 );
  MySetABPenDrMd( rp, apen, bpen, JAM1 );

  return( 0 );
}


//$$$
IPTR _DragNDrop_DropDraw(struct IClass *cl, Object *obj, struct MUIP_NList_DropDraw *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct MUI_NListtree_TreeNode *drawtarget;

  D(DBF_DRAGDROP, "DropType: %ld, DropPos: %ld", msg->type, msg->pos);

  DoMethod( obj, MUIM_NList_GetEntry, msg->pos, &drawtarget );

  DoMethod( obj, MUIM_NListtree_DropDraw, data->DropTargetPos, data->DropType, msg->minx + drawtarget->tn_ImagePos, msg->maxx, msg->miny, msg->maxy );

  return( 0 );
}


IPTR _DragNDrop_DragFinish(struct IClass *cl, Object *obj, Msg msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  IPTR result;

  ENTER();

  CLEAR_FLAG(data->Flags, NLTF_DRAGDROP);

  result = DoSuperMethodA(cl, obj, msg);

  RETURN(result);
  return result;
}


IPTR _DragNDrop_DragDrop(struct IClass *cl, Object *obj, UNUSED Msg msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct MUI_NListtree_TreeNode *tn, *tn2, *dtn, *nexttn = NULL;
  struct MUI_NListtree_ListNode *ln = NULL;

  dtn = data->DropTarget;

  D(DBF_DRAGDROP, "MUIM_DragDrop");

  if ( data->SelectedTable.tb_Entries > 0 )
  {
    struct MUI_NListtree_TreeNode *rettn;

    DoQuiet( data, TRUE );
    DeactivateNotify( data );
    data->IgnoreSelectionChange = 1;

    tn = (struct MUI_NListtree_TreeNode *)MUIV_NListtree_NextSelected_Start;

    for (;;)
    {
      DoMethod( obj, MUIM_NListtree_NextSelected, &tn );

      if ( (IPTR)tn == (IPTR)MUIV_NListtree_NextSelected_End )
        break;

      if((rettn = IsXChildOfListMemberNotSelf(tn, &data->SelectedTable)))
      {
        D(DBF_DRAGDROP, "Entry 0x%08lx - %s  already inserted through node 0x%08lx - %s", tn, tn->tn_Name, rettn, rettn->tn_Name);
      }
      else
      {
        switch( data->DropType )
        {
          case MUIV_NListtree_DropType_Above:
            D(DBF_DRAGDROP, "Inserting entry 0x%08lx - %s above L: %s, N: %s", tn, tn->tn_Name, dtn->tn_Parent->tn_Name, dtn->tn_Name);

            ln = CLN( GetParent( dtn ) );

            if ( !( tn2 = CTN( GetPred( (struct Node *)&dtn->tn_Node ) ) ) )
              tn2 = CTN( MUIV_NListtree_Move_NewTreeNode_Head );

            DoMethod( obj, MUIM_NListtree_Move, GetParent( tn ), tn, ln, tn2, 0 );

            break;


          case MUIV_NListtree_DropType_Below:
            D(DBF_DRAGDROP, "Inserting entry 0x%08lx below L: 0x%lx, N: 0x%lx", tn, dtn->tn_Parent, dtn);

            if ( !nexttn )
            {
              if(isFlagSet(dtn->tn_Flags, TNF_LIST) && isFlagSet(dtn->tn_Flags, TNF_OPEN))
              {
                ln    = CLN( dtn );
                tn2   = MUIV_NListtree_Move_NewTreeNode_Head;
              }
              else
              {
                ln    = CLN( GetParent( dtn ) );
                tn2   = dtn;
              }
            }
            else
            {
              tn2 = nexttn;
            }

            DoMethod( obj, MUIM_NListtree_Move, GetParent( tn ), tn, ln, tn2, 0 );

            break;


          case MUIV_NListtree_DropType_Onto:
          case MUIV_NListtree_DropType_Sorted:
            D(DBF_DRAGDROP, "Inserting entry 0x%08lx onto L: 0x%lx, N: 0x%lx", tn, dtn->tn_Parent, dtn);

            if(isFlagSet(dtn->tn_Flags, TNF_LIST))
              ln = CLN( dtn );
            else
              ln = CLN( GetParent( dtn ) );


            if ( data->SelectedTable.tb_Entries == 1 )
            {
              if(isFlagClear(dtn->tn_Flags, TNF_LIST) && isFlagClear(tn->tn_Flags, TNF_LIST))
              {
                tn2 = dtn;

                DoMethod( obj, MUIM_NListtree_Exchange, GetParent( tn ), tn,
                  ln, tn2, 0 );
              }
            }

            if(isFlagSet(dtn->tn_Flags, TNF_LIST))
            {
              tn2 = CTN(MUIV_NListtree_Move_NewTreeNode_Sorted);

              DoMethod( obj, MUIM_NListtree_Move, GetParent( tn ), tn, ln, tn2, 0 );
            }

            break;
        }

        nexttn = tn;
      }
    }

    data->IgnoreSelectionChange = 0;
    ActivateNotify( data );

    /* sba: the active node could be changed, but the notify calling was disabled */
//    data->ForceActiveNotify = 1;
    DoMethod(data->Obj, MUIM_NListtree_GetListActive, 0);

    DoQuiet( data, FALSE );
  }

  /*
  **  If active entry moved, deselect him.
  */
  /*
  if ( data->TempSelected )
  {
    TreeNodeSelectRemove( data, data->TempSelected );
    data->TempSelected = NULL;
  }
  */

  CLEAR_FLAG(data->Flags, NLTF_DRAGDROP);

  return( 0 );
}


IPTR _NList_Display(struct IClass *cl, Object *obj, struct MUIP_NList_Display *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct MUI_NListtree_TreeNode *tn = CTN(msg->entry);

  /*
  if ( !strcmp( "comp", tn->tn_Name ) )
    DEBUGBREAK;
  */

  // invoke the display method, this may be overridden by subclassed instances
  DoMethod(obj, MUIM_NListtree_Display, tn, msg->entry_pos, msg->strings, msg->preparses);

  if(tn != NULL)
  {
    D(DBF_DRAW, "render flags=%lx %s %s",tn->tn_Flags,isFlagSet(tn->tn_Flags, TNF_LIST)?" list":"",msg->strings[1]);

    if(msg->strings[data->TreeColumn] == NULL)
    {
      msg->strings[data->TreeColumn] = tn->tn_Name;
    }

    /*
    **  Reset image position. Will be updated inside DrawPos().
    */
    tn->tn_ImagePos = 0;

    // NUL terminate the string
    data->buf[0] = '\0';
    if(isFlagClear(data->Flags, NLTF_NO_TREE))
      DrawImages(tn, tn, data, 0);

    if(msg->preparses[data->TreeColumn] != NULL)
      strlcat(data->buf, msg->preparses[data->TreeColumn], DATA_BUF_SIZE);

    strlcat(data->buf, msg->strings[data->TreeColumn], DATA_BUF_SIZE);

    D(DBF_DRAW, "%s - %s", isFlagSet(data->Flags, NLTF_QUIET) ? "QUIET" : "RUN", data->buf);

    msg->strings[data->TreeColumn] = data->buf;
  }

  return (IPTR)tn;
}


IPTR _NListtree_Construct(struct IClass *cl, Object *obj, struct MUIP_NListtree_Construct *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  IPTR result;

  // we just fallback to the construct hook, if any is set
  if(data->ConstructHook != NULL)
    result = (IPTR)MyCallHook(data->ConstructHook, data, MUIA_NListtree_ConstructHook, msg->Name, msg->UserData, msg->MemPool, msg->Flags);
  else
    result = (IPTR)msg->UserData;

  return result;
}


IPTR _NListtree_Destruct(struct IClass *cl, Object *obj, struct MUIP_NListtree_Destruct *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);

  if(data->DestructHook != NULL)
    MyCallHook(data->DestructHook, data, MUIA_NListtree_DestructHook, msg->Name, msg->UserData, msg->MemPool, msg->Flags);

  return 0;
}


IPTR _NListtree_Display(struct IClass *cl, Object *obj, struct MUIP_NListtree_Display *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);

  // we just fallback to the display hook, if any is set
  if(data->DisplayHook != NULL)
    MyCallHook(data->DisplayHook, data, MUIA_NListtree_DisplayHook, msg->TreeNode, msg->EntryPos, msg->Array, msg->Preparse);
  else
    msg->Array[data->TreeColumn] = NULL;

  return 0;
}


IPTR _NListtree_Compare(struct IClass *cl, Object *obj, struct MUIP_NListtree_Compare *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  LONG cmp;

  // fall back to the compare hook if one is given, otherwise we assume node1 < node2
  if(data->CompareHook != NULL)
    cmp = (LONG)MyCallHook(data->CompareHook, data, MUIA_NListtree_CompareHook, msg->TreeNode1, msg->TreeNode2, 0);
  else
    cmp = -1;

  return cmp;
}


//$$$
IPTR _ContextMenuBuild(struct IClass *cl, Object *obj, struct MUIP_NList_ContextMenuBuild *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);

  STATIC struct NewMenu ContextMenu[] =
  {
    /*
    ** Type   Label               Key Flg MX  UserData
    ** ======== ===============================   === === === ==============
    */
    { NM_TITLE, (STRPTR)"NListtree",          0,  0,  0,  NULL,   },

    { NM_ITEM,  (STRPTR)"Copy to clipboard",  0,  0,  0,  NULL,   },

    { NM_SUB,   (STRPTR)"Unit 0",             0,  0,  0,  (APTR)0,  },
    { NM_SUB,   (STRPTR)"Unit 1",             0,  0,  0,  (APTR)1,  },
    { NM_SUB,   (STRPTR)"Unit 2",             0,  0,  0,  (APTR)2,  },
    { NM_SUB,   (STRPTR)"Unit 3",             0,  0,  0,  (APTR)3,  },

    { NM_END, NULL,               0,  0,  0,  NULL,   }
  };

  data->MenuChoice.pos = msg->pos;
  data->MenuChoice.col = msg->column;
  data->MenuChoice.ontop = msg->ontop;

  if ( !msg->ontop )
    return( (IPTR)MUI_MakeObject( MUIO_MenustripNM, ContextMenu, 0 ) );

  return( DoSuperMethodA( cl, obj, (Msg)msg) );
}


//$$$
IPTR _ContextMenuChoice(struct IClass *cl, Object *obj, struct MUIP_ContextMenuChoice *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct MUI_NListtree_TreeNode *tn;

  if ( !data->MenuChoice.ontop )
  {
    DoMethod( obj, MUIM_NList_GetEntry, data->MenuChoice.pos, &tn );
    DoMethod( obj, MUIM_NListtree_CopyToClip, tn, data->MenuChoice.pos, muiUserData( msg->item ) );

    return( 0 );
  }


  return( DoSuperMethodA( cl, obj, (Msg)msg) );
}



/*****************************************************************************\
*******************************************************************************
**
**  Class methods.
**
*******************************************************************************
\*****************************************************************************/

/****** NListtree.mcc/MUIM_NListtree_Open ***********************************
*
*   NAME
*
* MUIM_NListtree_Open -- Open the specified tree node. (V1)
*
*
*   SYNOPSIS
*
* DoMethod(obj, MUIM_NListtree_Open,
*   struct MUI_NListtree_TreeNode *listnode,
*   struct MUI_NListtree_TreeNode *treenode,
*   ULONG flags);
*
*
*   FUNCTION
*
* Opens a node in the listtree. To open a child, which isn't displayed,
* use 'MUIV_NListtree_Open_ListNode_Parent' to open all its parents, too.
*
* Only nodes can be opened.
*
*
*   INPUTS
*
* listnode -  Specify the node which list is used to open the node.
*
*   MUIV_NListtree_Open_ListNode_Root
*     The root list is used.
*
*   MUIV_NListtree_Open_ListNode_Parent
*     Indicates, that all the parents of the node specified in
*     'treenode' should be opened too.
*
*   MUIV_NListtree_Open_ListNode_Active
*     The list of the active node is used.
*
* treenode -  The node to open.
*
*   MUIV_NListtree_Open_TreeNode_Head
*     Opens the head node of the list.
*
*   MUIV_NListtree_Open_TreeNode_Tail
*     Opens the tail node of the list.
*
*   MUIV_NListtree_Open_TreeNode_Active
*     The active node will be opened.
*
*   MUIV_NListtree_Open_TreeNode_All:
*     All the nodes of the list are opened.
*
*
*   RESULT
*
*   EXAMPLE
*   // Open the active list.
*   DoMethod(obj, MUIM_NListtree_Open,
*     MUIV_NListtree_Open_ListNode_Active,
*     MUIV_NListtree_Open_TreeNode_Active, 0);
*
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
* MUIM_NListtree_Close
*
*
******************************************************************************
*
*/
IPTR _NListtree_Open(struct IClass *cl, Object *obj, struct MUIP_NListtree_Open *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct MUI_NListtree_TreeNode *tn, *tn2;
  struct MUI_NListtree_ListNode *ln;
  BOOL openall = FALSE;

  //D(bug( "Node: 0x%08lx - 0x%08lx", msg->ListNode, msg->TreeNode ) );

  /*
  **
  */
  switch( (IPTR)msg->ListNode )
  {
    case MUIV_NListtree_Open_ListNode_Root:
      ln = &data->RootList;
      break;

    case MUIV_NListtree_Open_ListNode_Parent:
      ln = &data->RootList;
      break;

    case MUIV_NListtree_Open_ListNode_Active:
      ln = data->ActiveList;
      break;

    default:
      ln = CLN( msg->ListNode );
      break;
  }

  /*
  **
  */
  switch( (IPTR)msg->TreeNode )
  {
    case MUIV_NListtree_Open_TreeNode_Head:
      tn = CTN( GetHead( (struct List *)&ln->ln_List ) );
      break;

    case MUIV_NListtree_Open_TreeNode_Tail:
      tn = CTN( GetTail( (struct List *)&ln->ln_List ) );
      break;

    case MUIV_NListtree_Open_TreeNode_Active:
      tn = data->ActiveNode;
      break;

    case MUIV_NListtree_Open_TreeNode_All:
      tn = CTN( ln ); // ***  Dummy for check below.
      openall = TRUE;
      break;

    default:
      tn = msg->TreeNode;
      break;
  }


  /*
  **  Determine if the node holds a list
  **  and the node is not open.
  */
  if ( ln && tn )
  {
    if(isFlagClear(tn->tn_Flags, TNF_LIST))
      tn = GetParentNotRoot( tn );

    if ( tn )
    {
      if((isFlagSet(tn->tn_Flags, TNF_LIST) && isFlagClear(tn->tn_Flags, TNF_OPEN)) || openall)
      {
        DeactivateNotify( data );
        DoQuiet( data, TRUE );

        //D(bug( "Node: 0x%08lx - %s%s", tn, tn->tn_Name, openall ? " (expand)" : "" ) );

        if ( openall )
          OpenTreeNodeListExpand( data, ln );
        else
          OpenTreeNode( data, tn );

        if ((IPTR)msg->ListNode == (IPTR)MUIV_NListtree_Open_ListNode_Parent)
        {
          tn2 = tn;

          while((tn2 = GetParentNotRoot(tn2)))
          {
            //D(bug( "Opening node: 0x%08lx - %s", tn2, tn2->tn_Name ) );

            OpenTreeNode( data, tn2 );
          }
        }

        if(isFlagSet(data->Flags, NLTF_OPENAUTOSCROLL) && !openall )
          ShowTree( data, tn );

        DoQuiet( data, FALSE );
        ActivateNotify( data );

        /* sba: the active note could be changed, but the notify calling was disabled */
        DoMethod(data->Obj, MUIM_NListtree_GetListActive, 0);
      }
    }
  }

  return( 0 );
}



/****** NListtree.mcc/MUIM_NListtree_Close ***********************************
*
*   NAME
*
* MUIM_NListtree_Close -- Close the specified list node. (V1)
*
*
*   SYNOPSIS
*
* DoMethod(obj, MUIM_NListtree_Close,
*   struct MUI_NListtree_TreeNode *listnode,
*   struct MUI_NListtree_TreeNode *treenode,
*   ULONG flags);
*
*
*   FUNCTION
*
* Close a node or nodes of a listtree. It is checked if the tree node
* is a node, not a leaf!
*
* When the active entry was a child of the closed node, the closed node
* will become active.
*
*
*   INPUTS
*
* listnode -  Specify the node which list is used to find the entry. The
*       search is started at the head of this list.
*
*   MUIV_NListtree_Close_ListNode_Root
*     The root list is used.
*
*   MUIV_NListtree_Close_ListNode_Parent
*     The list which is the parent of the active list is used.
*
*   MUIV_NListtree_Close_ListNode_Active
*     The list of the active node is used.
*
*
* treenode -  The node which should be closed. If there are children
*       of the node, they are also closed.
*
*   MUIV_NListtree_Close_TreeNode_Head
*     The head of the list defined in 'listnode' is closed.
*
*   MUIV_NListtree_Close_TreeNode_Tail:
*     Closes the tail of the list defined in 'listnode'.
*
*   MUIV_NListtree_Close_TreeNode_Active:
*     Closes the active node.
*
*   MUIV_NListtree_Close_TreeNode_All:
*     Closes all nodes of the list which is specified in
*     'listnode'.
*
*
*   RESULT
*
*   EXAMPLE
*
*   // Close the active list.
*   DoMethod(obj, MUIM_NListtree_Close,
*     MUIV_NListtree_Close_ListNode_Active,
*     MUIV_NListtree_Close_TreeNode_Active, 0);
*
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
* MUIM_NListtree_Open
*
*
******************************************************************************
*
*/
IPTR _NListtree_Close(struct IClass *cl, Object *obj, struct MUIP_NListtree_Close *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct MUI_NListtree_TreeNode *tn;
  struct MUI_NListtree_ListNode *ln;
  BOOL closeall = FALSE;

  /*
  **
  */
  switch( (IPTR)msg->ListNode )
  {
    case MUIV_NListtree_Close_ListNode_Root:
      ln = &data->RootList;
      break;

    case MUIV_NListtree_Close_ListNode_Parent:
      ln = CLN( data->ActiveList->ln_Parent );
      break;

    case MUIV_NListtree_Close_ListNode_Active:
      ln = data->ActiveList;
      break;

    default:
      ln = CLN( msg->ListNode );
      break;
  }

  /*
  **
  */
  switch( (IPTR)msg->TreeNode )
  {
    case MUIV_NListtree_Close_TreeNode_Head:
      tn = CTN( GetHead( (struct List *)&ln->ln_List ) );
      break;

    case MUIV_NListtree_Close_TreeNode_Tail:
      tn = CTN( GetTail( (struct List *)&ln->ln_List ) );
      break;

    case MUIV_NListtree_Close_TreeNode_Active:
      tn = data->ActiveNode;
      break;

    case MUIV_NListtree_Close_TreeNode_All:
      tn = CTN( ln ); // ***  Dummy for check below.
      closeall = TRUE;
      break;

    default:
      tn = msg->TreeNode;
      break;
  }


  /*
  **  Determine if the node holds a list
  **  and the node is not open.
  */
  if ( ln && tn )
  {
    if((isFlagSet(tn->tn_Flags, TNF_LIST) && isFlagSet(tn->tn_Flags, TNF_OPEN)) || closeall )
    {
      struct MUI_NListtree_TreeNode *temp = data->ActiveNode;

      DeactivateNotify( data );
      DoQuiet( data, TRUE );

      //D(bug( "Node: 0x%08lx - %s%s", tn, tn->tn_Name, closeall ? " (collapse)" : "" ) );

      if ( closeall )
      {
        tn = CTN( ln );

        CloseTreeNodeListCollapse( data, ln );
      }
      else
      {
        if(isFlagSet(data->Flags, NLTF_REMEMBER_STATUS))
          CloseTreeNode( data, tn );
        else
          CloseTreeNodeCollapse( data, tn );
      }

      DoQuiet( data, FALSE );
      ActivateNotify( data );

      if ( temp )
      {
        if(isFlagSet(temp->tn_IFlags, TNIF_VISIBLE))
          set( obj, MUIA_NListtree_Active, temp );
        else
          set( obj, MUIA_NListtree_Active, tn );
      }
    }
  }

  return( 0 );
}



/****** NListtree.mcc/MUIM_NListtree_Insert **********************************
*
*   NAME
*
* MUIM_NListtree_Insert -- Insert an entry at the specified position. (V1)
*
*
*   SYNOPSIS
*
* struct MUI_NListtree_TreeNode *treenode =
*   DoMethod(obj, MUIM_NListtree_Insert,
*     STRPTR name, APTR userdata,
*     struct MUI_NListtree_TreeNode *listnode,
*     struct MUI_NListtree_TreeNode *prevtreenode,
*     ULONG flags);
*
*
*   FUNCTION
*
* Insert an entry at the position, which is defined in 'listnode'
* and 'prevtreenode'. Name contains the name of the entry as string
* which is buffered. The user entry can be used as you like.
*
*
*   INPUTS
*
* name/userdata - What the names say ;-)
*
*
* listnode -    Specify the node which list is used to insert
*         the entry.
*
*   MUIV_NListtree_Insert_ListNode_Root
*     Use the root list.
*
*   MUIV_NListtree_Insert_ListNode_Active
*     Use the list of the active node.
*
*   MUIV_NListtree_Insert_ListNode_ActiveFallback
*     Use the list of the active node. If no list is active,
*     an automatic fallback to the root list is done.
*
*   MUIV_NListtree_Insert_ListNode_LastInserted
*     Insert entry in the list the last entry was inserted.
*
*
* prevtreenode -  The node which is the predecessor of the node
*         to insert.
*
*   MUIV_NListtree_Insert_PrevNode_Head
*     The entry will be inserted at the head of the list.
*
*   MUIV_NListtree_Insert_PrevNode_Tail
*     The entry will be inserted at the tail of the list.
*
*   MUIV_NListtree_Insert_PrevNode_Active
*     The entry will be inserted after the active node of
*     the list. If no entry is active, the entry will be
*     inserted at the tail.
*
*   MUIV_NListtree_Insert_PrevNode_Sorted:
*     The entry will be inserted using the defined sort hook.
*
* flags:
*
*   MUIV_NListtree_Insert_Flag_Active
*     The inserted entry will be set to active. This means the
*     cursor is moved to the newly inserted entry. If the entry
*     was inserted into a closed node, it will be opened.
*
*   MUIV_NListtree_Insert_Flag_NextNode
*     'prevtreenode' is the successor, not the predecessor.
*
*
*   RESULT
*
* A pointer to the newly inserted entry.
*
*
*   EXAMPLE
*
*   // Insert an entry after the active one and make it active.
*   DoMethod(obj, MUIM_NListtree_Insert, "Hello", NULL,
*     MUIV_NListtree_Insert_ListNode_Active,
*     MUIV_NListtree_Insert_PrevNode_Active,
*     MUIV_NListtree_Insert_Flag_Active);
*
*
*   NOTES
*
*   BUGS
*
* Not implemented yet:
*   MUIV_NListtree_Insert_Flag_NextNode
*
*
*   SEE ALSO
*
* MUIA_NListtree_ConstructHook, MUIA_NListtree_CompareHook
*
*
******************************************************************************
*
*/
IPTR _NListtree_Insert(struct IClass *cl, Object *obj, struct MUIP_NListtree_Insert *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct MUI_NListtree_TreeNode *tn = NULL;
  struct MUI_NListtree_ListNode *ln;
  APTR user;
  static const char *np = "*** NULL POINTER ***";

  D(DBF_LISTTREE, "MUIM_NListtree_Insert: name=%s flags=0x%lx listnode:0x%lx prevnode:0x%lx  %ld",msg->Name,msg->Flags,msg->ListNode,msg->PrevNode,data->NumEntries);

  /*
  **  Construct new user data
  */
  user = (APTR)DoMethod(obj, MUIM_NListtree_Construct, msg->Name, msg->User, data->TreePool, 0);

  /*
  **  Allocate memory for the new entry and, if
  **  list, initialize list structure for use.
  */
  if(isFlagSet(msg->Flags, TNF_LIST))
  {
    if((ln = CLN(AllocVecPooled(data->TreePool, sizeof(struct MUI_NListtree_ListNode)))))
    {
      tn = CTN( ln );

      NewList( (struct List *)&ln->ln_List );
    }
  }
  else
    tn = CTN( AllocVecPooled( data->TreePool, sizeof( struct MUI_NListtree_TreeNode ) ) );


  /*
  **  If allocated memory successfully, set up all fields,
  **  get special list and node pointers if needed and add
  **  to a list.
  */
  if ( tn )
  {
    struct MUI_NListtree_ListNode *li;

    if ( !msg->Name )
      msg->Name = (char *)np;

    /*
    **  Should we duplicate the supplied node name?
    */
    if(isFlagSet(data->Flags, NLTF_DUPNODENAMES))
    {
      int len = strlen( msg->Name ) + 1;
      tn->tn_Name = (STRPTR)AllocVecPooled( data->TreePool, len );
      strlcpy( tn->tn_Name, msg->Name, len );
      SET_FLAG(tn->tn_IFlags, TNIF_ALLOCATED);
    }
    else
      tn->tn_Name = msg->Name;

    if ( tn->tn_Name )
    {
      tn->tn_User   = user;
      tn->tn_Flags  = msg->Flags;

      /*
      **  Check out which list to use.
      */
      switch( (IPTR)msg->ListNode )
      {
        case MUIV_NListtree_Insert_ListNode_Root:
          li = &data->RootList;
          break;

        case MUIV_NListtree_Insert_ListNode_Active:
          li = data->ActiveList;
          break;

        case MUIV_NListtree_Insert_ListNode_ActiveFallback:
          if ( !( li = data->ActiveList ) )
            li = &data->RootList;
          break;

        case MUIV_NListtree_Insert_ListNode_LastInserted:
          li = data->LastInsertedList;
          break;

        default:
          li = CLN( msg->ListNode );
          D(DBF_LISTTREE, "li = %p",li);
          break;
      }

      if ( li )
      {
        struct MUI_NListtree_TreeNode *in;

        /*
        **  If given list node is not a list, get parent
        **  of the given node to avoid trouble.
        */
        if(isFlagClear(li->ln_Flags, TNF_LIST))
          li = CLN( GetParent( CTN( li ) ) );

        data->LastInsertedList = li;

        tn->tn_Parent = CTN( li );

        //DoQuiet( data, TRUE );
        //DeactivateNotify( data );

        /*
        **  Add the new created entry to the specified list.
        */
        switch( (IPTR)msg->PrevNode )
        {
          case MUIV_NListtree_Insert_PrevNode_Head:
            AddHead( (struct List *)&li->ln_List, (struct Node *)&tn->tn_Node );
            in = CTN( INSERT_POS_HEAD );
            break;

          case MUIV_NListtree_Insert_PrevNode_Tail:
            AddTail( (struct List *)&li->ln_List, (struct Node *)&tn->tn_Node );
            in = CTN( INSERT_POS_TAIL );
            break;

          case MUIV_NListtree_Insert_PrevNode_Active:
            if ( data->ActiveNode )
            {
              if ( (APTR)li != (APTR)data->ActiveNode )
              {
                Insert( (struct List *)&li->ln_List, (struct Node *)&tn->tn_Node, (struct Node *)&data->ActiveNode->tn_Node );
                in = data->ActiveNode;
              }
              else
              {
                AddHead( (struct List *)&li->ln_List, (struct Node *)&tn->tn_Node );
                in = CTN( INSERT_POS_HEAD );
              }
            }
            else
            {
              AddTail( (struct List *)&li->ln_List, (struct Node *)&tn->tn_Node );
              in = CTN( INSERT_POS_TAIL );
            }
            break;

          case MUIV_NListtree_Insert_PrevNode_Sorted:
            in = GetInsertNodeSorted( data, li, tn );

            if ( (IPTR)in == (IPTR)INSERT_POS_TAIL )
            {
              AddTail( (struct List *)&li->ln_List, (struct Node *)&tn->tn_Node );
            }
            else
            {
              Insert( (struct List *)&li->ln_List, (struct Node *)&tn->tn_Node, (struct Node *)&in->tn_Node );
            }
            break;

          default:
            Insert( (struct List *)&li->ln_List, (struct Node *)&tn->tn_Node, (struct Node *)&msg->PrevNode->tn_Node );
            in = msg->PrevNode;
            break;
        }

        //D(bug( "Inserting entry 0x%08lx into list 0x%08lx (0x%08lx) after node 0x%08lx - '%s' User: 0x%08lx", tn, li, msg->ListNode, msg->PrevNode, ( (LONG)in == INSERT_POS_HEAD ) ? "HEAD" : ( (LONG)in == INSERT_POS_TAIL ) ? "TAIL" : in->tn_Name, msg->User ) );


        /* Special AmiTradeCenter Debug (Enforcer Hits!!)
        {
          UBYTE *x = (UBYTE *)user + 28;
          UBYTE *lix = (UBYTE *)li->ln_User + 28;
          UBYTE *inx = (UBYTE *)in->tn_User + 28;

          char *name = (char *)x;
          char *liname = (char *)lix;
          char *inname = (char *)inx;

          D(bug( "Inserting '%s' into list '%s' after node '%s' at pos %ld (%s %s)\n",
            name,
            ( (LONG)msg->ListNode == MUIV_NListtree_Insert_ListNode_Root ) ? "ROOT" : ( (LONG)msg->ListNode == MUIV_NListtree_Insert_ListNode_Active ) ? "ACTIVE" : liname,
            ( (LONG)in == INSERT_POS_HEAD ) ? "HEAD" : ( (LONG)in == INSERT_POS_TAIL ) ? "TAIL" : inname,
            ( li->ln_Flags & TNF_OPEN ) ? GetVisualInsertPos( data, li, in ) : -1, ( tn->tn_Flags & TNF_OPEN ) ? "TNF_OPEN" : "", ( tn->tn_Flags & TNF_LIST ) ? "TNF_LIST" : "" ) );

          D(bug( "Inserting entry 0x%08lx - '%s' into list 0x%08lx (0x%08lx) - '%s' after node 0x%08lx (0x%08lx) - '%s'\n",
            tn, tn->tn_Name, li, msg->ListNode, li->ln_Name, in, msg->PrevNode, ( (LONG)in == INSERT_POS_HEAD ) ? "HEAD" : ( (LONG)in == INSERT_POS_TAIL ) ? "TAIL" : in->tn_Name ) );
        }
        */

        InsertTreeNodeVisible( data, tn, li, in );
        NLAddToTable( data, &li->ln_Table, tn );

        /*
        **  Make sure, that the inserted node is visible by opening
        **  all parents (if it should be set active).
        */
        if(isFlagSet(msg->Flags, MUIV_NListtree_Insert_Flag_Active))
        {
          if(isFlagClear(tn->tn_IFlags, TNIF_VISIBLE))
            DoMethod( obj, MUIM_NListtree_Open, MUIV_NListtree_Open_ListNode_Parent, tn, 0 );

          set( obj, MUIA_NListtree_Active, tn );
        }

        /*
        **  Add one to the global number of entries
        **  and the number of entries in list.
        */
        data->NumEntries++;


        if(isFlagSet(tn->tn_IFlags, TNIF_VISIBLE))
          DoRefresh( data );

        //ActivateNotify( data );
        //DoQuiet( data, FALSE );

        D(DBF_LISTTREE, "Result: 0x%lx  %ld",tn,data->NumEntries);

        return( (IPTR)tn );
      }

      /*
      **  Free memory if failed to allocate
      **  name space.
      */
      FreeVecPooled( data->TreePool, tn );
    }
  }

  return(0);
}



/****** NListtree.mcc/MUIM_NListtree_InsertStruct ****************************
*
*   NAME
*
* MUIM_NListtree_InsertStruct -- Insert a structure such as a path
*                 using a delimiter. (V1)
*
*
*   SYNOPSIS
*
* struct MUI_NListtree_TreeNode *treenode =
*   DoMethod(obj, MUIM_NListtree_InsertStruct,
*     STRPTR name, APTR userdata,
*     STRPTR delimiter, ULONG flags);
*
*
*   FUNCTION
*
* Insert a structure into the list such as a path or
* something similar (like ListtreeName.mcc does). The name is
* splitted using the supplied delimiter. For each name part a
* new tree entry is generated. If you have Images/aphaso/Image.mbr,
* the structure will be build es follows:
*
*   + Images
*     + aphaso
*       - Image.mbr
*
* If a part of the structure is already present, it will be used to
* insert.
*
*
*   INPUTS
*
* name -    Data containing (must not) one or more delimiters as
*       specified in delimiter (Images/aphaso/Image.mbr for
*       example).
*
* userdata -  Your personal data.
*
* delimiter - The delimiter(s) used in the name field (":/" or
*       something).
*
* flags:
*
*   Use normal insert flags here (see there).
*
*   In addition you can supply
*   MUIV_NListtree_InsertStruct_Flag_AllowDuplicates if you want the
*   last entry, Image.mbr in the above example, to occur multiple
*   times even if the name is the same.
*
*
*   RESULT
*
* A pointer to the last instance of newly inserted entries.
*
*
*   EXAMPLE
*
*   // Insert a directory path.
*       path = MyGetPath( lock );
*
*   DoMethod(obj, MUIM_NListtree_InsertStruct,
*     path, NULL, ":/", 0);
*
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
* MUIA_NListtree_Insert
*
*
******************************************************************************
*
*/
/*
**  Returns tokens delimited by delimiter on each call.
*/
INLINE STRPTR mystrtok( STRPTR string, STRPTR buf, ULONG bufsize, STRPTR delimiter )
{
  STRPTR str, del;

  str = string;

  while( *str )
  {
    del = delimiter;

    while( *del )
    {
      if ( *str == *del )
      {
        strncpy( buf, string, MIN( (LONG)bufsize - 1, ( str - string ) ) );
        buf[MIN( (LONG)bufsize - 1, ( str - string ) )] = 0;

        return( &str[1] );
      }

      del++;
    }

    str++;
  }

  strncpy( buf, string, MIN( (LONG)bufsize - 1, ( str - string ) ) );
  buf[MIN( (LONG)bufsize - 1, ( str - string ) )] = 0;

  return( str );
}



IPTR _NListtree_InsertStruct(struct IClass *cl, Object *obj, struct MUIP_NListtree_InsertStruct *msg)
{
  struct MUI_NListtree_TreeNode *temp, *lasttn = CTN( MUIV_NListtree_Insert_ListNode_Root );
  struct NListtree_Data *data = INST_DATA(cl, obj);
  STRPTR p, token;
  ULONG len;
  static const char *np = "*** NULL POINTER ***";

  if ( !msg->Name )
    msg->Name = (char *)np;

  if((token = AllocVecPooled(data->MemoryPool, len = (strlen(msg->Name) + 1))))
  {
    p = msg->Name;

    while( 1 )
    {
      p = mystrtok( p, token, len, msg->Delimiter );

      if  ( ( !*p && isFlagSet(msg->Flags, MUIV_NListtree_InsertStruct_Flag_AllowDuplicates) ) ||
        !( temp = CTN( DoMethod( obj, MUIM_NListtree_FindName, lasttn, token, MUIV_NListtree_FindName_Flag_SameLevel ) ) ) )
      {
        lasttn = CTN( DoMethod( obj, MUIM_NListtree_Insert, token, msg->User,
          lasttn, MUIV_NListtree_Insert_PrevNode_Tail, *p ? TNF_LIST | msg->Flags : msg->Flags ) );
      }
      else
      {
        lasttn = temp;
      }

      if ( !*p ) break;
    }

    FreeVecPooled( data->MemoryPool, token );
  }

  return( (IPTR)lasttn );
}



/****** NListtree.mcc/MUIM_NListtree_Remove **********************************
*
*   NAME
*
* MUIM_NListtree_Remove -- Remove the specified entry(ies). (V1)
*
*
*   SYNOPSIS
*
* DoMethod(obj, MUIM_NListtree_Remove,
*   struct MUI_NListtree_TreeNode *listnode,
*   struct MUI_NListtree_TreeNode *treenode,
*   ULONG flags);
*
*
*   FUNCTION
*
* Removes a node or nodes from the listtree. When the active entry
* is removed, the successor will become active.
*
*
*   INPUTS
*
* listnode -  Specify the node which list is used to find the entry
*       which should be removed. The search is started at the
*       begin of this list.
*
*   MUIV_NListtree_Remove_ListNode_Root
*     The root list is used.
*
*   MUIV_NListtree_Remove_ListNode_Active
*     The list of the active node is used.
*
*
* treenode -  The node which should be removed. If there are children
*       of this node, they are also removed.
*
*   MUIV_NListtree_Remove_TreeNode_Head
*     The head of the list defined in 'listnode' is removed.
*
*   MUIV_NListtree_Remove_TreeNode_Tail
*     The tail of the list defined in 'listnode' is removed.
*
*   MUIV_NListtree_Remove_TreeNode_Active
*     Removes the active node.
*
*   MUIV_NListtree_Remove_TreeNode_All
*     All nodes of the list which is specified in 'listnode',
*     are removed. Other nodes of parent lists are not
*     affected.
*
*   MUIV_NListtree_Remove_TreeNode_Selected
*     All selected nodes are removed.
*
*   RESULT
*
*   EXAMPLE
*
*   // Remove the active entry if delete is pressed!
*   DoMethod(bt_delete, MUIM_Notify, MUIA_Pressed, FALSE,
*     lt_list, 4, MUIM_NListtree_Remove,
*     MUIV_NListtree_Remove_ListNode_Active,
*     MUIV_NListtree_Remove_TreeNode_Active, 0);
*
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
* MUIM_NListtree_Insert, MUIA_NListtree_DestructHook,
* MUIM_NList_Active
*
*
******************************************************************************
*
*/
IPTR _NListtree_Remove(struct IClass *cl, Object *obj, struct MUIP_NListtree_Remove *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct MUI_NListtree_ListNode *li;
  struct MUI_NListtree_TreeNode *tn;
  LONG pos;

  D(DBF_LISTTREE, "NList Remove listnode: 0x%lx  treenode: 0x%lx",msg->ListNode,msg->TreeNode);

  DeactivateNotify( data );

  /*
  **  Removed, because of problems when deleting entries in a loop (all selected)
  */
  //DoQuiet( data, TRUE );

  data->TempActiveNode = data->ActiveNode;

  /*
  **
  */
  switch( (IPTR)msg->ListNode )
  {
    case MUIV_NListtree_Remove_ListNode_Root:
      li = &data->RootList;
      break;

    case MUIV_NListtree_Remove_ListNode_Active:
      li = data->ActiveList;
      break;

    default:
      li = CLN( msg->ListNode );
      break;
  }

  /*
  **
  */
  if ( li )
  {
    switch( (IPTR)msg->TreeNode )
    {
      case MUIV_NListtree_Remove_TreeNode_Head:
        tn = CTN( GetHead( (struct List *)&li->ln_List ) );
        break;

      case MUIV_NListtree_Remove_TreeNode_Tail:
        tn = CTN( GetTail( (struct List *)&li->ln_List ) );
        break;

      case MUIV_NListtree_Remove_TreeNode_Active:
        tn = data->ActiveNode;
        break;

      case MUIV_NListtree_Remove_TreeNode_All:

        pos = GetVisualPos( data, CTN( GetHead( (struct List *)&li->ln_List ) ) );

        while((tn = CTN( GetHead((struct List *)&li->ln_List))))
        {
          //D(bug( "Node: 0x%08lx - %s - pos: %ld", tn, tn->tn_Name, pos ) );

          RemoveNodes( data, li, tn, pos );
        }

        tn = NULL;
        break;

      case MUIV_NListtree_Remove_TreeNode_Selected:

        while( data->SelectedTable.tb_Entries )
        {
          tn = data->SelectedTable.tb_Table[0];

          pos = GetVisualPos( data, tn );

          //D(bug( "Node: 0x%08lx - %s - pos: %ld", tn, tn->tn_Name, pos ) );

          RemoveNodes( data, CLN( tn->tn_Parent ), tn, pos );
        }

        tn = NULL;
        break;

      default:
        tn = msg->TreeNode;
        break;
    }

    if ( tn )
    {
      BOOL dorefresh = FALSE;

      pos = GetVisualPos( data, tn );

      D(DBF_LISTTREE, "Node: 0x%08lx - Name: %s - pos: %ld", tn, tn->tn_Name, pos);

      if(isFlagSet(tn->tn_IFlags, TNIF_VISIBLE))
        dorefresh = TRUE;

      RemoveNodes( data, li, tn, pos );

      if ( dorefresh )
        DoRefresh( data );

      D(DBF_LISTTREE, "Activenode: 0x%lx   Trmpactivenode: 0x%lx",data->ActiveNode,data->TempActiveNode);

      /* sba: Notification is deactivated so we get not informed if the active node changed,
      ** do this by hand now
      */
      if (data->TempActiveNode != data->ActiveNode)
      {
        nnset(data->Obj, MUIA_NListtree_Active, data->TempActiveNode);
        data->ForceActiveNotify = 1;
      }
    }
  }

/*
  if(isFlagClear(msg->Flags, MUIV_NListtree_Remove_Flag_NoActive))
  {
    if ( data->TempActiveNode == tn )
      data->TempActiveNode = NULL;

    D(bug( "Set Active node: 0x%8lx\n",data->TempActiveNode) );

    MakeSet( data, MUIA_NListtree_Active, data->TempActiveNode );

    if ( data->TempActiveNode && ( data->MultiSelect != MUIV_NListtree_MultiSelect_None ) )
    {
      TreeNodeSelect( data, data->TempActiveNode, MUIV_NListtree_Select_On, 0, 0 );
    }
  }
*/
  data->TempActiveNode = NULL;

  /*
  **  Removed, because of problems when deleting entries in a loop (all selected)
  */
  //DoQuiet( data, FALSE );
  ActivateNotify( data );

  /* sba: the active note could be changed, but the notify calling was disabled */
  DoMethod(data->Obj, MUIM_NListtree_GetListActive, 0);
  return( 0 );
}



/****** NListtree.mcc/MUIM_NListtree_Clear ***********************************
*
*   NAME
*
* MUIM_NListtree_Clear -- Clear the complete listview. (V1)
*
*
*   SYNOPSIS
*
* DoMethod(obj, MUIM_NListtree_Clear, NULL, 0)
*
*
*   FUNCTION
*
* Clear the complete listview, calling destruct hook for each entry.
*
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   // Clear the listview!
*   DoMethod( nlisttree, MUIM_NListtree_Clear, NULL, 0 );
*
*
*   NOTES
*
* For now, when using this method, you MUST supply NULL for the list
* node and 0 for flags for future compatibility.
* This will definitely change!
*
*
*   BUGS
*
*   SEE ALSO
*
* MUIM_NListtree_Remove, MUIA_NListtree_DestructHook,
*
*
******************************************************************************
*
*/
IPTR _NListtree_Clear(struct IClass *cl, Object *obj, UNUSED struct MUIP_NListtree_Clear *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct MUI_NListtree_ListNode *ln = NULL;

  ENTER();

  ln = &data->RootList;

  /*
  **
  */
  /*
  switch( (ULONG)msg->ListNode )
  {
    case MUIV_NListtree_Clear_ListNode_Root:
      ln = &data->RootList;
      break;

    case MUIV_NListtree_Clear_ListNode_Active:
      ln = data->ActiveList;
      break;

    default:
      ln = CLN( msg->ListNode );
      break;
  }
  */


  if ( ln )
  {
    /*
    **  Quick clear the NList object, call the destruct hook for
    **  each entry (QuickRemoveNodes()), delete the tree pool and
    **  allocate it new, reset the root list and delete the table
    **  memory pointer which refers to the old pool. Then initialize
    **  active node and list.
    */
    DoMethod( obj, MUIM_NList_Clear, 0 );

    QuickRemoveNodes( data, ln );

    if(data->TreePool != NULL)
    {
      #if defined(__amigaos4__)
      FreeSysObject(ASOT_MEMPOOL, data->TreePool);
      #else
      DeletePool(data->TreePool);
      #endif
    }

    #if defined(__amigaos4__)
    data->TreePool = AllocSysObjectTags(ASOT_MEMPOOL, ASOPOOL_MFlags, MEMF_SHARED|MEMF_CLEAR,
                                                      ASOPOOL_Puddle, 16384,
                                                      ASOPOOL_Threshold, 4096,
                                                      ASOPOOL_Name, "NListtree.mcc tree pool",
                                                      ASOPOOL_LockMem, FALSE,
                                                      TAG_DONE);
    #else
    data->TreePool = CreatePool(MEMF_CLEAR, 16384, 4096);
    #endif

    NewList( (struct List *)&data->RootList.ln_List );
    data->RootList.ln_Table.tb_Table = NULL;
    data->RootList.ln_Table.tb_Entries = 0;
    data->RootList.ln_Table.tb_Size = 0;
    data->NumEntries = 0;

    data->ActiveNode = MUIV_NListtree_Active_Off;
    data->ActiveList = &data->RootList;

  }

  RETURN(0);
  return( 0 );
}



/****** NListtree.mcc/MUIM_NListtree_Exchange ********************************
*
*   NAME
*
* MUIM_NListtree_Exchange -- Exchanges two tree nodes. (V1)
*
*
*   SYNOPSIS
*
* DoMethod(obj, MUIM_NListtree_Exchange,
*   struct MUI_NListtree_TreeNode *listnode1,
*   struct MUI_NListtree_TreeNode *treenode1,
*   struct MUI_NListtree_TreeNode *listnode2,
*   struct MUI_NListtree_TreeNode *treenode2,
*   ULONG flags);
*
*
*   FUNCTION
*
* Exchange two tree nodes.
*
*
*   INPUTS
*
* listnode1 - Specify the list node of the entry which
*       should be exchanged.
*
*   MUIV_NListtree_Exchange_ListNode1_Root
*     The root list is used.
*
*   MUIV_NListtree_Exchange_ListNode1_Active
*     The active list (the list of the active node) is used.
*
* treenode1 - Specify the node which should be exchanged.
*
*   MUIV_NListtree_Exchange_TreeNode1_Head
*     The head of the list defined in 'listnode1' is
*     exchanged.
*
*   MUIV_NListtree_Exchange_TreeNode1_Tail
*     The tail of the list defined in 'listnode1' is
*     exchanged.
*
*   MUIV_NListtree_Exchange_TreeNode1_Active
*     The active node is exchanged.
*
* listnode2 - Specify the second list node which is used for exchange.
*
*   MUIV_NListtree_Exchange_ListNode2_Root
*     The root list.
*
*   MUIV_NListtree_Exchange_ListNode2_Active
*     The list of the active node.
*
* treenode2 - This node is the second entry which is exchanged.
*
*   MUIV_NListtree_Exchange_TreeNode2_Head
*     The node 'treenode1' is exchanged with the head of the
*     list defined in 'listnode2'.
*
*   MUIV_NListtree_Exchange_TreeNode2_Tail
*     The node 'treenode1' is exchanged with the tail of the
*     list defined in 'ln2'.
*
*   MUIV_NListtree_Exchange_TreeNode2_Active:
*     The node 'treenode1' is exchanged with the active node.
*
*   MUIV_NListtree_Exchange_TreeNode2_Up:
*     The node 'treenode1' is exchanged with the entry
*     previous to the one specified in 'treenode1'.
*
*   MUIV_NListtree_Exchange_TreeNode2_Down:
*     The node 'treenode1' is exchanged with the entry next
*     (the successor) to the one specified in 'treenode1'.
*
*
*   RESULT
*
*   EXAMPLE
*
*   // Exchange the active entry with the successor.
*   DoMethod(obj,
*     MUIV_NListtree_Exchange_ListNode1_Active,
*     MUIV_NListtree_Exchange_TreeNode1_Active,
*     MUIV_NListtree_Exchange_ListNode2_Active,
*     MUIV_NListtree_Exchange_TreeNode2_Down,
*     0);
*
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
* MUIM_NListtree_Move, MUIM_NListtree_Insert,
* MUIM_NListtree_Remove
*
*
******************************************************************************
*
*/
IPTR _NListtree_Exchange(struct IClass *cl, Object *obj, struct MUIP_NListtree_Exchange *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct MUI_NListtree_ListNode *ln1, *ln2;
  struct MUI_NListtree_TreeNode *tn1, *tn2;
  struct Node *insnode1, *insnode2;
  LONG pos1, pos2;

  /*
  **  Handle all special events.
  */
  switch( (IPTR)msg->ListNode1 )
  {
    case MUIV_NListtree_Exchange_ListNode1_Root:
      ln1 = &data->RootList;
      break;

    case MUIV_NListtree_Exchange_ListNode1_Active:
      ln1 = data->ActiveList;
      break;

    default:
      ln1 = CLN( msg->ListNode1 );
      break;
  }

  switch( (IPTR)msg->TreeNode1 )
  {
    case MUIV_NListtree_Exchange_TreeNode1_Head:
      tn1 = CTN( GetHead( (struct List *)&ln1->ln_List ) );
      break;

    case MUIV_NListtree_Exchange_TreeNode1_Tail:
      tn1 = CTN( GetTail( (struct List *)&ln1->ln_List ) );
      break;

    case MUIV_NListtree_Exchange_TreeNode1_Active:
      tn1 = data->ActiveNode;
      break;

    default:
      tn1 = msg->TreeNode1;
      break;
  }

  switch( (IPTR)msg->ListNode2 )
  {
    case MUIV_NListtree_Exchange_ListNode2_Root:
      ln2 = &data->RootList;
      break;

    case MUIV_NListtree_Exchange_ListNode2_Active:
      ln2 = data->ActiveList;
      break;

    default:
      ln2 = CLN( msg->ListNode2 );
      break;
  }

  switch( (IPTR)msg->TreeNode2 )
  {
    case MUIV_NListtree_Exchange_TreeNode2_Head:
      tn2 = CTN( GetHead( (struct List *)&ln1->ln_List ) );
      break;

    case MUIV_NListtree_Exchange_TreeNode2_Tail:
      tn2 = CTN( GetTail( (struct List *)&ln1->ln_List ) );
      break;

    case MUIV_NListtree_Exchange_TreeNode2_Active:
      tn2 = data->ActiveNode;
      break;

    case MUIV_NListtree_Exchange_TreeNode2_Up:
      if ( !( tn2 = CTN( GetPred( (struct Node *)&tn1->tn_Node ) ) ) )
        tn2 = CTN( tn1->tn_Node.mln_Pred );
      break;

    case MUIV_NListtree_Exchange_TreeNode2_Down:
      if ( !( tn2 = CTN( GetSucc( (struct Node *)&tn1->tn_Node ) ) ) )
        tn2 = CTN( tn1->tn_Node.mln_Succ );
      break;

    default:
      tn2 = msg->TreeNode2;
      break;
  }


  if ( tn1 != tn2 )
  {
    DoQuiet( data, TRUE );

    /*
    **  Get the nodes where to re-insert the removed nodes.
    **  If no previsous node available, then Insert() assumes
    **  AddHead() when "insnodeX" is NULL.
    */
    insnode1 = GetPred( (struct Node *)tn1 );
    insnode2 = GetPred( (struct Node *)tn2 );

    if ( insnode1 == (struct Node *)tn2 )
      insnode1 = GetPred( insnode1 );

    if ( insnode2 == (struct Node *)tn1 )
      insnode2 = GetPred( insnode2 );

    //D(bug( "Node1: 0x%08lx - %s, Node2: 0x%08lx - %s", tn1, tn1->tn_Name, tn2, tn2->tn_Name ) );

    /*
    **  Remove entry 1.
    */
    NLRemoveFromTable( data, &ln1->ln_Table, tn1 );
    RemoveTreeNodeVisible( data, tn1, &pos1 );
    Remove( (struct Node *)&tn1->tn_Node );

    /*
    **  Remove entry 2.
    */
    NLRemoveFromTable( data, &ln2->ln_Table, tn2 );
    RemoveTreeNodeVisible( data, tn2, &pos2 );
    Remove( (struct Node *)&tn2->tn_Node );


    /*
    **  Set new parent entries and visibility.
    */
    tn1->tn_Parent = CTN( ln2 );
    tn2->tn_Parent = CTN( ln1 );

    /*
    **  Insert first entry in the list.
    */
    if ( !insnode2 )
    {
      AddHead( (struct List *)&ln2->ln_List, (struct Node *)tn1 );
      InsertTreeNodeVisible( data, tn1, ln2, CTN( INSERT_POS_HEAD ) );
      NLAddToTable( data, &ln2->ln_Table, tn1 );
    }
    else
    {
      Insert( (struct List *)&ln2->ln_List, (struct Node *)tn1, insnode2 );
      InsertTreeNodeVisible( data, tn1, ln2, CTN( insnode2 ) );
      NLAddToTable( data, &ln2->ln_Table, tn1 );
    }

    /*
    **  Insert second entry in the list.
    */
    if ( !insnode1 )
    {
      if ( !insnode2 && ( pos1 > pos2 ) )
      {
        Insert( (struct List *)&ln1->ln_List, (struct Node *)tn2, (struct Node *)tn1 );
        InsertTreeNodeVisible( data, tn2, ln1, tn1 );
        NLAddToTable( data, &ln1->ln_Table, tn2 );
      }
      else
      {
        AddHead( (struct List *)&ln1->ln_List, (struct Node *)tn2 );
        InsertTreeNodeVisible( data, tn2, ln1, CTN( INSERT_POS_HEAD ) );
        NLAddToTable( data, &ln1->ln_Table, tn2 );
      }
    }
    else
    {
      Insert( (struct List *)&ln1->ln_List, (struct Node *)tn2, insnode1 );
      InsertTreeNodeVisible( data, tn2, ln1, CTN( insnode1 ) );
      NLAddToTable( data, &ln1->ln_Table, tn2 );
    }

    DoRefresh( data );

    if ( tn1 == data->ActiveNode )
      nnset( obj, MUIA_NListtree_Active, tn1 );
    else if ( tn2 == data->ActiveNode )
      nnset( obj, MUIA_NListtree_Active, tn2 );

    DoQuiet( data, FALSE );
  }

  return( 0 );
}



/****** NListtree.mcc/MUIM_NListtree_Move ************************************
*
*   NAME
*
* MUIM_NListtree_Move -- Move an entry to the specified position. (V1)
*
*
*   SYNOPSIS
*
* DoMethod(obj, MUIM_NListtree_Move,
*   struct MUI_NListtree_TreeNode *oldlistnode,
*   struct MUI_NListtree_TreeNode *oldtreenode,
*   struct MUI_NListtree_TreeNode *newlistnode,
*   struct MUI_NListtree_TreeNode *newtreenode,
*   ULONG flags);
*
*
*   FUNCTION
*
* Move an entry to the position after a defined node.
*
*
*   INPUTS
*
* oldlistnode - Specify the node which list is used to find the
*         entry. The search is started at the head of this
*         list.
*
*   MUIV_NListtree_Move_OldListNode_Root
*     The root list is used as the starting point.
*
*   MUIV_NListtree_Move_OldListNode_Active
*     The active list (the list of the active node) is used as
*     the starting point.
*
* oldtreenode - Specify the node which should be moved.
*
*   MUIV_NListtree_Move_OldTreeNode_Head
*     The head of the list defined in 'oldlistnode' is moved.
*
*   MUIV_NListtree_Move_OldTreeNode_Tail
*     The tail of the list defined in 'oldlistnode' is moved.
*
*   MUIV_NListtree_Move_OldTreeNode_Active
*     The active node is moved.
*
* newlistnode - Specify the node which list is used to find the
*         entry. The search is started at the head of this
*         list.
*
*   MUIV_NListtree_Move_NewListNode_Root
*     The root list.
*
*   MUIV_NListtree_Move_NewListNode_Active
*     The list of the active node.
*
* newtreenode - This node is the predecessor of the entry which is
*         inserted.
*
*   MUIV_NListtree_Move_NewTreeNode_Head
*     The node is moved to the head of the list defined in
*     'newlistnode'.
*
*   MUIV_NListtree_Move_NewTreeNode_Tail
*     The node is moved to the tail of the list defined in
*     'newlistnode'.
*
*   MUIV_NListtree_Move_NewTreeNode_Active:
*     The node is moved to one entry after the active node.
*
*   MUIV_NListtree_Move_NewTreeNode_Sorted:
*     The node is moved to the list using the sort hook.
*
* flags -   Some flags to adjust moving.
*
*   MUIV_NListtree_Move_Flag_KeepStructure
*     The full tree structure from the selected entry to
*     the root list is moved (created at destination).
*
*
*   RESULT
*
*   EXAMPLE
*
*   // Move an entry to the head of another list-node.
*   DoMethod(obj,
*     MUIV_NListtree_Move_OldListNode_Active,
*     MUIV_NListtree_Move_OldTreeNode_Active,
*     somelistmode,
*     MUIV_NListtree_Move_NewTreeNode_Head,
*     0);
*
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
* MUIM_NListtree_Insert, MUIM_NListtree_Remove,
* MUIM_NListtree_Exchange, MUIA_NListtree_CompareHook,
* MUIM_NListtree_Copy
*
******************************************************************************
*
*/
IPTR _NListtree_Move(struct IClass *cl, Object *obj, struct MUIP_NListtree_Move *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct MUI_NListtree_ListNode *ln1, *ln2;
  struct MUI_NListtree_TreeNode *tn1, *tn2;

  D(DBF_LISTTREE, "MUIM_NListtree_Move");

  /*
  **  Handle all special events.
  */
  switch( (IPTR)msg->OldListNode )
  {
    case MUIV_NListtree_Move_OldListNode_Root:
      ln1 = &data->RootList;
      break;

    case MUIV_NListtree_Move_OldListNode_Active:
      ln1 = data->ActiveList;
      break;

    default:
      ln1 = CLN( msg->OldListNode );
      break;
  }

  switch( (IPTR)msg->OldTreeNode )
  {
    case MUIV_NListtree_Move_OldTreeNode_Head:
      tn1 = CTN( GetHead( (struct List *)&ln1->ln_List ) );
      break;

    case MUIV_NListtree_Move_OldTreeNode_Tail:
      tn1 = CTN( GetTail( (struct List *)&ln1->ln_List ) );
      break;

    case MUIV_NListtree_Move_OldTreeNode_Active:
      tn1 = data->ActiveNode;
      break;

    default:
      tn1 = msg->OldTreeNode;
      break;
  }

  switch( (IPTR)msg->NewListNode )
  {
    case MUIV_NListtree_Move_NewListNode_Root:
      ln2 = &data->RootList;
      break;

    case MUIV_NListtree_Move_NewListNode_Active:
      ln2 = data->ActiveList;
      break;

    default:
      ln2 = CLN( msg->NewListNode );
      break;
  }

  switch( (IPTR)msg->NewTreeNode )
  {
    case MUIV_NListtree_Move_NewTreeNode_Head:
      tn2 = CTN( INSERT_POS_HEAD );
      break;

    case MUIV_NListtree_Move_NewTreeNode_Tail:
      tn2 = CTN( INSERT_POS_TAIL );
      break;

    case MUIV_NListtree_Move_NewTreeNode_Active:
      tn2 = data->ActiveNode;
      break;

    case MUIV_NListtree_Move_NewTreeNode_Sorted:
      tn2 = GetInsertNodeSorted( data, ln2, tn1 );
      break;

    default:
      tn2 = msg->NewTreeNode;
      break;
  }


  if ( tn1 != tn2 )
  {
    DoQuiet( data, TRUE );

    //D(bug( "1: L: %s, N: %s  -  2: L: %s, N: %s\n", ln1->ln_Name, tn1->tn_Name, ln2->ln_Name, ( (ULONG)tn2 == INSERT_POS_TAIL ) ? "TAIL" : ( (ULONG)tn2 == INSERT_POS_HEAD ) ? "HEAD" : tn2->tn_Name ) );

    /*
    **  Remove the entry visible and from list.
    */
    NLRemoveFromTable( data, &ln1->ln_Table, tn1 );
    RemoveTreeNodeVisible( data, tn1, NULL );
    Remove( (struct Node *)&tn1->tn_Node );


    /*
    **  Create structure identical to source.
    */
    if(isFlagSet(msg->Flags, MUIV_NListtree_Move_Flag_KeepStructure))
    {
      tn1 = CTN( CreateParentStructure( data, msg->MethodID, &ln2, &tn2, tn1, 0 ) );
    }

    if ( tn1 )
    {
      /*
      **  Insert entry into list.
      */
      InsertTreeNode( data, ln2, tn1, tn2 );
    }

    DoRefresh( data );

    if ( tn1 == data->ActiveNode )
      nnset( obj, MUIA_NListtree_Active, tn1 );

    DoQuiet( data, FALSE );
  }

  D(DBF_LISTTREE, "MUIM_NListtree_Move End");

  return( 0 );
}



/****** NListtree.mcc/MUIM_NListtree_Copy ************************************
*
*   NAME
*
* MUIM_NListtree_Copy -- Copy an entry (create it) to the spec. pos. (V1)
*
*
*   SYNOPSIS
*
* DoMethod(obj, MUIM_NListtree_Copy,
*   struct MUI_NListtree_TreeNode *srclistnode,
*   struct MUI_NListtree_TreeNode *srctreenode,
*   struct MUI_NListtree_TreeNode *destlistnode,
*   struct MUI_NListtree_TreeNode *desttreenode,
*   ULONG flags);
*
*
*   FUNCTION
*
* Copy an entry to the position after a defined node. The complete
* child structure will be copied.
*
*
*   INPUTS
*
* srclistnode - Specify the node which list is used to find the
*         entry. The search is started at the head of this
*         list.
*
*   MUIV_NListtree_Copy_SourceListNode_Root
*     The root list is used as the starting point.
*
*   MUIV_NListtree_Copy_SourceListNode_Active
*     The active list (the list of the active node) is used as
*     the starting point.
*
* srctreenode - Specifies the node which should be copied.
*
*   MUIV_NListtree_Copy_SourceTreeNode_Head
*     The head of the list defined in 'srclistnode' is copied.
*
*   MUIV_NListtree_Copy_SourceTreeNode_Tail
*     The tail of the list defined in 'srclistnode' is copied.
*
*   MUIV_NListtree_Copy_SourceTreeNode_Active
*     The active node is copied.
*
* destlistnode -  Specify the node which list is used to find the
*         entry. The search is started at the head of this
*         list.
*
*   MUIV_NListtree_Copy_DestListNode_Root
*     The root list.
*
*   MUIV_NListtree_Copy_DestListNode_Active
*     The list of the active node.
*
* desttreenode -  This node is the predecessor of the entry which is
*         inserted.
*
*   MUIV_NListtree_Copy_DestTreeNode_Head
*     The node is copied to the head of the list defined in
*     'destlistnode'.
*
*   MUIV_NListtree_Copy_DestTreeNode_Tail
*     The node is copied to the tail of the list defined in
*     'destlistnode'.
*
*   MUIV_NListtree_Copy_DestTreeNode_Active:
*     The node is copied to one entry after the active node.
*
*   MUIV_NListtree_Copy_DestTreeNode_Sorted:
*     The node is copied to the list using the sort hook.
*
* flags -   Some flags to adjust moving.
*
*   MUIV_NListtree_Copy_Flag_KeepStructure
*     The full tree structure from the selected entry to
*     the root list is copied (created) at destination.
*
*
*   RESULT
*
*   EXAMPLE
*
*   // Copy the active entry to the head of
*   // another list node.
*   DoMethod(obj,
*     MUIV_NListtree_Copy_SourceListNode_Active,
*     MUIV_NListtree_Copy_SourceTreeNode_Active,
*     anylistnode,
*     MUIV_NListtree_Copy_DestTreeNode_Head,
*     0);
*
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
* MUIM_NListtree_Insert, MUIM_NListtree_Remove,
* MUIM_NListtree_Exchange, MUIA_NListtree_CompareHook,
* MUIM_NListtree_Move
*
*
******************************************************************************
*
*/
IPTR _NListtree_Copy(struct IClass *cl, Object *obj, struct MUIP_NListtree_Copy *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct MUI_NListtree_ListNode *ln1, *ln2;
  struct MUI_NListtree_TreeNode *tn1, *tn2;


  /*
  **  Handle all special events.
  */
  switch( (IPTR)msg->SourceListNode )
  {
    case MUIV_NListtree_Copy_SourceListNode_Root:
      ln1 = &data->RootList;
      break;

    case MUIV_NListtree_Copy_SourceListNode_Active:
      ln1 = data->ActiveList;
      break;

    default:
      ln1 = CLN( msg->SourceListNode );
      break;
  }

  switch( (IPTR)msg->SourceTreeNode )
  {
    case MUIV_NListtree_Copy_SourceTreeNode_Head:
      tn1 = CTN( GetHead( (struct List *)&ln1->ln_List ) );
      break;

    case MUIV_NListtree_Copy_SourceTreeNode_Tail:
      tn1 = CTN( GetTail( (struct List *)&ln1->ln_List ) );
      break;

    case MUIV_NListtree_Copy_SourceTreeNode_Active:
      tn1 = data->ActiveNode;
      break;

    default:
      tn1 = msg->SourceTreeNode;
      break;
  }

  switch( (IPTR)msg->DestListNode )
  {
    case MUIV_NListtree_Copy_DestListNode_Root:
      ln2 = &data->RootList;
      break;

    case MUIV_NListtree_Copy_DestListNode_Active:
      ln2 = data->ActiveList;
      break;

    default:
      ln2 = CLN( msg->DestListNode );
      break;
  }

  switch( (IPTR)msg->DestTreeNode )
  {
    case MUIV_NListtree_Copy_DestTreeNode_Head:
      tn2 = CTN( INSERT_POS_HEAD );
      break;

    case MUIV_NListtree_Copy_DestTreeNode_Tail:
      tn2 = CTN( INSERT_POS_TAIL );
      break;

    case MUIV_NListtree_Copy_DestTreeNode_Active:
      tn2 = data->ActiveNode;
      break;

    case MUIV_NListtree_Copy_DestTreeNode_Sorted:
      tn2 = GetInsertNodeSorted( data, ln2, tn1 );
      break;

    default:
      tn2 = msg->DestTreeNode;
      break;
  }


  if ( tn1 != tn2 )
  {
    struct MUI_NListtree_TreeNode *savetn1 = tn1;

    //D(bug( "1: L: %s, N: %s  -  2: L: %s, N: %s\n", ln1->ln_Name, tn1->tn_Name, ln2->ln_Name, ( (ULONG)tn2 == INSERT_POS_TAIL ) ? "TAIL" : tn2->tn_Name ) );

    /*
    **  Create structure identical to source.
    */
    if(isFlagSet(msg->Flags, MUIV_NListtree_Copy_Flag_KeepStructure))
    {
      tn1 = CTN( CreateParentStructure( data, msg->MethodID, &ln2, &tn2, tn1, 0 ) );
    }
    else
    {
      tn1 = DuplicateNode(data, tn1);
    }

    tn1 = CreateChildStructure( data, ln2, CLN( tn1 ), CLN( savetn1 ), 0 );

    DoRefresh( data );

    /*
    if ( tn1 == data->ActiveNode )
      nnset( obj, MUIA_NListtree_Active, tn1 );
    */
  }

  return( 0 );
}



/****** NListtree.mcc/MUIM_NListtree_Rename **********************************
*
*   NAME
*
* MUIM_NListtree_Rename -- Rename the specified node. (V1)
*
*
*   SYNOPSIS
*
* struct MUI_NListtree_TreeNode *treenode =
*   DoMethod(obj, MUIM_NListtree_Rename,
*     struct MUI_NListtree_TreeNode *treenode,
*     STRPTR newname, ULONG flags);
*
*
*   FUNCTION
*
* Rename the specified node.
*
* If you want to rename the tn_User field (see flags below), the construct
* and destruct hooks are used!
* If you have not specified these hooks, only the pointers will be copied.
*
*
*   INPUTS
*
* treenode -  Specifies the node which should be renamed.
*
*   MUIV_NListtree_Rename_TreeNode_Active:
*     Rename the active tree node.
*
* newname - The new name or pointer.
*
* flags:
*
*   MUIV_NListtree_Rename_Flag_User
*     The tn_User field is renamed.
*
*   MUIV_NListtree_Rename_Flag_NoRefresh
*     The list entry will not be refreshed.
*
*
*   RESULT
*
* Returns the pointer of the renamed tree node.
*
*
*   EXAMPLE
*
*   // Rename the active tree node.
*   DoMethod(obj, MUIM_NListtree_Rename,
*     MUIV_NListtree_Rename_TreeNode_Active,
*     "Very new name", 0);
*
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
* MUIA_NListtree_ConstructHook, MUIA_NListtree_DestructHook
*
*
******************************************************************************
*
*/
IPTR _NListtree_Rename(struct IClass *cl, Object *obj, struct MUIP_NListtree_Rename *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct MUI_NListtree_TreeNode *tn;


  /*
  **  Handle special events.
  */
  if ( (IPTR)msg->TreeNode == (IPTR)MUIV_NListtree_Rename_TreeNode_Active )
    tn = data->ActiveNode;
  else
    tn = msg->TreeNode;

  /*
  **  User wants to rename the user field instead of name field.
  */
  if(isFlagSet(msg->Flags, MUIV_NListtree_Rename_Flag_User))
  {
    //D(bug( "Node: 0x%08lx - %s - Renaming user field\n", tn, tn->tn_Name ) );

    DoMethod(obj, MUIM_NListtree_Destruct, tn->tn_Name, tn->tn_User, data->TreePool, 0);

    /*
    **  Construct new user data
    */
    tn->tn_User = (APTR)DoMethod(obj, MUIM_NListtree_Construct, tn->tn_Name, msg->NewName, data->TreePool, 0);
  }
  else
  {
    //D(bug( "Node: 0x%08lx - %s - Renaming name field from \"%s\" to \"%s\"\n", tn, tn->tn_Name, tn->tn_Name, msg->NewName ) );

    /*
    **  Should we duplicate the supplied node name?
    */
    if(isFlagSet(data->Flags, NLTF_DUPNODENAMES))
    {
      int len = strlen( msg->NewName ) + 1;

      FreeVecPooled( data->TreePool, tn->tn_Name );

      tn->tn_Name = (STRPTR)AllocVecPooled( data->TreePool, len );
      strlcpy( tn->tn_Name, msg->NewName, len );
      SET_FLAG(tn->tn_IFlags, TNIF_ALLOCATED);
    }
    else
      tn->tn_Name = msg->NewName;
  }

  if(isFlagClear(msg->Flags, MUIV_NListtree_Rename_Flag_NoRefresh) && isFlagSet(tn->tn_IFlags, TNIF_VISIBLE))
    DoMethod( obj, MUIM_NList_Redraw, GetVisualPos( data, tn ) );

  return( (IPTR)tn );
}



/****** NListtree.mcc/MUIM_NListtree_FindName ********************************
*
*   NAME
*
* MUIM_NListtree_FindName -- Find node using name match. (V1)
*
*
*   SYNOPSIS
*
* struct MUI_NListtree_TreeNode *treenode =
*   DoMethod(obj, MUIM_NListtree_FindName,
*     struct MUI_NListtree_TreeNode *listnode,
*     STRPTR name, ULONG flags);
*
*
*   FUNCTION
*
* Find a node which name matches the specified one using the list node as
* start point..
*
*
*   INPUTS
* listnode -  Specify the node which list is used to find the name.
*
*   MUIV_NListtree_FindName_ListNode_Root
*     Use the root list as the base list.
*
*   MUIV_NListtree_FindName_ListNode_Active
*     Use the list of the active node as the base.
*
* name -    Specify the name of the node to find. But you can search
*       for anything in tn_Name or tn_User field here by simply
*       supplying the searched data and handling it in your
*       own FindNameHook.
*
* flags:
*
*   MUIV_NListtree_FindName_Flag_SameLevel
*     Only nodes on the same level are affected.
*
*   MUIV_NListtree_FindName_Flag_Visible
*     The node is only returned if it is visible (only visible
*     entries are checked).
*
*   MUIV_NListtree_FindName_Flag_Activate
*     If found, the entry will be activated.
*
*   MUIV_NListtree_FindName_Flag_Selected
*     Find only selected nodes.
*
*   MUIV_NListtree_FindName_Flag_StartNode
*     The specified entry in listnode is the start point for
*     search and must not be a list node. It can also be a
*     normal entry.
*
*   RESULT
*
* Returns the found node if available, NULL otherwise.
*
*
*   EXAMPLE
*
*   // Find 2nd node by name.
*   struct MUI_NListtree_TreeNode *treenode =
*     DoMethod(obj, MUIM_NListtree_FindName,
*       listnode, "2nd node",
*       MUIV_NListtree_FindName_SameLevel|
*       MUIV_NListtree_FindName_Visible);
*
*   if ( treenode == NULL )
*   {
*     PrintToUser( "No matching entry found." );
*   }
*
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
* MUIM_NListtree_FindUserData, MUIM_NListtree_GetEntry,
* MUIA_NListtree_FindNameHook
*
*
******************************************************************************
*
*/
IPTR _NListtree_FindName(struct IClass *cl, Object *obj, struct MUIP_NListtree_FindName *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct MUI_NListtree_ListNode *ln;
  struct MUI_NListtree_TreeNode *tn;


  /*
  **  Handle special events.
  */
  switch( (IPTR)msg->ListNode )
  {
    case MUIV_NListtree_FindName_ListNode_Root:
      ln = &data->RootList;
      break;

    case MUIV_NListtree_FindName_ListNode_Active:
      ln = data->ActiveList;
      break;

    default:
      ln = CLN( msg->ListNode );
      break;
  }

  //D(bug( "StartNode: 0x%08lx - %s - Searching for \"%s\"\n", ln, ln->ln_Name, msg->Name ) );

  if(isFlagSet(msg->Flags, MUIV_NListtree_FindName_Flag_Reverse))
  {
    if((tn = FindTreeNodeByNameRev(data, ln, msg->Name, msg->Flags)))
      if(isFlagSet(msg->Flags, MUIV_NListtree_FindName_Flag_Activate))
        nnset( obj, MUIA_NListtree_Active, tn );
  }
  else
  {
    if((tn = FindTreeNodeByName(data, ln, msg->Name, msg->Flags)))
      if(isFlagSet(msg->Flags, MUIV_NListtree_FindName_Flag_Activate))
        nnset( obj, MUIA_NListtree_Active, tn );
  }

  return( (IPTR)tn );
}



/****** NListtree.mcc/MUIM_NListtree_FindUserData ****************************
*
*   NAME
*
* MUIM_NListtree_FindUserData -- Find node upon user data. (V1)
*
*
*   SYNOPSIS
*
* struct MUI_NListtree_TreeNode *treenode =
*   DoMethod(obj, MUIM_NListtree_FindUserData,
*     struct MUI_NListtree_TreeNode *listnode,
*     APTR userdata, ULONG flags);
*
*
*   FUNCTION
*
* Find a node which user data matches the specified one using the list
* node as start point..
* This method is designed as a second possibility for searching.
* Because you are able to search for anything, you may set special
* hooks for searching two different fields in two different hooks with
* these two methods.
*
*
*   INPUTS
* listnode -  Specify the node which list is used to find the user data.
*
*   MUIV_NListtree_FindUserData_ListNode_Root
*     Use the root list as the base list.
*
*   MUIV_NListtree_FindUserData_ListNode_Active
*     Use the list of the active node as the base.
*
* userdata -  Specify the user data of the node to find. You can search
*       for anything in tn_Name or tn_User field here by simply
*       supplying the searched data and handling it in your
*       own FindUserDataHook.
*
* flags:
*
*   MUIV_NListtree_FindUserData_Flag_SameLevel
*     Only nodes on the same level are affected.
*
*   MUIV_NListtree_FindUserData_Flag_Visible
*     The node is only returned if it is visible (only visible
*     entries are checked).
*
*   MUIV_NListtree_FindUserData_Flag_Activate
*     If found, the entry will be activated.
*
*   MUIV_NListtree_FindUserData_Flag_Selected
*     Find only selected nodes.
*
*   MUIV_NListtree_FindUserData_Flag_StartNode
*     The specified entry in listnode is the start point for
*     search and must not be a list node. It can also be a
*     normal entry.
*
*   RESULT
*
* Returns the found node if available, NULL otherwise.
*
*
*   EXAMPLE
*
*   // Find node by user data.
*   struct MUI_NListtree_TreeNode *treenode =
*     DoMethod(obj, MUIM_NListtree_FindUserData,
*       listnode, "my data",
*       MUIV_NListtree_FindUserData_SameLevel|
*       MUIV_NListtree_FindUserData_Visible);
*
*   if ( treenode == NULL )
*   {
*     PrintToUser( "No matching entry found." );
*   }
*
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
* MUIM_NListtree_FindName, MUIM_NListtree_GetEntry,
* MUIA_NListtree_FindUserDataHook
*
*
******************************************************************************
*
*/
IPTR _NListtree_FindUserData(struct IClass *cl, Object *obj, struct MUIP_NListtree_FindUserData *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct MUI_NListtree_ListNode *ln;
  struct MUI_NListtree_TreeNode *tn;


  /*
  **  Handle special events.
  */
  switch( (IPTR)msg->ListNode )
  {
    case MUIV_NListtree_FindUserData_ListNode_Root:
      ln = &data->RootList;
      break;

    case MUIV_NListtree_FindUserData_ListNode_Active:
      ln = data->ActiveList;
      break;

    default:
      ln = CLN( msg->ListNode );
      break;
  }

  //D(bug( "StartNode: 0x%08lx - %s - Searching for \"%s\"\n", ln, ln->ln_Name, (STRPTR)msg->User ) );

  if(isFlagSet(msg->Flags, MUIV_NListtree_FindUserData_Flag_Reverse))
  {
    if((tn = FindTreeNodeByUserDataRev(data, ln, msg->User, msg->Flags)))
      if(isFlagSet(msg->Flags, MUIV_NListtree_FindUserData_Flag_Activate))
        nnset( obj, MUIA_NListtree_Active, tn );
  }
  else
  {
    if((tn = FindTreeNodeByUserData(data, ln, msg->User, msg->Flags)))
      if(isFlagSet(msg->Flags, MUIV_NListtree_FindUserData_Flag_Activate))
        nnset( obj, MUIA_NListtree_Active, tn );
  }

  return( (IPTR)tn );
}



/****** NListtree.mcc/MUIM_NListtree_GetEntry ********************************
*
*   NAME
*
* MUIM_NListtree_GetEntry -- Get another node in relation to this. (V1)
*
*
*   SYNOPSIS
*
* struct MUI_NListtree_TreeNode *rettreenode =
*   DoMethod(obj, MUIM_NListtree_GetEntry,
*     struct MUI_NListtree_TreeNode *treenode,
*     LONG pos, ULONG flags);
*
*
*   FUNCTION
*
* Get another node in relation to the specified list or node.
*
*
*   INPUTS
*
* treenode -  Define the node which is used to find another one.
*       This can also be a list node, if the position is
*       related to a list.
*
*   MUIV_NListtree_GetEntry_ListNode_Root
*     The root list is used.
*
*   MUIV_NListtree_GetEntry_ListNode_Active:
*     The list with the active entry is used.
*
* pos - The relative position of the node 'treenode'.
*
*   MUIV_NListtree_GetEntry_Position_Head
*     The head of the list is returned.
*
*   MUIV_NListtree_GetEntry_Position_Tail
*     The tail of the list is returned.
*
*   MUIV_NListtree_GetEntry_Position_Active
*     The active node is returned. If there is no active entry,
*     NULL is returned.
*
*   MUIV_NListtree_GetEntry_Position_Next
*     The node next to the specified node is returned. Returns NULL
*     if there is no next entry.
*
*   MUIV_NListtree_GetEntry_Position_Previous
*     The node right before the specified node is returned.
*     Returns NULL if there is no previous entry (if 'treenode'
*     is the head of the list.
*
*   MUIV_NListtree_GetEntry_Position_Parent
*     The list node of the specified 'treenode' is returned.
*
* flags:
*
*   MUIV_NListtree_GetEntry_Flag_SameLevel:
*     Only nodes in the same level are affected.
*
*   MUIV_NListtree_GetEntry_Flag_Visible:
*     The position is counted on visible entries only.
*
*
*   RESULT
*
* Returns the requested node if available, NULL otherwise.
*
*
*   EXAMPLE
*
*   // Get the next entry.
*   struct MUI_NListtree_TreeNode *treenode =
*     DoMethod(obj, MUIM_NListtree_GetEntry, treenode,
*     MUIV_NListtree_GetEntry_Position_Next, 0);
*
*   if ( treenode )
*   {
*     PrintToUser( "Next entry found!" );
*   }
*
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
* MUIM_NList_GetEntry
*
*
******************************************************************************
*
*/
IPTR _NListtree_GetEntry(struct IClass *cl, Object *obj, struct MUIP_NListtree_GetEntry *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct MUI_NListtree_TreeNode *tn = NULL;
  struct MUI_NListtree_ListNode *ln;
  LONG pos = 0;

  /*
  **  Handle special events.
  */
  switch( (IPTR)msg->Node )
  {
    case MUIV_NListtree_GetEntry_ListNode_Root:
      ln = &data->RootList;
      break;

    case MUIV_NListtree_GetEntry_ListNode_Active:
      ln = data->ActiveList;
      break;

    case -1:
      ln = data->ActiveList;
      break;

    default:
      ln = CLN( msg->Node );
      break;
  }

  switch( (IPTR)(SIPTR)msg->Position )
  {
    case MUIV_NListtree_GetEntry_Position_Head:

      if(isFlagSet(ln->ln_Flags, TNF_LIST))
        tn = CTN( GetHead( (struct List *)&ln->ln_List ) );
      break;

    case MUIV_NListtree_GetEntry_Position_Tail:

      if(isFlagSet(ln->ln_Flags, TNF_LIST))
        tn = CTN( GetTail( (struct List *)&ln->ln_List ) );
      break;

    case MUIV_NListtree_GetEntry_Position_Active:

      tn = data->ActiveNode;
      break;

    case MUIV_NListtree_GetEntry_Position_Next:

      tn = CTN( GetSucc( (struct Node *)&ln->ln_Node ) );
      break;

    case MUIV_NListtree_GetEntry_Position_Previous:

      tn = CTN( GetPred( (struct Node *)&ln->ln_Node ) );
      break;

    case MUIV_NListtree_GetEntry_Position_Parent:

      tn = GetParent( CTN( ln ) );
      break;

    case MUIV_NListtree_GetEntry_Position_Root:

      tn = CTN( (APTR)&data->RootList );
      break;

    default:
      tn = GetEntryByTotalPos( ln, msg->Position, &pos, msg->Flags );
      break;
  }

  //D(bug( "ListNode: 0x%08lx - Pos: 0x%08lx, Flags: 0x%08lx ==> Entry: 0x%08lx\n", msg->Node, msg->Position, msg->Flags, tn ) );

  return( (IPTR)tn );
}



/****** NListtree.mcc/MUIM_NListtree_GetNr ***********************************
*
*   NAME
*
* MUIM_NListtree_GetNr -- Get the position number of a tree node. (V1)
*
*
*   SYNOPSIS
*
* ULONG number = DoMethod(obj, MUIM_NListtree_GetNr,
*   struct MUI_NListtree_TreeNode *treenode, ULONG flags);
*
*
*   FUNCTION
*
* Get the position number of the specified tree node.
*
*
*   INPUTS
*
* treenode -  Specify the node to count the position of.
*
*   MUIV_NListtree_GetNr_TreeNode_Active:
*     The position is counted related to the active node.
*
*   MUIV_NListtree_GetNr_TreeNode_Root:
*     The position is counted related to the root list.
*
* flags:
*
*   MUIV_NListtree_GetNr_Flag_CountAll
*     Returns the number of all entries.
*
*   MUIV_NListtree_GetNr_Flag_CountLevel
*     Returns the number of entries of the list the
*     specified node is in.
*
*   MUIV_NListtree_GetNr_Flag_CountList
*     Returns the number of entries in the specified
*     treenode if it holds other entries/is a list.
*
*   MUIV_NListtree_GetNr_Flag_ListEmpty
*     Returns TRUE if the specified list node is empty.
*
*   MUIV_NListtree_GetNr_Flag_Visible
*     Returns the position number of an visible entry. -1 if the
*     entry is invisible. The position is counted on visible entries
*     only.
*
*
*   RESULT
*
*   EXAMPLE
*
*   // Check if the active (list) node is empty.
*   ULONG empty = DoMethod(obj, MUIM_NListtree_GetNr,
*     MUIV_NListtree_GetNr_TreeNode_Active,
*     MUIV_NListtree_GetNr_Flag_ListEmpty);
*
*   if ( empty == TRUE )
*   {
*     AddThousandEntries();
*   }
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
* MUIM_NListtree_GetEntry
*
******************************************************************************
*
*/
IPTR _NListtree_GetNr(struct IClass *cl, Object *obj, struct MUIP_NListtree_GetNr *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct MUI_NListtree_TreeNode *tn;
  struct MUI_NListtree_ListNode *ln;
  IPTR ret = 0;
  LONG pos = 0;

  D(DBF_LISTTREE, "GetNr: NListtree entries %ld",data->NumEntries);

  /*
  **  Handle special events.
  */
  if ( (IPTR)msg->TreeNode == (IPTR)MUIV_NListtree_GetNr_TreeNode_Active )
    tn = data->ActiveNode;
  else if ( (IPTR)msg->TreeNode == (IPTR)MUIV_NListtree_GetNr_TreeNode_Root )
    tn = CTN((APTR)&data->RootList);
  else
    tn = msg->TreeNode;

  ln = CLN( GetParent( tn ) );

  if(isFlagSet(msg->Flags, MUIV_NListtree_GetNr_Flag_ListEmpty))
  {
    if(isFlagSet(tn->tn_Flags, TNF_LIST))
      ret = IsListEmpty( ((struct List *)&CLN(tn)->ln_List) );
  }

  else if(isFlagSet(msg->Flags, MUIV_NListtree_GetNr_Flag_CountList))
  {
    if(isFlagSet(tn->tn_Flags, TNF_LIST))
      ret = CLN(tn)->ln_Table.tb_Entries;
  }

  else if(ln && isFlagSet(msg->Flags, MUIV_NListtree_GetNr_Flag_CountLevel))
  {
    ret = ln->ln_Table.tb_Entries;
  }

  else if(isFlagSet(msg->Flags, MUIV_NListtree_GetNr_Flag_CountAll))
  {
    ret = data->NumEntries;
  }

  else if(isFlagSet(msg->Flags, MUIV_NListtree_GetNr_Flag_Visible))
  {
    ret = (IPTR)GetVisualPos( data, tn );
  }

  else
  {
    ret = (IPTR)-1;

    if ( GetEntryPos( &data->RootList, tn, &pos ) )
      ret = pos;
  }

  //D(bug( "Node: 0x%08lx - %s - Flags: 0x%08lx ==> Nr: %ld\n", tn, tn->tn_Name, msg->Flags, ret ) );

  return( (IPTR)ret );
}



/****** NListtree.mcc/MUIM_NListtree_Sort ************************************
*
*   NAME
*
* MUIM_NListtree_Sort -- Sort the specified list node. (V1)
*
*
*   SYNOPSIS
*
* DoMethod(obj, MUIM_NListtree_Sort,
*   struct MUI_NListtree_TreeNode *listnode,
*   ULONG flags);
*
*
*   FUNCTION
*
* Sort the specified list node using the sort hook.
*
*
*   INPUTS
*
* listnode -  List node to sort.
*
*   MUIV_NListtree_Sort_ListNode_Root
*     Sort the root list.
*
*   MUIV_NListtree_Sort_ListNode_Active
*     Sort the list node of the active entry.
*
*   MUIV_NListtree_Sort_TreeNode_Active
*     Sorts the childs of the active entry if a list.
*
* flags -   Control the part where sorting is done.
*
*   MUIV_NListtree_Sort_Flag_RecursiveOpen
*     Sort the list recursive. All open child nodes of the
*     node specified in 'listnode' will be sorted too.
*
*   MUIV_NListtree_Sort_Flag_RecursiveAll
*     Sort the list recursive with ALL child nodes of the
*     node specified in 'listnode'.
*
*   RESULT
*
*   EXAMPLE
*
*   // Sort the list of the active node.
*   DoMethod(obj, MUIM_NListtree_Sort,
*     MUIV_NListtree_Sort_ListNode_Active, 0);
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
* MUIA_NListtree_SortHook
*
******************************************************************************
*
*/
IPTR _NListtree_Sort(struct IClass *cl, Object *obj, struct MUIP_NListtree_Sort *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct MUI_NListtree_ListNode *ln;
  LONG pos;

  /*
  **  Handle special events.
  */
  switch( (IPTR)msg->ListNode )
  {
    case MUIV_NListtree_Sort_ListNode_Active:
      ln = data->ActiveList;
      break;

    case MUIV_NListtree_Sort_ListNode_Root:
      ln = &data->RootList;
      break;

    case MUIV_NListtree_Sort_TreeNode_Active:
      ln = data->ActiveList;

      if ( data->ActiveNode )
      {
        if(isFlagSet(data->ActiveNode->tn_Flags, TNF_LIST))
          ln = CLN( data->ActiveNode );
      }
      break;

    default:
      ln = CLN( msg->ListNode );
      break;
  }

  pos = GetVisualPos( data, CTN( ln ) ) + 1;

  DoQuiet( data, TRUE );
  DeactivateNotify( data );

  ListNode_Sort( ln, data, msg->Flags );
  ReplaceTreeVisibleSort( data, CTN( ln ), &pos );

  ActivateNotify( data );
  DoQuiet( data, FALSE );

  /* sba: the active note could be changed, but the notify calling was disabled */
  DoMethod(data->Obj, MUIM_NListtree_GetListActive, 0);

/*    nnset( obj, MUIA_NListtree_Active, CTN( data->ActiveNode ) );*/

  return( 0 );
}



/****** NListtree.mcc/MUIM_NListtree_TestPos *********************************
*
*   NAME
*
* MUIM_NListtree_TestPos -- Get information about entry at x/y pos. (V1)
*
*
*   SYNOPSIS
*
* DoMethod(obj, MUIM_NListtree_TestPos, LONG xpos, LONG ypos,
*   struct MUI_NListtree_TestPos_Result *testposresult);
*
*
*   FUNCTION
*
* Find out some information about the currently displayed entry at a
* certain position (x/y-pos).
*
* This is very useful for Drag&Drop operations.
*
*
*   INPUTS
*
* xpos -      X-position.
* ypos -      Y-position.
* testposresult - Pointer to a valid MUI_NListtree_TestPos_Result
*         structure.
*
*
*   RESULT
*
* tpr_TreeNode -  The tree node under the requested position or NULL
*         if there is no entry displayed.
*
* The tpr_Type field contains detailed information about the relative
* position:
*
*   MUIV_NListtree_TestPos_Result_Above
*   MUIV_NListtree_TestPos_Result_Below
*   MUIV_NListtree_TestPos_Result_Onto
*   MUIV_NListtree_TestPos_Result_Sorted
*
* tpr_Column -  The column unter the specified position or -1 if
*         no valid column.
*
*
*   EXAMPLE
*
*   // Entry under the cursor?
*   struct MUI_NListtree_TestPos_Result tpres;
*
*   DoMethod(obj, MUIM_NListtree_TestPos, msg->imsg->MouseX,
*     msg->imsg->MouseY, &tpres);
*
*   if ( tpres.tpr_Entry != NULL )
*   {
*     // Do something very special here...
*   }
*
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
* MUIM_NList_TestPos
*
*
******************************************************************************
*
*/
IPTR _NListtree_TestPos( UNUSED struct IClass *cl, Object *obj, struct MUIP_NListtree_TestPos *msg)
{
  struct MUI_NListtree_TestPos_Result *res = (struct MUI_NListtree_TestPos_Result *)msg->Result;
  struct MUI_NList_TestPos_Result lres;

  lres.char_number = -2;

  DoMethod( obj, MUIM_NList_TestPos, msg->X, msg->Y, &lres );

  if ( lres.entry != -1 )
  {
    DoMethod( obj, MUIM_NList_GetEntry, lres.entry, &res->tpr_TreeNode );

    if ( lres.yoffset < -3 )    res->tpr_Type = MUIV_NListtree_TestPos_Result_Above;
    else if ( lres.yoffset > 3 )  res->tpr_Type = MUIV_NListtree_TestPos_Result_Below;
    else              res->tpr_Type = MUIV_NListtree_TestPos_Result_Onto;

    res->tpr_ListEntry  = lres.entry;
    res->tpr_ListFlags  = lres.flags;
    res->tpr_Column   = lres.column;
  }
  else
  {
    res->tpr_TreeNode = NULL;
    res->tpr_Type   = 0;

    res->tpr_ListEntry  = -1;
    res->tpr_ListFlags  = 0;
    res->tpr_Column   = -1;
  }

  //D(bug( "X: %ld, Y: %ld ==> Node: 0x%08lx - %s - YOffset: %ld, Type: 0x%08lx\n", msg->X, msg->Y, res->tpr_TreeNode, res->tpr_TreeNode->tn_Name, lres.yoffset, res->tpr_Type ) );


  return( 0 );
}



/****** NListtree.mcc/MUIM_NListtree_Redraw **********************************
*
*   NAME
*
* MUIM_NListtree_Redraw -- Redraw the specified tree node. (V1)
*
*
*   SYNOPSIS
*
* DoMethod(obj, MUIM_NListtree_Redraw,
*   struct MUI_NListtree_TreeNode *treenode, ULONG flags);
*
*
*   FUNCTION
*
* Redraw the specified entry. See special values for completeness.
*
*
*   INPUTS
*
* treenode -  The tree node to be redrawn.
*
*   MUIV_NListtree_Redraw_Active
*     Redraw the active entry.
*
*   MUIV_NListtree_Redraw_All
*     Redraw the complete visible tree.
*
*
* flags:
*
*   MUIV_NListtree_Redraw_Flag_Nr
*     The data specified in 'treenode' is the entry number,
*     not the tree node itself.
*
*
*   RESULT
*
*   EXAMPLE
*
*   // Redraw the active entry.
*   DoMethod(obj, MUIM_NListtree_Redraw,
*     MUIV_NListtree_Redraw_Active, 0);
*
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
* MUIM_NList_TestPos
*
*
******************************************************************************
*
*/
IPTR _NListtree_Redraw(struct IClass *cl, Object *obj, struct MUIP_NListtree_Redraw *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);

  D(DBF_DRAW, "redraw treenode %08lx flags %08lx", msg->TreeNode, msg->Flags);

  if(((IPTR)msg->TreeNode != (IPTR)MUIV_NListtree_Redraw_Active) && ((IPTR)msg->TreeNode != (IPTR)MUIV_NListtree_Redraw_All) && isFlagClear(msg->Flags, MUIV_NListtree_Redraw_Flag_Nr))
  {
    LONG pos = GetVisualPos(data, msg->TreeNode);

    D(DBF_DRAW, "treenode %08lx has visual position %ld", msg->TreeNode, pos);
    // redraw the given node only if it is visible
    if(pos != -1)
      DoMethod(obj, MUIM_NList_Redraw, pos);
  }
  else
  {
    D(DBF_DRAW, "EXTERNAL REDRAW REQUESTED!");
    DoMethod(obj, MUIM_NList_Redraw, (IPTR)msg->TreeNode);
  }

  return(0);
}



/****** NListtree.mcc/MUIM_NListtree_Select **********************************
*
*   NAME
*
* MUIM_NListtree_Select -- Select the specified tree node. (V1)
*
*
*   SYNOPSIS
*
* DoMethod(obj, MUIM_NListtree_Select,
*   struct MUI_NListtree_TreeNode *treenode, LONG seltype,
*   LONG selflags, LONG *state);
*
*
*   FUNCTION
*
* Select or unselect a tree entry or ask an entry about its state.
* See special values for completeness.
*
*
*   INPUTS
*
* treenode -  The tree node to be selected/unselected/asked.
*
*   MUIV_NListtree_Select_Active  For the active entry.
*   MUIV_NListtree_Select_All   For all entries.
*   MUIV_NListtree_Select_Visible For all visible entries.
*
* seltype - Type of selection/unselection/ask
*
*   MUIV_NListtree_Select_Off   Unselect entry.
*   MUIV_NListtree_Select_On    Select entry.
*   MUIV_NListtree_Select_Toggle  Toggle entries state.
*   MUIV_NListtree_Select_Ask   Just ask about the state.
*
* selflags -  Some kind of specials.
*
*   MUIV_NListtree_Select_Flag_Force
*     Adding this flag to seltype forces the selection by
*     bypassing the multi test hook.
*
* state -   Pointer to a longword. If not NULL, it will be filled
*       with the current selection state of the entry.
*
*
*   RESULT
*
*   EXAMPLE
*
*   // Select the active entry.
*   LONG retstate;
*
*   DoMethod(obj, MUIM_NListtree_Select,
*     MUIV_NListtree_Select_Active, MUIV_NListtree_Select_On,
*     0, &retstate);
*
*   // We must check this, because the multi test hook may
*   // cancel our selection.
*   if (retstate == MUIV_NListtree_Select_On) {
*     ...
*   }
*
*
*   NOTES
*
* If ( treenode == MUIV_NListtree_Select_All ) and
* ( seltype == MUIV_NListtree_Select_Ask ), state will be filled
* with the total number of selected entries.
*
* NEW for final 18.6:
* If (treenode == MUIV_NListtree_Select_Active ) and
* ( seltype == MUIV_NListtree_Select_Ask ), state will be the
* active entry, if any, or NULL.
*
* If only the active entry is selected, has a cursor mark (see
* MUIM_NListtree_NextSelected for that), you will receive 0 as
* the number of selected entries.
*
*
*   BUGS
*
*   SEE ALSO
*
* MUIA_NListtree_MultiTestHook
*
*
******************************************************************************
*
*/
IPTR _NListtree_Select(struct IClass *cl, Object *obj, struct MUIP_NListtree_Select *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  LONG state = 0;

  ENTER();

  SET_FLAG(data->Flags, NLTF_SELECT_METHOD);

  /*
  **  Handle special events.
  */
  switch( (IPTR)msg->TreeNode )
  {
    case MUIV_NListtree_Select_Active:
      if ( msg->SelType == MUIV_NListtree_Select_Ask )
      {
        state = (LONG)(SIPTR)data->ActiveNode;
      }
      else
      {
        if ( data->ActiveNode )
        {
          TreeNodeSelect( data, data->ActiveNode, msg->SelType, msg->SelFlags, &state );
        }
      }
      break;

    case MUIV_NListtree_Select_All:

      if ( msg->SelType == MUIV_NListtree_Select_Ask )
      {
        state = data->SelectedTable.tb_Entries;
      }
      else
      {
        TreeNodeSelectAll( data, &data->RootList, msg->SelType, msg->SelFlags, &state );
      }
      break;

    case MUIV_NListtree_Select_Visible:
      TreeNodeSelectVisible( data, &data->RootList, msg->SelType, msg->SelFlags, &state );
      break;

    default:
      TreeNodeSelect( data, msg->TreeNode, msg->SelType, msg->SelFlags, &state );
      break;
  }

  if ( msg->State )
  {
    *msg->State = state;
  }

  CLEAR_FLAG(data->Flags, NLTF_SELECT_METHOD);

  RETURN(state);
  return( (IPTR)state );
}



/****** NListtree.mcc/MUIM_NListtree_NextSelected ****************************
*
*   NAME
*
* MUIM_NListtree_NextSelected -- Get next selected tree node. (V1)
*
*
*   SYNOPSIS
*
* DoMethod(obj, MUIM_NListtree_NextSelected,
*   struct MUI_NListtree_TreeNode **treenode);
*
*
*   FUNCTION
*
* Iterate through the selected entries of a tree. This method steps
* through the contents of a (multi select) list tree and returns
* every entry that is currently selected. When no entry is selected
* but an entry is active, only the active entry will be returned.
*
* This behaviour will result in not returning the active entry when
* you have some other selected entries somewhere in your list. Since
* the active entry just acts as some kind of cursor mark, this seems
* to be the only sensible possibility to handle multi selection
* together with keyboard control.
*
*
*   INPUTS
*
* treenode -  A pointer to a pointer of struct MUI_NListtree_TreeNode
*       that will hold the returned entry. Must be set to
*       MUIV_NListtree_NextSelected_Start at start of iteration
*       and is set to MUIV_NListtree_NextSelected_End when
*       iteration is finished.
*
*   MUIV_NListtree_NextSelected_Start Set this to start iteration.
*   MUIV_NListtree_NextSelected_End   Will be set to this, if
*                     last selected entry reached.
*
*
*   RESULT
*
*   EXAMPLE
*
* // Iterate through a list
* struct MUI_NListtree_TreeNode *treenode;
*
* treenode = MUIV_NListtree_NextSelected_Start;
*
* for (;;)
* {
*   DoMethod(listtree, MUIM_NListtree_NextSelected, &treenode);
*
*   if (treenode==MUIV_NListtree_NextSelected_End)
*     break;
*
*   printf("selected: %s\n", treenode->tn_Name);
* }
*
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
* MUIM_NListtree_PrevSelected, MUIM_NListtree_Select
*
*
******************************************************************************
*
*/
IPTR _NListtree_NextSelected(struct IClass *cl, Object *obj, struct MUIP_NListtree_NextSelected *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  LONG curr = data->SelectedTable.tb_Current;

  //D(bug( "TreeNode: 0x%08lx ", *msg->TreeNode ) );

  if ( (IPTR)*msg->TreeNode == (IPTR)MUIV_NListtree_NextSelected_Start )
    curr = 0;
  else
    curr++;

  if ( ( curr > -1 ) && data->SelectedTable.tb_Entries )
  {
    if ( curr < data->SelectedTable.tb_Entries )
    {
      data->SelectedTable.tb_Current = curr;
      *msg->TreeNode = data->SelectedTable.tb_Table[curr];
    }
    else if(((IPTR)*msg->TreeNode == (IPTR)MUIV_NListtree_NextSelected_Start) &&
             (data->ActiveNode != MUIV_NListtree_Active_Off))
    {
      *msg->TreeNode = data->ActiveNode;
    }
    else
    {
      *msg->TreeNode = CTN( MUIV_NListtree_NextSelected_End );
    }
  }
  else
  {
    *msg->TreeNode = CTN( MUIV_NListtree_NextSelected_End );
  }

  //D(bugn( "==> Entry: 0x%08lx\n", *msg->TreeNode ) );

  return( 0 );
}


/****** NListtree.mcc/MUIM_NListtree_PrevSelected ****************************
*
*   NAME
*
* MUIM_NListtree_PrevSelected -- Get previous selected tree node. (V1)
*
*
*   SYNOPSIS
*
* DoMethod(obj, MUIM_NListtree_PrevSelected,
*   struct MUI_NListtree_TreeNode **treenode);
*
*
*   FUNCTION
*
* Iterate reverse through the selected entries of a tree. This method
* steps through the contents of a (multi select) list tree and returns
* every entry that is currently selected. When no entry is selected
* but an entry is active, only the active entry will be returned.
*
* This behaviour will result in not returning the active entry when
* you have some other selected entries somewhere in your list. Since
* the active entry just acts as some kind of cursor mark, this seems
* to be the only sensible possibility to handle multi selection
* together with keyboard control.
*
*
*   INPUTS
*
* treenode -  A pointer to a pointer of struct MUI_NListtree_TreeNode
*       that will hold the returned entry. Must be set to
*       MUIV_NListtree_PrevSelected_Start at start of iteration
*       an the end and is set to MUIV_NListtree_PrevSelected_End
*       when first selected entry is reached and iteration is
*       finished.
*
*   MUIV_NListtree_PrevSelected_Start Set this to start iteration.
*   MUIV_NListtree_PrevSelected_End   Will be set to this, if
*                     last selected entry reached.
*
*
*   RESULT
*
*   EXAMPLE
*
* // Iterate through a list (reverse)
* struct MUI_NListtree_TreeNode *treenode;
*
* treenode = MUIV_NListtree_PrevSelected_Start;
*
* for (;;)
* {
*   DoMethod(listtree, MUIM_NListtree_PrevSelected, &treenode);
*
*   if (treenode==MUIV_NListtree_PrevSelected_End)
*     break;
*
*   printf("selected: %s\n", treenode->tn_Name);
* }
*
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
* MUIM_NListtree_NextSelected, MUIM_NListtree_Select
*
*
******************************************************************************
*
*/

IPTR _NListtree_PrevSelected(struct IClass *cl, Object *obj, struct MUIP_NListtree_PrevSelected *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  LONG curr = data->SelectedTable.tb_Current;

  //D(bug( "TreeNode: 0x%08lx ", *msg->TreeNode ) );

  if((IPTR)*msg->TreeNode == (IPTR)MUIV_NListtree_PrevSelected_Start)
    curr = data->SelectedTable.tb_Entries;

  curr--;

  if ( curr < data->SelectedTable.tb_Entries )
  {
    if ( curr > -1 )
    {
      data->SelectedTable.tb_Current = curr;
      *msg->TreeNode = data->SelectedTable.tb_Table[curr];
    }
    else if(((IPTR)*msg->TreeNode == (IPTR)MUIV_NListtree_NextSelected_Start) &&
             (data->ActiveNode != MUIV_NListtree_Active_Off))
    {
      *msg->TreeNode = data->ActiveNode;
    }
    else
    {
      *msg->TreeNode = CTN( MUIV_NListtree_PrevSelected_End );
    }
  }
  else
  {
    *msg->TreeNode = CTN( MUIV_NListtree_PrevSelected_End );
  }

  //D(bugn( "==> Entry: 0x%08lx\n", *msg->TreeNode ) );

  return( 0 );
}



/****** NListtree.mcc/MUIM_NListtree_CopyToClip ******************************
*
*   NAME
*
* MUIM_NListtree_CopyToClip -- Called for every clipboard copy action. (V1)
*
*
*   SYNOPSIS
*
* DoMethodA(obj, MUIM_NListtree_CopyToClip,
*   struct MUIP_NListtree_CopyToClip *ctcmessage);
*
*
*   FUNCTION
*
* Do a copy to clipboard from an entry/entry content.
*
*
*   INPUTS
*
*   TreeNode  - Tree node to copy contents from. Use
*                     MUIV_NListtree_CopyToClip_Active to copy the
*           active entry.
*
*   Pos     - Entry position.
*
*   Unit    - Clipboard unit to copy entry contents to.
*
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
* MUIA_NListtree_CopyToClipHook
*
*
******************************************************************************
*
*/
IPTR _NListtree_CopyToClip(struct IClass *cl, Object *obj, struct MUIP_NListtree_CopyToClip *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);

  // make sure anything is selected at all or calling CopyToClip doesn't make
  // much sense
  if(msg->TreeNode != NULL && msg->TreeNode->tn_Name != NULL)
  {
    STRPTR string = NULL;
    BOOL alloc = FALSE;

    if(data->CopyToClipHook != NULL)
    {
      if((SIPTR)(string = (STRPTR)MyCallHook(data->CopyToClipHook, data, MUIA_NListtree_CopyToClipHook, msg->TreeNode, msg->Pos, msg->Unit)) == (SIPTR)MUIV_NListtree_CopyToClip_Active)
        string = msg->TreeNode->tn_Name;
    }
    else
    {
      STRPTR str;

      string = msg->TreeNode->tn_Name;
      if((str = AllocVecPooled(data->MemoryPool, strlen(string) + 1)) != NULL)
      {
        STRPTR s = string;

        alloc = TRUE;
        string = str;

        // copy the string and remove all escape sequences
        while(*s != '\0')
        {
          if(*s == 0x1b)
          {
            s += 2;

            if(*s == '[')
              while(++*s != ']');

            s++;
          }

          *str++ = *s++;
        }
      }
    }

    if(string != NULL)
    {
      //D(bug( "String: %s, Unit: %ld, pos: %ld\n", string, msg->Unit, msg->Pos ) );

      StringToClipboard(msg->Unit, string);

      if(alloc == TRUE)
        FreeVecPooled(data->MemoryPool, string);
    }
  }

  return(0);
}



/****** NListtree.mcc/MUIM_NListtree_MultiTest *******************************
*
*   NAME
*
* MUIM_NListtree_MultiTest -- Called for every selection. (V1)
*
*
*   SYNOPSIS
*
* DoMethodA(obj, MUIM_NListtree_MultiTest,
*   struct MUIP_NListtree_MultiTest *multimessage);
*
*
*   FUNCTION
*
* This method must not be called directly. It will be called by
* NListtree just before multiselection. You can overload it and
* return TRUE or FALSE whether you want the entry to be multi-
* selectable or not.
*
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
* MUIM_NListtree_Select, MUIA_NListtree_MultiTest
*
*
******************************************************************************
*
*/


/****** NListtree.mcc/MUIM_NListtree_Active **********************************
*
*   NAME
*
* MUIM_NListtree_Active -- Called for every active change. (V1)
*
*
*   SYNOPSIS
*
* DoMethodA(obj, MUIM_NListtree_Active,
*   struct MUIP_NListtree_Active *activemessage);
*
*
*   FUNCTION
*
* This method must not be called directly. It will be called by
* NListtree if the active entry changes. This is an addition to
* MUIA_NListtree_Active
*
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
* MUIA_NListtree_Active
*
*
******************************************************************************
*
*/


/****** NListtree.mcc/MUIM_NListtree_DoubleClick *****************************
*
*   NAME
*
* MUIM_NListtree_DoubleClick -- Called for every double click. (V1)
*
*
*   SYNOPSIS
*
* DoMethodA(obj, MUIM_NListtree_DoubleClick,
*   struct MUIP_NListtree_DoubleClick *doubleclickmsg);
*
*
*   FUNCTION
*
* This method must not be called directly. It will be called by
* NListtree if an double click event occurs. This is an addition to
* MUIA_NListtree_DoubleClick
*
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
* MUIA_NListtree_DoubleClick
*
*
******************************************************************************
*
*/


/****** NListtree.mcc/MUIM_NListtree_DropType ********************************
*
*   NAME
*
* MUIM_NListtree_DropType --
*
*
*   SYNOPSIS
*
* DoMethod(obj, MUIM_NListtree_DropType, LONG *pos, LONG *type,
*   LONG minx, LONG maxx, LONG miny, LONG maxy,
*   LONG mousex, LONG mousey);
*
*
*   FUNCTION
*
*   This method MUST NOT be called directly !
*
*   It will be called by NListree while the DragReport, with
*   default *pos and *type values depending on the drag pointer
*   position that you can modify as you want.
*
*   For further information see method MUIM_NList_DropDraw.
*
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
* MUIM_NList_DropType
*
*
******************************************************************************
*
*/


/****** NListtree.mcc/MUIM_NListtree_DropDraw ********************************
*
*   NAME
*
* MUIM_NListtree_DropDraw --
*
*
*   SYNOPSIS
*
* DoMethod(obj, MUIM_NListtree_DropDraw, LONG pos, LONG type,
*   LONG minx, LONG maxx, LONG miny, LONG maxy);
*
*
*   FUNCTION
*
*   This method MUST NOT be called directly!
*
*       It will be called by NListtree, and will draw the drop mark
*   previously fixed (pos and type) by MUIM_NListtree_DropType
*   within the minx, maxx, miny, maxy in the _rp(obj) rasport.
*
*   For further information see method MUIM_NList_DropDraw.
*
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
* MUIM_NList_DropDraw
*
*
******************************************************************************
*
*/




/****i* NListtree.mcc/MUIM_NListtree_GetListActive ***************************
*
*   NAME
*
* MUIM_NListtree_GetListActive -- Set the active list in data structure. (V1)
*
*
*   SYNOPSIS
*
* DoMethod(obj, MUIM_NListtree_GetListActive, 0);
*
*
*   FUNCTION
*
* Set current active list in the internal data structure depending on the
* currently active entry.
*
*
*   INPUTS
*
*   RESULT
*
* data->ActiveList will be set to data->ActiveEntry->tn_Parent.
*
*
*   EXAMPLE
*
* DoMethod(obj, MUIM_NListtree_GetListActive, 0);
*
*
*   NOTES
*
* Only internal used function for notification of MUIA_NList_Active.
*
*
*   BUGS
*
*   SEE ALSO
*
* MUIA_NList_Active
*
*
******************************************************************************
*
*/
IPTR _NListtree_GetListActive(struct IClass *cl, Object *obj, UNUSED struct MUIP_NListtree_GetListActive *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct MUI_NListtree_TreeNode *tn;

  /*
  **  Get pointer to the selected node.
  **  Set active node in global
  **  data structure.
  */
  DoMethod( obj, MUIM_NList_GetEntry, MUIV_NList_GetEntry_Active, &tn );

  D(DBF_LISTTREE, "Got Active notify from NList 0x%08lx - %s (0x%08lx - Name: %s) force=%ld", tn, (tn != NULL && tn->tn_Name != NULL) ? tn->tn_Name : (STRPTR)"NULL", data->ActiveNode, (data->ActiveNode != NULL && data->ActiveNode->tn_Name != NULL) ? data->ActiveNode->tn_Name : (STRPTR)"NULL", data->ForceActiveNotify);

  if ( tn != data->ActiveNode || data->ForceActiveNotify)
  {
    //DoMethod( obj, MUIM_NListtree_Active, tn, pos );

    //MakeNotify( data, MUIA_NListtree_Active, tn );
    MakeSet( data, MUIA_NListtree_Active, tn );
    data->ForceActiveNotify = 0;
  }

  return( 0 );
}


IPTR _NListtree_GetDoubleClick(struct IClass *cl, Object *obj, UNUSED struct MUIP_NListtree_GetListActive *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);

  ENTER();

  if ( data->DoubleClick != MUIV_NListtree_DoubleClick_NoTrigger )
  {
    struct MUI_NListtree_TreeNode *tn;

    /*
    **  Get pointer to the selected node.
    */
    DoMethod( obj, MUIM_NList_GetEntry, data->LDClickEntry, &tn );

    if ( (SIPTR)tn != -1 )
    {
      DoMethod( obj, MUIM_NListtree_DoubleClick, tn, data->LDClickEntry, data->LDClickColumn );
      MakeNotify( data, MUIA_NListtree_DoubleClick, tn );
      //DoMethod( obj, MUIM_Set, MUIA_NListtree_DoubleClick, tn );
    }
  }

  RETURN(0);
  return( 0 );
}



/****i* NListtree.mcc/MUIM_NListtree_Data ************************************
*
*   NAME
*
* MUIM_NListtree_Data -- Get/Set some internal data. (V1)
*
*
*   SYNOPSIS
*
* DoMethod(obj, MUIM_NListtree_Data, dataqual, set);
*
*
*   FUNCTION
*
* Get some internal data or set if "set" is != NULL.
*
*
*   INPUTS
*
*   dataqual:
*     MUIV_NListtree_Data_MemPool
*     MUIV_NListtree_Data_VersInfo (not implemented)
*     MUIV_NListtree_Data_Sample (not implemented)
*
*
*   RESULT
*
*   The requested data.
*
*
*   EXAMPLE
*
*   result = DoMethod(obj, MUIM_NListtree_Data,
*     MUIV_NListtree_Data_MemPool, NULL);
*
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
******************************************************************************
*
*/
IPTR _NListtree_Data(struct IClass *cl, Object *obj, struct MUIP_NListtree_Data *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);

  switch( msg->ID )
  {
    case MUIV_NListtree_Data_MemPool:
      return( (IPTR)data->MemoryPool );

    case MUIV_NListtree_Data_VersInfo:
    case MUIV_NListtree_Data_Sample:
    default:
      break;
  }

  return( 0 );
}



IPTR _NListtree_SelectChange(struct IClass *cl, Object *obj, struct MUIP_NList_SelectChange *msg)
{
  struct NListtree_Data *data = INST_DATA(cl, obj);
  struct MUI_NListtree_TreeNode *tn;

  D(DBF_LISTTREE, "NList selection change: Entry %ld changed to %ld%s", msg->pos, msg->state,
    isFlagSet(msg->flags, MUIV_NList_SelectChange_Flag_Multi) ? " while holding down mouse button" : "");

  if (data->IgnoreSelectionChange)
  {
    D(DBF_LISTTREE, "  which is ignored");
    return 0;
  }

  DoMethod( obj, MUIM_NList_GetEntry, msg->pos, &tn );

  if ( msg->state == MUIV_NList_Select_Active )
  {
    if ( tn != data->ActiveNode )
    {
      /*
      **  !! Setting this NOT makes problems with getting the active entry
      **  in applications. Handle carefully if changed/removed !!
      */
      //MakeNotify( data, MUIA_NListtree_Active, tn );
      //data->ActiveNode = tn;

      /* sba: Changed to not notify, because the new active element is not drawn
      ** here (current NList restriction) a MUIA_NList_Active trigger
      ** will follow and we will use this for the notification
      */
      nnset( obj, MUIA_NListtree_Active, tn );

      /* sba: the MUIA_NList_Notification only notifies if neccessary, but as the
      ** notifications stayed out we set a flag to force it
      **/
      data->ForceActiveNotify = 1;
    }
  }

  else if ( msg->state == MUIV_NList_Select_On )
  {
    if ( NLFindInTable( &data->SelectedTable, tn ) == -1 )
    {
      TreeNodeSelectAdd( data, tn );
      SET_FLAG(tn->tn_Flags, TNF_SELECTED);

      MakeNotify( data, MUIA_NListtree_SelectChange, (APTR)TRUE );
    }
  }

  else if ( msg->state == MUIV_NList_Select_Off )
  {
    if ( NLFindInTable( &data->SelectedTable, tn ) != -1 )
    {
      TreeNodeSelectRemove( data, tn );
      CLEAR_FLAG(tn->tn_Flags, TNF_SELECTED);

      MakeNotify( data, MUIA_NListtree_SelectChange, (APTR)TRUE );
    }
  }

  return( 0 );
}



/****i* NListtree.mcc/_Dispatcher ******************************************
*
*   NAME
*
* _Dispatcher -- Call methods depending on the supplied MethodID (V1)
*
*
*   SYNOPSIS
*
*   FUNCTION
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
******************************************************************************
*
*/

DISPATCHER(_Dispatcher)
{
  switch( msg->MethodID )
  {
    /*
    **  Base class methods.
    */
    case OM_NEW:            return( _New            ( cl, obj, (APTR)msg) );
    case OM_DISPOSE:          return( _Dispose          ( cl, obj, (APTR)msg) );
    case OM_SET:            return( _Set            ( cl, obj, (APTR)msg) );
    case OM_GET:            return( _Get            ( cl, obj, (APTR)msg) );

    /*
    **  MUI base class methods.
    */
    case MUIM_Setup:          return( _Setup            ( cl, obj, (APTR)msg) );
    case MUIM_Cleanup:          return( _Cleanup          ( cl, obj, (APTR)msg) );
    case MUIM_Show:           return( _Show           ( cl, obj, (APTR)msg) );
    case MUIM_Hide:           return( _Hide           ( cl, obj, (APTR)msg) );
    case MUIM_Draw:           return( _Draw           ( cl, obj, (APTR)msg) );
    case MUIM_AskMinMax:        return( _AskMinMax          ( cl, obj, (APTR)msg) );
    case MUIM_HandleEvent:        return( _HandleEvent        ( cl, obj, (APTR)msg) );

    /*
    **  MUI/NList drag&drop methods.
    */
    case MUIM_DragQuery:        return( _DragNDrop_DragQuery    ( cl, obj, (APTR)msg) );
    case MUIM_DragBegin:        return( _DragNDrop_DragBegin    ( cl, obj, (APTR)msg) );
    case MUIM_DragReport:       return( _DragNDrop_DragReport   ( cl, obj, (APTR)msg) );
    case MUIM_DragFinish:       return( _DragNDrop_DragFinish   ( cl, obj, (APTR)msg) );
    case MUIM_DragDrop:         return( _DragNDrop_DragDrop     ( cl, obj, (APTR)msg) );
    case MUIM_NList_DropType:     return( _DragNDrop_DropType     ( cl, obj, (APTR)msg) );
    case MUIM_NList_DropDraw:     return( _DragNDrop_DropDraw     ( cl, obj, (APTR)msg) );
    case MUIM_NListtree_DropDraw:   return( _DragNDrop_NDropDraw    ( cl, obj, (APTR)msg) );

    case MUIM_NList_Display:    return( _NList_Display(cl, obj, (APTR)msg) );

    /*
    **  Specials.
    */
    case MUIM_NList_ContextMenuBuild: return( _ContextMenuBuild     ( cl, obj, (APTR)msg) );
    case MUIM_ContextMenuChoice:    return( _ContextMenuChoice      ( cl, obj, (APTR)msg) );


    /*
    **  NListtree methods sorted by kind.
    */
    case MUIM_NListtree_Open:     return( _NListtree_Open       ( cl, obj, (APTR)msg) );
    case MUIM_NListtree_Close:      return( _NListtree_Close      ( cl, obj, (APTR)msg) );

    case MUIM_NListtree_Insert:     return( _NListtree_Insert     ( cl, obj, (APTR)msg) );
    case MUIM_NListtree_InsertStruct: return( _NListtree_InsertStruct   ( cl, obj, (APTR)msg) );
    case MUIM_NListtree_Remove:     return( _NListtree_Remove     ( cl, obj, (APTR)msg) );
    case MUIM_NListtree_Clear:      return( _NListtree_Clear      ( cl, obj, (APTR)msg) );

    case MUIM_NListtree_Move:     return( _NListtree_Move       ( cl, obj, (APTR)msg) );
    case MUIM_NListtree_Copy:     return( _NListtree_Copy       ( cl, obj, (APTR)msg) );
    case MUIM_NListtree_Rename:     return( _NListtree_Rename     ( cl, obj, (APTR)msg) );
    case MUIM_NListtree_Exchange:   return( _NListtree_Exchange     ( cl, obj, (APTR)msg) );

    case MUIM_NListtree_FindName:   return( _NListtree_FindName     ( cl, obj, (APTR)msg) );
    case MUIM_NListtree_FindUserData: return( _NListtree_FindUserData   ( cl, obj, (APTR)msg) );

    case MUIM_NListtree_GetEntry:   return( _NListtree_GetEntry     ( cl, obj, (APTR)msg) );
    case MUIM_NListtree_GetNr:      return( _NListtree_GetNr      ( cl, obj, (APTR)msg) );

    case MUIM_NListtree_Sort:     return( _NListtree_Sort       ( cl, obj, (APTR)msg) );
    case MUIM_NListtree_Redraw:     return( _NListtree_Redraw     ( cl, obj, (APTR)msg) );
    case MUIM_NListtree_TestPos:    return( _NListtree_TestPos      ( cl, obj, (APTR)msg) );

    case MUIM_NListtree_Select:     return( _NListtree_Select     ( cl, obj, (APTR)msg) );
    case MUIM_NListtree_NextSelected: return( _NListtree_NextSelected   ( cl, obj, (APTR)msg) );
    case MUIM_NListtree_PrevSelected: return( _NListtree_PrevSelected   ( cl, obj, (APTR)msg) );

    case MUIM_NListtree_CopyToClip:   return( _NListtree_CopyToClip   ( cl, obj, (APTR)msg) );

    case MUIM_NListtree_Construct:    return( _NListtree_Construct    ( cl, obj, (APTR)msg) );
    case MUIM_NListtree_Destruct:     return( _NListtree_Destruct     ( cl, obj, (APTR)msg) );
    case MUIM_NListtree_Display:      return( _NListtree_Display      ( cl, obj, (APTR)msg) );
    case MUIM_NListtree_Compare:      return( _NListtree_Compare      ( cl, obj, (APTR)msg) );

    /*
    **  Methods, user can use for info or overload
    **  to control NListtree.
    */
    case MUIM_NListtree_Active:     return( TRUE );
    case MUIM_NListtree_DoubleClick:  return( TRUE );
    case MUIM_NListtree_MultiTest:    return( TRUE );
    case MUIM_NListtree_DropType:   return( TRUE );


    /*
    **  Internal methods.
    */
    case MUIM_NListtree_GetListActive:  return( _NListtree_GetListActive  ( cl, obj, (APTR)msg) );
    case MUIM_NListtree_GetDoubleClick: return( _NListtree_GetDoubleClick ( cl, obj, (APTR)msg) );
    case MUIM_NListtree_Data:     return( _NListtree_Data       ( cl, obj, (APTR)msg) );

    case MUIM_NList_SelectChange:   return( _NListtree_SelectChange   ( cl, obj, (APTR)msg) );

    #ifdef MYDEBUG
    case MUIM_NList_Remove:
    {
      ULONG rc;
      D(DBF_LISTTREE, "MUIM_NList_Remove");
      rc = DoSuperMethodA(cl, obj, msg);
      D(DBF_LISTTREE, "MUIM_NList_Remove_End");
      return rc;
    }
    #endif

    /*
    case MUIM_NList_UseImage:
    {
      ULONG res;

      res = DoSuperMethodA(cl, obj, msg);

      D(bug( "USEIMAGE ==> result: 0x%08lx    obj: 0x%08lx, imgnum: %ld, flags: 0x%08lx\n", res, m->obj, m->imgnum, m->flags ) );

      return( res );
    }

    case MUIM_NList_CreateImage:
    {
      struct MUIP_NList_UseImage *m = (struct MUIP_NList_UseImage *)msg;
      ULONG res;

      res = DoSuperMethodA(cl, obj, msg);

      D(bug( "CREATEIMAGE ==> result: 0x%08lx    obj: 0x%08lx, flags: 0x%08lx\n", res, m->obj, m->flags ) );

      return( res );
    }
    */
  }

  return( DoSuperMethodA( cl, obj, msg) );
}
