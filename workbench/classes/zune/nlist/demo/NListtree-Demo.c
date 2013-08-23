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

#if defined(__AROS__)
#define MUI_OBSOLETE 1
#endif

/*
**	Includes
*/
#include <proto/muimaster.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#if !defined(__amigaos4__)
#include <clib/alib_protos.h>
#endif

#include <exec/memory.h>
#include <exec/types.h>

#include <mui/NListtree_mcc.h>
#include <mui/NListview_mcc.h>
#include <mui/NList_mcc.h>

#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef MYDEBUG
 #define bug kprintf
 #define D(x)
 void kprintf( UBYTE *fmt, ... );
#else
 #define bug
 #define D(x)
#endif


#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d) ((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))
#endif

#include "SDI_hook.h"

#ifndef MUIA_Slider_Level
#define MUIA_Slider_Level                   0x8042ae3a /* V4  isg LONG              */
#endif

/*
**	Do not use stack sizes below 8KB!!
*/
LONG __stack = 16384;


/*
**	MUI library base.
*/
struct Library *MUIMasterBase = NULL;

#if defined(__amigaos4__)
struct Library *IntuitionBase = NULL;
#else
struct IntuitionBase *IntuitionBase = NULL;
#endif

#if defined(__amigaos4__)
struct IntuitionIFace *IIntuition = NULL;
struct MUIMasterIFace *IMUIMaster = NULL;
#endif

struct MUI_NListtree_TreeNode *tntest;

/*
**	MUI objects.
*/
STATIC APTR		app, window,lt_nodes,
				tx_info1,	tx_info2,	tx_info3,
				sl_treecol,	st_string,
				bt_open,	bt_close,	bt_expand,	bt_collapse,
				bt_insert,	bt_remove,	bt_exchange,bt_rename,
				bt_move,	bt_copy,	bt_moveks,	bt_copyks,
				bt_find,	bt_parent,	bt_sort,	bt_getnr,
				bt_redraw,	bt_selected,bt_showtree,bt_seltogg,
				bt_test,	bt_test2,	bt_test3,	bt_test4;


/*
**	Sample tree structure.
*/
struct SampleArray
{
	const char *name;
	ULONG flags;
};

STATIC const struct SampleArray sa[] =
{
	{ "comp", TNF_LIST | TNF_OPEN },
	{	 "sys", TNF_LIST | TNF_OPEN },
	{		 "amiga", TNF_LIST | TNF_OPEN },
	{			 "misc", 0x8000 },
	{		 "mac", TNF_LIST },
	{			 "system", 0x8000 },

	{"de", TNF_LIST | TNF_OPEN },
	{	 "comm", TNF_LIST },
	{		 "software", TNF_LIST },
	{			 "ums", 0x8000 },
	{	 "comp", TNF_LIST | TNF_OPEN },
	{		 "sys", TNF_LIST | TNF_OPEN },
	{			 "amiga", TNF_LIST },
	{				 "misc", 0x8000 },
	{				 "tech", 0x8000 },
	{			 "amiga", 0x8000 },

	{"sort test", TNF_LIST | TNF_OPEN },
	{	 "a", 0 },
	{	 "x", TNF_LIST },
	{	 "v", 0 },
	{	 "g", TNF_LIST },
	{	 "h", 0 },
	{	 "k", TNF_LIST },
	{	 "u", 0 },
	{	 "i", TNF_LIST },
	{	 "t", 0 },
	{	 "e", TNF_LIST },
	{	 "q", 0 },
	{	 "s", TNF_LIST },
	{	 "c", 0 },
	{	 "f", TNF_LIST },
	{	 "p", 0 },
	{	 "l", TNF_LIST },
	{	 "z", 0 },
	{	 "w", TNF_LIST },
	{	 "b", 0 },
	{	 "o", TNF_LIST },
	{	 "d", 0 },
	{	 "m", TNF_LIST },
	{	 "r", 0 },
	{	 "y", TNF_LIST },
	{	 "n", 0 },
	{	 "j", TNF_LIST },


	{"m", TNF_LIST },
	{	 "i", TNF_LIST },
	{		 "c", TNF_LIST },
	{			 "h", TNF_LIST },
	{				 "e", TNF_LIST },
	{					 "l", TNF_LIST },
	{						 "a", TNF_LIST },
	{							 "n", TNF_LIST },
	{								 "g", TNF_LIST },
	{									 "e", TNF_LIST },
	{										 "l", TNF_LIST },
	{											 "o", 0 },

	{"end", TNF_LIST },
	{	 "of", TNF_LIST },
	{		 "tree", 0 },

	{ "Sort Test 2", TNF_LIST },

	{ NULL, 0 }
};

