/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Version CLI command
    Lang: english
*/

#define ENABLE_RT		1
#define ENABLE_RT_INTUITION	0

#include <aros/rt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <dos/datetime.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

static const char version[] = "$VER: Version 41.0 (14.6.1997)\n";

#define ERROR_HEADER "Version"

#define ARGSTRING "NAME,FILE/S,FULL/S,RES/S"
struct
{
    STRPTR name;
    IPTR   file;
    IPTR   full;
    IPTR   res;
} args = { NULL, 0L, 0L, 0L };

struct
{
    STRPTR name;
    UWORD  version;
    UWORD  revision;
    ULONG  days;
} parsedver = { NULL, 0, 0, 0L };

STRPTR verbuffer;

/**************************** support functions ************************/

int power(int base, int pow)
{
    int num = 1;

    for (; pow > 0; pow--)
	num *= base;
    return(num);
}

/* make a string from an unsigned number - returns length of string */
int number2string(int number, STRPTR string)
{
    int length = 0;
    int len;
    int firstnum, pow;

    if (number == 0)
    {
	string[0] = '0';
	string[1] = 0x00;
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
    string[length] = 0x00;

    return(length);
}

/* skip all whitespace-characters (SPACE, TAB) */
char *skipwhites(char *buffer)
{
    for(;;)
    {
	if ((buffer[0] != ' ' && buffer[0] != '\t') || buffer[0] == '\0')
	    return buffer;
	buffer++;
    }
}

/* strip all whitespace-characters from the end of a string */
void stripwhites(char *buffer)
{
    int len = strlen(buffer);

    while(len > 0)
    {
	if (buffer[len-1] != ' ' && buffer[len-1] != '\t')
	{
	    buffer[len] = 0x00;
	    return;
	}
	len--;
    }
    buffer[len] = 0x00;
}

/* searches for a given string in a file and stores up to *lenptr characters
   into the buffer beginning with the first character after the given string */
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
	    if (strncmp(buffer + pos, string, stringlen) == 0)
	    {
		int findstrlen; /* length of the string, after the string to
				   find */
		findstrlen = len - pos - stringlen;

		memmove(buffer, buffer + pos + stringlen, findstrlen);
		len = Read(file, buffer + findstrlen, buflen - findstrlen);
		if (len >= 0)
		    *lenptr = findstrlen + len;
		else
		    error = RETURN_FAIL;
		ready = TRUE;
		break;
	    }
	    pos++;
	}
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

/* The following function is not tested! */
int makedatefromstring(char *buffer)
{
    /* !!! */
    return RETURN_OK;
}

/* Check whether the given string contains a version in the form
   <version>.<revision> . If not return -1, else fill in parsedver. */
int makeversionfromstring(char *buffer)
{
    char numberbuffer[6];
    int pos;

    for (pos=0;;pos++)
    {
	if (((pos == 5) && (buffer[pos] != '.')) || (buffer[pos] == 0x00))
	    return -1;
	if (buffer[pos] == '.')
	{
	    if (pos == 0)
		return -1;
	    numberbuffer[pos] = 0x00;
	    break;
	}
	if ((buffer[pos] < '0') || (buffer[pos] > '9'))
	    return -1;
	numberbuffer[pos] = buffer[pos];
    }
    parsedver.version = strtoul(numberbuffer, NULL, 10);
    buffer = &buffer[pos+1];
    for (pos=0;;pos++)
    {
	if ((pos == 5) && ((buffer[pos] != ' ') || (buffer[pos] != 0x00)))
	    return -1;
	if ((buffer[pos] == ' ') || (buffer[pos] == 0x00))
	{
	    if (pos == 0)
		return -1;
	    numberbuffer[pos] = 0x00;
	    break;
	}
	if ((buffer[pos] < '0') || (buffer[pos] > '9'))
	    return -1;
	numberbuffer[pos] = buffer[pos];
    }
    parsedver.revision = strtoul(numberbuffer, NULL, 10);

    return RETURN_OK;
}

