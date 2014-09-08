/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2013 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/openurl.h>

#include <exec/memory.h>
#include <dos/dos.h>
#include <libraries/openurl.h>
#include <intuition/intuition.h>

#include <string.h>
#include <stdio.h>

#include "SmartReadArgs.h"
#include "macros.h"
#include "version.h"

#include "debug.h"

static const char USED_VAR version[] = "$VER: OpenURL " LIB_REV_STRING " [" SYSTEMSHORT "/" CPU "] (" LIB_DATE ") " LIB_COPYRIGHT;

/**************************************************************************/

#define TEMPLATE "URL/A,NOSHOW/S,NOFRONT/S,NEWWIN/S,NOLAUNCH/S,PUBSCREEN/K,FILE/S"

enum
{
   A_URL, A_NOSHOW, A_NOFRONT, A_NEWWIN, A_NOLAUNCH, A_PUBSCREEN, A_FILE, A_MAX
};

/* Maximum length an URL can get according to w3c. Currently, CLI
 * imposes even a smaller limit of 512.
 */
#define MAXIMUM_URL_LENGTH 1024

#if defined(__amigaos4__)
struct Library *IntuitionBase;
struct Library *IconBase;
struct Library *UtilityBase;
struct Library *OpenURLBase;
struct IntuitionIFace *IIntuition = NULL;
struct IconIFace *IIcon = NULL;
struct UtilityIFace *IUtility = NULL;
struct OpenURLIFace * IOpenURL = NULL;
#else
struct IntuitionBase *IntuitionBase;
struct Library *IconBase;
struct Library *UtilityBase;
struct Library *OpenURLBase;
#endif

#define OPENURL_VERSION      4   /* Minimum version of openurl.library */
#define OPENURL_VERSION_STR "4"  /* Used by error message */
#define MAXIMUM_ERROR_LENGTH 120 /* Size of buffer for error messages */

#if defined(__amigaos4__)
#define AllocVecShared(size, flags)  AllocVecTags((size), AVT_Type, MEMF_SHARED, AVT_Lock, FALSE, ((flags)&MEMF_CLEAR) ? AVT_ClearWithValue : TAG_IGNORE, 0, TAG_DONE)
#else
#define AllocVecShared(size, flags)  AllocVec((size), (flags))
#endif

/* Some shit necessary because C standard libraries suck */
ULONG ulong_max(ULONG a, ULONG b)
{
    return a>b ? a : b;
}

/* Main function
 *
 * Note that the startup code of SAS/c sets argc=0 when started from
 * workbench. This fact is used to auto-enable FILE in such a case.
 */
