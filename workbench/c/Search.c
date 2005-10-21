/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************

    NAME

    Search

    SYNOPSIS

    Search [FROM] {(name | pattern} [SEARCH] (string | pattern) [ALL] 
           [NONUM] [QUIET] [QUICK] [FILE] [PATTERN] [LINES=Number]

    LOCATION

    Workbench:C

    FUNCTION

    Search looks through the files contained in the FROM directory for
    a specified string (SEARCH); in case the ALL switch is specified,
    the subdirectories of the FROM directory are also searched. The name
    of all files containing the SEARCH string is displayed together with
    the numbers of the lines where the string occurred.
        If CTRL-C is pressed, the search will be abandoned. CTRL-D will
    abandon searching the current file.

    INPUTS

    NONUM    --  no line numbers are printed
    QUIET    --  don't display the name of the file being searched
    QUICK    --  more compact output
    FILE     --  look for a file with a specific name rather than a string
                 in a file
    PATTERN  --  use pattern matching when searching
    CASE     --  use case sensitive pattern matching when searching
    LINES    --  extra lines after a line match which should be shown

    RESULT

    If the object is found, the condition flag is set to 0. Otherwise it's
    set to WARN.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

    Author: Neil Cafferkey
    Placed in the public domain by Neil Cafferkey.
    Changes by: Johan 'S.Duvan' Alfredsson

******************************************************************************/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/locale.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <libraries/locale.h>

#include <string.h>

// *****  Layout and version parameters  ***********

#define LOCALE_VERSION   38
#define PATH_BUF_SIZE    512
#define SPACES_SIZE      (160 + 1)
#define MARGIN           3
#define INDENT           5
#define DIR_MARGIN       2


// *****  Command line arguments *******************

enum
{
    ARG_FROM,
    ARG_SEARCH,
    ARG_ALL,
    ARG_NONUM,
    ARG_QUIET,
    ARG_QUICK,
    ARG_FILE,
    ARG_PATTERN,
    ARG_CASE,
    ARG_LINES,
    ARG_COUNT
};

int LocaleBase_version = LOCALE_VERSION;

// *****  Prototypes for internal functions  *******

VOID  PrintFullName(TEXT *buffer, UWORD cut_off, struct AnchorPath *anchor);
UWORD GetDirName(struct AnchorPath *anchor, TEXT *buffer);
BOOL  FindString(struct AnchorPath *anchor, ULONG *args, TEXT *pattern,
		 struct Locale *locale, UBYTE *pi);
BOOL  MatchStringNoCase(TEXT *string, TEXT *text, TEXT *text_end, UBYTE *pi,
			struct Locale *locale);
BOOL MatchString(TEXT *string, TEXT *text, TEXT *text_end, UBYTE *pi,
                 struct Locale *locale);


// *****  String information (version, messages) ***

const TEXT template[] =
     "FROM/M,SEARCH/A,ALL/S,NONUM/S,QUIET/S,QUICK/S,FILE/S,PATTERN/S,CASE/S,LINES/N";
const TEXT version_string[] = "$VER: Search 42.3 (18.10.2005)";
const TEXT locale_name[]    = "locale.library";

const TEXT control_codes[]  = { 0x9b, 'K', 13 };
const TEXT wild_card[]      = { '#', '?'};
const TEXT new_line[]       = "\n";
const TEXT abandon_msg[]    = "** File abandoned\n";


int __nocommandline;

int main(void)
{
    IPTR              *args;
    struct RDArgs     *read_args;
    struct AnchorPath *anchor;
    LONG               error;
    LONG               return_code = RETURN_WARN;
    TEXT              *text, *spaces, *pattern = NULL, *path_buffer,
	              *user_pattern = NULL, *p, ch, **from;
    BOOL               found, success = TRUE, new_dir, print_names;
    UWORD              indent = 0, pat_buf_length, cut_off, pat_length;
    UBYTE              k, q;
    struct Locale     *locale;

    /* Allocate buffers */

    spaces      = AllocMem(SPACES_SIZE, MEMF_CLEAR);
    anchor      = AllocMem(sizeof(struct AnchorPath), MEMF_CLEAR);
    args        = AllocMem(ARG_COUNT*sizeof(APTR), MEMF_CLEAR);
    path_buffer = AllocMem(PATH_BUF_SIZE,MEMF_ANY);

    if(args && anchor && spaces && path_buffer)
    {
	locale = OpenLocale(NULL);

	for(text = spaces + SPACES_SIZE - 1; text > spaces; *(--text) = ' ');

	/* Parse arguments */
	
	read_args = ReadArgs((STRPTR)template, args, NULL);
	
	if(locale && read_args)
	{
	    /* Prepare the pattern to be matched */
	    
	    pat_length = strlen((TEXT *)args[ARG_SEARCH]);
	    pat_buf_length = pat_length*2 + 3;
	    user_pattern = AllocMem(pat_length + 5, MEMF_CLEAR);
	    pattern = AllocMem(pat_buf_length, MEMF_ANY);
	    
	    if(user_pattern && pattern)
	    {
		if(args[ARG_PATTERN] || args[ARG_FILE])
		{
		    if(args[ARG_FILE])
			text = user_pattern;
		    else
		    {
			text = user_pattern + 2;
			CopyMem(wild_card, user_pattern, 2);
			CopyMem(wild_card, text + pat_length, 2);
		    }
		    
		    CopyMem((TEXT *)args[ARG_SEARCH], text, pat_length);
		    if (args[ARG_CASE])
		    {
			if (ParsePattern(user_pattern, pattern, pat_buf_length) < 0)
			{
				success = FALSE;
			}
		    }
		    else
		    {		    
		        if(ParsePatternNoCase(user_pattern, pattern, 
					  pat_buf_length) < 0)
			    success = FALSE;
		    }
		}
		else
		{
		    /* Copy the search string and convert it to uppercase */
		    
		    text = pattern;
		    
		    for(p = (TEXT *)args[ARG_SEARCH]; (ch = *p) != '\0'; p++)
			*(text++) = ConvToUpper(locale, ch);
		    
		    *text = '\0';
		    
		    /* Construct prefix table for Knuth-Morris-Pratt
		       algorithm */
		    
		    *user_pattern = 0;
		    k = 0;
		    
		    for(q = 1; q < pat_length; q++)
		    {
			while(k && (pattern[k] != pattern[q]))
			    k = user_pattern[k - 1];
			
			if(pattern[k] == pattern[q])
			    k++;
			
			user_pattern[q] = k;
		    }
		}
	    }
	    else
		success = FALSE;
	    
	    /* Get the next starting point */
	    
	    for(from = *(((TEXT ***)args) + ARG_FROM); from && *from && success; 
		from++)
	    {
		
		/* Initialise file search */
		
		anchor->ap_BreakBits = SIGBREAKF_CTRL_C;
		anchor->ap_FoundBreak = 0;
		anchor->ap_Flags = 0;
		error = MatchFirst(*from, anchor);
		
		/* Work out if more than one file is being searched */
		
		print_names = (*(*(((TEXT ***)args) + ARG_FROM) + 1))
		    || (anchor->ap_Flags & APF_ITSWILD);
		
		/* Enter sub-dir if the pattern was an explicitly named dir */
		
		if(!(anchor->ap_Flags & APF_ITSWILD)
		   && (anchor->ap_Info.fib_DirEntryType > 0))
		    anchor->ap_Flags |= APF_DODIR;
		
		/* Set flag to get name of starting directory */
		
		new_dir = TRUE;
		
		/* Traverse the directory */
		
		while(!error && success)
		{
		    found = FALSE;
		    
		    if(anchor->ap_Info.fib_DirEntryType > 0)
		    {
			/* Enter sub-dir if the ALL switch was supplied and
			   we're not on the way out of it */
			
			if(!(anchor->ap_Flags & APF_DIDDIR))
			{
			    if(!(args[ARG_FILE] || args[ARG_QUIET] ||
				 args[ARG_QUICK]))
			    {
				WriteChars(spaces, MARGIN + INDENT*indent + 
					   DIR_MARGIN);
				text = (TEXT *)&(anchor->ap_Info.fib_FileName);
				VPrintf("%s (dir)\n", (IPTR *)&text);
			    }

			    if(args[ARG_ALL] || (anchor->ap_Flags & APF_DODIR))
			    {
				anchor->ap_Flags |= APF_DODIR;
				indent++;
				print_names = TRUE;
			    }
			}
			else
			{
			    indent--;
			}
			
			new_dir = TRUE;
			anchor->ap_Flags &= ~APF_DIDDIR;
		    }
		    else
		    {
			/* Deal with a file */
			
			if(anchor->ap_Flags & APF_DirChanged)
			    new_dir = TRUE;
			
			if(new_dir)
			{
			    if(!(cut_off = GetDirName(anchor, path_buffer)))
				success = FALSE;
			    
			    new_dir = FALSE;
			}
			
			if(args[ARG_FILE])
			{
			    found = MatchPatternNoCase(pattern,
				      (TEXT *)&(anchor->ap_Info.fib_FileName));
			}
			else
			{
			    if(args[ARG_QUICK])
			    {
				PrintFullName(path_buffer, cut_off, anchor);
				WriteChars((STRPTR)control_codes, 3);
			    }
			    else if(!args[ARG_QUIET] && print_names)
			    {
				WriteChars(spaces, MARGIN + INDENT*indent);
				text = (TEXT *)&(anchor->ap_Info.fib_FileName);
				VPrintf("%s..\n", (IPTR *)&text);
			    }
			    
			    found = FindString(anchor, args, pattern, locale,
					       user_pattern);
			    
			}
			
			if(found)
			{
			    if((args[ARG_FILE] || args[ARG_QUIET]) &&
			       !args[ARG_QUICK])
			    {
				PrintFullName(path_buffer, cut_off, anchor);
				PutStr(new_line);
			    }
			    
			    return_code = RETURN_OK;
			}
		    }
		    
		    error = MatchNext(anchor);
		    
		    if(error && (error != ERROR_NO_MORE_ENTRIES))
		    {
			success = FALSE;
			SetIoErr(error);
		    }		    
		}
	    }
	    
	    /* Clear line for next shell prompt */
	    
	    if(args[ARG_QUICK] && !args[ARG_FILE])
		WriteChars((STRPTR)control_codes, 2);
	    
	    MatchEnd(anchor);
	    
	    if(success)
		SetIoErr(0);
	}
	
	FreeArgs(read_args);
	
	CloseLocale(locale);
    }
    else
    {
	SetIoErr(ERROR_NO_FREE_STORE);
    }
    
    /* Free memory */
    
    if(args)
	FreeMem(args, ARG_COUNT*sizeof(APTR));
    if(anchor)
	FreeMem(anchor, sizeof(struct AnchorPath));
    if(spaces)
	FreeMem(spaces, SPACES_SIZE);
    if(path_buffer)
	FreeMem(path_buffer, PATH_BUF_SIZE);
    if(user_pattern)
	FreeMem(user_pattern, pat_length + 5);
    if(pattern)
	FreeMem(pattern, pat_buf_length);
    
    /* Check and reset signals */
    
    if(SetSignal(0, -1) & SIGBREAKF_CTRL_C)
	SetIoErr(ERROR_BREAK);
    
    /* Exit */
    
    if((error = IoErr()) != 0)
    {
	PrintFault(error, NULL);
	return RETURN_FAIL;
    }
    
    return return_code;
}



VOID PrintFullName(TEXT *buffer, UWORD cut_off, struct AnchorPath *anchor)
{
    buffer[cut_off] = '\0';
    
    if(AddPart(buffer, (TEXT *)&(anchor->ap_Info.fib_FileName), PATH_BUF_SIZE))
    {
	PutStr(buffer);
    }
    
    return;
}


UWORD GetDirName(struct AnchorPath *anchor, TEXT *buffer)
{
    if(NameFromLock(anchor->ap_Current->an_Lock, buffer, PATH_BUF_SIZE))
	return strlen(buffer);

    return 0;
}


BOOL FindString(struct AnchorPath *anchor, ULONG *args, TEXT *pattern,
		struct Locale *locale, UBYTE *pi)
{
    BOOL   found = FALSE, end_early = FALSE, line_matches, at_end;
    BPTR   old_lock, file;
    TEXT  *p, *q, *r, *line, *buffer = NULL, ch;
    ULONG  max_line_length = 0, line_length, offset = 0, file_size, buf_size,
	   line_start = 0, line_count = 1;
    ULONG Lines=0;
    ULONG  sigs;
    LONG   read_length = 1;
    
    /* Move into the file's directory */
    
    old_lock = CurrentDir(anchor->ap_Current->an_Lock);
    
    /* Open the file for reading */
    
    if((file = Open((TEXT *)&(anchor->ap_Info.fib_FileName), MODE_OLDFILE)) != NULL)
    {
	/* Get a buffer for the file */
	
	file_size = anchor->ap_Info.fib_Size;
	buf_size  = file_size + 1;
	
	while(!buffer && buf_size)
	{
	    if(!(buffer = AllocMem(buf_size, MEMF_ANY)))
		buf_size >>= 1;
	}
	
	/* Check size of buffer */
	
	if((buf_size <= file_size) && buffer)
	{
	    /* Get length of longest line */
	    
	    while(read_length > 0)
	    {
		read_length = Read(file, buffer, buf_size - 1);
		q = buffer + read_length;
		
		if(!read_length)
		    q++;
		
		for(p = buffer; p < q; p++)
		{
		    if((*p=='\n')||!read_length)
		    {
			line_length = offset + (p - buffer) - line_start;
			
			if(line_length > max_line_length)
			    max_line_length = line_length;
			
			line_start = offset + (p - buffer) + 1;
		    }
		}
		
		offset += read_length;
	    }
	    
	    /* Ensure buffer is big enough for longest line */
	    
	    if(buf_size <= max_line_length)
	    {
		FreeMem(buffer, buf_size);
		buf_size = max_line_length + 1;
		buffer = AllocMem(buf_size, MEMF_ANY);
	    }
	}
	
	/* Test every line against the pattern */
	
	if(buffer && pattern)
	{
	    
	    read_length = Seek(file, 0, OFFSET_BEGINNING) + 1;
	    
	    while(((read_length = Read(file, buffer, buf_size - 1)) > 0) &&
		  !end_early)
	    {
		q = buffer + read_length;
		at_end = Seek(file, 0, OFFSET_CURRENT) == file_size;
		
		if(at_end)
		    *(q++) = '\0';
		
		line = buffer;
		
		for(p = buffer; (p < q) && !end_early; p++)
		{
		    ch = *p;
		    
		    if((ch == '\n') || (ch == '\0'))
		    {
			*p = '\0';
			
			if (args[ARG_CASE])
			{
				if (args[ARG_PATTERN])
					line_matches = MatchPattern(pattern, line);
				else
					line_matches = MatchString(pattern, line, p, pi, locale);
			}
			else
			{
				if(args[ARG_PATTERN])
			    		line_matches = MatchPatternNoCase(pattern, line);
				else
			    		line_matches = MatchStringNoCase(pattern, line, p, pi, locale);
			}
			if(line_matches)
			{
			    if(!found && args[ARG_QUICK])
				PutStr(new_line);
			    
			    found = TRUE;
			    
			    if(args[ARG_QUIET])
			    {
				end_early = TRUE;
			    }
			    else
			    {
				if(!args[ARG_NONUM])
				    VPrintf("%6lu ", &line_count);
				
				/* Replace invisible characters with dots */
				
				for(r = line; r < p; r++)
				{
				    if(!IsPrint(locale, *r))
					*r='.';
				}
				
				VPrintf("%s\n", (IPTR *)&line);
				if (args[ARG_LINES])
				{
					Lines = *((ULONG*) args[ARG_LINES]);
				}			    }
			}
			else
			{
				if (Lines != 0)
				{
					Printf("%6lu: ", line_count);
					/* Replace invisible characters with dots */

					for (r = line; r < p; r++)
					{
						if (!IsPrint(locale, *r))
							*r='.';
					}
					PutStr(line); PutStr("\n");
					Lines--;
				}
			}			
			line = p + 1;
			
			if(ch == '\n')
			    line_count++;
			
			sigs = SetSignal(0, SIGBREAKF_CTRL_D);
			
			if(sigs & (SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D))
			{
			    end_early = TRUE;
			    
			    if(sigs & SIGBREAKF_CTRL_D)
			    {
				PutStr(abandon_msg);
			    }
			}
		    }
		}
		
		/* Start reading again at start of most recent line */
		
		if(!at_end)
		    Seek(file, line - q, OFFSET_CURRENT);
	    }
	}
	
	if(buffer)
	    FreeMem(buffer, buf_size);

	Close(file);
    }
    
    CurrentDir(old_lock);
    
    return found;
}


BOOL MatchStringNoCase(TEXT *string, TEXT *text, TEXT *text_end, UBYTE *pi,
		       struct Locale *locale)
{
    TEXT *s, ch;
    
    s = string;
    
    while(text < text_end)
    {
	ch = ConvToUpper(locale, *(text++));
	
	while((s != string) && (*s != ch))
	    s = string + pi[s - string - 1];
	
	if(ch == *s)
	    s++;
	
	if(!*s)
	    return TRUE;
    }

    return FALSE;
}

BOOL MatchString(TEXT *string, TEXT *text, TEXT *text_end, UBYTE *pi,
                 struct Locale *locale)
{
	TEXT *s, ch;

	s = string;

	while (text < text_end)
	{
		ch = *(text++);

		while ((s != string) && (*s != ch))
			s = string + pi[s - string - 1];

		if (ch == *s)
			s++;

		if (!*s)
			return TRUE;
	}

	return FALSE;
}