/*
**	This function draws the sample tree structure.
*/
STATIC VOID DrawSampleTree( Object *ltobj )
{
	struct MUI_NListtree_TreeNode *tn1, *tn2, *tn3;
	char txt[128];
	WORD i = 0, j;

	tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, MUIV_NListtree_Insert_ListNode_Root, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
	tn2 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
	tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, tn2, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
	tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
	tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, tn2, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
	tntest = tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;

	tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, MUIV_NListtree_Insert_ListNode_Root, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
	tn2 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
	tn2 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, tn2, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
	tn2 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, tn2, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
	tn3 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
	tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, tn3, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
	tn2 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
	tn3 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, tn2, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
	tn3 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, tn2, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
	tn3 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;

	tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, MUIV_NListtree_Insert_ListNode_Root, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;

	for( j = 0; j < 26; j++ )
	{
		DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
	}

	tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, MUIV_NListtree_Insert_ListNode_Root, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;

	for( j = 0; j < 11; j++ )
	{
		tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
	}

	tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, MUIV_NListtree_Insert_ListNode_Root, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
	tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
	tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;


	tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, (IPTR)sa[i].flags, MUIV_NListtree_Insert_ListNode_Root, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;

	for( i = 0; i < 2500; i++ )
	{
		if ( i % 2 )	snprintf( txt, sizeof(txt), "Sort Entry %d", i + 1 );
		else			snprintf( txt, sizeof(txt), "Entry Sort %d", i + 1 );

		tn2 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, txt, 0, ( i % 5 ) ? tn2 : tn1, MUIV_NListtree_Insert_PrevNode_Tail, ( i % 5 ) ? TNF_LIST : 0 );
	}


	DoMethod( ltobj, MUIM_NListtree_InsertStruct, "This/is/a/very/long/test/for/Blafasel_InsertStruct", 0, "/", 0 );
	DoMethod( ltobj, MUIM_NListtree_InsertStruct, "This/is/another/very/long/test/for/Blafasel_InsertStruct", 0, "/", 0 );
}



static struct MUI_NListtree_TreeNode *GetParent( Object *obj, struct MUI_NListtree_TreeNode *tn )
{
	if ( tn != NULL )
	{
		return( (struct MUI_NListtree_TreeNode *)DoMethod( obj, MUIM_NListtree_GetEntry, MUIV_NListtree_GetEntry_ListNode_Root, MUIV_NListtree_GetEntry_Position_Parent, 0 ) );
	}

	return( NULL );
}


static struct MUI_NListtree_TreeNode *GetParentNotRoot( Object *obj, struct MUI_NListtree_TreeNode *tn )
{
	if((tn = GetParent( obj, tn)))
	{
		if ( GetParent( obj, tn ) )
		{
			return( tn );
		}
	}

	return( NULL );
}


struct MUI_NListtree_TreeNode *IsXChildOfY( Object *obj, struct MUI_NListtree_TreeNode *x, struct MUI_NListtree_TreeNode *y )
{
	do
	{
		if ( y == x )
		{
			return( y );
		}
	}
	while((y = GetParentNotRoot( obj, y)));

	return( NULL );
}



/*
**	Allocates memory for each entry we create.
*/
HOOKPROTONHNO(confunc, SIPTR, struct MUIP_NListtree_ConstructMessage *msg)
{
	struct SampleArray *sa;

	/*
	**	Allocate needed piece of memory for the new entry.
	*/
	if((sa = (struct SampleArray *)AllocVec( sizeof( struct SampleArray) + strlen( msg->Name ) + 1, MEMF_CLEAR)))
	{
		/*
		**	Save the user data field right after the
		**	array structure.
		*/
		strcpy( (STRPTR)&sa[1], msg->Name );
		sa->name = (STRPTR)&sa[1];

		sa->flags = (IPTR)msg->UserData;
	}

	return( (SIPTR)sa );
}
MakeStaticHook(conhook, confunc);


/*
**	Free memory we just allocated above.
*/
HOOKPROTONHNO(desfunc, LONG, struct MUIP_NListtree_DestructMessage *msg)
{
	if ( msg->UserData != NULL )
	{
		FreeVec( msg->UserData );
		msg->UserData = NULL;
	}

	return( 0 );
}
MakeStaticHook(deshook, desfunc);


/*
**	Compare hook function.
*/
HOOKPROTONHNO(compfunc, LONG, struct MUIP_NListtree_CompareMessage *msg)
{
	return( stricmp( msg->TreeNode1->tn_Name, msg->TreeNode2->tn_Name ) );
}
MakeStaticHook(comphook, compfunc);


/*
**	MultiTest hook function.
*/
HOOKPROTONHNO(mtfunc, LONG, struct MUIP_NListtree_MultiTestMessage *msg)
{
	if ( msg->TreeNode->tn_Flags & TNF_LIST )
		return( FALSE );

	return( TRUE );
}
MakeStaticHook(mthook, mtfunc);


/*
**	Format the entry data for displaying.
*/
HOOKPROTONHNO(dspfunc, LONG, struct MUIP_NListtree_DisplayMessage *msg)
{
	STATIC CONST_STRPTR t1 = "Newsgroups", t2 = "Flags", t3 = "subscribed", t4 = "\0", t5 = "Count";
	STATIC char buf[10];

	if ( msg->TreeNode != NULL )
	{
		/*
		**	The user data is a pointer to a SampleArray struct.
		*/
		struct SampleArray *a = (struct SampleArray *)msg->TreeNode->tn_User;

		snprintf( buf, sizeof(buf), "%3d", (unsigned int)(IPTR)msg->Array[-1] );

		*msg->Array++	= msg->TreeNode->tn_Name;
		*msg->Array++	= (STRPTR)(( a->flags & 0x8000 ) ? t3 : t4);
		*msg->Array++	= buf;
	}
	else
	{
		*msg->Array++	= (STRPTR)t1;
		*msg->Array++	= (STRPTR)t2;
		*msg->Array++	= (STRPTR)t5;

		*msg->Preparse++	= (STRPTR)"\033b\033u";
		*msg->Preparse++	= (STRPTR)"\033b\033u";
		*msg->Preparse++	= (STRPTR)"\033b\033u";
	}

	return( 0 );
}
MakeStaticHook(dsphook, dspfunc);


