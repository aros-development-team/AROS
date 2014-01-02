/******************************************************************************\
********************************************************************************
***
***	$Source$
***	$Revision$
***	$Date$
***
***	$Id$
***
***	$Author$
***
***
***
***	UNLESS OTHERWISE NOTED, ALL FILES ARE
***	COPYRIGHT (C) 2000 BY "APOCALYPSE HARD- UND SOFTWARE CARSTEN SCHOLLING".
***	ALL RIGHTS RESERVED.
***
***	NO PART OF THIS SOFTWARE MAY BE COPIED, REPRODUCED, TRANSMITTED, REDIST-
***	RIBUTED,  DISCLOSED OR DUPLICATED  IN ANY FORM OR BY ANY MEANS,  WITHOUT
***	THE PRIOR WRITTEN PERMISSION OF "APOCALYPSE HARD- UND SOFTWARE".
***
***
***	DISCLAIMER
***
***	THIS SOFTWARE AND INFORMATION IS PROVIDED "AS IS". NO REPRESENTATIONS OR
***	WARRANTIES ARE MADE WITH RESPECT TO THE ACCURACY,  RELIABILITY,  PERFOR-
***	MANCE,  CURRENTNESS OR  OPERATION OF THIS SOFTWARE AND INFORMATION,  AND
***	ALL USE IS AT YOUR OWN RISK. NEITHER "APOCALYPSE HARD- UND SOFTWARE" NOR
***	THE AUTHORS  ITSELF ASSUME  ANY RESPONSIBILITY  OR LIABILITY  WHATSOEVER
***	WITH RESPECT TO YOUR  USE OF THIS SOFTWARE AND INFORMATION.
***
***
***
***
***	$Log$
***	Revision 1.1  2004/07/10 20:31:44  damato
***	- inital checkin
***
***
***
********************************************************************************
\******************************************************************************/

#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/intuition.h>
#include <clib/alib_protos.h>
#include <libraries/mui.h>

#include <compiler.h>
#include <debug.h>


#include "ClosedNode_rev.h"

#define IMAGE "ClosedNode"


#define libfunc(x)	image_Lib ## x
#define reqfunc		muilink_use_image
#define addname(x)	image_ ## x


/*
**	$setver$
*/
STATIC const char UserLibID[] = VERSTAG;


struct Data {

	struct MUI_RenderInfo	*mri;
};


#include <MUI/imageheader.c>


STATIC ULONG ASM _BoopsiQuery( REG(a0) struct IClass *cl, REG(a2) Object *obj, REG(a1) struct MUI_BoopsiQuery *mbq )
{
	struct Data *data = INST_DATA( cl, obj );

	//D(bug( "\n" ) );

	if ( mbq->mbq_Flags & MBQF_HORIZ )
	{
		mbq->mbq_MinWidth	= 8;
		mbq->mbq_MaxWidth	= MBQ_MUI_MAXMAX;
		mbq->mbq_DefWidth	= 8;

		mbq->mbq_MinHeight	= 8;
		mbq->mbq_MaxHeight	= MBQ_MUI_MAXMAX;
		mbq->mbq_DefHeight	= 8;
	}
	else
	{
		mbq->mbq_MinWidth	= 8;
		mbq->mbq_MaxWidth	= MBQ_MUI_MAXMAX;
		mbq->mbq_DefWidth	= 8;

		mbq->mbq_MinHeight	= 8;
		mbq->mbq_MaxHeight	= MBQ_MUI_MAXMAX;
		mbq->mbq_DefHeight	= 8;
	}

	data->mri = mbq->mbq_RenderInfo;

	return( MUIM_BoopsiQuery );
}



STATIC ULONG ASM _Draw( REG(a0) struct IClass *cl, REG(a2) Object *obj, REG(a1) struct impDraw *msg )
{
	#define img ( (struct Image *)obj )
	struct Data *data	= INST_DATA( cl, obj );
	struct RastPort *rp	= msg->imp_RPort;

	if ( rp != NULL )
	{
		ULONG	l	= img->LeftEdge	+ msg->imp_Offset.X;
		ULONG	t	= img->TopEdge	+ msg->imp_Offset.Y;
		ULONG	w	= img->Width;
		ULONG	h	= img->Height;
		ULONG	r	= l + w - 1;
		ULONG	b	= t + h - 1;
		BOOL	sel	= ( msg->imp_State == IDS_SELECTED ) ? TRUE : FALSE;
		UBYTE	backgroundpen	= data->mri ? data->mri->mri_Pens[MPEN_BACKGROUND]	: msg->imp_DrInfo->dri_Pens[BACKGROUNDPEN];
		UBYTE	halfshinepen	= data->mri ? data->mri->mri_Pens[MPEN_HALFSHINE]	: msg->imp_DrInfo->dri_Pens[BACKGROUNDPEN];
		UBYTE	shadowpen		= data->mri ? data->mri->mri_Pens[MPEN_SHADOW]		: msg->imp_DrInfo->dri_Pens[SHADOWPEN];
		UBYTE	shinepen		= data->mri ? data->mri->mri_Pens[MPEN_SHINE]		: msg->imp_DrInfo->dri_Pens[SHINEPEN];

		//D(bug( "\n" ) );

		SetDrMd( rp, JAM1 );

		/*
		if ( ( w > 2 ) && ( h > 2 ) )
		{
			SetAPen( rp, backgroundpen );
			RectFill( rp, l, t, r, b );
		}
		*/

		SetAPen( rp, sel ? shadowpen : shinepen );

		/*
		**	##++++++
		**	#+##++++
		**	#+++##++
		**	#+++++##
		**	#+++++++
		**	#+++++++
		**	#+++++++
		**	++++++++
		*/
		Move( rp, l,	b - 1 );
		Draw( rp, l,	t );
		Draw( rp, r,	t + 3 );

		SetAPen( rp, sel ? shinepen : shadowpen );

		/*
		**	**++++++
		**	*+**++++
		**	*+++**++
		**	*+++++**
		**	*+++++##
		**	*+++##++
		**	*+##++++
		**	##++++++
		*/
		Move( rp, l, b );
		Draw( rp, r, b - 3 );

		/*
		**	Arrow fill...
		*/
		SetAPen( rp, halfshinepen );

		/*
		**	**++++++
		**	*#**++++
		**	*###**++
		**	*#####**
		**	*#####**
		**	*###**++
		**	*#**++++
		**	**++++++
		*/
		Move( rp, l + 1,	t + 1 );
		Draw( rp, l + 1,	b - 1 );

		Move( rp, l + 2,	t + 2 );
		Draw( rp, l + 2,	b - 2 );

		Move( rp, l + 3,	t + 2 );
		Draw( rp, l + 3,	b - 2 );

		Move( rp, l + 4,	t + 3 );
		Draw( rp, l + 4,	b - 3 );

		Move( rp, l + 5,	t + 3 );
		Draw( rp, l + 5,	b - 3 );
	}

	return( 0 );
}


ULONG ASM SAVEDS _Dispatcher( REG(a0) struct IClass *cl, REG(a2) Object *obj, REG(a1) Msg msg )
{
	switch ( msg->MethodID )
	{
		case IM_DRAW:			return( _Draw(			cl, obj, (APTR)msg ) );
		case MUIM_BoopsiQuery:	return( _BoopsiQuery(	cl, obj, (APTR)msg ) );
	}

	return( DoSuperMethodA( cl, obj, msg ) );
}


