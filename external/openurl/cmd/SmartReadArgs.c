/*
 * SmartReadArgs.c -- CLI/Workbench transparent ReadArgs()
 *
 * $VER: SmartReadArgs.c 1.7 (7.9.98)
 *
 * Copyright 1998 by Thomas Aglassinger <agi@sbox.tu-graz.ac.at>
 *
 * Based on ExtReadArgs Copyright 1994,1995 by Stefan Ruppert
 *
 * Ported to OS4 by Alexandre Balaban <alexandre@balaban.name>
 */

/****** SmartReadArgs/--history-- *******************************************
 * HISTORY
 *   Version 1.7
 *   - TODO: use dos/FindArg() instead of is_in_template()
 *   - Fixed two minor "size_t vs. LONG" warnings in
 *
 *   Version 1.6, 7-Sep-1998
 *
 *   - Changed name to SmartReadArgs to avoid confusion with other work based
 *     on the same material
 *   - Changed function parameters for SmartReadArgs() so that no more SAS/c
 *     specific values of argc/argv are required (Of course it still works
 *     with SAS/c, but you have to provide the WBStartup from "outside").
 *   - Changed all #include <clib/...> to #include <proto/..>, except for
 *     <clib/alib_stdio_protos.h> in "test.c". Where the hell is this one?
 *   - Added feature to ignore tooltypes that are not in the template
 *   - Added some missing includes in SmartReadArgs.c so the source codes
 *     compile without warnings
 *   - Changed #include <debug.h> to #include "debug.h" and provided a proper
 *     debug.h
 *   - The WINDOW tooltype is handled properly even if it is not entirely
 *     written in upper case.
 *   - Requires "utility.library" to be open as Stricmp() is used several
 *     times
 *   - Changed from Printf() to printf() using stdio of amiga.lib to make the
 *     code compile easier on non-SAS environments
 *   - Changed autodoc tool to Robodoc
 *   - Fixed enforcer hit if no tooltypes were provided at all
 *   - Remove some "char filename[34]" stuff and replaced the array dimension
 *     by MAXIMUM_FILENAME_LENGTH for future compatibility
 *   - Cleaned-up autodocs
 * ANCIENT HISTORY
 *   ExtReadArgs() by Stefan Ruppert
 *
 *   See aminet:dev/misc/extrdargs_v1.5.lha for the original version.
 *
 *   $HISTORY
 *   08.01.95 : 001.005 :  changed to ExtReadArgs()
 *   24.09.94 : 001.004 :  now checks after ReadArgs the SIGBREAKF_CTRL_C
 *                         flag, thus if anyone canceled during ReadArgs()
 *                         help ExtReadArgs() fails
 *   08.09.94 : 001.003 :  between two switches (no equal sign) there was
 *                         no space, this is fixed
 *   08.09.94 : 001.002 :  wb files now enclosed in quotes
 *   04.09.94 : 001.001 :  bumped to version 1
 *   19.05.94 : 000.001 :  initial
 ***************************************************************************/

/* ------------------------------ include's ------------------------------- */

#include "debug.h"
#include "SmartReadArgs.h"
#include "SDI_compiler.h"

#include <exec/memory.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <dos/exall.h>
#include <utility/tagitem.h>

#include <string.h>

#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/exec.h>
#include <proto/utility.h>

#if defined(__amigaos4__)
#include <dos/obsolete.h>
#endif

/* ---------------------------- local defines ----------------------------- */

#define TEMPSIZE                     512
#ifndef MAX
#define MAX(a,b)  (((a) > (b)) ? (a) : (b))
#endif

#ifndef EOS
#define EOS    '\0'
#endif

#ifndef ZERO
#define ZERO   ((BPTR)0)
#endif

#define MODE_MULTI      1

#define MAXIMUM_FILENAME_LENGTH 108

#if defined(__amigaos4__)
#define AllocVecShared(size, flags)  AllocVecTags((size), AVT_Type, MEMF_SHARED, AVT_Lock, FALSE, ((flags)&MEMF_CLEAR) ? AVT_ClearWithValue : TAG_IGNORE, 0, TAG_DONE)
#else
#define AllocVecShared(size, flags)  AllocVec((size), (flags))
#endif

