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

static const char version[] = "$VER: Version 41.6 (13.09.1998)\n";

static const char ERROR_HEADER[] = "Version";

#define ARGSTRING "NAME,VERSION/N,REVISION/N,FILE/S,FULL/S,RES/S"
struct
{
    STRPTR name;
    UWORD *version;
    UWORD *revision;
    IPTR   file;
    IPTR   full;
    IPTR   res;
} args = { NULL, NULL, NULL, 0L, 0L, 0L };

struct
{
    STRPTR name;
    UWORD  version;
    UWORD  revision;
    LONG  days;
} parsedver = { NULL, 0, 0, 0L };

/* This points to the full (unparsed) version string. */
STRPTR verbuffer = NULL;

/**************************** support functions ************************/

int power(int base, int pow)
{
    int num = 1;

    for (; pow > 0; pow--)
	num *= base;
    return(num);
}

/* Makes a string from an unsigned number. Returns the actual length of the
   string. */
int number2string(unsigned int number, STRPTR string)
{
    int length = 0;
    int len;
    int firstnum, pow;

    if (number == 0)
    {
	string[0] = '0';
	string[1] = '\0';
	return(1);
    }

    while (power(10, length) <= number)
	length++;

    for (len = length; len > 0; len--)
    {
	pow = power(10, len - 1);
	firstnum = number / pow;
	string[length - len] = firstnum + 48;
	number -= firstnum * pow;
    }
    string[length] = '\0';

    return(length);
}

/* Return a pointer to a string, stripped by all leading whitespace characters
   (SPACE, TAB). */
char *skipwhites(char *buffer)
{
    for(;; buffer++)
	if(buffer[0] == '\0' || !isspace(buffer[0]))
	    return(buffer);
}

/* Strip all whitespace-characters from the end of a string. Note that the
   buffer passed in will be modified! */
