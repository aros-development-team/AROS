/**********************************************************************
 text.datatype - (c) 2000 by Sebastian Bauer

 This module provides some support functions
***********************************************************************/

#include <string.h>

#include <graphics/clip.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>
#include <proto/layers.h>

/* Define the following to enable the debug version */
//#define MYDEBUG
#include "debug.h"

#include "../dtio64.h"

DTIO_DOS64_SUPPORT()

/**************************************************************************************************/

struct MinNode *Node_Next(APTR node)
{
    if(node == NULL) return NULL;
    if(((struct MinNode*)node)->mln_Succ == NULL) return NULL;
    if(((struct MinNode*)node)->mln_Succ->mln_Succ == NULL)
	return NULL;

    return ((struct MinNode*)node)->mln_Succ;
}

/**************************************************************************************************/

struct MinNode *Node_Prev(APTR node)
{
    if(node == NULL) return NULL;
    if(((struct MinNode*)node)->mln_Pred == NULL) return NULL;
    if(((struct MinNode*)node)->mln_Pred->mln_Pred == NULL)
	return NULL;

    return ((struct MinNode*)node)->mln_Pred;
}


/**************************************************************************************************/

struct MinNode *List_First(APTR list)
{
    if( !((struct MinList*)list)->mlh_Head) return NULL;
    if(((struct MinList*)list)->mlh_Head->mln_Succ == NULL) return NULL;

    return ((struct MinList*)list)->mlh_Head;
}

/**************************************************************************************************/

#if 1

struct MinNode *List_Last(APTR list)
{
    if( !((struct MinList*)list)->mlh_TailPred) return NULL;
    if(((struct MinList*)list)->mlh_TailPred->mln_Pred == NULL) return NULL;

    return ((struct MinList*)list)->mlh_TailPred;
}

#endif

/**************************************************************************************************/

#if 0

ULONG List_Length(APTR list)
{
    struct MinNode *node = List_First(list);
    ULONG 	   len=0;

    while(node)
    {
	len++;
	node = Node_Next(node);
    }
    
    return len;
}

#endif

/**************************************************************************************************/

#if 0

struct MinNode *List_Find(APTR list, ULONG num)
{
    struct MinNode *node = List_First(list);

    while(num--)
    {
	if(!(node = Node_Next(node))) break;
    }

    return node;
}

#endif


/**************************************************************************
 Dupplicates a string
**************************************************************************/
STRPTR StrCopy( const STRPTR str )
{
    STRPTR dst;

    if( !str ) return NULL;
    if( !*str) return NULL;

    dst = (STRPTR)AllocVec(strlen(str) + 1, 0);
    if(dst) strcpy(dst, str);

    return dst;
}

/**************************************************************************
 Returns the size of a file (64-bit, via dos64.library when available)
**************************************************************************/
QUAD GetFileSize( BPTR fileh )
{
    return DTIO_GetFileSize(fileh);
}

/**************************************************************************
 Prepares the clipboard so that text can be written into it
**************************************************************************/
struct IFFHandle *PrepareClipboard(void)
{
    struct IFFHandle *iff = AllocIFF();

    if(iff)
    {
	if((iff->iff_Stream = (IPTR) OpenClipboard (0)))
	{
	    InitIFFasClip(iff);
	    if(!OpenIFF(iff,IFFF_WRITE))
	    {
		if(!PushChunk(iff, MAKE_ID('F','T','X','T'), MAKE_ID('F','O','R','M'), IFFSIZE_UNKNOWN))
		{
		    if(!PushChunk(iff, 0, MAKE_ID('C','H','R','S'), IFFSIZE_UNKNOWN))
		    {
			return iff;
		    }
		    PopChunk(iff);
		}
		CloseIFF(iff);
	    }
	    CloseClipboard((struct ClipboardHandle*)iff->iff_Stream);
	}
	FreeIFF(iff);
    }

    return NULL;
}

/**************************************************************************
 Free all resources allocated in PrepareClipboard
**************************************************************************/
VOID FreeClipboard(struct IFFHandle *iff)
{
    PopChunk(iff);
    PopChunk(iff);
    CloseIFF(iff);
    CloseClipboard((struct ClipboardHandle*)iff->iff_Stream);
    FreeIFF(iff);
}

/**************************************************************************
 Install a clip region
**************************************************************************/
struct Region *installclipregion (struct Layer *l, struct Region *r)
{
    BOOL update;
    struct Region *or;

    if (l->Flags & LAYERUPDATING)
    {
	EndUpdate(l,FALSE);
	update = TRUE;
    } else update = FALSE;

    or = InstallClipRegion (l, r);

    if (update)
	BeginUpdate(l);

    return or;
}