/*
**	Insert a new entry which name is given in
**	the string gadget.
*/
HOOKPROTONHNP(insertfunc, LONG, Object *obj)
{
	STRPTR x = NULL;

	/*
	**	Get user string.
	*/
	get( st_string, MUIA_String_Contents, &x );

	/*
	**	Insert the new entry after
	**	the active entry.
	*/
	DoMethod( obj, MUIM_NListtree_Insert, x, 0, MUIV_NListtree_Insert_ListNode_Active,
		MUIV_NListtree_Insert_PrevNode_Active, MUIV_NListtree_Insert_Flag_Active );

	return( 0 );
}
MakeStaticHook(inserthook, insertfunc);


/*
**	Exchange two entries.
*/
HOOKPROTONH(exchangefunc, LONG, Object *obj, ULONG **para)
{
	STATIC struct MUI_NListtree_TreeNode *tn1, *tn2;
	STATIC LONG exchcnt = 0;

	if ( ( exchcnt == 0 ) && ( (IPTR)*para == 42 ) )
	{
		get( obj, MUIA_NListtree_Active, &tn1 );

		if ( tn1 != MUIV_NListtree_Active_Off )
		{
			nnset( tx_info3, MUIA_Text_Contents, "Select entry to exchange selected entry with." );

			exchcnt++;
		}
	}

	else if ( exchcnt == 1 )
	{
		get( obj, MUIA_NListtree_Active, &tn2 );

		if ( ( tn2 != MUIV_NListtree_Active_Off ) && ( tn1 != tn2 ) )
		{
			if ( !IsXChildOfY( obj, tn1, tn2 ) && !IsXChildOfY( obj, tn1, tn2 ) )
			{
				struct MUI_NListtree_ListNode *ln1;

				if((ln1 = (struct MUI_NListtree_ListNode *)DoMethod( obj, MUIM_NListtree_GetEntry, tn1, MUIV_NListtree_GetEntry_Position_Parent, 0)))
				{
					DoMethod( obj, MUIM_NListtree_Exchange, ln1, tn1, MUIV_NListtree_Exchange_ListNode2_Active, MUIV_NListtree_Exchange_TreeNode2_Active, 0 );

					nnset( tx_info3, MUIA_Text_Contents, "Entries successfully exchanged!" );
				}
				else
					nnset( tx_info3, MUIA_Text_Contents, "Something went wrong! Try again to select." );
			}
			else
				nnset( tx_info3, MUIA_Text_Contents, "You can not exchange with childs!" );
		}
		else
			nnset( tx_info3, MUIA_Text_Contents, "You should not exchange an entry with itself!" );

		exchcnt = 0;
	}

	return( 0 );
}
MakeStaticHook(exchangehook, exchangefunc);


/*
**	Rename the selected entry with the name is given in
**	the string gadget.
*/
HOOKPROTONHNP(renamefunc, LONG, Object *obj)
{
	struct MUI_NListtree_TreeNode *tn = NULL;
	STRPTR x = NULL;

	/*
	**	Get user string.
	*/
	get( st_string, MUIA_String_Contents, &x );
	get( obj, MUIA_NListtree_Active, &tn );

	/*
	**	Insert the new entry sorted (compare hook)
	**	into the active list node.
	*/
	DoMethod( obj, MUIM_NListtree_Rename, tn,
		x, 0 );

	return( 0 );
}
MakeStaticHook(renamehook, renamefunc);


/*
**	Insert a new entry which name is given in
**	the string gadget.
*/
HOOKPROTONH(movefunc, LONG, Object *obj, ULONG **para)
{
	STATIC struct MUI_NListtree_TreeNode *tn1, *tn2;
	STATIC LONG movecnt = 0;

	if ( ( movecnt == 0 ) && ( (IPTR)*para == 42 ) )
	{
		get( obj, MUIA_NListtree_Active, &tn1 );

		if ( tn1 != MUIV_NListtree_Active_Off )
		{
			nnset( tx_info3, MUIA_Text_Contents, "Select entry to insert after by simple click." );

			movecnt++;
		}
	}

	else if ( movecnt == 1 )
	{
		get( obj, MUIA_NListtree_Active, &tn2 );

		if ( ( tn2 != MUIV_NListtree_Active_Off ) && ( tn1 != tn2 ) )
		{
			if ( !IsXChildOfY( obj, tn1, tn2 ) && !IsXChildOfY( obj, tn1, tn2 ) )
			{
				struct MUI_NListtree_ListNode *ln1;

				if((ln1 = (struct MUI_NListtree_ListNode *)DoMethod( obj, MUIM_NListtree_GetEntry, tn1, MUIV_NListtree_GetEntry_Position_Parent, 0)))
				{
					DoMethod( obj, MUIM_NListtree_Move, ln1, tn1, MUIV_NListtree_Move_NewListNode_Active, tn2, 0 );

					nnset( tx_info3, MUIA_Text_Contents, "Entry successfully moved!" );
				}
				else
					nnset( tx_info3, MUIA_Text_Contents, "Something went wrong! Try again to select destination." );
			}
			else
				nnset( tx_info3, MUIA_Text_Contents, "You can not move childs!" );
		}
		else
			nnset( tx_info3, MUIA_Text_Contents, "You should not move an entry to itself!" );

		movecnt = 0;
	}

	return( 0 );
}
MakeStaticHook(movehook, movefunc);


