/*
    (C) 1995-97 AROS - The Amiga Research OS
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

        DIR/M,DATES/S,NODATES/S,QUICK/S,NOHEAD/S,FILES/S,DIRS/S,LFORMAT/S

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
#include <dos/datetime.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

#include <stdio.h>

static const char version[] = "$VER: list 41.4 (11.10.1997)\n";

#define TEMPLATE "DIR/M,DATES/S,NODATES/S,QUICK/S,NOHEAD/S,FILES/S,DIRS/S,LFORMAT/K"

#define ARG_DIR		0
#define ARG_DATES	1
#define ARG_NODATES	2
#define ARG_QUICK 	3
#define ARG_NOHEAD	4
#define ARG_FILES	5
#define ARG_DIRS	6
#define ARG_LFORMAT     7

#define ARG_NUM 	8


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
    %t: Time
    %b: Blocks
    %f: label of device
    %a: flags
    %k: keys
    %l: size
    %m: name  (.-names are suppressed; x.info isprinted as x)
    %s: name
    %n: name
*/

struct lfstruct
{
  BOOL isdir;
  STRPTR date;
  STRPTR time;
  STRPTR flags;
  STRPTR filename;
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
                   
                   tmp = lf->size / 1024;
                   if (lf->size % 1024)
                     tmp++;
                  
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
                  BOOL show_dirs)
{
    int error = RETURN_OK;
    UBYTE date[LEN_DATSTRING];
    UBYTE time[LEN_DATSTRING];
    struct DateTime dt;
    char flags[8];

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
          struct lfstruct lf = {dir, date, time, flags, filename, size};
          print_lformat((STRPTR)args[ARG_LFORMAT], &lf);
          VPrintf("\n", NULL);
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
        }
      }
    } else if (show_files) {

        if (args[ARG_LFORMAT])
        {
          struct lfstruct lf = {dir, date, time, flags, filename, size};
          print_lformat((STRPTR)args[ARG_LFORMAT], &lf);
          VPrintf("\n", NULL);
        }
        else
        {

          IPTR argv[2];
        
          if (VPrintf("%-25.s ",(IPTR *)&filename) < 0)
            error = RETURN_ERROR;
        
          if (!args[ARG_QUICK])
          {
            argv[0] = (IPTR)size;
            argv[1] = (IPTR)flags;
            VPrintf("%7.ld %7.s ",argv);
          }
        
          if (args[ARG_DATES] || (!args[ARG_QUICK] && !args[ARG_NODATES]))
          {
            argv[0] = (IPTR)date;
            argv[1] = (IPTR)time;
            VPrintf("%-11.s %s",argv);
          }

          VPrintf("\n",NULL);
        }
    }
    if ((!error) && (filenote) && (filenote[0]))
        error = VPrintf(": %s\n", (IPTR *)&filenote);

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


/* print information about all files in a directory */
int scandir(BPTR dir, IPTR * args, BOOL show_files, BOOL show_dirs)
{
    int error = RETURN_OK;
    LONG files=0, dirs=0, blocks=0;
    struct ExAllControl *eac;
    struct ExAllData *ead;
    static UBYTE buffer[4096];
    BOOL loop;

    eac = AllocDosObject(DOS_EXALLCONTROL,NULL);
    if (eac) {
        eac->eac_LastKey = 0;
        do {
            loop = ExAll(dir, (struct ExAllData *)buffer, sizeof(buffer), ED_COMMENT, eac);
            if ((loop) || (IoErr() == ERROR_NO_MORE_ENTRIES)) {
                if (eac->eac_Entries) {
                    ead = (struct ExAllData *)buffer;
                    do {
                        int tmperror;
                        struct DateStamp ds = {
                            ead->ed_Days,
                            ead->ed_Mins,
                            ead->ed_Ticks
                        };
                        tmperror = printfiledata(ead->ed_Name,
                                                 ead->ed_Type>=0?TRUE:FALSE,
                                                 &ds,
                                                 ead->ed_Prot,
                                                 ead->ed_Size,
                                                 ead->ed_Comment,
                                                 args,
                                                 show_files,
                                                 show_dirs);
                        error = MAX(error,tmperror);
                        if (ead->ed_Type >= 0)
                            dirs++;
                        else {
                            files++;
                            blocks += ead->ed_Size;
                        }
                        ead = ead->ed_Next;
                    } while (ead != NULL);
                }
            } else
                error = RETURN_ERROR;
        } while ((loop) && (!error));
        FreeDosObject(DOS_EXALLCONTROL, eac);
    } else {
        SetIoErr(ERROR_NO_FREE_STORE);
        error = RETURN_FAIL;
    }

    if (!error)
        printsummary(files, dirs, blocks, args);

    return error;
}


