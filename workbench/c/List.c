/*
    Copyright (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: List the contents of a directory
    Lang: english
*/
/*****************************************************************************

    NAME

        List

    FORMAT

        List [<directory>]
		
    TEMPLATE

	DIR/M,P=PAT/K,DATES/S,NODATES/S,QUICK/S,BLOCK/S,NOHEAD/S,FILES/S,DIRS/S,LFORMAT/S

    LOCATION

        Workbench:C/
	   
    FUNCTION

        Lists detailed information about the files and directories in the 
        current directory or in the directory specified by DIR.

        The information for each file or directory is presented on a separate 
        line, containing the following information:
         
        name
        size (in bytes)
        protection bits
        date and time
        
    INPUTS

        DIR - The directory to list. If left out, the current directory
              will be listed.

    RESULT

        Standard DOS return codes.

    EXAMPLE

        1> List C:
        Directory "C:" on Wednesday 12-Dec-99
        AddBuffers                  444 --p-rwed 02-Sep-99 11:51:31
        Assign                     3220 --p-rwed 02-Sep-99 11:51:31
        Avail                       728 --p-rwed 02-Sep-99 11:51:31
        Copy                       3652 --p-rwed 02-Sep-99 11:51:31
        Delete                     1972 --p-rwed 02-Sep-99 11:51:31
        Execute                    4432 --p-rwed 02-Sep-99 11:51:31
        List                       5108 --p-rwed 02-Sep-99 11:51:31
        Installer                109956 ----rwed 02-Sep-99 11:51:31
        Which                      1068 --p-rwed 02-Sep-99 11:51:31
        9 files - 274 blocks used        
        
    BUGS

    SEE ALSO

        Dir

    INTERNALS

    HISTORY

******************************************************************************/

#include <clib/macros.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/datetime.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/dosasl.h>
#include <dos/datetime.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

#include <stdio.h>

static const char version[] = "$VER: list 41.4 (11.10.1997)\n";

#define TEMPLATE "DIR/M,P=PAT/K,DATES/S,NODATES/S,QUICK/S,BLOCK/S,NOHEAD/S,FILES/S,DIRS/S,LFORMAT/K"


#define ARG_DIR		0
#define ARG_PAT		1
#define ARG_DATES	2
#define ARG_NODATES	3
#define ARG_QUICK 	4
#define ARG_BLOCK	5
#define ARG_NOHEAD	6
#define ARG_FILES	7
#define ARG_DIRS	8
#define ARG_LFORMAT     9

#define ARG_NUM 	10


#define BLOCKSIZE 	1024


int printdirheader(STRPTR dirname, IPTR * mainargs)
{
    struct DateTime dt;
    char datestr[LEN_DATSTRING], dow[LEN_DATSTRING];
    IPTR args[] = { (IPTR)dirname, (IPTR)dow, (IPTR)datestr };

    if (!mainargs[ARG_NOHEAD])
    {
      DateStamp((struct DateStamp *)&dt);
      dt.dat_Format = FORMAT_DEF;
      dt.dat_Flags = 0;
      dt.dat_StrDay = dow;
      dt.dat_StrDate = datestr;
      dt.dat_StrTime = NULL;
      DateToStr(&dt);

      if (VPrintf("Directory \"%s\" on %s %s:\n", args) < 0)
         return RETURN_ERROR;
    }
    return RETURN_OK;
}

/*
 Formatted output.
 This function knows:
    %d: Date
    %c: Comment
    %t: Time
    %b: Blocks
    %c: ???
    %f: label of device
    %a: flags
    %k: keys
    %l: size
    %m: name  (.-names are suppressed; x.info is printed as x)
    %s: name
    %n: name
    %e: name  (ending of name; x.info is printed as info)
*/

struct lfstruct
{
  BOOL isdir;
  STRPTR date;
  STRPTR time;
  STRPTR flags;
  STRPTR filename;
  STRPTR comment;
  ULONG size;
};


