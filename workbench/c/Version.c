/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Version CLI command
    Lang: english
*/

#include <aros/arosbase.h>
#include <aros/config.h>
#include <aros/inquire.h>
#include <proto/aros.h>

#define ENABLE_RT 1
#include <aros/rt.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <proto/exec.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <dos/datetime.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

static const char version[] = "$VER: Version 42.0 (18.10.2005)\n";

static const char ERROR_HEADER[] = "Version";

#define TEMPLATE "NAME/M,MD5SUM/S,VERSION/N,REVISION/N,FILE/S,FULL/S,RES/S"
struct
{
	CONST_STRPTR *arg_name;
	IPTR    arg_md5sum;
	LONG   *arg_version;
	LONG   *arg_revision;
	IPTR    arg_file;
	IPTR    arg_full;
	IPTR    arg_res;
}
args = { NULL, NULL, NULL, 0L, 0L, 0L };

LONG mversion, mrevision;

struct
{
	STRPTR pv_name;
	ULONG  pv_flags;
	LONG   pv_version;
	LONG   pv_revision;
	//LONG   pv_days;
	STRPTR pv_vername;
	STRPTR pv_revname;
	STRPTR pv_datestr;
	STRPTR pv_extralf;
	STRPTR pv_extrastr;
	UBYTE  pv_md5sum[16];
}
parsedver = { NULL, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, {0}};

#define PVF_MD5SUM		(1 << 0)
#define PVF_NOVERSION	(1 << 1)

static
int makeverstring(CONST_STRPTR name);
static
void printverstring(void);
static
void freeverstring(void);
static
int makesysver(void);
static
int cmpargsparsed(void);


/**************************** support functions ************************/

/* Duplicate string, by given length or -1 for full length
 */
static
STRPTR dupstr(CONST_STRPTR buffer, LONG len)
{
	STRPTR ret = NULL;

	if (buffer)
	{
		if (len == -1)
			len = strlen(buffer);

		ret = AllocVec(len + 1, MEMF_ANY);
		if (ret)
		{
			CopyMem((STRPTR) buffer, ret, len);
			ret[len] = '\0';
		}
	}

	return ret;
}


static
inline int myisspace(int c)
{
	return (c == ' ' || c == '\t');
}


/* Return a pointer to a string, stripped by all leading whitespace characters
 * (SPACE, TAB).
 */
static
STRPTR skipwhites(CONST_STRPTR buffer)
{
	for (;; buffer++)
	{
		if (buffer[0] == '\0' || !isspace(buffer[0]))
		{
			return (STRPTR) buffer;
		}
	}
}


/* Return a pointer to a string, stripped by all leading space characters
 * (SPACE).
 */
static
STRPTR skipspaces(CONST_STRPTR buffer)
{
	for (;; buffer++)
	{
		if (buffer[0] == '\0' || buffer[0] != ' ')
		{
			return (STRPTR) buffer;
		}
	}
}


/* Strip all whitespace-characters from the end of a string. Note that the
 * buffer passed in will be modified!
 */
static
void stripwhites(STRPTR buffer)
{
	int len = strlen(buffer);

	while (len > 0)
	{
		if (!isspace(buffer[len-1]))
		{
			buffer[len] = '\0';
			return;
		}
		len--;
	}
	buffer[len] = '\0';
}


/* Searches for a given string in a file and stores up to *lenptr characters
 * into the buffer beginning with the first character after the given string.
 */
static
int findinfile(BPTR file, CONST_STRPTR string, STRPTR buffer, int *lenptr, unsigned char digest[16])
{
	int error = RETURN_OK;
	int buflen = *lenptr, len = 0, pos, stringlen;
	BOOL ready = FALSE;
	MD5_CTX md5ctx;
	STRPTR bufpos;
	STRPTR tmp;

	tmp = AllocMem(buflen, MEMF_PUBLIC);
	if (!tmp)
	{
		return RETURN_FAIL;
	}

	stringlen = strlen(string);
	*lenptr = -1;

	if (args.arg_md5sum)
	{
		MD5Init(&md5ctx);
	}

	bufpos = tmp;
	while ((len = Read(file, &tmp[len], buflen - len)) > 0)
	{
		pos = 0;

		if (args.arg_md5sum)
		{
			MD5Update(&md5ctx, bufpos, len);
		}

		if (ready)
		{
			/* If we get here we're scanning the rest of the file for md5sum. - Piru */
			len = 0;
		}
		else
		{
			while ((len - pos) >= stringlen)
			{
				/* Compare the current buffer position with the supplied string. */
				if (strncmp(&tmp[pos], string, stringlen) == 0)
				{
					/* It is equal! Now move the rest of the buffer to the top of
					 * the buffer and fill it up.
					 */
					int findstrlen = len - pos;

					memcpy(buffer, &tmp[pos + stringlen], findstrlen);

					len = Read(file, &buffer[findstrlen], buflen - findstrlen);
					if (len >= 0)
					{
						if (args.arg_md5sum)
						{
							MD5Update(&md5ctx, &buffer[findstrlen], len);
						}

						*lenptr = findstrlen + len;
					}
					else
					{
						error = RETURN_FAIL;
					}
					ready = TRUE;
					break;
				}
				pos++;
			}
			/* Move the rest of the buffer that could not be compared (because it
			 * is smaller than the string to compare) to the top of the buffer.
			 */
			if (!ready)
			{
				memmove(tmp, &tmp[len - stringlen], stringlen);
			}
			else
			{
				/* If we're not md5summing, stop file scanning now. - Piru */
				if (!args.arg_md5sum)
				{
					break;
				}
			}
			len = stringlen;
		}

		bufpos = &tmp[len];
	}

	FreeMem(tmp, buflen);

	if (len == -1)
	{
		error = RETURN_FAIL;
	}

	if (args.arg_md5sum)
	{
		memset(digest, 0, 16);
		MD5Final(digest, &md5ctx);
	}

	return error;
}


