/* $Id$ */
/* $Log: support.c $
 * Revision 10.11  1998/09/27  11:26:37  Michiel
 * ErrorMsg extra param.
 *
 * Revision 10.10  1996/01/30  12:51:14  Michiel
 * --- working tree overlap ---
 * Free memory functions check for NULL pointers
 *
 * Revision 10.9  1995/10/04  14:03:17  Michiel
 * amigalib memorypools -> support 2.0 too
 *
 * Revision 10.8  1995/09/01  11:17:41  Michiel
 * ErrorMsg adaption (see disk.c and volume.c)
 *
 * Revision 10.7  1995/08/04  11:50:37  Michiel
 * From default allocate-retry to default not-retry-but-fail
 *
 * Revision 10.6  1995/07/21  06:57:10  Michiel
 * fixed bug in AllocBufmemR (double size registration)
 *
 * Revision 10.5  1995/07/11  17:29:31  Michiel
 * ErrorMsg () calls use messages.c variables now.
 *
 * Revision 10.4  1995/06/16  10:02:17  Michiel
 * added pool for buffer memory
 *
 * Revision 10.3  1995/06/15  18:56:21  Michiel
 * pooled mem
 * functions added
 *
 * Revision 10.2  1995/01/29  07:34:57  Michiel
 * Minor changes
 *
 * Revision 10.1  1994/10/24  11:16:28  Michiel
 * first RCS revision
 * */

static void OutOfMemory (globaldata *g);

/*
 * Allocate from main pool (type MEMF_CLEAR)
 * without retry
 */
void *AllocPooledVec (ULONG size, globaldata *g)
{
  ULONG *buffer;

	buffer = LibAllocPooled (g->mainPool, size+sizeof(ULONG));
	if (buffer)
		*buffer++ = size;

	return buffer;
}

void FreePooledVec (void *mem, globaldata *g)
{
	if (mem)
		LibFreePooled (g->mainPool, (ULONG *)mem - 1, *((ULONG *)mem - 1) + sizeof(ULONG));
}

/*
 * Buffer allocation
 */
void *AllocPooledBuf (ULONG size, globaldata *g)
{
  ULONG *buffer;

	buffer = LibAllocPooled (g->bufferPool, size+sizeof(ULONG));
	if (buffer)
		*buffer++ = size;

	if (((IPTR)buffer) & ~g->dosenvec->de_Mask)
		ErrorMsg (AFS_WARNING_MEMORY_MASK, NULL, g);

	return buffer;
}

void FreePooledBuf (void *mem, globaldata *g)
{
	if (mem)
		LibFreePooled (g->bufferPool, (ULONG *)mem - 1, *((ULONG *)mem - 1) + sizeof(ULONG));
}


/*
 * retrying variants of 'globaldata' allocation functions
 */
void *AllocMemPR (ULONG size, globaldata *g)
{
  ULONG *buffer;

	while (!(buffer = AllocMemP (size, g)))
		OutOfMemory (g);

	return buffer;
}

void *AllocBufmemR (ULONG size, globaldata *g)
{
  ULONG *buffer;

	while (!(buffer = AllocBufmem (size, g)))
		OutOfMemory (g);

	return buffer;
}

/*
 * Retrying AllocVec
 */
VOID *AllocMemR (ULONG size, ULONG flags, globaldata *g)
{
 UBYTE *buffer;

	while (!(buffer=AllocVec (size, flags)))
	{
		OutOfMemory (g);
	}

	return buffer;
}

static void OutOfMemory (globaldata *g)
{
	FreeUnusedResources (g->currentvolume, g);
	NormalErrorMsg (AFS_ERROR_PLEASE_FREE_MEM, NULL, 1);	// I MUST have memory!
}


/* SUPPORTFUNCTION dstricmp
** TRUE: dstring == cstring
** FALSE: cstring <> cstring
*/
BOOL dstricmp (DSTR dstring, STRPTR cstring)
{
  BOOL result;
  UBYTE temp[PATHSIZE];

	ctodstr (cstring, temp);
	intltoupper (temp);
	result = intlcmp (temp, dstring);  
	return result;
}

/* ddstricmp
** compare two dstrings
*/
BOOL ddstricmp (DSTR dstr1, DSTR dstr2)
{
  BOOL result;
  UBYTE temp[PATHSIZE];

	strncpy (temp,dstr1,*dstr1+1);
	intltoupper (temp);
	result = intlcmp (temp,dstr2);
	return result;
} 


// BCPLtoCString converts BCPL string to a CString. 
UBYTE *BCPLtoCString(STRPTR dest, DSTR src)
{
#ifdef AROS_FAST_BSTR
  strcpy(dest, (CONST_STRPTR)src);
  return dest;
#else
  UBYTE len, *to;

	len  = *(src++);
	len	 = min (len, PATHSIZE);
	to	 = dest;

	while (len--)
		*(dest++) = *(src++);
	*dest = 0x0;

	return to;
#endif
}