int print_lformat(STRPTR format,
                  struct lfstruct * lf)
{
  char c;
  while ('\0' != (c = *format++))
  {
    if ('%' == c)
    {
      ULONG pos;
      BOOL right = TRUE;
      char buf[80];
      char * buf_ptr = &buf[80];
      ULONG width = 0, size;
      ULONG tmp;
      
      buf[80] = '\0';
      
      if ('-' == *format)
      {
        right = FALSE;
        format++;
      }
      
      while (*format >= '0' && *format <= '9')
        width = width * 10 + (*format++) - '0';
      
      switch (*format++)
      {
        case 'C':
        case 'c':  buf_ptr = lf->comment;
        break;
        
        case 'D':
        case 'd':  buf_ptr = lf->date;
        break;
        
        case 'T':
        case 't':  buf_ptr = lf->time;
        break;
        
        case 'B':
        case 'b':  if (TRUE == lf->isdir)
                   {
                     buf_ptr = "Dir";
                     break;
                   }
                   
                   tmp = (lf->size + BLOCKSIZE - 1) / BLOCKSIZE;
                  
                   if (0 == tmp)
                     buf_ptr = "empty";
                   else
                     do
                     {
                       *--buf_ptr = (tmp % 10) + '0';
                       tmp /= 10;
                     }
                     while(tmp);
        break;
        
        case 'F':
        case 'f':
        break;
        
        case 'A':
        case 'a':  buf_ptr = lf->flags;
        break;
        
        case 'K':
        case 'k':
        break;
        
        case 'L':
        case 'l':  if (TRUE == lf->isdir)
                   {
                     buf_ptr = "Dir";
                     break;
                   }
                   
                   tmp = lf->size;
                  
                   if (0 == tmp)
                     buf_ptr = "empty";
                   else
                     do
                     {
                       *--buf_ptr = (tmp % 10) + '0';
                       tmp /= 10;
                     }
                     while(tmp);
        break;
        
        case 'M':
        case 'm':  if (!(*lf->filename == '.'))
                   {
                     buf_ptr = &buf[0];
                     size = strlen(lf->filename);
                     CopyMem(lf->filename, buf_ptr, size+1);
                     
                     while (size > 0)
                     {
                       if ('.' == buf[size])
                       {
                         buf[size] = '\0';
                         break;
                       }
                       size --;
                     }
                     
                   }
        break;
        
        case 'S':
        case 's':  buf_ptr = lf->filename;
        break;
        
        case 'N':
        case 'n':  buf_ptr = lf->filename;
        break;
        
        case 'E':
        case 'e': size = strlen(lf->filename);
                  pos = size;
                  while (pos > 0)
                  {
                    if ('.' == lf->filename[pos])
                    {
                      if (size != pos)
                      {
                        buf_ptr = &buf[0];
                        CopyMem(&lf->filename[pos+1], buf_ptr, size-pos);
                        break;
                      }
                    }
                    pos--;
                  }
        break;
        
        case '\0':
          return 0;
        break;
        
        default:
          *--buf_ptr = *(format-1);
          *--buf_ptr = '%';
      }
      
      
      size = strlen(buf_ptr);
      
      /* print the string */
      
      pos = 0;
      
        /* right aligned ? */
      if (TRUE == right && width > size)
      {
        while (pos < (width - size))
        { 
          pos++;
          printf(" ");
        }
      }
      
      /* the string itself */
      while (*buf_ptr)
      {
        pos++;
        printf("%c",*buf_ptr++);
      }
      
      /* left aligned? */
      if (FALSE == right || pos < width)
        while(pos++ < width)
          printf(" ");
    }
    else
    {
      printf("%c",c);
    }
  }

  return 0;
}