/*************************** parsing functions *************************/

/* Convert a date in the form DD.MM.YY or DD.MM.YYYY into a numerical
 * value. Return FALSE, if buffer doesn't contain a valid date.
 */
static
BOOL makedatefromstring(CONST_STRPTR *bufptr)
{
	CONST_STRPTR buffer = *bufptr;
	struct DateTime dt;
	CONST_STRPTR headerstart, end;
	STRPTR newbuf;
	LONG res;
	int len, i;
	UBYTE c;

	//if (isspace(buffer[0]))
	//  buffer++;

	headerstart = buffer;

	buffer = strchr(buffer, '(');
	if (!buffer)
	{
		return FALSE;
	}
	buffer++;
	end = strchr(buffer, ')');
	if (!end)
	{
		return FALSE;
	}
	len = (int)(end - buffer);
	newbuf = dupstr(buffer, len);
	if (!newbuf)
	{
		return FALSE;
	}
	for (i = 0; i < len; i++)
	{
		c = newbuf[i];

		if (c == '.' || c == '/')
			newbuf[i] = '-';
		else if (!isalnum(c))
		{
			end = buffer + i;
			newbuf[i] = '\0';
			break;
		}
	}

	//Printf("date: \"%s\"\n", (LONG) newbuf);

	dt.dat_Format  = FORMAT_CDN;
	dt.dat_Flags   = 0;
	dt.dat_StrDay  = NULL;
	dt.dat_StrDate = newbuf;
	dt.dat_StrTime = NULL;
	res = StrToDate(&dt);
	if (!res)
	{
		dt.dat_Format  = FORMAT_DOS;
		res = StrToDate(&dt);
		if (!res)
		{
			//Printf("StrToDate failed!\n");
			FreeVec(newbuf);
			return FALSE;
		}
	}
	FreeVec(newbuf);

	//parsedver.pv_days = dt.dat_Stamp.ds_Days;

	parsedver.pv_datestr = AllocVec(buffer - headerstart + LEN_DATSTRING + 2, MEMF_ANY);
	if (!parsedver.pv_datestr)
	{
		return FALSE;
	}

	dt.dat_Stamp.ds_Minute = 0;
	dt.dat_Stamp.ds_Tick   = 0;

	dt.dat_StrDate = parsedver.pv_datestr + (buffer - headerstart);
	dt.dat_Format  = FORMAT_DEF;
	if (!DateToStr(&dt))
	{
		//Printf("DateToStr failed!\n");
		return FALSE;
	}

	CopyMem((STRPTR) headerstart, parsedver.pv_datestr, buffer - headerstart);
	res = strlen(parsedver.pv_datestr);
	parsedver.pv_datestr[res++] = ')';
	parsedver.pv_datestr[res] = '\0';

	*bufptr = end + 1;

	return TRUE;
}


/* Check whether the given string contains a version in the form
 * <version>.<revision> . If not return FALSE, otherwise fill in parsedver and
 * return TRUE.
 */