/* fill in parsedver from provided string */
int makedatafromstring(char *buffer)
{
    int error = RETURN_OK;
    int pos;

    for (pos = 0; buffer[pos] != 0x00; pos++)
    {
	if (buffer[pos] == ' ')
	{
	    /* Version is missing in $VER: string */
	    if (buffer[pos+1] == '(')
	    {
		parsedver.name = AllocVec(pos + 1, MEMF_ANY);
		if (parsedver.name == NULL)
		{
		    PrintFault(ERROR_NO_FREE_STORE, ERROR_HEADER);
		    return RETURN_FAIL;
		}
		CopyMem(buffer, parsedver.name, pos);
		parsedver.name[pos] = 0x00;
		makedatefromstring(&buffer[pos+1]);
		break;
	    /* Version is there */
	    } else if ((buffer[pos+1] >= '0') && (buffer[pos+1] <='9'))
	    {
		/* Is it really a version at the current position? */
		if ((error = makeversionfromstring(&buffer[pos+1])) == RETURN_OK)
		{
		    /* It is! */
		    parsedver.name = AllocVec(pos + 1, MEMF_ANY);
		    if (parsedver.name == NULL)
		    {
			PrintFault(ERROR_NO_FREE_STORE, ERROR_HEADER);
			return RETURN_FAIL;
		    }
		    CopyMem(buffer, parsedver.name, pos);
		    parsedver.name[pos] = 0x00;
		    for (; buffer[pos] != 0x00 && buffer[pos] != ' '; pos++);
		    pos = skipwhites(&buffer[pos]) - buffer;
		    makedatefromstring(&buffer[pos]);
		    break;
		} else if (error != -1)
		    return error;
	    }
	}
    }
    /* strip any whitespaces at the tail of the program-name */
    if (parsedver.name != NULL)
	stripwhites(parsedver.name);

    return error;
}

/* build information from resident modules */
int makeresidentver(STRPTR name)
{
    /* !!! */
    SetIoErr(ERROR_NOT_IMPLEMENTED);
    return(-1);
}

/* build information from file */
#define BUFFERSIZE 1024
int makefilever(STRPTR name)
{
    int error = RETURN_OK;
    BPTR file;
    char buffer[BUFFERSIZE];

    file = Open(name, MODE_OLDFILE);
    if (file != NULL)
    {
	int len = BUFFERSIZE - 1;

	error = findinfile(file, "$VER:", buffer, &len);
	if (error == RETURN_OK)
	{
	    if (len >= 0)
	    {
		char *startbuffer;

		buffer[len] = 0x00;
		startbuffer = skipwhites(buffer);
		len = strlen(startbuffer);
		verbuffer = AllocVec(len + 1, MEMF_ANY);
		if (verbuffer == NULL)
		{
		    PrintFault(ERROR_NO_FREE_STORE, ERROR_HEADER);
		    error = RETURN_FAIL;
		} else
		{
		    CopyMem(startbuffer, verbuffer, len + 1);
		    error = makedatafromstring(startbuffer);
		}
	    } else
	    {
		printf("No version information found\n");
		error = RETURN_ERROR;
	    }
	} else
	    PrintFault(IoErr(), ERROR_HEADER);
	Close(file);
    } else
    {
	if (IoErr() == ERROR_OBJECT_NOT_FOUND)
	    error = -1;
	else
	{
	    PrintFault(IoErr(), ERROR_HEADER);
	    error = RETURN_FAIL;
	}
    }
    return(error);
}