int printfiledata(STRPTR filename, 
                  BOOL dir, 
                  struct DateStamp *ds, 
                  ULONG protection, 
                  ULONG size, 
                  STRPTR filenote,
                  IPTR * args,
                  BOOL show_files,
                  BOOL show_dirs,
                  STRPTR pattern,
                  ULONG * files,
                  ULONG * dirs,
                  ULONG * blocks)
{
    int error = RETURN_OK;
    UBYTE date[LEN_DATSTRING];
    UBYTE time[LEN_DATSTRING];
    struct DateTime dt;
    char flags[8];

    if (args[ARG_PAT] && 
        FALSE == MatchPatternNoCase(pattern, filename))
      return 0;      

    CopyMem(ds, &dt.dat_Stamp, sizeof(struct DateStamp));
    dt.dat_Format  = FORMAT_DOS;
    dt.dat_Flags   = DTF_SUBST;
    dt.dat_StrDay  = NULL;
    dt.dat_StrDate = date;
    dt.dat_StrTime = time;
    DateToStr(&dt); /* returns 0 if invalid */

    flags[0] = protection&FIBF_SCRIPT?'s':'-';
    flags[1] = protection&FIBF_PURE?'p':'-';
    flags[2] = protection&FIBF_ARCHIVE?'a':'-';
    /* The following flags are high-active! */
    flags[3] = protection&FIBF_READ?'-':'r';
    flags[4] = protection&FIBF_WRITE?'-':'w';
    flags[5] = protection&FIBF_EXECUTE?'-':'e';
    flags[6] = protection&FIBF_DELETE?'-':'d';
    flags[7] = 0x00;

    if (dir) {
      if (show_dirs) {
      
        if (args[ARG_LFORMAT])
        {
          struct lfstruct lf = {dir, date, time, flags, filename, filenote, size};
          print_lformat((STRPTR)args[ARG_LFORMAT], &lf);
          VPrintf("\n", NULL);
          
          *dirs += 1;
        }
        else
        {
          IPTR argv[2];
          
          if (VPrintf("%-25.s ",(IPTR *)&filename) < 0)
            error = RETURN_ERROR;
        
          if (!args[ARG_QUICK])
          {
            argv[0] = (IPTR)flags;
            VPrintf("  <Dir> %7.s ",argv); // don't try &flags!!
          }

          if (args[ARG_DATES] || (!args[ARG_QUICK] && !args[ARG_NODATES]))
          {
            argv[0] = (IPTR)date;
            argv[1] = (IPTR)time;
            VPrintf("%-11.s %s",argv);
          }

          
          VPrintf("\n",NULL);
          
          *dirs += 1;
        }
      }
    } else if (show_files) {

        if (args[ARG_LFORMAT])
        {
          struct lfstruct lf = {dir, date, time, flags, filename, filenote, size};
          print_lformat((STRPTR)args[ARG_LFORMAT], &lf);
          VPrintf("\n", NULL);

          *files += 1;
          *blocks += size;

        }
        else
        {

          IPTR argv[2];
        
          if (VPrintf("%-25.s ",(IPTR *)&filename) < 0)
            error = RETURN_ERROR;
        
          if (!args[ARG_QUICK])
          {
            if (0 != size)
            {
              if (args[ARG_BLOCK])
                argv[0] = (IPTR)(size + BLOCKSIZE - 1) / BLOCKSIZE;
              else
                argv[0] = (IPTR)size;
              
              argv[1] = (IPTR)flags;
              VPrintf("%7.ld %7.s ",argv);
            }
            else
            {
              argv[0] = (IPTR)flags;
              VPrintf("  empty %7.s ",argv);
            }
          }
        
          if (args[ARG_DATES] || (!args[ARG_QUICK] && !args[ARG_NODATES]))
          {
            argv[0] = (IPTR)date;
            argv[1] = (IPTR)time;
            VPrintf("%-11.s %s",argv);
          }

          if (!args[ARG_QUICK] && '\0' != filenote[0])
          {
            argv[0] = (IPTR)filenote;
            VPrintf("\n: %s",argv);
          }

          VPrintf("\n",NULL);
          *files  += 1;
          *blocks += size;
        }
    }

    return error;
}


