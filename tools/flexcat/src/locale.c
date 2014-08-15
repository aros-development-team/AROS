
/****************************************************************

   This file was created automatically by `FlexCat 2.15'
   from "locale/FlexCat.pot".

   Do NOT edit by hand!

****************************************************************/

/* Include files */
#include <libraries/gadtools.h>
#include <proto/locale.h>
#include <string.h>

#include "FlexCat_cat.h"

/* Variables */
struct FC_String FlexCat_Strings[] =
{
   { "Usage:", 0 },
   { "  CDFILE         Catalog description file to scan\n  CTFILE         Catalog translation file to scan\n  POFILE         Catalog translation in PO-style format\n  CATALOG        Catalog file to create\n  NEWCTFILE      Catalog translation file to create\n  SOURCES        Sources to create; must be something like SFILE=SDFILE,\n                 where SFILE is a source file and SDFILE is a source\n                 description file\n  WARNCTGAPS     Warn about identifiers missing in translation\n  NOOPTIM        Do not skip unchanged strings in translation/description\n  FILL           Fill missing identifiers with original text\n  FLUSH          Flush memory after the catalog is created\n  NOBEEP         No DisplayBeep() on errors and warnings\n  QUIET          No warnings\n  NOLANGTOLOWER  Prevent #language name from being lowercased\n  NOBUFFEREDIO   Disable I/O buffers\n  MODIFIED       Create catalog only if description/translation have changed\n  COPYMSGNEW     Copy ***NEW*** markers over from old translation\n  OLDMSGNEW      Custom marker in old translation\n  CODESET        Codeset to force in output file (e.g. 'UTF-8')\n  NOAUTODATE     no operation - kept for compatibility\n  NOSPACES       no operation - kept for compatibility", 1 },
   { "File '%s' is up to date", 2 },
   { "%s, line %d - warning:", 100 },
   { "%s, line %d - ERROR:", 101 },
   { "expected hex character (one of [0-9a-fA-F])", 102 },
   { "expected octal character (one of [0-7])", 103 },
   { "lengthbytes cannot be larger than %d (sizeof long)", 104 },
   { "unknown catalog description command", 105 },
   { "unexpected blanks", 106 },
   { "missing identifier", 107 },
   { "unexpected end of file (missing catalog strings)", 115 },
   { "unknown command in translation", 118 },
   { "'%s' missing in catalog description", 119 },
   { "unknown string type", 120 },
   { "unexpected end of line (missing ')')", 121 },
   { "binary characters in string type None", 122 },
   { "'%s' missing in catalog translation", 123 },
   { "catalog language declared twice", 124 },
   { "catalog version declared twice", 125 },
   { "incorrect RCS Id", 126 },
   { "out of memory!", 127 },
   { "cannot open catalog description '%s'", 128 },
   { "cannot open catalog translation '%s'", 129 },
   { "missing catalog translation version\nUse either '## version' or '## rcsid' and '## name'", 130 },
   { "cannot open catalog file '%s'", 131 },
   { "cannot create catalog translation '%s'", 132 },
   { "missing catalog translation language", 133 },
   { "cannot open source file '%s'", 134 },
   { "cannot open source description file '%s'", 135 },
   { "creating a catalog requires a translation file", 136 },
   { "cannot get datestamp of '%s'", 137 },
   { "Catalog translation file name not specified at command line or as basename in description", 140 },
   { "catalog file name not specified at command line or as basename in description", 141 },
   { "error processing 'FlexCat.prefs' variable, falling back to defaults\nTemplate:", 142 },
   { "invalid language in catalog translation file\nLanguage MUST be a string with alphabetical characters and no inlined or trailing spaces", 144 },
   { "catalog codeset declared twice", 146 },
   { "invalid codeset in catalog translation file\nCodeset MUST be a decimal number without any trailing spaces", 147 },
   { "missing catalog translation codeset", 148 },
   { "%s - ERROR:", 149 },
   { "invalid version string in catalog translation file\nVersion should be something like\n## version $VER: name version.revision (date)\nwithout any spaces in the name", 150 },
   { "%s - Warning:", 154 },
   { "missing translation for identifier '%s'", 155 },
   { "empty translation for identifier '%s'", 156 },
   { "mismatching trailing control characters", 157 },
   { "identifier '%s' declared twice", 159 },
   { "string too short for identifier '%s'", 160 },
   { "string too long for identifier '%s'", 161 },
   { "original string has a trailing ellipsis ('...') for identifier '%s'", 162 },
   { "original string doesn't have a trailing ellipsis ('...') for identifier '%s'", 163 },
   { "original string has trailing blanks for identifier '%s'", 164 },
   { "original string doesn't have trailing blanks for identifier '%s'", 165 },
   { "mismatching placeholders for identifier '%s'", 166 },
   { "missing placeholders for identifier '%s'", 167 },
   { "excessive placeholders for identifier '%s'", 168 },
   { "missing '(' for identifier '%s'", 169 },
   { "missing ')' for identifier '%s'", 170 },
   { "ID number used twice for identifier '%s'", 171 },
   { "expected MinLen (character '/') for identifier '%s'", 172 },
   { "expected MaxLen (character '/') for identifier '%s'", 173 },
   { "extra characters at the end of the line", 174 },
   { "extra characters at the end of the line for identifier '%s'", 175 },
   { "non-ASCII character 0x%02x found in original string for identifier '%s'", 176 },
   { "no catalog revision information found, using revision 0", 177 },
   { NULL, 0 }
};

static struct Catalog *FlexCatCatalog;

void OpenFlexCatCatalog()
{
   if(LocaleBase != NULL)
   {
      if ((FlexCatCatalog = OpenCatalog(NULL, (STRPTR)"FlexCat.catalog",
                                         OC_BuiltInLanguage, (STRPTR)"english",
                                         OC_Version, 3,
                                         TAG_DONE)) != NULL)
      {
         struct FC_String *fc;

         for(fc = FlexCat_Strings; fc->Str; fc++)
         {
            fc->Str = (const char *)GetCatalogStr(FlexCatCatalog, fc->Id, (STRPTR)fc->Str);
         }
      }
   }
}

void CloseFlexCatCatalog()
{
   if(FlexCatCatalog != NULL)
   {
      CloseCatalog(FlexCatCatalog);
      FlexCatCatalog = NULL;
   }
}

void LocalizeStringArray(STRPTR *Array)
{
   STRPTR *x;

   for(x = Array; *x != NULL; x++)
   {
      *x = strdup(FlexCat_Strings[(int)*x].Str);
   }
}