void stripwhites(char *buffer)
{
    int len = strlen(buffer);

    while(len > 0)
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
   into the buffer beginning with the first character after the given string.
*/
int findinfile(BPTR file, STRPTR string, STRPTR buffer, int *lenptr)
{
    int error = RETURN_OK;
    int buflen = *lenptr, len = 0, pos, stringlen;
    BOOL ready = FALSE;

    stringlen = strlen(string);
    *lenptr = -1;

    while ((len = Read(file, &buffer[len], buflen - len)) > 0)
    {
	pos = 0;
	while ((len - pos) >= stringlen)
	{
            /* Compare the current buffer position with the supplied string. */
	    if (strncmp(&buffer[pos], string, stringlen) == 0)
	    {
                /* It is equal! Now move the rest of the buffer to the top of
                   the buffer and fill it up. */
		int findstrlen = len - pos - stringlen;

		memmove(buffer, &buffer[pos+stringlen], findstrlen);
		len = Read(file, &buffer[findstrlen], buflen - findstrlen);
		if (len >= 0)
		    *lenptr = findstrlen + len;
		else
		    error = RETURN_FAIL;
		ready = TRUE;
		break;
	    }
	    pos++;
	}
        /* Move the rest of the buffer that could not be compared (because it
           is smaller than the string to compare) to the top of the buffer. */
	if (ready == FALSE)
	    memmove(buffer, &buffer[len - stringlen], stringlen);
	else
	    break;
	len = stringlen;
    }

    if (len == -1)
	error = RETURN_FAIL;

    return(error);
}

/*************************** parsing functions *************************/

/* Convert a date in the form DD.MM.YY or DD.MM.YYYY into a numerical
   value. Return FALSE, if buffer doesn't contain a valid date. */
BOOL makedatefromstring(char *buffer)
{
    struct DateTime dt;
    char *end, *newbuf;
    int len;

    buffer = strchr(buffer, '(');
    if(!buffer)
        return FALSE;

    end = strchr(buffer, ')');
    if(!end)
        return FALSE;

    len = (int)(end - buffer);
    newbuf = AllocVec(len + 1, MEMF_CLEAR);
    if(!newbuf)
        return FALSE;
    CopyMem(buffer, newbuf, len);

    dt.dat_Format = FORMAT_CDN;
    dt.dat_Flags = 0;
    dt.dat_StrDate = newbuf;
    dt.dat_StrTime = NULL;
    if(!StrToDate(&dt)) {
        FreeVec(newbuf);
        return FALSE;
    }
    FreeVec(newbuf);

    parsedver.days = dt.dat_Stamp.ds_Days;
    return TRUE;
}

/* Check whether the given string contains a version in the form
   <version>.<revision> . If not return FALSE, otherwise fill in parsedver and
   return TRUE. */
BOOL makeversionfromstring(char *buffer)
{
    char numberbuffer[6];
    int pos;

    for (pos = 0;; pos++)
    {
	if (((pos == 5) && (buffer[pos] != '.')) || (buffer[pos] == '\0'))
	    return(FALSE);
	if (buffer[pos] == '.')
	{
	    if (pos == 0)
		return(FALSE);
	    numberbuffer[pos] = '\0';
	    break;
	}
	if ((buffer[pos] < '0') || (buffer[pos] > '9'))
	    return(FALSE);
	numberbuffer[pos] = buffer[pos];
    }
    parsedver.version = strtoul(numberbuffer, NULL, 10);
    buffer = &buffer[pos+1];
    for (pos = 0;; pos++)
    {
	if ((pos == 5) && (buffer[pos] != ' ') && (buffer[pos] != '\t') && (buffer[pos] != '\0'))
	{
	    parsedver.version = 0;
	    return(FALSE);
	}
	if ((buffer[pos] == ' ') || (buffer[pos] == '\0'))
	{
	    if (pos == 0)
	    {
		parsedver.version = 0;
		return(FALSE);
	    }
	    numberbuffer[pos] = '\0';
	    break;
	}
	if ((buffer[pos] < '0') || (buffer[pos] > '9'))
	{
	    parsedver.version = 0;
	    return(FALSE);
	}
	numberbuffer[pos] = buffer[pos];
    }
    parsedver.revision = strtoul(numberbuffer, NULL, 10);

    return(TRUE);
}

/* Retrieves version information from string. The data is stored in the
   global struct parsedver. */
int makedatafromstring(char *buffer)
{
    int error = RETURN_OK;
    int pos = 0;

    while(buffer[pos])
    {
	if ((buffer[pos] == ' ') &&
            (buffer[pos+1] >= '0') && (buffer[pos+1] <='9'))
        {
            /* Found something, which looks like a version. Now check, if it
               really is. */
            if (makeversionfromstring(&buffer[pos+1]))
            {
                /* It is! */
                /* Copy the program-name into a buffer. */
                parsedver.name = AllocVec(pos + 1, MEMF_ANY);
                if (parsedver.name == NULL)
                {
                    PrintFault(ERROR_NO_FREE_STORE, (char *)ERROR_HEADER);
                    return(RETURN_FAIL);
                }
                CopyMem(buffer, parsedver.name, pos);
                parsedver.name[pos] = '\0';

                /* Now find the date. */
                for (; buffer[pos] != '\0' && buffer[pos] != ' '; pos++);
                makedatefromstring(&buffer[pos]);
                break;
            } else
            {
                /* It is not so skip to the first non-numeric character. */
                pos++;
                while ((buffer[pos+1] >= '1') && (buffer[pos+1] <'0'))
                    pos++;
            }
        }
        pos++;
    }
    /* Strip any whitespaces from the tail of the program-name. */
    if (parsedver.name != NULL)
	stripwhites(parsedver.name);

    return(error);
}

/* Retrieve information from resident modules. Returns -1, if the named module
   was not found. */
int makeresidentver(STRPTR name)
{
#warning FIXME: not implemented
    SetIoErr(ERROR_NOT_IMPLEMENTED);
    return(-1);
}

/* Retrieve information from file. Returns -1, if the file was not found. */
#define BUFFERSIZE 1024
int makefilever(STRPTR name)
{
    int error = RETURN_OK;
    BPTR file;
    char buffer[BUFFERSIZE];

    file = Open(name, MODE_OLDFILE);
    if (file != (BPTR)NULL)
    {
	int len = BUFFERSIZE - 1;

	error = findinfile(file, "$VER:", buffer, &len);
	if (error == RETURN_OK)
	{
	    if (len >= 0)
	    {
		char *startbuffer;

		buffer[len] = '\0';
		startbuffer = skipwhites(buffer);
		len = strlen(startbuffer);
		verbuffer = AllocVec(len + 1, MEMF_ANY);
		if (verbuffer == NULL)
		{
		    PrintFault(ERROR_NO_FREE_STORE, (char *)ERROR_HEADER);
		    error = RETURN_FAIL;
		} else
		{
		    CopyMem(startbuffer, verbuffer, len + 1);
		    error = makedatafromstring(startbuffer);
		}
	    } else
	    {
		VPrintf("No version information found\n", NULL);
		error = RETURN_ERROR;
	    }
	} else
	    PrintFault(IoErr(), (char *)ERROR_HEADER);
	Close(file);
    } else
    {
	if (IoErr() == ERROR_OBJECT_NOT_FOUND)
	    error = -1;
	else
	{
	    PrintFault(IoErr(), (char *)ERROR_HEADER);
	    error = RETURN_FAIL;
	}
    }
    return(error);
}

/* Build information from internal kickstart data. */
int ArosBase_version = 0;

int makekickver()
{
    int len;
    struct DateTime dt;

    /* Fill in struct parsedver. */
    ArosInquire(
                AI_ArosVersion, (ULONG)&parsedver.version,
                AI_ArosReleaseMinor, (ULONG)&parsedver.revision,
                AI_ArosReleaseDate, (ULONG)&parsedver.days,
                TAG_DONE);

#define KICKSTRLEN 5
    parsedver.name = AllocVec(KICKSTRLEN, MEMF_ANY);
    if (parsedver.name == NULL)
    {
	PrintFault(ERROR_NO_FREE_STORE, (char *)ERROR_HEADER);
	return RETURN_FAIL;
    }
    CopyMem("AROS", parsedver.name, KICKSTRLEN);

    /* Make string used by output with FULL switch. */
#define MAXKICKSTRLEN (KICKSTRLEN + LEN_DATSTRING + 15)
    verbuffer = AllocVec(MAXKICKSTRLEN, MEMF_ANY);
    if (verbuffer == NULL)
    {
	PrintFault(ERROR_NO_FREE_STORE, (char *)ERROR_HEADER);
	return(RETURN_FAIL);
    }
    CopyMem("AROS ", verbuffer, 5);
    len = 5;
    len += number2string(parsedver.version, &verbuffer[len]);
    verbuffer[len] = '.';
    len++;
    len += number2string(parsedver.revision, &verbuffer[len]);
    verbuffer[len++] = ' ';
    verbuffer[len++] = '(';
    /* Make date for string. */
    dt.dat_Stamp.ds_Days = parsedver.days;
    dt.dat_Stamp.ds_Minute = 0L;
    dt.dat_Stamp.ds_Tick = 0L;
    dt.dat_Format = FORMAT_DEF;
    dt.dat_Flags = 0;
    dt.dat_StrDay = NULL;
    dt.dat_StrDate = &verbuffer[len];
    dt.dat_StrTime = NULL;
    DateToStr(&dt);
    len = strlen(verbuffer);
    /* Fill up string. */
    verbuffer[len] = ')';
    verbuffer[len+1] = '\0';

    return(RETURN_OK);
}

/* Determine, by which means to get the version-string. */
int makeverstring()
{
    int error = RETURN_OK;

    if (args.name == NULL)
    {
	error = makekickver();
    } else
    {
	error = -1;
	if (args.file == 0L && error == -1)
	    error = makeresidentver(args.name);
	if (args.res == 0L && error == -1)
	    error = makefilever(args.name);
	if (error == -1)
	{
	    PrintFault(ERROR_OBJECT_NOT_FOUND, (char *)ERROR_HEADER);
	    error = RETURN_FAIL;
	}
    }

    return(error);
}

/* Clean up. */
void freeverstring()
{
    FreeVec(verbuffer);
    FreeVec(parsedver.name);
}

/**************************** output functions **************************/

/* Print only the short version string. */
void printparsedstring()
{
    IPTR args[3];
    args[0] = (IPTR)parsedver.name;
    args[1] = (IPTR)parsedver.version;
    args[2] = (IPTR)parsedver.revision;
    VPrintf("%s %ld.%ld\n", args);
}

/* Print the full version string. */
void printstring()
{
    VPrintf("%s\n", (IPTR *)&verbuffer);
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
    LONG error = RETURN_OK;

    RT_Init();

    rda = ReadArgs (ARGSTRING, (IPTR *)&args, NULL);

    if (rda != NULL)
    {
        error = verifyargs();
	if (error == RETURN_OK)
	{
	    error = makeverstring();
	    if (error == RETURN_OK)
	    {
		if (args.full == 0L)
		    printparsedstring();
		else
		    printstring();
		error = cmpargsparsed();
	    }
	    freeverstring();
	}
	FreeArgs (rda);
    }
    else
    {
	PrintFault(IoErr(), (char *)ERROR_HEADER);
	error = RETURN_FAIL;
    }

    RT_Exit();

    return(error);
}