/* build information from internal kickstart data */
int makekickver()
{
    int len;
    struct DateTime dt;

    /* make parsedver */
    parsedver.version  = SysBase->LibNode.lib_Version;
    parsedver.revision = SysBase->LibNode.lib_Revision;
    parsedver.days = 0L; /* !!! should be the real date !!! */
#define KICKSTRLEN 10
    parsedver.name = AllocVec(KICKSTRLEN, MEMF_ANY);
    if (parsedver.name == NULL)
    {
	PrintFault(ERROR_NO_FREE_STORE, ERROR_HEADER);
	return(RETURN_FAIL);
    }
    CopyMem("Kickstart", parsedver.name, KICKSTRLEN);

    /* make string */
#define MAXKICKSTRLEN (25 + LEN_DATSTRING)
    verbuffer = AllocVec(MAXKICKSTRLEN, MEMF_ANY);
    if (verbuffer == NULL)
    {
	PrintFault(ERROR_NO_FREE_STORE, ERROR_HEADER);
	return(RETURN_FAIL);
    }
    CopyMem("Kickstart ", verbuffer, 10);
    len = 10;
    len += number2string(parsedver.version, &verbuffer[len]);
    verbuffer[len] = '.';
    len++;
    len += number2string(parsedver.revision, &verbuffer[len]);
    verbuffer[len++] = ' ';
    verbuffer[len++] = '(';
    /* make date */
    dt.dat_Stamp.ds_Days = parsedver.days;
    dt.dat_Stamp.ds_Minute = 0L;
    dt.dat_Stamp.ds_Tick = 0L;
    dt.dat_Format = FORMAT_DEF;
    dt.dat_Flags = 0;
    dt.dat_StrDay = NULL;
    dt.dat_StrDate = verbuffer + len;
    dt.dat_StrTime = NULL;
    DateToStr(&dt);
    len = strlen(verbuffer);
    verbuffer[len] = ')';
    verbuffer[len + 1] = 0x00;

    return(RETURN_OK);
}

/* determine, which information to build */
int makeverstring()
{
    int error = RETURN_OK;

    if (args.name == NULL)
    {
	error = makekickver();
    } else
    {
	error = -1;
	if (args.file == 0L)
	    error = makeresidentver(args.name);
	if (args.res == 0L && error == -1)
	    error = makefilever(args.name);
	if (error == -1)
	{
	    PrintFault(ERROR_OBJECT_NOT_FOUND, ERROR_HEADER);
	    error = RETURN_FAIL;
	}
    }

    return(error);
}

void freeverstring()
{
    FreeVec(verbuffer);
    FreeVec(parsedver.name);
}

/**************************** output functions **************************/

/* print only the short version string */
void printparsedstring()
{
    printf("%s %d.%d\n", parsedver.name, parsedver.version, parsedver.revision);
}

/* print the full version string */
void printstring()
{
    printf("%s\n", verbuffer);
}

/******************************* main program ****************************/

/* Check whether the arguments are correct */
int verifyargs()
{
    int error = RETURN_OK;

    if (args.file != 0L && args.res != 0L)
	error = RETURN_FAIL;
    if (args.name == NULL && (args.res != 0L || args.file != 0L))
	error = RETURN_FAIL;

    if (error == RETURN_FAIL)
    {
	PrintFault(ERROR_BAD_TEMPLATE, ERROR_HEADER);
	error = RETURN_FAIL;
    }

    return(error);
}

int main (int argc, char ** argv)
{
    struct RDArgs *rda;
    LONG error = RETURN_OK;

    RT_Init();

    rda = ReadArgs (ARGSTRING, (IPTR *)&args, NULL);

    if (rda != NULL)
    {
	if ((error = verifyargs()) == RETURN_OK)
	{
	    error = makeverstring();
	    if (error == RETURN_OK)
	    {
		if (args.full == 0L)
		    printparsedstring();
		else
		    printstring();
	    }
	    freeverstring();
	}
	FreeArgs (rda);
    }
    else
    {
	PrintFault(IoErr(), ERROR_HEADER);
	error = RETURN_FAIL;
    }

    RT_Exit();

    return(error);
}
