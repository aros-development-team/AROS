/**************************************************************************************************/

#include "text_intern.h"

/**************************************************************************************************/

#ifndef _AROS

/**************************************************************************************************/

#ifndef __MAXON__

STATIC VOID SR_CopyFunc(VOID)
{
    __emit(0x16c0); // MOVE.B  D0,(A3)+
}

#else
VOID SR_CopyFunc(VOID);
#endif

//-------------------------------------
int sprintf( char *buf, const char *fmt,...)
{
    RawDoFmt((STRPTR)fmt, (STRPTR*)&fmt + 1, (void(*)())SR_CopyFunc, buf);
    
    return 1;
}

/**************************************************************************************************/

#ifndef __MAXON__

/**************************************************************************************************/

char *strncpy(char *dest, const char *src, size_t n)
{
    LONG i;
    char *destPtr = dest;
    
    for(i = 0; i < n; i++)
    {
	char c = *src++;
	
	*destPtr++ = c;
	if(c == 0) break;
    }
    
    while(i<n)
    {
	*destPtr++ = 0;
	i++;
    }
    
    return dest;
}

/**************************************************************************************************/

int strncmp(const char *str1, const char *str2, size_t n)
{
    LONG i;
    
    for(i = 0; i < n; i++)
    {
	char a = *str1++;
	char b = *str2++;
	char c = a - b;

	if( c != 0) return c;
	if( a == 0) return 0;
    }
    
    return 0;
}

/**************************************************************************************************/

#endif /* __MAXON__ */

/**************************************************************************************************/

#endif /* _AROS */

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

#if 0

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


/**************************************************************************************************/

STRPTR StrCopy( const STRPTR str )
{
    STRPTR dst;

    if( !str ) return NULL;
    if( !*str) return NULL;

    dst = (STRPTR)AllocVec(strlen(str) + 1, 0);
    if(dst) strcpy(dst, str);

    return dst;
}

/**************************************************************************************************/

#if 0

STRPTR StrCopyPool( APTR pool, const STRPTR str )
{
    STRPTR dst;

    if( !str ) return NULL;
    if( !*str) return NULL;

    dst = (STRPTR)AllocVecPooled(pool, strlen(str) + 1);
    if(dst) strcpy(dst,str);

    return dst;
}

#endif

/**************************************************************************************************/

#ifndef _AROS

STRPTR StrNCopyPool( APTR pool, const STRPTR str, LONG len)
{
    STRPTR s = NULL;
    
    if(pool)
    {
	if((s=(STRPTR)AllocVecPooled(pool, len + 1)))
	{
	    strncpy(s, str, len);
	    s[len] = 0;
	    
	    return s;
	}
    }
    
    return s;
}

#endif

/**************************************************************************************************/

LONG GetFileSize( BPTR fileh )
{
    struct FileInfoBlock *fib = (struct FileInfoBlock*)AllocDosObject( DOS_FIB, NULL );
    LONG		 size = -1;
    
    if (fib)
    {
	if (ExamineFH( fileh, fib ))
	{
	    if ((fib->fib_DirEntryType > 0) && (fib->fib_DirEntryType != ST_SOFTLINK))
	    {
		size = 0;
	    }
	    size = fib->fib_Size;
	}
	FreeDosObject( DOS_FIB, fib );
    }
    
    return size;
}

/**************************************************************************************************/

struct IFFHandle *PrepareClipboard(void)
{
    struct IFFHandle *iff = AllocIFF();

    if(iff)
    {
	if((iff->iff_Stream = (ULONG) OpenClipboard (0)))
	{
	    InitIFFasClip(iff);
	    if(!OpenIFF(iff,IFFF_WRITE))
	    {
		if(!PushChunk(iff, 'FTXT', 'FORM', IFFSIZE_UNKNOWN))
		{
		    if(!PushChunk(iff, 0, 'CHRS', IFFSIZE_UNKNOWN))
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

/**************************************************************************************************/

VOID FreeClipboard(struct IFFHandle *iff)
{
    PopChunk(iff);
    PopChunk(iff);
    CloseIFF(iff);
    CloseClipboard((struct ClipboardHandle*)iff->iff_Stream);
    FreeIFF(iff);
}

/**************************************************************************************************/