int main(int argc,char **argv)
{
   int              return_code = RETURN_FAIL;
   LONG             error_code = 0;
   CONST_STRPTR     error_cause = NULL;
   struct WBStartup *wb_startup = NULL;
   TEXT             error_buffer[MAXIMUM_ERROR_LENGTH] = "";

   // setup the debugging stuff
   #if defined(DEBUG)
   SetupDebug();
   #endif

   if((IntuitionBase = (APTR)OpenLibrary("intuition.library", 37)) != NULL &&
   	  GETINTERFACE(IIntuition, IntuitionBase))
   {
     if((IconBase = OpenLibrary("icon.library", 37)) != NULL &&
     	  GETINTERFACE(IIcon, IconBase))
     {
       if((UtilityBase = OpenLibrary("utility.library", 37)) != NULL &&
       	  GETINTERFACE(IUtility, UtilityBase))
       {
         if((OpenURLBase = OpenLibrary("openurl.library", OPENURL_VERSION)) != NULL &&
            GETINTERFACE(IOpenURL, OpenURLBase))
         {
            struct TagItem tags[8];
            struct SmartArgs smart_args;
            IPTR args[A_MAX];
            STRPTR real_url = NULL;
            STRPTR filename = NULL;

            /* Prepare argument parsing */
            memset(&args, 0, sizeof(args));
            memset(&smart_args, 0, sizeof(smart_args));
            smart_args.sa_Template = TEMPLATE;
            smart_args.sa_Parameter = (IPTR*)args;
            smart_args.sa_FileParameter = A_URL;
            smart_args.sa_Window = (STRPTR)"CON:////OpenURL/AUTO/CLOSE/WAIT";

            return_code = RETURN_ERROR;

            /* Auto-enable FILE switch when started from WB */
            if (argc==0)
            {
               wb_startup = (struct WBStartup *)argv;
               args[A_FILE] = 1;
            }

            /* Parse arguments, either from Workbench or CLI */
            error_code = SmartReadArgs(wb_startup,&smart_args);

            /* Allocate string buffers. This reduces stack usage and also
             * makes the string buffers being checked by Mungwall.
             *
             * Yes, this wastes some memory if FILE is not set, but makes the
             * error handling easier.
             *
             * Yes, we could use a 2K buffer with filename = real_url + 1024 to
             * use only one allocation. This however would reduce the possibilty
             * for out-of-bounds checks for the end of one/beginning of the
             * other string. */
            if (error_code==0)
            {
               /* Allocate string buffers */
               real_url = AllocVecShared(MAXIMUM_URL_LENGTH, MEMF_ANY);
               filename = AllocVecShared(MAXIMUM_URL_LENGTH, MEMF_ANY);
               if (!real_url || !filename)
               {
                  /* Not enough memory */
                  SetIoErr(ERROR_NO_FREE_STORE);
                  error_code = IoErr();
               }
            }

            if (error_code==0)
            {
               if (args[A_FILE])
               {
                  /* Expand the filename to a fully qualified URL */
                  BPTR lock = Lock((STRPTR)args[A_URL],ACCESS_READ);

                  if (lock)
                  {
                     if (NameFromLock(lock,filename,MAXIMUM_URL_LENGTH))
                     {
                        strlcpy(real_url,"file://localhost/",MAXIMUM_URL_LENGTH);
                        strlcat(real_url,filename,MAXIMUM_URL_LENGTH);
                     }
                     else
                     {
                        error_cause = "Error obtaining full filename";
                        error_code  = IoErr();
                     }

                     UnLock(lock);
                  }
                  else
                  {
                     error_cause = "Error opening input file";
                     error_code  = IoErr();
                  }
               }
               else
               {
                  /* Simply use the URL passed in arguments, assuming it is
                   * an already fully qualified URL of any protocol. Possible
                   * errors are now treated by the browser. */
                  strlcpy(real_url,(STRPTR)args[A_URL],MAXIMUM_URL_LENGTH);
               }

               if (error_code==0)
               {
                  int i = 0;
                  if ((args[A_NOSHOW]) != 0)
                  {
                     tags[i].ti_Tag = URL_Show;
                     tags[i].ti_Data = FALSE; i++;
                  }
                  if ((args[A_NOFRONT]) != 0)
                  {
                     tags[i].ti_Tag = URL_BringToFront;
                     tags[i].ti_Data = FALSE; i++;
                  }
                  if ((args[A_NEWWIN]) != 0)
                  {
                     tags[i].ti_Tag = URL_NewWindow;
                     tags[i].ti_Data = TRUE; i++;
                  }
                  if ((args[A_NOLAUNCH]) != 0)
                  {
                     tags[i].ti_Tag = URL_Launch;
                     tags[i].ti_Data = FALSE; i++;
                  }
                  if (args[A_PUBSCREEN])
                  {
                     tags[i].ti_Tag = URL_PubScreenName;
                     tags[i].ti_Data = args[A_PUBSCREEN]; i++;
                  }
                  tags[i].ti_Tag = TAG_DONE; tags[i].ti_Data = 0;

                  if (URL_OpenA(real_url, tags))
                  {
                     return_code = RETURN_OK;
                  }
                  else
                  {
                     error_code = 0;
                     if (args[A_NOLAUNCH])
                     {
                        error_cause = "Could not find browser port";
                     }
                     else
                     {
                        error_cause = "Could not launch browser";
                     }
                  }
               }
            }
            else
            {
               error_cause = "Error in arguments";
            }

            /* Free extended read args (even in case of error) */
            SmartFreeArgs(&smart_args);

            /* Release all other resources */
            if (filename) FreeVec(filename);
            if (real_url) FreeVec(real_url);

            DROPINTERFACE(IOpenURL);
            CloseLibrary(OpenURLBase);
         }
         else
         {
            error_cause = "Could not find 'openurl.library', version " OPENURL_VERSION_STR;
         }

         DROPINTERFACE(IUtility);
         CloseLibrary(UtilityBase);
       }
       else
       {
       }

       DROPINTERFACE(IIcon);
       CloseLibrary(IconBase);
     }
     else
     {
     }

     DROPINTERFACE(IIntuition);
     CloseLibrary((struct Library *)IntuitionBase);
   }
   else
   {
   }

   /* Create error message in error_buffer (if any) */
   if (error_code!=0)
   {
      Fault(error_code, (STRPTR)error_cause, error_buffer, MAXIMUM_ERROR_LENGTH);
   }
   else if (error_cause!=NULL)
   {
      strncpy(error_buffer,error_cause,MAXIMUM_ERROR_LENGTH-1);
   }

   /* Display error message in CLI or Requester (if any) */
   if (error_buffer[0] != '\0')
   {
      if (wb_startup)
      {
         struct EasyStruct error_requester;

         memset(&error_requester, 0, sizeof(error_requester));
         error_requester.es_StructSize = sizeof(struct EasyStruct);
         error_requester.es_Title = (STRPTR)"OpenURL Error";
         error_requester.es_TextFormat = (STRPTR)"%s";
         error_requester.es_GadgetFormat = (STRPTR)"Cancel";

         EasyRequestArgs(NULL, &error_requester, NULL, &error_buffer);
      }
      else
      {
         FPuts(Output(),error_buffer);
         FPuts(Output(),"\n");
      }
   }

   return return_code;
}

/**************************************************************************/