/*
**	Insert a new entry which name is given in
**	the string gadget.
*/
HOOKPROTONH(copyfunc, LONG, Object *obj, ULONG **para)
{
	STATIC struct MUI_NListtree_TreeNode *tn1, *tn2;
	STATIC LONG copycnt = 0;

	if ( ( copycnt == 0 ) && ( (IPTR)*para == 42 ) )
	{
		get( obj, MUIA_NListtree_Active, &tn1 );

		if ( tn1 != MUIV_NListtree_Active_Off )
		{
			nnset( tx_info3, MUIA_Text_Contents, "Select entry to insert after by simple click." );

			copycnt++;
		}
	}

	else if ( copycnt == 1 )
	{
		get( obj, MUIA_NListtree_Active, &tn2 );

		if ( ( tn2 != MUIV_NListtree_Active_Off ) && ( tn1 != tn2 ) )
		{
			struct MUI_NListtree_ListNode *ln1;

			if((ln1 = (struct MUI_NListtree_ListNode *)DoMethod( obj, MUIM_NListtree_GetEntry, tn1, MUIV_NListtree_GetEntry_Position_Parent, 0)))
			{
				DoMethod( obj, MUIM_NListtree_Copy, ln1, tn1, MUIV_NListtree_Copy_DestListNode_Active, tn2, 0 );

				nnset( tx_info3, MUIA_Text_Contents, "Entry successfully copied!" );
			}
			else
				nnset( tx_info3, MUIA_Text_Contents, "Something went wrong! Try again to select destination." );
		}
		else
			nnset( tx_info3, MUIA_Text_Contents, "You should not copy an entry to itself!" );

		copycnt = 0;
	}

	return( 0 );
}
MakeStaticHook(copyhook, copyfunc);


/*
**	Move KeepStructure
*/
HOOKPROTONH(moveksfunc, LONG, Object *obj, ULONG **para)
{
	STATIC struct MUI_NListtree_TreeNode *tn1, *tn2;
	STATIC LONG movekscnt = 0;

	if ( ( movekscnt == 0 ) && ( (IPTR)*para == 42 ) )
	{
		get( obj, MUIA_NListtree_Active, &tn1 );

		if ( tn1 != MUIV_NListtree_Active_Off )
		{
			nnset( tx_info3, MUIA_Text_Contents, "Select entry to make KeepStructure move with." );

			movekscnt++;
		}
	}

	else if ( movekscnt == 1 )
	{
		get( obj, MUIA_NListtree_Active, &tn2 );

		if ( ( tn2 != MUIV_NListtree_Active_Off ) && ( tn1 != tn2 ) )
		{
			struct MUI_NListtree_ListNode *ln1;

			if((ln1 = (struct MUI_NListtree_ListNode *)DoMethod( obj, MUIM_NListtree_GetEntry, tn1, MUIV_NListtree_GetEntry_Position_Parent, 0)))
			{
				DoMethod( obj, MUIM_NListtree_Move, ln1, tn1, MUIV_NListtree_Move_NewListNode_Active, tn2, MUIV_NListtree_Move_Flag_KeepStructure );

				nnset( tx_info3, MUIA_Text_Contents, "Entry successfully moved (structure keeped)" );
			}
			else
				nnset( tx_info3, MUIA_Text_Contents, "Something went wrong! Try again to select destination." );
		}
		else
			nnset( tx_info3, MUIA_Text_Contents, "You should not move an entry to itself!" );

		movekscnt = 0;
	}

	return( 0 );
}
MakeStaticHook(movekshook, moveksfunc);

