
/****************************************************************

   This file was created automatically by `FlexCat 2.15'
   from "locale/FlexCat.pot".

   Do NOT edit by hand!

****************************************************************/

/* Copyright (C) 1993-1999 by Jochen Wiedmann and Marcin Orlowski
 * Copyright (C) 2002-2010 by the FlexCat Open Source Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/****************************************************************

   This file provides at least minimal compatibility with
   the AmigaOS catalog system on other platforms. It creates
   the necessary string tables (hardcoded ATM) and functions
   to make code work under any operating system.

   Carlos

****************************************************************/

void OpenFlexCatCatalog( void )
{}
void CloseFlexCatCatalog( void )
{}

const char * FlexCat_Strings[64] = {
    "Usage:",
    "  CDFILE         Catalog description file to scan\n  CTFILE         Catalog translation file to scan\n  POFILE         Catalog translation in PO-style format\n  CATALOG        Catalog file to create\n  NEWCTFILE      Catalog translation file to create\n  SOURCES        Sources to create; must be something like SFILE=SDFILE,\n                 where SFILE is a source file and SDFILE is a source\n                 description file\n  WARNCTGAPS     Warn about identifiers missing in translation\n  NOOPTIM        Do not skip unchanged strings in translation/description\n  FILL           Fill missing identifiers with original text\n  FLUSH          Flush memory after the catalog is created\n  NOBEEP         No DisplayBeep() on errors and warnings\n  QUIET          No warnings\n  NOLANGTOLOWER  Prevent #language name from being lowercased\n  NOBUFFEREDIO   Disable I/O buffers\n  MODIFIED       Create catalog only if description/translation have changed\n  COPYMSGNEW     Copy ***NEW*** markers over from old translation\n  OLDMSGNEW      Custom marker in old translation\n  CODESET        Codeset to force in output file (e.g. 'UTF-8')\n  NOAUTODATE     no operation - kept for compatibility\n  NOSPACES       no operation - kept for compatibility",
    "File '%s' is up to date",
    "%s, line %d - warning:",
    "%s, line %d - ERROR:",
    "expected hex character (one of [0-9a-fA-F])",
    "expected octal character (one of [0-7])",
    "lengthbytes cannot be larger than %d (sizeof long)",
    "unknown catalog description command",
    "unexpected blanks",
    "missing identifier",
    "unexpected end of file (missing catalog strings)",
    "unknown command in translation",
    "'%s' missing in catalog description",
    "unknown string type",
    "unexpected end of line (missing ')')",
    "binary characters in string type None",
    "'%s' missing in catalog translation",
    "catalog language declared twice",
    "catalog version declared twice",
    "incorrect RCS Id",
    "out of memory!",
    "cannot open catalog description '%s'",
    "cannot open catalog translation '%s'",
    "missing catalog translation version\nUse either '## version' or '## rcsid' and '## name'",
    "cannot open catalog file '%s'",
    "cannot create catalog translation '%s'",
    "missing catalog translation language",
    "cannot open source file '%s'",
    "cannot open source description file '%s'",
    "creating a catalog requires a translation file",
    "cannot get datestamp of '%s'",
    "Catalog translation file name not specified at command line or as basename in description",
    "catalog file name not specified at command line or as basename in description",
    "error processing 'FlexCat.prefs' variable, falling back to defaults\nTemplate:",
    "invalid language in catalog translation file\nLanguage MUST be a string with alphabetical characters and no inlined or trailing spaces",
    "catalog codeset declared twice",
    "invalid codeset in catalog translation file\nCodeset MUST be a decimal number without any trailing spaces",
    "missing catalog translation codeset",
    "%s - ERROR:",
    "invalid version string in catalog translation file\nVersion should be something like\n## version $VER: name version.revision (date)\nwithout any spaces in the name",
    "%s - Warning:",
    "missing translation for identifier '%s'",
    "empty translation for identifier '%s'",
    "mismatching trailing control characters",
    "identifier '%s' declared twice",
    "string too short for identifier '%s'",
    "string too long for identifier '%s'",
    "original string has a trailing ellipsis ('...') for identifier '%s'",
    "original string doesn't have a trailing ellipsis ('...') for identifier '%s'",
    "original string has trailing blanks for identifier '%s'",
    "original string doesn't have trailing blanks for identifier '%s'",
    "mismatching placeholders for identifier '%s'",
    "missing placeholders for identifier '%s'",
    "excessive placeholders for identifier '%s'",
    "missing '(' for identifier '%s'",
    "missing ')' for identifier '%s'",
    "ID number used twice for identifier '%s'",
    "expected MinLen (character '/') for identifier '%s'",
    "expected MaxLen (character '/') for identifier '%s'",
    "extra characters at the end of the line",
    "extra characters at the end of the line for identifier '%s'",
    "non-ASCII character 0x%02x found in original string for identifier '%s'",
    "no catalog revision information found, using revision 0"
};