static
BOOL makeversionfromstring(CONST_STRPTR *bufptr)
{
	LONG pos, ver, rev;
	CONST_STRPTR buffer = *bufptr;
	CONST_STRPTR verstart, revstart;

	//Printf("makeversionfromstring: buffer \"%s\"\n", (LONG) buffer);

	/* Do version */

	verstart = buffer;
	pos = StrToLong((STRPTR) buffer, &ver);
	if (pos == -1)
	{
		return FALSE;
	}
	parsedver.pv_version  = ver;
	parsedver.pv_revision = -1;

	parsedver.pv_vername = dupstr(verstart, pos);
	if (!parsedver.pv_vername)
	{
		return FALSE;
	}

	/* Do revision */

	buffer += pos;
	revstart = buffer;
	buffer = skipspaces(buffer);  /* NOTE: skipspaces, not skipwhites! */
	if (*buffer != '.')
	{
		*bufptr = buffer;
		return TRUE;
	}

	buffer++;
	pos = StrToLong((STRPTR) buffer, &rev);
	if (pos == -1)
	{
		*bufptr = buffer;
		return TRUE;
	}

	parsedver.pv_revision = rev;

	/* calc the revision string len */
	pos = buffer + pos - revstart;
	parsedver.pv_revname = dupstr(revstart, pos);
	if (!parsedver.pv_revname)
	{
		return FALSE;
	}

	*bufptr = revstart + pos;

	return TRUE;
}


static
void printverstring(void)
{
	if (args.arg_md5sum)
	{
		if (parsedver.pv_flags & PVF_MD5SUM)
		{
			VPrintf("%08lX%08lX%08lX%08lX  ", parsedver.pv_md5sum);
		}
		else
		{
			/*     "01234567012345670123456701234567: " */
			PutStr("<no md5sum available>             ");
		}
	}

	if (parsedver.pv_flags & PVF_NOVERSION)
	{
		Printf("%s\n", (LONG) parsedver.pv_name);
	}
	else
	{
		if (args.arg_full)
		{
			/* If md5sum output was there, avoid linefeed to allow parsing the output - Piru */
			if (args.arg_md5sum)
			{
				parsedver.pv_extralf = " ";
			}

			Printf("%s%s%s%s%s%s%s\n",
			       (LONG) parsedver.pv_name, (LONG) (*parsedver.pv_name ? " " : ""),
			       (LONG) parsedver.pv_vername, (LONG) parsedver.pv_revname,
			       (LONG) parsedver.pv_datestr, (LONG) parsedver.pv_extralf, (LONG) parsedver.pv_extrastr);
		}
		else
		{
			Printf("%s%s%s%s\n",
			       (LONG) parsedver.pv_name, (LONG) (*parsedver.pv_name ? " " : ""),
			       (LONG) parsedver.pv_vername, (LONG) parsedver.pv_revname);
		}
	}
}


static
int makedata(CONST_STRPTR buffer, CONST_STRPTR ptr, int pos)
{
	//Printf("makedata: buffer \"%s\" prt \"%s\"\n", (LONG) buffer, (LONG) ptr);

	if (makeversionfromstring(&ptr))
	{
		CONST_STRPTR endp;
		BOOL doskip;

		/* It is! */
		/* Copy the program-name into a buffer. */
		parsedver.pv_name = dupstr(buffer, pos);
		if (!parsedver.pv_name)
		{
			PrintFault(ERROR_NO_FREE_STORE, (STRPTR) ERROR_HEADER);
			return RETURN_FAIL;
		}

		/* Now find the date */
		//Printf("makedatafromstring: ptr #1: \"%s\"\n", (LONG) ptr);
		doskip = strchr(ptr, '(') ? TRUE : FALSE;
		(void) makedatefromstring(&ptr);

		//Printf("makedatafromstring: ptr #2: \"%s\"\n", (LONG) ptr);
		if (doskip)
			ptr = skipspaces(ptr);	/* NOTE: not skipwhites! */
		for (endp = ptr; *endp != '\0' && *endp != '\r' && *endp != '\n'; endp++)
			;
		pos = endp - ptr;
		if (pos)
		{
			parsedver.pv_extrastr = dupstr(ptr, pos);
			if (!parsedver.pv_extrastr)
			{
				PrintFault(ERROR_NO_FREE_STORE, (STRPTR) ERROR_HEADER);
				return RETURN_FAIL;
			}

			if (doskip)
				parsedver.pv_extralf = "\n";
		}

		return 1;
	}

	return 0;
}


/* Retrieves version information from string. The data is stored in the
 * global struct parsedver.pv_
 */

static
int makedatafromstring(CONST_STRPTR buffer)
{
	int error = RETURN_OK;
	int pos = 0;
	LONG add, dummy;

	while (buffer[pos] && buffer[pos] != '\r' && buffer[pos] != '\n')
	{
		/* NOTE: Not isspace()! - Piru */
		if (myisspace(buffer[pos]) &&
		    (add = StrToLong((STRPTR) buffer + pos + 1, &dummy)) != -1)
		{
			CONST_STRPTR ptr;

			/* Found something, which looks like a version. Now check, if it
			 * really is.
			 */

			//Printf("makedatafromstring: buffer + %ld: \"%s\"\n", pos, (LONG) buffer + pos);

			ptr = buffer + pos + 1;

			if (makedata(buffer, ptr, pos))
			{
				break;
			}
		}
		pos++;
	}

	if (!buffer[pos] || buffer[pos] == '\r' || buffer[pos] == '\n')
	{
		CONST_STRPTR endp;

		/* use the whatever is after ver tag as name */
		for (endp = buffer; *endp != '\0' && *endp != '\r' && *endp != '\n'; endp++)
			;
		pos = endp - buffer;

		parsedver.pv_name = dupstr(buffer, pos);
		if (!parsedver.pv_name)
		{
			PrintFault(ERROR_NO_FREE_STORE, (STRPTR) ERROR_HEADER);
			return RETURN_FAIL;
		}
	}

	/* Strip any whitespaces from the tail of the program-name.
	 */
	if (parsedver.pv_name)
	{
		stripwhites(parsedver.pv_name);
	}

	return error;
}