/* --------------------------- library bases ------------------------------ */
extern struct Library *IconBase;
#if defined(__AROS__)
extern struct UtilityBase *UtilityBase;
#else
extern struct Library *UtilityBase;
#endif

#if defined(__GNUC__) && !defined(__amigaos4__)
extern struct WBStartup *_WBenchMsg;
#endif

/* -------------------------- static prototypes --------------------------- */

static struct DiskObject *smart_get_icon(struct SmartArgs *args, struct WBStartup *wbarg);
static void fstrcpy(struct SmartArgs *args, CONST_STRPTR string);
static void get_arg_name(struct SmartArgs *args, STRPTR buffer, ULONG size, ULONG * modes);
static void get_wbarg_name(struct WBArg *wbarg, STRPTR buffer, ULONG size);
static BOOL is_in_template(STRPTR name, CONST_STRPTR template);

/****** SmartReadArgs/--background-- ****************************************
 * COPYRIGHT
 *   SmartReadArgs is Copyright 1998 Thomas Aglassinger
 *
 *   ExtReadArgs, its prequel, is Copyright 1994,1995 Stefan Ruppert
 *
 *   Permission is granted to freely distribute the material (also only
 *   parts of it) as long this ReadMe is included and all the copyright
 *   notes are left unaltered except for a description of your changes.
 * MOTIVATION
 *   The way of parsing ToolTypes provided by "icon.library" is rather
 *   clumsy. This is particular annoying as many programmers and users got
 *   used to ReadArgs(), which does the argument handling for many CLI
 *   commands and ARexx ports.
 *
 *   Unfortunately, ReadArgs lacks a interface to Workbench tooltypes, thus
 *   its usage preventes your programs from being started from Workbench.
 *
 *   SmartReadArgs() copies all Workbench arguments in a single string and
 *   passes this string to the ReadArgs() function. If started from CLI, it
 *   calls ReadArgs() without this step.
 * AUTHOR
 *   Stefan Ruppert wrote most parts of the source code, designed the general
 *   interface and implemented loads of nice features. Basically he did the
 *   "hard work".
 *
 *   He got the main idea for the implementation from Stefan Winterstein,
 *   the author of ARoach.
 *
 *   Thomas Aglassinger <agi@sbox.tu-graz.ac.at> did some minor changes,
 *   established a more consistent naming schema, reworked the documentation
 *   and also added support for gcc/libnix.
 *
 *   Contact him in case of problems or if you made some enhancements.
 *
 *   Updates are available from aminet:dev/misc/SmartReadArgs.lha.
 * DISCLAIMER
 *   There is no warranty for this software package. Although the author
 *   has tried to prevent errors, he can't guarantee that the software
 *   package described in this document is 100% reliable. You are
 *   therefore using this material at your own risk. The author cannot be
 *   made responsible for any damage which is caused by using this
 *   software package.
 ****************************************************************************/

/* The define below is used to rename the example main() used in the autodoc
 * to dummy_main(). Using two main()s would cause problems for the linker. */
#define main dummy_main

