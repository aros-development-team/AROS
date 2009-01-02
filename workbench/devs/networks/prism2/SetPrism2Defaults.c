/*

Author: Neil Cafferkey
Copyright (C) 2004,2005 Neil Cafferkey

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.

*/


#include <exec/memory.h>
#include <dos/dos.h>
#include <utility/utility.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include "wireless.h"


#ifndef UPINT
#ifdef __AROS__
typedef IPTR UPINT;
typedef SIPTR PINT;
#else
typedef ULONG UPINT;
typedef LONG PINT;
#endif
#endif

#define UTILITY_VERSION 39
#define DOS_VERSION 36

#ifndef __AROS__
IMPORT struct ExecBase *AbsExecBase;
#endif

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct UtilityBase *UtilityBase;

static UPINT ParseHexString(TEXT *str, UBYTE *buffer, UPINT buffer_size);
static UPINT StrLen(const TEXT *s);

const TEXT template[] =
   "SSID/K,KEY/K,TEXTKEY/K,NOKEY/S,MANAGED/S,ADHOC/S,CHANNEL/K/N";
const TEXT version_string[] = "$VER: SetPrism2Defaults 1.2 (23.7.2005)";
const TEXT dos_name[] = DOSNAME;
const TEXT utility_name[] = UTILITYNAME;
const TEXT options_name[] = "Prism 2 options";
static const struct TagItem tag_list_template[] =
{
   {P2OPT_SSID, 0},
   {P2OPT_WEPKey, 0},
   {P2OPT_Encryption, S2ENC_NONE},
   {P2OPT_PortType, S2PORT_MANAGED},
   {P2OPT_Channel, 0},
   {TAG_END, 0}
};
static const struct TagItem name_tag_list[] =
{
   {ANO_NameSpace, TRUE},   /* Work-around for old MorphOS bug */
   {TAG_END, 0}
};