/* Case-insensitive FindResident()
 */
static
struct Resident *findresident(CONST_STRPTR name)
{
	struct Resident **rp;
	struct Resident *resident;

	rp = (struct Resident **) SysBase->ResModules;

	while ((resident = *rp++))
	{
		if (((LONG) resident) > 0)
		{
			if (!Stricmp(resident->rt_Name, (STRPTR) name))
			{
				break;
			}
		}
		else
		{
			rp = (struct Resident **) (((ULONG) resident) & (ULONG) ~(1 << 31));
		}
	}

	return resident;
}


/* Case-insensitive FindName()
 */
static
struct Node *findname(struct List *list, CONST_STRPTR name)
{
	struct Node *node;

	ForeachNode(list, node)
	{
		if (!Stricmp(node->ln_Name, (STRPTR) name))
		{
			return node;
		}
	}

	return NULL;
}


/* Retrieve information from resident modules. Returns 0 for success.
 */
static
int createresidentver(struct Resident *MyResident)
{
	STRPTR buffer = NULL, tmpbuffer;
	int error, pos = 0, foundver = FALSE;

	if (MyResident->rt_IdString)
	{
		buffer = skipwhites(MyResident->rt_IdString);
		//Printf("createresidentver: buffer \"%s\"\n", (LONG) buffer);

		/* Locate version part */
		while (buffer[pos])
		{
			LONG dummy;

			/* NOTE: Not isspace()! - Piru */
			if (myisspace(buffer[pos]) &&
			    StrToLong(buffer + pos + 1, &dummy) != -1)
			{
				buffer += pos;
				foundver = TRUE;
				break;
			}
			pos++;
		}
		//Printf("createresidentver: buffer: \"%s\"\n", (LONG) buffer);
	}


	/* If could not find any version info, use the resident rt_Name */
	if (!foundver)
		buffer = "";

	//Printf("createresidentver: buffer: \"%s\"\n", (LONG) buffer);

	tmpbuffer = AllocVec(strlen(MyResident->rt_Name) + strlen(buffer) + 1, MEMF_ANY);
	if (!tmpbuffer)
	{
		PrintFault(ERROR_NO_FREE_STORE, (STRPTR) ERROR_HEADER);
		return RETURN_FAIL;
	}

	strcpy(tmpbuffer, MyResident->rt_Name);
	strcat(tmpbuffer, buffer);
	//Printf("createresidentver: tmpbuffer: \"%s\"\n", (LONG) tmpbuffer);
	error = makedatafromstring(tmpbuffer);

	FreeVec(tmpbuffer);

	return error;
}


/* Retrieve version information from library. Returns 0 for success.
 */
static
int createlibraryver(struct Library *MyLibrary)
{
	STRPTR buffer, tmpbuffer;
	int error, foundver = FALSE, pos;

	if (MyLibrary->lib_IdString)
	{
		//Printf("createlibraryver: lib_IdString \"%s\"\n", (LONG) MyLibrary->lib_IdString);
		buffer = skipwhites(MyLibrary->lib_IdString);

		//Printf("createlibraryver: buffer \"%s\"\n", (LONG) buffer);

		/* Find full 'ver.rev' version info
		 */
		pos = 0;
		while (buffer[pos])
		{
			LONG dummy, add;

			/* NOTE: Not isspace()! - Piru */
			if (myisspace(buffer[pos]) &&
			    (add = StrToLong(buffer + pos + 1, &dummy)) != -1 &&
				buffer[pos + 1 + add] == '.')
			{
				buffer += pos;
				pos = 0;
				foundver = TRUE;
				break;
			}
			pos++;
		}

		/* If could not find 'ver.rev', find any numeric */
		if (!foundver)
		{
			pos = 0;
			while (buffer[pos])
			{
				LONG dummy;

				/* NOTE: Not isspace()! - Piru */
				if (myisspace(buffer[pos]) &&
				    StrToLong(buffer + pos + 1, &dummy) != -1)
				{
					buffer += pos;
					pos = 0;
					foundver = TRUE;
					break;
				}
				pos++;
			}
		}
	}

	/* If could not find any version info, use the resident rt_Name */
	if (!foundver)
	{
		buffer = "";
		error = RETURN_WARN;
	}

	tmpbuffer = AllocVec(strlen(MyLibrary->lib_Node.ln_Name) + strlen(buffer) + 1, MEMF_ANY);
	if (!tmpbuffer)
	{
		PrintFault(ERROR_NO_FREE_STORE, (STRPTR) ERROR_HEADER);
		return RETURN_FAIL;
	}

	strcpy(tmpbuffer, MyLibrary->lib_Node.ln_Name);
	strcat(tmpbuffer, buffer);
	//Printf("createlibraryver: tmpbuffer: \"%s\"\n", (LONG) tmpbuffer);
	pos = makedatafromstring(tmpbuffer);
	if (pos > error)
	{
		error = pos;
	}

	FreeVec(tmpbuffer);

	return error;
}