/*
**	Copy KeepStructure
*/
HOOKPROTONH(copyksfunc, LONG, Object *obj, ULONG **para)
{
	STATIC struct MUI_NListtree_TreeNode *tn1, *tn2;
	STATIC LONG copykscnt = 0;

	if ( ( copykscnt == 0 ) && ( (IPTR)*para == 42 ) )
	{
		get( obj, MUIA_NListtree_Active, &tn1 );

		if ( tn1 != MUIV_NListtree_Active_Off )
		{
			nnset( tx_info3, MUIA_Text_Contents, "Select entry to make KeepStructure copy with." );

			copykscnt++;
		}
	}

	else if ( copykscnt == 1 )
	{
		get( obj, MUIA_NListtree_Active, &tn2 );

		if ( ( tn2 != MUIV_NListtree_Active_Off ) && ( tn1 != tn2 ) )
		{
			struct MUI_NListtree_ListNode *ln1;

			if((ln1 = (struct MUI_NListtree_ListNode *)DoMethod( obj, MUIM_NListtree_GetEntry, tn1, MUIV_NListtree_GetEntry_Position_Parent, 0)))
			{
				DoMethod( obj, MUIM_NListtree_Copy, ln1, tn1, MUIV_NListtree_Copy_DestListNode_Active, tn2, MUIV_NListtree_Copy_Flag_KeepStructure );

				nnset( tx_info3, MUIA_Text_Contents, "Entry successfully copied (structure keeped)" );
			}
			else
				nnset( tx_info3, MUIA_Text_Contents, "Something went wrong! Try again to select destination." );
		}
		else
			nnset( tx_info3, MUIA_Text_Contents, "You should not copy an entry to itself!" );

		copykscnt = 0;
	}

	return( 0 );
}
MakeStaticHook(copykshook, copyksfunc);


HOOKPROTONHNO(fudf, LONG, struct MUIP_NListtree_FindUserDataMessage *msg)
{
	nnset( tx_info1, MUIA_Text_Contents, "FindUserData Hook passed!" );
	return( strncmp( (STRPTR)msg->User, (STRPTR)msg->UserData, strlen( (STRPTR)msg->User ) ) );
}
MakeStaticHook(fudh, fudf);

/*
**	Find the specified tree node by name.
*/
HOOKPROTONHNP(findnamefunc, LONG, Object *obj)
{
	struct MUI_NListtree_TreeNode *tn;
	STRPTR x = NULL;

	/*
	**	Let us see, which string the user wants to search for...
	*/
	get( st_string, MUIA_String_Contents, &x );

	/*
	**	Is it somewhere in the tree?
	*/
	if((tn = (struct MUI_NListtree_TreeNode *)DoMethod(obj, MUIM_NListtree_FindUserData, MUIV_NListtree_FindUserData_ListNode_Root, x, MUIV_NListtree_FindUserData_Flag_Activate)))
	{
		/*
		**	Found! Inform the user.
		*/
		nnset( tx_info3, MUIA_Text_Contents, "Found your node!" );
	}
	else
	{
		/*
		**	Not found. Inform the user.
		*/
		nnset( tx_info3, MUIA_Text_Contents, "NOT found specified node!" );
	}

	return( 0 );
}
MakeStaticHook(findnamehook, findnamefunc);


/*
**	Sort the active list.
*/
HOOKPROTONHNP(sortfunc, LONG, Object *obj)
{
	clock_t start, end;
	LONG lastactive = 0;

	get( obj, MUIA_NListtree_Active, &lastactive );
	set( obj, MUIA_NListtree_Active, MUIV_NListtree_Active_Off );

	start = clock();
	DoMethod( obj, MUIM_NListtree_Sort, lastactive, 0 );
	end = clock();

	set( obj, MUIA_NListtree_Active, lastactive );

	DoMethod( tx_info3, MUIM_SetAsString, MUIA_Text_Contents, "Sort took %ld.%03lds", ( end - start ) / CLOCKS_PER_SEC, ( end - start ) % CLOCKS_PER_SEC );

	return( 0 );
}
MakeStaticHook(sorthook, sortfunc);


/*
**	Find the specified tree node by name.
*/
HOOKPROTONHNP(getnrfunc, LONG, Object *obj)
{
	LONG temp, temp2;

	temp = DoMethod( obj, MUIM_NListtree_GetNr,
		MUIV_NListtree_GetNr_TreeNode_Active, MUIV_NListtree_GetNr_Flag_CountLevel );

	if ( temp == 1 )
		DoMethod( tx_info1, MUIM_SetAsString, MUIA_Text_Contents, "1 entry in parent node." );
	else
		DoMethod( tx_info1, MUIM_SetAsString, MUIA_Text_Contents, "%ld entries in parent node.", temp );


	temp = DoMethod( obj, MUIM_NListtree_GetNr,
		MUIV_NListtree_GetNr_TreeNode_Active, MUIV_NListtree_GetNr_Flag_CountAll );

	if ( temp == 1 )
		DoMethod( tx_info2, MUIM_SetAsString, MUIA_Text_Contents, "1 entry total." );
	else
		DoMethod( tx_info2, MUIM_SetAsString, MUIA_Text_Contents, "%ld entries total.", temp );


	temp = DoMethod( obj, MUIM_NListtree_GetNr,
		MUIV_NListtree_GetNr_TreeNode_Active, 0 );

	temp2 = DoMethod( obj, MUIM_NListtree_GetNr,
		MUIV_NListtree_GetNr_TreeNode_Active, MUIV_NListtree_GetNr_Flag_Visible );

	DoMethod( tx_info3, MUIM_SetAsString, MUIA_Text_Contents, "Active entry pos: %ld (visible: %ld).", temp, temp2 );

	return( 0 );
}
MakeStaticHook(getnrhook, getnrfunc);