/****** SmartReadArgs/SmartReadArgs ******************************************
 * NAME
 *   SmartReadArgs -- Workbench/CLI transparent ReadArgs().
 * SYNOPSIS
 *   error = SmartReadArgs(wb_startup, smart_args);
 *
 *   LONG SmartReadArgs(struct WBStartup *, struct SmartArgs *);
 * FUNCTION
 *   This function is a CLI/Workbench transparent interface to ReadArgs().
 *
 *   In case of a Workbench start, it analyzes the WBStartup message and
 *   possible tooltypes. These are converted to a text string that can be
 *   passed to ReadArgs() like before.
 *
 *   Tooltypes that are not part of the template are ignored. This includes
 *   tooltypes being disabled with "(...)", NewIcons image data on systems
 *   without NewIcons installed and all this «« Icon by some idiot »» crap.
 *
 *   If the application was stared from CLI, it simply calls ReadArgs()
 *   without the conversion step.
 *
 *   If all went well you get a return value of zero. This means the passed
 *   arguments fit the template and are ready to use. Otherwise you get a
 *   IoErr()-like return code.
 * INPUTS
 *    wb_startup - Workbench startup message. Refer to your compiler manual
 *        to learn how to obtain it. If this is NULL, the program was
 *        started from CLI.
 *    smart_args - structure which holds all information used by
 *        SmartReadArgs(). You have to setup the following fields before the
 *        call:
 *
 *        sa_Template - The template passed to ReadArgs()
 *        sa_Parameter - ReadArgs() LONG WORD array to hold the arguments
 *        sa_FileParameter - number of Argument in the template to use
 *            for the files passed via WBStartup->sm_ArgList or -1, that
 *            means you don't want any files
 *        sa_Window - Window description string to open when the program
 *            is started from the workbench. NULL means no window. If the
 *            icon has a WINDOW tooltype, this value is ignored.
 *        sa_RDArgs - RDArgs structure to use for ReadArgs() call. This
 *            can be used to provide extended help.
 *        sa_Buffer - Pointer to a buffer to use for the Workbench startup
 *            or NULL, that means SmartReadArgs() allocates a buffer for you
 *        sa_BufferSize - Size of the optional buffer. If it is smaller than
 *            SA_MINIMUM_BUFFER_SIZE it will be adjusted.
 *
 *        All other fields should be set to NULL.
 * RESULTS
 *   Zero for success. You can check the sa_Flags field for the
 *   SAF_WORKBENCH flag to learn how the program was started.
 *
 *   Otherwise an IoErr()-like error code is returned. This can be passed
 *   directly to PrintFault() or similar.
 * NOTES
 *   Always call SmartFreeArgs(), even if SmartReadArgs() failed! See example
 *   below.
 *
 *   This function requires "dos.library", "icon.library" and
 *   "utility.library" to be opened by the application. Normally this
 *   already has been done by the compiler startup code.
 *
 *   There is a not widely known feature of ReadArgs(): with templates like
 *   "FROM/M/A,TO/A", you can select the files from workbench performing the
 *   following steps:
 *
 *   - Select the program
 *   - Select the FROM files
 *   - Select and double click the TO file
 *
 *   This is available because ReadArgs() grabs the last string from a
 *   multi-argument FROM and uses it as the TO parameter, if none is passed
 *   explicitely.
 * BUGS
 *   There are some known problems when used with GCC, mostly related to the
 *   fact that I never bothered creating a useable developer environment
 *   around it (and I'm not sure if this is even possible >:) ...):
 *
 *   - Debugging output shows up in the console instead of SER:. Does
 *     debug.lib exist for gcc? (Wasn't there this strange hunk2gcc
 *     converter?)
 *   - "Read from 0" Enforcer hit in SmartReadArgs(). Couldn't figure out
 *     the exact location yet because of the asynchronous debugging output
 *     mentioned above.
 *
 *   For someone with a reasonable experience with GCC, it should be easy to
 *   fix this.
 *
 *   The SAS/c implementation does not have these problems.
 * SEE ALSO
 *   SmartFreeArgs(), dos.library/ReadArgs(), icon.library/GetDiskObjectNew()
 * EXAMPLE
 *   The main archiev comes with a "test.c" and a couple of icons to start
 *   the corresponding executable "test". Take a look at the source code and
 *   play with it.
 *
 *   See below for a smaller code segment that expects the "dos.library",
 *   "icon.library" and "utility.library" to be open already.
 * SOURCE
 */