/* Create default strings
 */
static
int createdefvers(CONST_STRPTR name)
{
	FreeVec(parsedver.pv_revname);
	FreeVec(parsedver.pv_vername);
	FreeVec(parsedver.pv_name);

	parsedver.pv_name    = dupstr(name, -1);
	parsedver.pv_vername = AllocVec(14, MEMF_ANY);
	parsedver.pv_revname = AllocVec(15, MEMF_ANY);

	if (parsedver.pv_name &&
	    parsedver.pv_vername &&
	    parsedver.pv_revname)
	{
		sprintf(parsedver.pv_vername, "%ld", parsedver.pv_version);
		sprintf(parsedver.pv_revname, ".%ld", parsedver.pv_revision);

		return RETURN_OK;
	}

	return RETURN_FAIL;
}


/* Create version info from named resident
 */
static
int makeresidentver(CONST_STRPTR name)
{
	struct Resident *MyResident;
	int error = -1;

	if ((MyResident = findresident(name)))
	{
		error = createresidentver(MyResident);
		if (error != RETURN_OK)
		{
			/* get values from residenttag */
			parsedver.pv_version  = MyResident->rt_Version;
			parsedver.pv_revision = -1;
			error = createdefvers(MyResident->rt_Name);
		}
	}

	return error;
}


/* Create version info from named list node
*/
static
int makeexeclistver(struct List *list, CONST_STRPTR name)
{
	struct Library *MyLibrary;
	int error = -1;

	Forbid();

	MyLibrary = (struct Library *) findname(list, name);
	if (MyLibrary)
	{
		/* get values from library */
		ULONG ver = MyLibrary->lib_Version;
		ULONG rev = MyLibrary->lib_Revision;

		error = createlibraryver(MyLibrary);
		if (error != RETURN_OK ||
		    parsedver.pv_version != ver ||
		    parsedver.pv_revision != rev)
		{
			/* special case where createlibraryrev was successful, but
			 * version or revision don't match.
			 */
			if (error == RETURN_OK)
			{
				/* If there is extrastr, make sure there's linefeed, too.
				 */
				if (parsedver.pv_extrastr)
				{
					parsedver.pv_extralf = "\n";
				}
			}

			/* get values from library */
			parsedver.pv_version  = ver;
			parsedver.pv_revision = rev;

			error = createdefvers(MyLibrary->lib_Node.ln_Name);
		}
	}

	Permit();

	return error;
}


/* Find resident from seglist, return NULL if none found
*/
static
struct Resident *FindLibResident(BPTR	Segment)
{
	while (Segment)
	{
		ULONG		*MySegment;
		UWORD		*MyBuffer;
		UWORD		*EndBuffer;

		MySegment	= (ULONG*) BADDR(Segment);
		MyBuffer	= (UWORD*) &MySegment[1];
		EndBuffer	= (UWORD*) &MySegment[(MySegment[-1] - sizeof(ULONG) * 4) / sizeof(ULONG)];

		while (MyBuffer < EndBuffer)
		{
			struct Resident	*MyResident;

			MyResident = (struct Resident*) MyBuffer;
			if (MyResident->rt_MatchWord == RTC_MATCHWORD &&
			    MyResident->rt_MatchTag == MyResident)
			{
				return MyResident;
			}

			MyBuffer++;
		}

		Segment	=(BPTR) MySegment[0];
	}

	SetIoErr(ERROR_OBJECT_NOT_FOUND);
	return NULL;
}