/*
**	Find the specified tree node by name.
*/
HOOKPROTONHNP(numselfunc, LONG, Object *obj)
{
	LONG temp = 0;

	DoMethod( obj, MUIM_NListtree_Select, MUIV_NListtree_Select_All,
		MUIV_NListtree_Select_Ask, 0, &temp );

	if ( temp == 1 )
		DoMethod( tx_info1, MUIM_SetAsString, MUIA_Text_Contents, "1 node selected." );
	else
		DoMethod( tx_info1, MUIM_SetAsString, MUIA_Text_Contents, "%ld nodes selected.", temp );

	{
		struct MUI_NListtree_TreeNode *tn;

		tn = (struct MUI_NListtree_TreeNode *)MUIV_NListtree_NextSelected_Start;

		for (;;)
		{
			DoMethod( obj, MUIM_NListtree_NextSelected, &tn );

			if ( (IPTR)tn == (IPTR)MUIV_NListtree_NextSelected_End )
				break;

			D(bug( "Next TreeNode: 0x%08lx - %s\n", tn, tn->tn_Name ) );
		}

		D(bug( "\n" ) );

		tn = (struct MUI_NListtree_TreeNode *)MUIV_NListtree_PrevSelected_Start;

		for (;;)
		{
			DoMethod( obj, MUIM_NListtree_PrevSelected, &tn );

			if ( (IPTR)tn == (IPTR)MUIV_NListtree_PrevSelected_End )
				break;

			D(bug( "Prev TreeNode: 0x%08lx - %s\n", tn, tn->tn_Name ) );
		}
	}

	return( 0 );
}
MakeStaticHook(numselhook, numselfunc);

/*
**	Test func
*/
HOOKPROTONHNP(testfunc, LONG, Object *obj)
{
	SIPTR id;
	ULONG num;

	id = MUIV_NListtree_NextSelected_Start;

	for(;;)
	{
		DoMethod( obj, MUIM_NListtree_NextSelected, &id );

		if(id == (SIPTR)MUIV_NListtree_NextSelected_End )
			break;

		//GetAttr( MUIA_List_Entries, obj, &num );

		num = DoMethod( obj, MUIM_NListtree_GetNr,
				MUIV_NListtree_GetNr_TreeNode_Active, MUIV_NListtree_GetNr_Flag_CountAll );

		if ( num > 1 )
			DoMethod( obj, MUIM_NListtree_Remove, MUIV_NListtree_Remove_ListNode_Active, id, 0 );
		else
			break;
	}

	return( 0 );
}
MakeStaticHook(testhook, testfunc);


#if defined(__amigaos4__)
#define GETINTERFACE(iface, base)	(iface = (APTR)GetInterface((struct Library *)(base), "main", 1L, NULL))
#define DROPINTERFACE(iface)			(DropInterface((struct Interface *)iface), iface = NULL)
#else
#define GETINTERFACE(iface, base)	TRUE
#define DROPINTERFACE(iface)
#endif