/****************************************************************************/
LONG SmartReadArgs(struct WBStartup * wb_startup, struct SmartArgs * args)
{
   LONG error;

#if defined(__amigaos4__)
   struct ExecIFace *IExec = (struct ExecIFace *)(((struct ExecBase *)SysBase)->MainInterface);
#endif

   args->sa_Flags = 0;

   D(DBF_STARTUP, "UtilityBase = 0x%08lx", (ULONG)UtilityBase);
   D(DBF_STARTUP, "IconBase    = 0x%08lx", (ULONG)IconBase);
   D(DBF_STARTUP, "WBStartup   = 0x%08lx", (ULONG)wb_startup);

   if (wb_startup != NULL)
   {
      struct WBArg *wbarg = wb_startup->sm_ArgList;
      LONG arg_counter = 0;

      D(DBF_STARTUP, "  numArgs   = %ld", wb_startup->sm_NumArgs);
      while (arg_counter < wb_startup->sm_NumArgs)
      {
         D(DBF_STARTUP, "  name[%ld] = '%s'", arg_counter, wbarg->wa_Name);
         wbarg += 1;
         arg_counter += 1;
      }
   }

   if (wb_startup != NULL)
   {
      if (!(args->sa_RDArgs = AllocDosObject(DOS_RDARGS, NULL)))
      {
         return (ERROR_NO_FREE_STORE);
      }
      else
      {
         args->sa_Flags |= SAF_ALLOCRDARGS;

         if (!args->sa_Buffer)
         {
            args->sa_BufferSize = MAX(SA_MINIMUM_BUFFER_SIZE, args->sa_BufferSize);
            args->sa_Buffer = AllocVecShared(args->sa_BufferSize, MEMF_ANY);
            args->sa_Flags |= SAF_ALLOCBUFFER;
         }

         if (!args->sa_Buffer)
            return (ERROR_NO_FREE_STORE);
         else
         {
            struct DiskObject *dobj;

            args->sa_ActualPtr = args->sa_Buffer;
            args->sa_EndPtr = args->sa_Buffer + args->sa_BufferSize - 1;

            if (!(dobj = smart_get_icon(args, wb_startup)))
            {
               return (ERROR_OBJECT_NOT_FOUND);
            }
            else
            {
               struct WBArg *wbarg = args->sa_WBArg;
               ULONG num = args->sa_NumArgs;

               STRPTR *tooltypes = (STRPTR *) dobj->do_ToolTypes;
               STRPTR name;
               STRPTR temp;
               STRPTR ptr;

               if (num > 1 && args->sa_FileParameter >= 0 && (temp = AllocVecShared(TEMPSIZE, MEMF_ANY)))
               {
                  ULONG modes = 0;

                  get_arg_name(args, temp, TEMPSIZE, &modes);
                  fstrcpy(args, temp);
                  fstrcpy(args, " ");

                  /* no "/M" specifier in the ReadArgs() template, thus use only the first file */
                  if (modes != MODE_MULTI)
                     num = 2;

                  while (num > 1)
                  {
                     get_wbarg_name(wbarg, temp, TEMPSIZE);
                     fstrcpy(args, "\"");
                     fstrcpy(args, temp);
                     fstrcpy(args, "\" ");
                     num--;
                     wbarg++;
                  }

                  FreeVec(temp);
               }

               D(DBF_STARTUP, "tooltypes=%08lx", (ULONG)tooltypes);
               if (tooltypes)
               {
                  while (*tooltypes)
                  {
                     ptr = *tooltypes;
                     name = ptr;

                     /* check if this tooltype enabled and part of the
                      * template */
                     if ((*ptr != '(')
                         && is_in_template(name, args->sa_Template))
                     {
                        while (*ptr != '=' && *ptr != EOS)
                           ptr++;

                        if (*ptr == '=')
                        {
                           *ptr = EOS;

                           if (!Stricmp(name, "WINDOW"))
                           {
                              STRPTR win;
                              if ((win = AllocVecShared((ULONG) strlen(ptr + 1) + 1, MEMF_ANY)))
                              {
                                 strcpy(win, ptr + 1);
                                 args->sa_Window = win;
                                 args->sa_Flags |= SAF_ALLOCWINDOW;
                              }

                           }
                           else
                           {
                              fstrcpy(args, name);

                              /* enclose the argument in "" */
                              if (*(ptr + 1) == '"')
                              {
                                 fstrcpy(args, "=");
                                 fstrcpy(args, ptr + 1);
                              }
                              else
                              {
                                 fstrcpy(args, "=\"");
                                 fstrcpy(args, ptr + 1);
                                 fstrcpy(args, "\"");
                              }

                              *ptr = '=';
                           }
                        }
                        else
                           fstrcpy(args, name);

                        fstrcpy(args, " ");
                     }
                     tooltypes++;
                  }             /* while (*tooltypes) */
               }                /* if (tooltypes) */
               fstrcpy(args, "\n");

               D(DBF_STARTUP, "final wb command line : '%s'", args->sa_Buffer);
            }
         }
      }

      args->sa_RDArgs->RDA_Source.CS_Buffer = args->sa_Buffer;
      args->sa_RDArgs->RDA_Source.CS_Length = strlen(args->sa_Buffer);

      args->sa_Flags |= SAF_WORKBENCH;
   }

   SetIoErr(0);

   args->sa_FreeArgs = ReadArgs(args->sa_Template, (APTR)args->sa_Parameter, args->sa_RDArgs);

   if (SetSignal(0L, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
   {
      SetIoErr(ERROR_BREAK);
   }

   if ((error = IoErr()) == 0 && (wb_startup != NULL))
   {
      if (args->sa_Window)
      {
         args->sa_WindowFH = Open(args->sa_Window, MODE_NEWFILE);
         if (args->sa_WindowFH)
         {
            args->sa_OldOutput = SelectOutput(args->sa_WindowFH);
            args->sa_OldInput = SelectInput(args->sa_WindowFH);
         }
      }
   }

   return (error);
}

/****** SmartReadArgs/SmartFreeArgs ******************************************
 * NAME
 *   SmartFreeArgs -- Free all resources allocated by SmartReadArgs().
 * SYNOPSIS
 *   SmartFreeArgs(smart_args);
 *
 *   void SmartFreeArgs(struct SmartArgs *);
 * FUNCTION
 *   Free all resources allocated by a previous call to SmartReadArgs().
 * INPUTS
 *   smart_args - Same pointer as passed to SmartReadArgs() before
 * NOTES
 *   Always call SmartFreeArgs(), even if SmartReadArgs() failed! Take a look
 *   at the example for SmartReadArgs().
 * SEE ALSO
 *   SmartReadArgs()
 ****************************************************************************/
void SmartFreeArgs(struct SmartArgs *args)
{
   /* FreeArgs() can handle a NULL pointer */
   FreeArgs(args->sa_FreeArgs);

   if (args->sa_Flags & SAF_ALLOCRDARGS)
      if (args->sa_RDArgs)
         FreeDosObject(DOS_RDARGS, args->sa_RDArgs);

   if (args->sa_Flags & SAF_ALLOCBUFFER)
      FreeVec(args->sa_Buffer);

   if (args->sa_WindowFH)
   {
      SelectOutput(args->sa_OldOutput);
      SelectInput(args->sa_OldInput);
      Close(args->sa_WindowFH);
   }

   if (args->sa_Flags & SAF_ALLOCWINDOW && args->sa_Window)
      FreeVec(args->sa_Window);

}

/* This code was grapped from IconImage/wbarg.c/IconFromWBArg()
 * Commodore-Amiga Example code
 */
static struct DiskObject *smart_get_icon(struct SmartArgs *args, struct WBStartup *wb_startup)
{
   struct DiskObject *dob = NULL;
   struct WBArg *wbarg = wb_startup->sm_ArgList;
   ULONG num = wb_startup->sm_NumArgs;

   TEXT work_name[MAXIMUM_FILENAME_LENGTH];
   BPTR old_lock, new_lock;

   /* Copy the WBArg contents */
   strncpy(work_name, wbarg->wa_Name, MAXIMUM_FILENAME_LENGTH - 1);

   new_lock = DupLock(wbarg->wa_Lock);
   if (new_lock != ZERO)
   {
      D(DBF_STARTUP, "work_name : '%s'", work_name);

      /* go to the directory where the icon resides */
      old_lock = CurrentDir(new_lock);

      dob = GetDiskObjectNew(work_name);

      /* test, if the first icon is a project icon and if so, get its icon */
      if (wb_startup->sm_NumArgs > 1)
      {
         BPTR new_lock2;

         if ((new_lock2 = DupLock(wbarg[1].wa_Lock)))
         {
            struct DiskObject *prj;

            CurrentDir(new_lock2);

            UnLock(new_lock);
            new_lock = new_lock2;

            strncpy(work_name, wbarg[1].wa_Name, MAXIMUM_FILENAME_LENGTH - 1);
            D(DBF_STARTUP, "work_name2 : '%s'", work_name);

            if ((prj = GetDiskObjectNew(work_name)))
            {
               if (prj->do_Type == WBPROJECT)
               {
                  BPTR test;

                  /* if this is only an icon skip it */
                  if (!(test = Lock(work_name, SHARED_LOCK)))
                  {
                     wbarg++;
                     num--;
                  }
                  else
                     UnLock(test);

                  if (dob)
                     FreeDiskObject(dob);

                  dob = prj;
               }
            }
         }
      }

      if (dob)
      {
         D(DBF_STARTUP, "dobj window : '%s'", dob->do_ToolWindow);
      }

      /* go back to where we used to be */
      CurrentDir(old_lock);

      /* release the duplicated lock */
      UnLock(new_lock);

      args->sa_WBArg = wbarg + 1;
      args->sa_NumArgs = num;
   }

   D(DBF_STARTUP, "return (dob)");

   return (dob);
}

static void fstrcpy(struct SmartArgs *args, CONST_STRPTR string)
{
   STRPTR ptr = args->sa_ActualPtr;
   STRPTR end = args->sa_EndPtr;

   while (ptr < end && *string)
      *ptr++ = *string++;

   *ptr = EOS;                  /* Mark end of string */

   args->sa_ActualPtr = ptr;
}

static void get_arg_name(struct SmartArgs *args, STRPTR buffer, ULONG size, ULONG * modes)
{
   ULONG num = args->sa_FileParameter;
   CONST_STRPTR ptr = args->sa_Template;

   *modes = 0;

   while (num > 0)
   {
      while (*ptr != ',' && *ptr != EOS)
         ptr++;

      if (*ptr == ',')
         ptr++;
      num--;
   }

   if (*ptr != EOS)
   {
      while (*ptr != ',' && *ptr != '/' && *ptr != EOS && size > 0)
      {
         *buffer++ = *ptr++;
         size--;
      }

      while (*ptr == '/')
      {
         ptr++;

         if (*ptr == 'M' || *ptr == 'm')
            *modes = MODE_MULTI;

         ptr++;
      }
   }

   *buffer = EOS;
}

static void get_wbarg_name(struct WBArg *wbarg, STRPTR buffer, ULONG size)
{
   BPTR new;

   if ((new = DupLock(wbarg->wa_Lock)))
   {
      if (!NameFromLock(new, buffer, size))
         *buffer = EOS;
      else if (!AddPart(buffer, wbarg->wa_Name, size))
         *buffer = EOS;

      UnLock(new);
   }
   else
      *buffer = EOS;
}

static BOOL is_in_template(STRPTR name, CONST_STRPTR template)
{
   BOOL found = FALSE;
   CONST_STRPTR current_word = template;
   BOOL skip_switch = FALSE;
   size_t name_length;

   /* Evaluate length of name part of whole tooltype */
   name_length = 0;
   while ((name[name_length] != EOS)
          && (name[name_length] != '='))
   {
      name_length += 1;
   }

   D(DBF_TEMPLATE, "find '%s' in template '%s'\n", name, template);
   while ((current_word[0] != '\0') && (!found))
   {
      STRPTR next_word = strpbrk(current_word, "/=,");
      size_t current_word_length;

      if (next_word == NULL)
      {
         next_word = (STRPTR)current_word + strlen(current_word);
      }
      current_word_length = next_word - current_word;

      if (skip_switch)
      {
         D(DBF_TEMPLATE, "  skip  ('%s', %lu)", current_word, current_word_length);
         skip_switch = FALSE;
      }
      else
      {
         D(DBF_TEMPLATE, "  check ('%s', %lu)", current_word, current_word_length);
         if ((name_length == current_word_length)
             && !Strnicmp(name, current_word, (LONG) name_length))
         {
            D(DBF_TEMPLATE, "  found!");
            found = TRUE;
         }
      }

      current_word = next_word;
      if (current_word[0] != '\0')
      {
         if (current_word[0] == '/')
         {
            skip_switch = TRUE;
         }
         current_word += 1;
      }
   }

   return found;
}