/* Find $VER: tag from seglist, return NULL if none found
   Returns dupstr()d string or NULL.
*/
static
STRPTR FindSegmentVER(BPTR	Segment)
{
	while (Segment)
	{
		ULONG		*MySegment;
		CONST_STRPTR	MyBuffer;
		CONST_STRPTR	EndBuffer;
		CONST_STRPTR	SegmentEnd;

		MySegment	= (ULONG*) BADDR(Segment);
		MyBuffer	= (CONST_STRPTR) &MySegment[1];
		SegmentEnd	= ((CONST_STRPTR) MySegment) + MySegment[-1] - sizeof(ULONG) * 2;
		EndBuffer	= SegmentEnd - 5;

		while (MyBuffer < EndBuffer)
		{
			if (MyBuffer[0] == '$' &&
			    MyBuffer[1] == 'V' &&
			    MyBuffer[2] == 'E' &&
			    MyBuffer[3] == 'R' &&
			    MyBuffer[4] == ':')
			{
				CONST_STRPTR EndPtr;

				MyBuffer += 5;
				/* Required because some smartass could end his $VER: tag
				 * without '\0' in the segment to save space. - Piru
				 */
				for (EndPtr = MyBuffer; EndPtr < SegmentEnd && *EndPtr; EndPtr++)
					;
				if (EndPtr - MyBuffer)
				{
					return dupstr(MyBuffer, EndPtr - MyBuffer);
				}
			}

			MyBuffer++;
		}

		Segment	=(BPTR) MySegment[0];
	}

	SetIoErr(ERROR_OBJECT_NOT_FOUND);
	return NULL;
}


/* Create version info from named device
*/
static
int makedevicever(CONST_STRPTR name)
{
	struct DevProc *MyDevProc;
	int error = -1;

	MyDevProc = GetDeviceProc((STRPTR) name, NULL);
	if (MyDevProc)
	{
		if (MyDevProc->dvp_DevNode->dol_Type == DLT_DEVICE)
		{
			BPTR SegList;

			SegList = MyDevProc->dvp_DevNode->dol_misc.dol_handler.dol_SegList;
			if (SegList)
			{
				struct Resident *MyResident;

				MyResident = FindLibResident(SegList);
				if (MyResident)
				{
					error = createresidentver(MyResident);
				}
				else
					error = RETURN_FAIL;
			}
		}
		FreeDeviceProc(MyDevProc);
	}

	if (error != RETURN_OK && error != -1)
	{
		Printf("Could not find version information for '%s'\n", (LONG) name);
	}

	return error;
}


/* Retrieve version information from file. Return 0 for success.
 */
#define BUFFERSIZE (16384 + 1)
static
int makefilever(CONST_STRPTR name)
{
	BPTR file;
	int error; // = RETURN_OK;

	file = Open((STRPTR) name, MODE_OLDFILE);
	if (file)
	{
		UBYTE *buffer;

		buffer = AllocMem(BUFFERSIZE, MEMF_PUBLIC);
		if (buffer)
		{
			int len = BUFFERSIZE - 1;

			error = findinfile(file, "$VER:", buffer, &len, parsedver.pv_md5sum);
			if (error == RETURN_OK)
			{
				parsedver.pv_flags |= PVF_MD5SUM;

				if (len >= 0)
				{
					STRPTR startbuffer;

					buffer[len] = '\0';
					startbuffer = skipwhites(buffer);

					//Printf("startbuffer \"%s\"\n", startbuffer);
					error = makedatafromstring(startbuffer);
				}
				else
				{
					/* Try LoadSeg
					 */
					error = RETURN_ERROR;

					Close(file);

					file = LoadSeg((STRPTR) name);
					if (file)
					{
						struct Resident *MyResident;

						MyResident = FindLibResident(file);
						if (MyResident /*&&
						    (MyResident->rt_Type == NT_LIBRARY ||
						     MyResident->rt_Type == NT_DEVICE)*/)
						{
							error = createresidentver(MyResident);
						}

						UnLoadSeg(file);
					}
					file = NULL;

					if (error != RETURN_OK)
					{
						/* If user didn't ask for md5sum or we could not calculate it.
						*/
						if (!args.arg_md5sum || (!(parsedver.pv_flags & PVF_MD5SUM)))
						{
							Printf("Could not find version information for '%s'\n", (LONG) name);
						}
					}
				}
			}
			else
			{
				PrintFault(IoErr(), (STRPTR) ERROR_HEADER);
			}

			FreeMem(buffer, BUFFERSIZE);
		}
		else
		{
			error = RETURN_FAIL;
			PrintFault(IoErr(), (STRPTR) ERROR_HEADER);
		}

		if (file)
			Close(file);
	}
	else
	{
		LONG ioerr = IoErr();

		if (ioerr == ERROR_OBJECT_NOT_FOUND ||
		    ioerr == ERROR_OBJECT_WRONG_TYPE)
		{
			error = -1;
		}
		else
		{
			PrintFault(IoErr(), (STRPTR) ERROR_HEADER);
			error = RETURN_FAIL;
		}
	}

	return error;
}