/*
**	Main
*/
int main(UNUSED int argc, UNUSED char *argv[])
{
	ULONG signals;
	static const char *const UsedClasses[] = { 
		"NList.mcc",
		"NListtree.mcc",
		"NListviews.mcc",
		NULL
	};

	/*
	**	Is MUI V19 available?
	*/
	if((IntuitionBase = (APTR)OpenLibrary("intuition.library", 36)) &&
		GETINTERFACE(IIntuition, IntuitionBase))
	if((MUIMasterBase = OpenLibrary("muimaster.library", 19)) &&
		GETINTERFACE(IMUIMaster, MUIMasterBase))
	{
		/*
		**	Create application object.
		*/
		app = ApplicationObject,
			MUIA_Application_Title,       "NListtree-Demo",
			MUIA_Application_Version,     "$VER: NListtree-Demo 1.0 (" __DATE__ ")",
			MUIA_Application_Copyright,   "Copyright (C) 2001-2013 by NList Open Source Team",
			MUIA_Application_Author,      "NList Open Source Team",
			MUIA_Application_Description, "Demonstration program for MUI class NListtree.mcc",
			MUIA_Application_Base,        "NLISTTREEDEMO",
			MUIA_Application_UsedClasses, UsedClasses,

			/*
			**	Build the window.
			*/
			SubWindow, window = WindowObject,
				MUIA_Window_Title,			"NListtree-Demo",
				MUIA_Window_ID,				MAKE_ID( 'N', 'L', 'T', 'R' ),
				MUIA_Window_AppWindow,		TRUE,
				WindowContents,				VGroup,

					/*
					**	Create a NListview embedded NListtree object
					*/
					Child, NListviewObject,
						MUIA_ShortHelp,			"The NListtree object...",
						MUIA_NListview_NList,	lt_nodes = NListtreeObject,
							InputListFrame,
							MUIA_CycleChain,				TRUE,
							MUIA_NList_MinLineHeight,		18,
							MUIA_NListtree_MultiSelect,		MUIV_NListtree_MultiSelect_Shifted,
							MUIA_NListtree_MultiTestHook,	&mthook,
							MUIA_NListtree_DisplayHook,		&dsphook,
							MUIA_NListtree_ConstructHook,	&conhook,
							MUIA_NListtree_DestructHook,	&deshook,	/* This is the same as MUIV_NListtree_CompareHook_LeavesMixed. */
							MUIA_NListtree_CompareHook,		&comphook,
							MUIA_NListtree_DoubleClick,		MUIV_NListtree_DoubleClick_Tree,
							MUIA_NListtree_EmptyNodes,		FALSE,
							MUIA_NListtree_TreeColumn,		0,
							MUIA_NListtree_DragDropSort,	TRUE,
							MUIA_NListtree_Title,			TRUE,
							MUIA_NListtree_Format,			",,",
							MUIA_NListtree_FindUserDataHook,&fudh,
							//MUIA_NListtree_NoRootTree,		TRUE,
						End,
					End,

					/*
					**	Build some controls.
					*/
					Child, tx_info1 = TextObject,
						MUIA_Background, MUII_TextBack,
						TextFrame,
					End,

					Child, tx_info2 = TextObject,
						MUIA_Background, MUII_TextBack,
						TextFrame,
					End,

					Child, tx_info3 = TextObject,
						MUIA_Background, MUII_TextBack,
						TextFrame,
					End,

					Child, ColGroup( 2 ),
						Child, FreeKeyLabel( "TreeCol:", 'c' ),
						Child, sl_treecol	= Slider( 0, 2, 0 ),
					End,

					Child, HGroup,
						Child, st_string = StringObject,
							StringFrame,
							MUIA_String_MaxLen, 50,
						End,
					End,


					Child, ColGroup( 4 ),
						Child, bt_open		= KeyButton( "Open",		'o' ),
						Child, bt_close		= KeyButton( "Close",		'c' ),
						Child, bt_expand	= KeyButton( "Expand",		'e' ),
						Child, bt_collapse	= KeyButton( "Collapse",	'a' ),

						Child, bt_insert	= KeyButton( "Insert",		'i' ),
						Child, bt_remove	= KeyButton( "Remove",		'r' ),
						Child, bt_exchange	= KeyButton( "Exchange",	'x' ),
						Child, bt_rename	= KeyButton( "Rename",		'r' ),

						Child, bt_move		= KeyButton( "Move",		'm' ),
						Child, bt_copy		= KeyButton( "Copy",		'y' ),
						Child, bt_moveks	= KeyButton( "Move KS",		'v' ),
						Child, bt_copyks	= KeyButton( "Copy KS",		'k' ),

						Child, bt_find		= KeyButton( "FindName",	'f' ),
						Child, bt_parent	= KeyButton( "Parent",		'p' ),
						Child, bt_sort		= KeyButton( "Sort",		's' ),
						Child, bt_getnr		= KeyButton( "GetNr",		'n' ),

						Child, bt_redraw	= KeyButton( "Redraw",		'w' ),
						Child, bt_selected	= KeyButton( "Selected",	'd' ),
						Child, bt_seltogg	= KeyButton( "Sel Togg",	't' ),
						Child, bt_showtree	= KeyButton( "Show tree",	'h' ),

						Child, bt_test		= KeyButton( "Test", ' ' ),
						Child, bt_test2		= KeyButton( "Test2", ' ' ),
						Child, bt_test3		= KeyButton( "Test3", ' ' ),
						Child, bt_test4		= KeyButton( "Test4", ' ' ),
					End,

				End,

			End,
		End;


		if( app )
		{
			/*
			**	generate notifications
			*/
			DoMethod( window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
				app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

			/*
			**	open/close/expand/collapse
			*/
			DoMethod( bt_open, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 4, MUIM_NListtree_Open, MUIV_NListtree_Open_ListNode_Active, MUIV_NListtree_Open_TreeNode_Active, 0 );

			DoMethod( bt_close, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 4, MUIM_NListtree_Close, MUIV_NListtree_Close_ListNode_Active, MUIV_NListtree_Close_TreeNode_Active, 0 );

			DoMethod( bt_expand, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 4, MUIM_NListtree_Open, MUIV_NListtree_Open_ListNode_Root, MUIV_NListtree_Open_TreeNode_All, 0 );

			DoMethod( bt_collapse, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 4, MUIM_NListtree_Close, MUIV_NListtree_Close_ListNode_Root, MUIV_NListtree_Close_TreeNode_All, 0 );



			/*
			**	insert/remove/exchange/rename
			*/
			DoMethod( bt_insert, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 2, MUIM_CallHook, &inserthook );

			DoMethod( bt_remove, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 4, MUIM_NListtree_Remove, MUIV_NListtree_Remove_ListNode_Root, MUIV_NListtree_Remove_TreeNode_Selected, 0 );

			DoMethod( bt_exchange, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 3, MUIM_CallHook, &exchangehook, 42 );

			DoMethod( lt_nodes, MUIM_Notify, MUIA_NListtree_Active, MUIV_EveryTime,
				lt_nodes, 3, MUIM_CallHook, &exchangehook, 0 );

			DoMethod( bt_rename, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 2, MUIM_CallHook, &renamehook );


			/*
			**	move/copy/moveks/copyks
			*/
			DoMethod( bt_move, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 3, MUIM_CallHook, &movehook, 42 );

			DoMethod( lt_nodes, MUIM_Notify, MUIA_NListtree_Active, MUIV_EveryTime,
				lt_nodes, 3, MUIM_CallHook, &movehook, 0 );

			DoMethod( bt_copy, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 3, MUIM_CallHook, &copyhook, 42 );

			DoMethod( lt_nodes, MUIM_Notify, MUIA_NListtree_Active, MUIV_EveryTime,
				lt_nodes, 3, MUIM_CallHook, &copyhook, 0 );

			DoMethod( bt_moveks, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 3, MUIM_CallHook, &movekshook, 42 );

			DoMethod( lt_nodes, MUIM_Notify, MUIA_NListtree_Active, MUIV_EveryTime,
				lt_nodes, 3, MUIM_CallHook, &movekshook, 0 );

			DoMethod( bt_copyks, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 3, MUIM_CallHook, &copykshook, 42 );

			DoMethod( lt_nodes, MUIM_Notify, MUIA_NListtree_Active, MUIV_EveryTime,
				lt_nodes, 3, MUIM_CallHook, &copykshook, 0 );


			/*
			**	find/parent/sort/getnr
			*/
			DoMethod( bt_find, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 2, MUIM_CallHook, &findnamehook );

			DoMethod( bt_parent, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 3, MUIM_Set, MUIA_NListtree_Active, MUIV_NListtree_Active_Parent );

			DoMethod( bt_sort, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 2, MUIM_CallHook, &sorthook );

			/*
			DoMethod( bt_sort, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 3, MUIM_NListtree_Sort, MUIV_NListtree_Sort_TreeNode_Active, 0 );
			*/

			DoMethod( bt_getnr, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 2, MUIM_CallHook, &getnrhook );


			/*
			**	redraw/selected/seltogg/showtree
			*/
			DoMethod( bt_redraw, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 3, MUIM_NListtree_Redraw, MUIV_NListtree_Redraw_All );

			DoMethod( bt_selected, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 2, MUIM_CallHook, &numselhook );

			DoMethod( bt_seltogg, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 5, MUIM_NListtree_Select, MUIV_NListtree_Select_All, MUIV_NListtree_Select_Toggle, 0, NULL );

			DoMethod( bt_showtree, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 3, MUIM_Set, MUIA_NListtree_ShowTree, MUIV_NListtree_ShowTree_Toggle );


			/*
			**	misc
			*/
			DoMethod( sl_treecol, MUIM_Notify, MUIA_Slider_Level, MUIV_EveryTime,
				lt_nodes, 3, MUIM_Set, MUIA_NListtree_TreeColumn, MUIV_TriggerValue );

			DoMethod( lt_nodes, MUIM_Notify, MUIA_NListtree_Active, MUIV_EveryTime,
				tx_info1, 4, MUIM_SetAsString, MUIA_Text_Contents, "Active node: 0x%08lx", MUIV_TriggerValue );

			DoMethod( lt_nodes, MUIM_Notify, MUIA_NListtree_ActiveList, MUIV_EveryTime,
				tx_info2, 4, MUIM_SetAsString, MUIA_Text_Contents, "Active list: 0x%08lx", MUIV_TriggerValue );

			DoMethod( lt_nodes, MUIM_Notify, MUIA_NListtree_DoubleClick, MUIV_EveryTime,
				tx_info3, 4, MUIM_SetAsString, MUIA_Text_Contents, "Double clicked on node: 0x%08lx", MUIV_TriggerValue );

			DoMethod( lt_nodes, MUIM_Notify, MUIA_NListtree_SelectChange, TRUE,
				tx_info3, 3, MUIM_SetAsString, MUIA_Text_Contents, "Selection state changed" );


			/*
			**	test
			*/
			DoMethod( bt_test, MUIM_Notify, MUIA_Pressed, FALSE,
				lt_nodes, 2, MUIM_CallHook, &testhook );



			/*
			**	Open the window.
			**
			*/
			set( window, MUIA_Window_Open, TRUE );


			/*
			**	Set the tree into quiet state.
			*/
			set( lt_nodes, MUIA_NListtree_Quiet, TRUE );


			/*
			**	Insert sample nodes.
			*/
			DrawSampleTree( lt_nodes );


			/*
			**	Set the tree back to normal state.
			*/
			set( lt_nodes, MUIA_NListtree_Quiet, FALSE );

			/*
			**	Minimal input loop.
			*/
			while((LONG)DoMethod( app, MUIM_Application_NewInput, &signals ) != (LONG)MUIV_Application_ReturnID_Quit )
			{
				if ( signals )
				{
					signals = Wait( signals | SIGBREAKF_CTRL_C );

					if ( signals & SIGBREAKF_CTRL_C )
						break;
				}
			}

			/*
			**	Clear the list.
			*/
			DoMethod( lt_nodes, MUIM_NListtree_Clear, NULL, 0 );


			/*
			**	Close the window.
			*/
			set( window, MUIA_Window_Open, FALSE );


			/*
			**	Shutdown
			*/
			MUI_DisposeObject( app );
		}
		else
			printf( "Failed to create Application.\n" );
	}

	if(MUIMasterBase)
  {
    DROPINTERFACE(IMUIMaster);
		CloseLibrary(MUIMasterBase);
  }

	if(IntuitionBase)
  {
    DROPINTERFACE(IIntuition);
		CloseLibrary((struct Library *)IntuitionBase);
  }

	return( 0 );
}


