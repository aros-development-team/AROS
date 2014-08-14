
/****************************************************************

   This file was created automatically by `FlexCat 2.15'
   from "locale/FlexCat.pot".

   Do NOT edit by hand!

****************************************************************/

#ifndef FlexCat_CAT_H
#define FlexCat_CAT_H


#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

void LocalizeStringArray(STRPTR *Array);
void OpenFlexCatCatalog();
void CloseFlexCatCatalog();

struct FC_String
{
   const char *Str;
   const LONG Id;
};

extern struct FC_String FlexCat_Strings[];

#define MSG_USAGE_HEAD (FlexCat_Strings[0].Str)
#define _MSG_USAGE_HEAD 0
#define MSG_USAGE (FlexCat_Strings[1].Str)
#define _MSG_USAGE 1
#define MSG_FILEUPTODATE (FlexCat_Strings[2].Str)
#define _MSG_FILEUPTODATE 2
#define MSG_ERR_WARNING (FlexCat_Strings[3].Str)
#define _MSG_ERR_WARNING 3
#define MSG_ERR_ERROR (FlexCat_Strings[4].Str)
#define _MSG_ERR_ERROR 4
#define MSG_ERR_EXPECTEDHEX (FlexCat_Strings[5].Str)
#define _MSG_ERR_EXPECTEDHEX 5
#define MSG_ERR_EXPECTEDOCTAL (FlexCat_Strings[6].Str)
#define _MSG_ERR_EXPECTEDOCTAL 6
#define MSG_ERR_NOLENGTHBYTES (FlexCat_Strings[7].Str)
#define _MSG_ERR_NOLENGTHBYTES 7
#define MSG_ERR_UNKNOWNCDCOMMAND (FlexCat_Strings[8].Str)
#define _MSG_ERR_UNKNOWNCDCOMMAND 8
#define MSG_ERR_UNEXPECTEDBLANKS (FlexCat_Strings[9].Str)
#define _MSG_ERR_UNEXPECTEDBLANKS 9
#define MSG_ERR_NOIDENTIFIER (FlexCat_Strings[10].Str)
#define _MSG_ERR_NOIDENTIFIER 10
#define MSG_ERR_MISSINGSTRING (FlexCat_Strings[11].Str)
#define _MSG_ERR_MISSINGSTRING 11
#define MSG_ERR_UNKNOWNCTCOMMAND (FlexCat_Strings[12].Str)
#define _MSG_ERR_UNKNOWNCTCOMMAND 12
#define MSG_ERR_UNKNOWNIDENTIFIER (FlexCat_Strings[13].Str)
#define _MSG_ERR_UNKNOWNIDENTIFIER 13
#define MSG_ERR_UNKNOWNSTRINGTYPE (FlexCat_Strings[14].Str)
#define _MSG_ERR_UNKNOWNSTRINGTYPE 14
#define MSG_ERR_NOTERMINATEBRACKET (FlexCat_Strings[15].Str)
#define _MSG_ERR_NOTERMINATEBRACKET 15
#define MSG_ERR_NOBINCHARS (FlexCat_Strings[16].Str)
#define _MSG_ERR_NOBINCHARS 16
#define MSG_ERR_CTGAP (FlexCat_Strings[17].Str)
#define _MSG_ERR_CTGAP 17
#define MSG_ERR_DOUBLECTLANGUAGE (FlexCat_Strings[18].Str)
#define _MSG_ERR_DOUBLECTLANGUAGE 18
#define MSG_ERR_DOUBLECTVERSION (FlexCat_Strings[19].Str)
#define _MSG_ERR_DOUBLECTVERSION 19
#define MSG_ERR_WRONGRCSID (FlexCat_Strings[20].Str)
#define _MSG_ERR_WRONGRCSID 20
#define MSG_ERR_NOMEMORY (FlexCat_Strings[21].Str)
#define _MSG_ERR_NOMEMORY 21
#define MSG_ERR_NOCATALOGDESCRIPTION (FlexCat_Strings[22].Str)
#define _MSG_ERR_NOCATALOGDESCRIPTION 22
#define MSG_ERR_NOCATALOGTRANSLATION (FlexCat_Strings[23].Str)
#define _MSG_ERR_NOCATALOGTRANSLATION 23
#define MSG_ERR_NOCTVERSION (FlexCat_Strings[24].Str)
#define _MSG_ERR_NOCTVERSION 24
#define MSG_ERR_NOCATALOG (FlexCat_Strings[25].Str)
#define _MSG_ERR_NOCATALOG 25
#define MSG_ERR_NONEWCTFILE (FlexCat_Strings[26].Str)
#define _MSG_ERR_NONEWCTFILE 26
#define MSG_ERR_NOCTLANGUAGE (FlexCat_Strings[27].Str)
#define _MSG_ERR_NOCTLANGUAGE 27
#define MSG_ERR_NOSOURCE (FlexCat_Strings[28].Str)
#define _MSG_ERR_NOSOURCE 28
#define MSG_ERR_NOSOURCEDESCRIPTION (FlexCat_Strings[29].Str)
#define _MSG_ERR_NOSOURCEDESCRIPTION 29
#define MSG_ERR_NOCTARGUMENT (FlexCat_Strings[30].Str)
#define _MSG_ERR_NOCTARGUMENT 30
#define MSG_ERR_CANTCHECKDATE (FlexCat_Strings[31].Str)
#define _MSG_ERR_CANTCHECKDATE 31
#define MSG_ERR_NOCTFILENAME (FlexCat_Strings[32].Str)
#define _MSG_ERR_NOCTFILENAME 32
#define MSG_ERR_NOCATFILENAME (FlexCat_Strings[33].Str)
#define _MSG_ERR_NOCATFILENAME 33
#define MSG_ERR_BADPREFS (FlexCat_Strings[34].Str)
#define _MSG_ERR_BADPREFS 34
#define MSG_ERR_BADCTLANGUAGE (FlexCat_Strings[35].Str)
#define _MSG_ERR_BADCTLANGUAGE 35
#define MSG_ERR_DOUBLECTCODESET (FlexCat_Strings[36].Str)
#define _MSG_ERR_DOUBLECTCODESET 36
#define MSG_ERR_BADCTCODESET (FlexCat_Strings[37].Str)
#define _MSG_ERR_BADCTCODESET 37
#define MSG_ERR_NOCTCODESET (FlexCat_Strings[38].Str)
#define _MSG_ERR_NOCTCODESET 38
#define MSG_ERR_ERROR_QUICK (FlexCat_Strings[39].Str)
#define _MSG_ERR_ERROR_QUICK 39
#define MSG_ERR_BADCTVERSION (FlexCat_Strings[40].Str)
#define _MSG_ERR_BADCTVERSION 40
#define MSG_ERR_WARNING_QUICK (FlexCat_Strings[41].Str)
#define _MSG_ERR_WARNING_QUICK 41
#define MSG_ERR_MISSINGTRANSLATION (FlexCat_Strings[42].Str)
#define _MSG_ERR_MISSINGTRANSLATION 42
#define MSG_ERR_EMPTYTRANSLATION (FlexCat_Strings[43].Str)
#define _MSG_ERR_EMPTYTRANSLATION 43
#define MSG_ERR_MISMATCHINGCONTROLCHARACTERS (FlexCat_Strings[44].Str)
#define _MSG_ERR_MISMATCHINGCONTROLCHARACTERS 44
#define MSG_ERR_DOUBLE_IDENTIFIER (FlexCat_Strings[45].Str)
#define _MSG_ERR_DOUBLE_IDENTIFIER 45
#define MSG_ERR_STRING_TOO_SHORT (FlexCat_Strings[46].Str)
#define _MSG_ERR_STRING_TOO_SHORT 46
#define MSG_ERR_STRING_TOO_LONG (FlexCat_Strings[47].Str)
#define _MSG_ERR_STRING_TOO_LONG 47
#define MSG_ERR_TRAILING_ELLIPSIS (FlexCat_Strings[48].Str)
#define _MSG_ERR_TRAILING_ELLIPSIS 48
#define MSG_ERR_NO_TRAILING_ELLIPSIS (FlexCat_Strings[49].Str)
#define _MSG_ERR_NO_TRAILING_ELLIPSIS 49
#define MSG_ERR_TRAILING_BLANKS (FlexCat_Strings[50].Str)
#define _MSG_ERR_TRAILING_BLANKS 50
#define MSG_ERR_NO_TRAILING_BLANKS (FlexCat_Strings[51].Str)
#define _MSG_ERR_NO_TRAILING_BLANKS 51
#define MSG_ERR_MISMATCHING_PLACEHOLDERS (FlexCat_Strings[52].Str)
#define _MSG_ERR_MISMATCHING_PLACEHOLDERS 52
#define MSG_ERR_MISSING_PLACEHOLDERS (FlexCat_Strings[53].Str)
#define _MSG_ERR_MISSING_PLACEHOLDERS 53
#define MSG_ERR_EXCESSIVE_PLACEHOLDERS (FlexCat_Strings[54].Str)
#define _MSG_ERR_EXCESSIVE_PLACEHOLDERS 54
#define MSG_ERR_NO_LEADING_BRACKET (FlexCat_Strings[55].Str)
#define _MSG_ERR_NO_LEADING_BRACKET 55
#define MSG_ERR_NO_TRAILING_BRACKET (FlexCat_Strings[56].Str)
#define _MSG_ERR_NO_TRAILING_BRACKET 56
#define MSG_ERR_DOUBLE_ID (FlexCat_Strings[57].Str)
#define _MSG_ERR_DOUBLE_ID 57
#define MSG_ERR_NO_MIN_LEN (FlexCat_Strings[58].Str)
#define _MSG_ERR_NO_MIN_LEN 58
#define MSG_ERR_NO_MAX_LEN (FlexCat_Strings[59].Str)
#define _MSG_ERR_NO_MAX_LEN 59
#define MSG_ERR_EXTRA_CHARACTERS (FlexCat_Strings[60].Str)
#define _MSG_ERR_EXTRA_CHARACTERS 60
#define MSG_ERR_EXTRA_CHARACTERS_ID (FlexCat_Strings[61].Str)
#define _MSG_ERR_EXTRA_CHARACTERS_ID 61
#define MSG_ERR_NON_ASCII_CHARACTER (FlexCat_Strings[62].Str)
#define _MSG_ERR_NON_ASCII_CHARACTER 62
#define MSG_ERR_NO_CAT_REVISION (FlexCat_Strings[63].Str)
#define _MSG_ERR_NO_CAT_REVISION 63

#endif