static
int makerescmdver(CONST_STRPTR name)
{
	int error = -1;
	struct Segment *segment;

	Forbid();

	segment = FindSegment((STRPTR) name, NULL, 0);
	if (!segment)
	{
		segment = FindSegment((STRPTR) name, NULL, 1);
	}

	if (segment)
	{
		if (segment->seg_UC == CMD_INTERNAL ||
		    segment->seg_UC == CMD_DISABLED)
		{
			Permit();
			error = makeresidentver("shell");
			Forbid();
		}
		else
		{
			STRPTR buffer = FindSegmentVER(segment->seg_Seg);
			if (buffer)
			{
				STRPTR startbuffer;

				startbuffer = skipwhites(buffer);

				//Printf("startbuffer \"%s\"\n", (LONG) startbuffer);
				error = makedatafromstring(startbuffer);

				FreeVec(buffer);
			}
		}
	}

	Permit();

	return error;
}

static
int setvervar(CONST_STRPTR name, LONG ver, LONG rev)
{
	UBYTE buf[32];

	sprintf(buf, "%ld.%ld", (LONG) ver, (LONG) rev);

	return SetVar((STRPTR) name, buf, -1, GVF_LOCAL_ONLY | LV_VAR) ? RETURN_OK : -1;
}


static
int makekickversion(void)
{
	parsedver.pv_version  = SysBase->LibNode.lib_Version;
	parsedver.pv_revision = SysBase->SoftVer;

	setvervar("Kickstart",
	          parsedver.pv_version,
	          parsedver.pv_revision);

	Printf("Kickstart %ld.%ld",
	       (LONG) parsedver.pv_version, (LONG) parsedver.pv_revision);

	return RETURN_OK;
}


static
int makewbversion(void)
{
	int error = -1;
	struct Library *VersionBase;

	VersionBase = OpenLibrary("version.library", 0);
	if (VersionBase)
	{
		error = makeexeclistver(&SysBase->LibList, "version.library");

		if (error == RETURN_OK)
		{
			STRPTR newname = dupstr("Workbench", -1);
			if (newname)
			{
				FreeVec(parsedver.pv_name);
				parsedver.pv_name = newname;
			}
			setvervar("Workbench", parsedver.pv_version, parsedver.pv_revision);
		}

		CloseLibrary(VersionBase);
	}

	return error;
}


static
int makesysver(void)
{
	int error;

	error = makekickversion();
	if (error == RETURN_OK)
	{
		error = makewbversion();

		if (error == RETURN_OK)
		{
			PutStr(", ");
		}
		else
		{
			/* prevent silly errormsg if no version.library */
			PutStr("\n");
			error = RETURN_WARN;
		}
	}

	return error;
}


/* Determine, by which means to get the version-string.
 */
static
int makeverstring(CONST_STRPTR name)
{
	int error; // = RETURN_OK;
	BOOL volume = name[strlen(name) - 1] == ':';
	CONST_STRPTR filepart = FilePart(name);

	error = -1;

	if (!volume && !args.arg_file)
	{
		if (*filepart)
		{
			error = makeresidentver(filepart);
			if (error != RETURN_OK)
			{

				/* Try libraries
				 */
				error = makeexeclistver(&SysBase->LibList, filepart);
				if (error != RETURN_OK)
				{
					STRPTR namebuf;
					ULONG namelen = strlen(filepart);

					/* 12 is "MOSSYS:LIBS/" */
					if ((namebuf = AllocVec(12 + namelen + 4 + 1, MEMF_PUBLIC)))
					{
						strcpy(namebuf, "LIBS:");
						strcat(namebuf, filepart);
						error = makefilever(namebuf);

						/* Try devices
						*/
						if (error != RETURN_OK)
						{
							error = makeexeclistver(&SysBase->DeviceList, filepart);
							if (error != RETURN_OK)
							{
								strcpy(namebuf, "DEVS:");
								strcat(namebuf, filepart);
								error = makefilever(namebuf);
							}
						}
						FreeVec(namebuf);
					}
				}
			}
		}
	}

	if (!args.arg_res && error == -1)
	{
		if (volume)
		{
			error = makedevicever(name);
		}
		else
		{
			if (*filepart)
			{
				error = makefilever(name);
			}
		}
	}

	if (!args.arg_file && error == -1)
	{
		error = makerescmdver(name);
	}

	if (error)
	{
		/* If user asked for md5sum, and we could calculate it, don't print error
		 *  but the md5sum + file.
		 */
		if (args.arg_md5sum && (parsedver.pv_flags & PVF_MD5SUM))
		{
			parsedver.pv_name = dupstr(name, -1);
			parsedver.pv_flags |= PVF_NOVERSION;
			error = RETURN_OK;
		}
	}

	if (error == -1)
	{
		PrintFault(ERROR_OBJECT_NOT_FOUND, (STRPTR) ERROR_HEADER);
		error = RETURN_FAIL;
	}

	return error;
}