/* print directory summary information */
int printsummary(int files, int dirs, int blocks, IPTR * args)
{
    int error = RETURN_OK;

    if (!args[ARG_NOHEAD])
    {

      if ((files == 0) && (dirs == 0)) {
          if (VPrintf("Directory is empty\n", NULL) < 0)
              error = RETURN_ERROR;
      } else {
          if ((files)) {
              if (VPrintf("%ld files - ", (IPTR *)&files) < 0)
                  error = RETURN_ERROR;
          }
          if ((dirs) && (!error)) {
              if (VPrintf("%ld directories - ", (IPTR *)&dirs) < 0)
                  error = RETURN_ERROR;
          }
          if (!error) {
              if (VPrintf("%ld bytes used\n", (IPTR *)&blocks) < 0)
                  error = RETURN_ERROR;
          }
      }
    }
    return error;
}


int listfile(STRPTR filename, 
             IPTR * args, 
             BOOL show_files, 
             BOOL show_dirs,
             STRPTR pattern)
{
  struct AnchorPath ap;
  ULONG files = 0, dirs = 0, blocks = 0;
  ULONG error;
 
  memset(&ap, 0x0, sizeof(struct AnchorPath));
  error = MatchFirst(filename, &ap);

  if (0 == strcmp(ap.ap_Info.fib_FileName, filename))
  {
    if (ap.ap_Info.fib_DirEntryType >= 0)
    {
      error = printdirheader(filename, args);
      ap.ap_Flags |= APF_DODIR;
      MatchNext(&ap);
    }
  }
 
  if (0 == error)
  {
    do
    {
      /*
      ** There's something to show.
      */
      if (0 == (ap.ap_Flags & APF_DIDDIR))
      {

        error = printfiledata(ap.ap_Info.fib_FileName,
                              ap.ap_Info.fib_DirEntryType >= 0 ? TRUE:FALSE,
                              &ap.ap_Info.fib_Date,
                              ap.ap_Info.fib_Protection,
                              ap.ap_Info.fib_Size,
                              ap.ap_Info.fib_Comment,
                              args,
                              show_files,
                              show_dirs,
                              pattern,
                              &files,
                              &dirs,
                              &blocks);
      }
      else
      {
        ap.ap_Flags &= ~APF_DIDDIR;
        error = printsummary(files, dirs, blocks, args);
        files = 0;
        dirs = 0;
        blocks = 0;
      }
      
    }
    while (0 == MatchNext(&ap));
  }
  
  MatchEnd(&ap);

  printsummary(files, dirs, blocks, args);
  return 0;
}


int main (int argc, char **argv)
{
    IPTR args[ARG_NUM] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    struct RDArgs *rda;
    LONG error = RETURN_OK;
    STRPTR *filelist;
    BOOL show_files = TRUE;
    BOOL show_dirs  = TRUE;
    STRPTR pattern = NULL;
    LONG i;

    rda = ReadArgs(TEMPLATE, args, NULL);

    if (rda)
    {
      if (args[ARG_DIRS])
        if (!args[ARG_FILES])
          show_files = FALSE;
    
      if (args[ARG_FILES])
        if (!args[ARG_DIRS])
          show_dirs = FALSE;    
    
      if (args[ARG_PAT])
      {
        i = strlen((STRPTR)args[ARG_PAT]) * 2 + 2;
        pattern = AllocVec(i, MEMF_ANY);
        ParsePatternNoCase((STRPTR)args[ARG_PAT],pattern, i);
      }
    
      filelist = (STRPTR *)args[ARG_DIR];
      if ((filelist) && (*filelist)) 
      {
        while ((*filelist) && (!error)) 
        {
          error = listfile(filelist[0],args,show_files,show_dirs,pattern);
          filelist++;
          if ((filelist[0]) && (!error))
            VPrintf("\n", NULL);
        }
      } 
      else
        /* No file to list given. Just list the current directory */
        error = listfile("#?",args,show_files,show_dirs,pattern);
      
      FreeArgs(rda);
    } 
    else
      error=RETURN_FAIL;

    if (error)
      PrintFault(IoErr(),"List");

    if (pattern)
      FreeVec(pattern);

    return error;
}