int listfile(STRPTR filename, IPTR * args, BOOL show_files, BOOL show_dirs)
{
    int error = 0;
    BPTR dir;
    struct FileInfoBlock *fib;

    dir = Lock(filename, SHARED_LOCK);
    if (dir) {
        fib = AllocDosObject(DOS_FIB, NULL);
        if (fib) {
            if (Examine(dir, fib)) {
                if (fib->fib_DirEntryType >= 0) {
                    error = printdirheader(filename, args);
                    if (!error)
                      error = scandir(dir, args, show_files, show_dirs);
                } else {
                    /* The lock is just an ordinary file. */
                    STRPTR dirname;
                    int dirlen = strlen(filename) + 1;
                    dirname = AllocVec(dirlen, MEMF_ANY);
                    if (dirname) {
                        CopyMem(filename, dirname, dirlen);
                        PathPart(dirname)[0] = 0x00;
                        error = printdirheader(dirname, args);
                        FreeVec(dirname);
                        if (!error) {
                            error = printfiledata(fib->fib_FileName,
                                                  fib->fib_DirEntryType>=0?TRUE:FALSE,
                                                  &fib->fib_Date,
                                                  fib->fib_Protection,
                                                  fib->fib_NumBlocks,
                                                  fib->fib_Comment,
                                                  args,
                                                  show_files,
                                                  show_dirs);
                            if (!error)
                                error = printsummary(1, 0, fib->fib_Size, args);
                        }
                    } else {
                        SetIoErr(ERROR_NO_FREE_STORE);
                        error = RETURN_FAIL;
                    }
                }
            } else
                error = RETURN_FAIL;
            FreeDosObject(DOS_FIB, fib);
        } else {
            SetIoErr(ERROR_NO_FREE_STORE);
            error = RETURN_FAIL;
        }
        UnLock(dir);
    } else
        error = RETURN_FAIL;

    return error;
}


int main (int argc, char **argv)
{
    IPTR args[ARG_NUM] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    struct RDArgs *rda;
    LONG error = RETURN_OK;
    STRPTR *filelist;
    BOOL show_files = TRUE;
    BOOL show_dirs  = TRUE;

    rda = ReadArgs(TEMPLATE, args, NULL);

    if (args[ARG_DIRS])
      if (!args[ARG_FILES])
        show_files = FALSE;
    
    if (args[ARG_FILES])
      if (!args[ARG_DIRS])
        show_dirs = FALSE;    
    
    if (rda) {
        filelist = (STRPTR *)args[ARG_DIR];
        if ((filelist) && (*filelist)) {
            while ((*filelist) && (!error)) {
                error = listfile(filelist[0],args,show_files,show_dirs);
                filelist++;
                if ((filelist[0]) && (!error))
                    VPrintf("\n", NULL);
            }
        } else
            /* No file to list given. Just list the current directory */
            error = listfile("",args,show_files,show_dirs);
	FreeArgs(rda);
    } else
	error=RETURN_FAIL;

    if (error)
	PrintFault(IoErr(),"List");

    return error;
}