static
void freeverstring(void)
{
	parsedver.pv_flags    = 0;
	parsedver.pv_version  = 0;
	parsedver.pv_revision = 0;

	FreeVec(parsedver.pv_extrastr);
	parsedver.pv_extrastr = NULL;
	parsedver.pv_extralf = NULL;
	FreeVec(parsedver.pv_datestr);
	parsedver.pv_datestr  = NULL;
	FreeVec(parsedver.pv_revname);
	parsedver.pv_revname  = NULL;
	FreeVec(parsedver.pv_vername);
	parsedver.pv_vername  = NULL;
	FreeVec(parsedver.pv_name);
	parsedver.pv_name     = NULL;
}

/* Compare the version given as argument with the version from the object.
 * Return RETURN_WARN, if args-v>object-v, otherwise return RETURN_OK.
 */
static
int cmpargsparsed(void)
{
	if (args.arg_version)
	{
		if (*(args.arg_version) > parsedver.pv_version)
		{
			return RETURN_WARN;
		}
		else if (*(args.arg_version) == parsedver.pv_version && args.arg_revision)
		{
			if (*(args.arg_revision) > parsedver.pv_revision)
			{
				return RETURN_WARN;
			}
		}
	}
	else if (args.arg_revision)
	{
		if (*(args.arg_revision) > parsedver.pv_revision)
		{
			return RETURN_WARN;
		}
	}
	return RETURN_OK;
}

/******************************* main program ****************************/

/* Compare the version given as argument with the version from the object.
   Return RETURN_WARN, if args-v>object-v, otherwise return RETURN_OK. */
int cmpargsparsed()
{
    if (args.version != NULL)
    {
	if (*(args.version) > parsedver.version)
	    return(RETURN_WARN);
	else if (*(args.version) == parsedver.version && args.revision != NULL)
	{
	    if (*(args.revision) > parsedver.revision)
		return(RETURN_WARN);
	}
    } else if (args.revision != NULL)
    {
	if (*(args.revision) > parsedver.revision)
	    return(RETURN_WARN);
    }
    return(RETURN_OK);
}

/* Check whether the arguments are correct. */
int verifyargs()
{
    int error = RETURN_OK;

    if (args.file != 0L && args.res != 0L)
	error = RETURN_FAIL;
    if (args.name == NULL && (args.res != 0L || args.file != 0L))
	error = RETURN_FAIL;

    if (error == RETURN_FAIL)
	PrintFault(ERROR_BAD_TEMPLATE, (char *)ERROR_HEADER);

    return(error);
}

int __nocommandline;

int main (void)
{
	struct RDArgs *rda;

	rda = ReadArgs(TEMPLATE, (IPTR *) &args, NULL);
	if (rda)
	{
		if (!args.arg_name || !*args.arg_name)
		{
			/* No args, make system version */
			error = makesysver();
			if (error == RETURN_OK)
			{
				printverstring();
					if (parsedver.pv_flags & PVF_NOVERSION)
				{
					error = RETURN_FAIL;
				}
				if (error == RETURN_OK)
				{
					error = cmpargsparsed();
				}
			}
			freeverstring();
		}
		else
		{
			CONST_STRPTR *name;
			BOOL multifile;
#if 1
			/* Workaround for:
			 * version file ver
			 * version file ver rev
			 */
			if (!args.arg_version && !args.arg_revision)
			{
				LONG narg = 1;
				while (args.arg_name[narg]) { narg++; }
				if (narg == 2 || narg == 3)
				{
					if (StrToLong(args.arg_name[1], &mversion) > 0)
					{
						args.arg_version = &mversion;
							args.arg_name[1] = args.arg_name[2];
						if (narg == 3)
						{
							args.arg_name[2] = NULL;
						}
							if (narg == 3)
						{
							if (StrToLong(args.arg_name[1], &mrevision) > 0)
							{
								args.arg_revision = &mrevision;
								args.arg_name[1] = NULL;
							}
						}
					}
				}
			}
#endif
			multifile = args.arg_name[1] != NULL;

			for (name = args.arg_name; *name; name++)
			{
				error = makeverstring(*name);
				if (error == RETURN_OK)
				{
					printverstring();

					if (!multifile)
					{
						/* Single args, do compare stuff also */
						if (parsedver.pv_flags & PVF_NOVERSION)
						{
							error = RETURN_FAIL;
						}
						if (error == RETURN_OK)
						{
							error = cmpargsparsed();
						}
					}
				}
				freeverstring();

			}
		}

		FreeArgs(rda);
	}
	else
	{
		PrintFault(IoErr(), (STRPTR) ERROR_HEADER);
		error = RETURN_FAIL;
	}

        RT_Exit();

        return(error);
}