LONG Main(VOID)
{
   struct RDArgs *read_args;
   LONG error = 0, result = RETURN_OK;
   UBYTE key_buffer[IEEE802_11_WEP128LEN];
   struct
   {
      TEXT *ssid;
      TEXT *key;
      TEXT *textkey;
      PINT nokey;
      PINT managed;
      PINT adhoc;
      PINT *channel;
   }
   args = {NULL, NULL, NULL, FALSE, FALSE, FALSE, NULL};
   struct NamedObject *options;
   struct TagItem *tag_list, *tag_item;
   TEXT *ssid;
   UPINT length;
   struct WEPKey *key;
   UWORD key_option_count = 0;

   /* Open libraries */

#ifdef __mc68000
   SysBase = AbsExecBase;
#endif
   DOSBase = (struct DosLibrary *)OpenLibrary(dos_name, DOS_VERSION);
   if(DOSBase == NULL)
      return RETURN_FAIL;
   UtilityBase =
      (struct UtilityBase *)OpenLibrary(utility_name, UTILITY_VERSION);

   if(UtilityBase == NULL)
      error = IoErr();

   /* Parse arguments */

   read_args = ReadArgs(template, (PINT *)&args, NULL);
   if(read_args == NULL)
      error = IoErr();
   else
   {
      if(args.key != NULL)
         key_option_count++;
      if(args.textkey != NULL)
         key_option_count++;
      if(args.nokey)
         key_option_count++;
      if(key_option_count > 1 || args.managed && args.adhoc)
         error = ERROR_TOO_MANY_ARGS;
   }

   /* Get pre-existing options object or create a new one */

   if(error == 0)
   {
      options = FindNamedObject(NULL, options_name, NULL);

      if(options == NULL)
      {
         options = AllocNamedObjectA(options_name, name_tag_list);
         if(options != NULL)
         {
            if(AddNamedObject(NULL, options))
            {
               tag_list = CloneTagItems(tag_list_template);
               if(tag_list != NULL)
               {
                  options->no_Object = tag_list;
                  ssid = AllocMem(IEEE802_11_MAXIDLEN,
                     MEMF_PUBLIC | MEMF_CLEAR);
                  if(ssid == NULL)
                     error = IoErr();
                  key = AllocMem(sizeof(struct WEPKey),
                     MEMF_PUBLIC | MEMF_CLEAR);
                  if(key == NULL)
                     error = IoErr();
                  if(error == 0)
                  {
                     key->length = IEEE802_11_WEP64LEN;
                     tag_item = FindTagItem(P2OPT_SSID, tag_list);
                     tag_item->ti_Data = (UPINT)ssid;
                     tag_item = FindTagItem(P2OPT_WEPKey, tag_list);
                     tag_item->ti_Data = (UPINT)key;
                  }
               }
               else
                  error = IoErr();
            }
            else
            {
               error = IoErr();
               FreeNamedObject(options);
            }
         }
         else
            error = IoErr();
      }
   }

   /* Set new options */

   if(error == 0)
   {
      tag_list = (APTR)options->no_Object;

      if(args.ssid != NULL)
      {
         tag_item = FindTagItem(P2OPT_SSID, tag_list);
         length = StrLen(args.ssid);
         if(length <= IEEE802_11_MAXIDLEN)
            CopyMem(args.ssid, (APTR)tag_item->ti_Data, length);
         else
            error = IoErr();
      }

      if(args.key != NULL)
      {
         tag_item = FindTagItem(P2OPT_WEPKey, tag_list);
         length =
            ParseHexString(args.key, key_buffer, IEEE802_11_WEP128LEN);
         if(length == 0)
            error = ERROR_BAD_NUMBER;
         else if(length != IEEE802_11_WEP64LEN &&
            length != IEEE802_11_WEP128LEN)
            error = ERROR_BAD_NUMBER;
         else
         {
            key = (APTR)tag_item->ti_Data;
            key->length = length;
            CopyMem(key_buffer, key->key, length);
         }
      }

      if(args.textkey != NULL)
      {
         tag_item = FindTagItem(P2OPT_WEPKey, tag_list);
         length = StrLen(args.textkey);
         if(length == 0)
            error = ERROR_INVALID_COMPONENT_NAME;
         else if(length != IEEE802_11_WEP64LEN &&
            length != IEEE802_11_WEP128LEN)
            error = ERROR_INVALID_COMPONENT_NAME;
         else
         {
            key = (APTR)tag_item->ti_Data;
            key->length = length;
            CopyMem(args.textkey, key->key, length);
         }
      }
   }

   if(error == 0)
   {
      tag_item = FindTagItem(P2OPT_Encryption, tag_list);
      if(args.key != NULL || args.textkey != NULL)
         tag_item->ti_Data = S2ENC_WEP;
      else if(args.nokey)
         tag_item->ti_Data = S2ENC_NONE;

      tag_item = FindTagItem(P2OPT_PortType, tag_list);
      if(args.managed)
         tag_item->ti_Data = S2PORT_MANAGED;
      else if(args.adhoc)
         tag_item->ti_Data = S2PORT_ADHOC;

      if(args.channel != NULL)
      {
         tag_item = FindTagItem(P2OPT_Channel, tag_list);
         if(*args.channel >= 3 && *args.channel <= 14)
            tag_item->ti_Data = *args.channel;
         else
            error = ERROR_BAD_NUMBER;
      }
   }

   FreeArgs(read_args);

   /* Print error message */

#ifdef USE_HACKS
   SetIoErr(error);
#endif
   PrintFault(error, NULL);

   if(error != 0)
      result = RETURN_FAIL;

   /* Close libraries and exit */

   CloseLibrary((struct Library *)UtilityBase);
   CloseLibrary((struct Library *)DOSBase);

   return result;
}



static UPINT ParseHexString(TEXT *str, UBYTE *buffer, UPINT buffer_size)
{
   BOOL success = TRUE;
   UBYTE n = 0, *end;
   TEXT ch;
   UPINT i = 0;

   end = buffer + buffer_size;
   while((ch = *str++) != '\0' && buffer < end)
   {
      n <<= 4;

      ch = ToUpper(ch);
      if(ch != '-' && ch != ':' && ch != ' ')
      {
         if(ch >= '0' && ch <= '9')
            n |= ch - '0';
         else if(ch >= 'A' && ch <= 'F')
            n |= ch - 'A' + 10;
         else
            success = FALSE;

         if((++i & 0x1) == 0)
         {
            *buffer++ = n;
            n = 0;
         }
      }
   }

   if((i & 0x1) != 0)
      success = FALSE;

   if(!success)
      i = 0;

   return i >> 1;
}



static UPINT StrLen(const TEXT *s)
{
   const TEXT *p;

   for(p = s; *p != '\0'; p++);
   return p - s;
}



