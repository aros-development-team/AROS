
/****************************************************************

   This file was created automatically by `FlexCat 2.15'
   from "locale/FlexCat.pot".

   Do NOT edit by hand!

****************************************************************/

/****************************************************************

   This file is a quick-hack-solution for all of you wanting
   to compile FlexCat on non-amiga platform. It creates the
   necessary string tables (hardcoded ATM) and functions to
   make code work under any operating system.

   Carlos

****************************************************************/

#ifndef FlexCat_CAT_H
#define FlexCat_CAT_H

extern char *FlexCat_Strings[];

void OpenFlexCatCatalog( void );
void CloseFlexCatCatalog( void );

#define MSG_USAGE_HEAD FlexCat_Strings[0]
#define MSG_USAGE FlexCat_Strings[1]
#define MSG_FILEUPTODATE FlexCat_Strings[2]
#define MSG_ERR_WARNING FlexCat_Strings[3]
#define MSG_ERR_ERROR FlexCat_Strings[4]
#define MSG_ERR_EXPECTEDHEX FlexCat_Strings[5]
#define MSG_ERR_EXPECTEDOCTAL FlexCat_Strings[6]
#define MSG_ERR_NOLENGTHBYTES FlexCat_Strings[7]
#define MSG_ERR_UNKNOWNCDCOMMAND FlexCat_Strings[8]
#define MSG_ERR_UNEXPECTEDBLANKS FlexCat_Strings[9]
#define MSG_ERR_NOIDENTIFIER FlexCat_Strings[10]
#define MSG_ERR_MISSINGSTRING FlexCat_Strings[11]
#define MSG_ERR_UNKNOWNCTCOMMAND FlexCat_Strings[12]
#define MSG_ERR_UNKNOWNIDENTIFIER FlexCat_Strings[13]
#define MSG_ERR_UNKNOWNSTRINGTYPE FlexCat_Strings[14]
#define MSG_ERR_NOTERMINATEBRACKET FlexCat_Strings[15]
#define MSG_ERR_NOBINCHARS FlexCat_Strings[16]
#define MSG_ERR_CTGAP FlexCat_Strings[17]
#define MSG_ERR_DOUBLECTLANGUAGE FlexCat_Strings[18]
#define MSG_ERR_DOUBLECTVERSION FlexCat_Strings[19]
#define MSG_ERR_WRONGRCSID FlexCat_Strings[20]
#define MSG_ERR_NOMEMORY FlexCat_Strings[21]
#define MSG_ERR_NOCATALOGDESCRIPTION FlexCat_Strings[22]
#define MSG_ERR_NOCATALOGTRANSLATION FlexCat_Strings[23]
#define MSG_ERR_NOCTVERSION FlexCat_Strings[24]
#define MSG_ERR_NOCATALOG FlexCat_Strings[25]
#define MSG_ERR_NONEWCTFILE FlexCat_Strings[26]
#define MSG_ERR_NOCTLANGUAGE FlexCat_Strings[27]
#define MSG_ERR_NOSOURCE FlexCat_Strings[28]
#define MSG_ERR_NOSOURCEDESCRIPTION FlexCat_Strings[29]
#define MSG_ERR_NOCTARGUMENT FlexCat_Strings[30]
#define MSG_ERR_CANTCHECKDATE FlexCat_Strings[31]
#define MSG_ERR_NOCTFILENAME FlexCat_Strings[32]
#define MSG_ERR_NOCATFILENAME FlexCat_Strings[33]
#define MSG_ERR_BADPREFS FlexCat_Strings[34]
#define MSG_ERR_BADCTLANGUAGE FlexCat_Strings[35]
#define MSG_ERR_DOUBLECTCODESET FlexCat_Strings[36]
#define MSG_ERR_BADCTCODESET FlexCat_Strings[37]
#define MSG_ERR_NOCTCODESET FlexCat_Strings[38]
#define MSG_ERR_ERROR_QUICK FlexCat_Strings[39]
#define MSG_ERR_BADCTVERSION FlexCat_Strings[40]
#define MSG_ERR_WARNING_QUICK FlexCat_Strings[41]
#define MSG_ERR_MISSINGTRANSLATION FlexCat_Strings[42]
#define MSG_ERR_EMPTYTRANSLATION FlexCat_Strings[43]
#define MSG_ERR_MISMATCHINGCONTROLCHARACTERS FlexCat_Strings[44]
#define MSG_ERR_DOUBLE_IDENTIFIER FlexCat_Strings[45]
#define MSG_ERR_STRING_TOO_SHORT FlexCat_Strings[46]
#define MSG_ERR_STRING_TOO_LONG FlexCat_Strings[47]
#define MSG_ERR_TRAILING_ELLIPSIS FlexCat_Strings[48]
#define MSG_ERR_NO_TRAILING_ELLIPSIS FlexCat_Strings[49]
#define MSG_ERR_TRAILING_BLANKS FlexCat_Strings[50]
#define MSG_ERR_NO_TRAILING_BLANKS FlexCat_Strings[51]
#define MSG_ERR_MISMATCHING_PLACEHOLDERS FlexCat_Strings[52]
#define MSG_ERR_MISSING_PLACEHOLDERS FlexCat_Strings[53]
#define MSG_ERR_EXCESSIVE_PLACEHOLDERS FlexCat_Strings[54]
#define MSG_ERR_NO_LEADING_BRACKET FlexCat_Strings[55]
#define MSG_ERR_NO_TRAILING_BRACKET FlexCat_Strings[56]
#define MSG_ERR_DOUBLE_ID FlexCat_Strings[57]
#define MSG_ERR_NO_MIN_LEN FlexCat_Strings[58]
#define MSG_ERR_NO_MAX_LEN FlexCat_Strings[59]
#define MSG_ERR_EXTRA_CHARACTERS FlexCat_Strings[60]
#define MSG_ERR_EXTRA_CHARACTERS_ID FlexCat_Strings[61]
#define MSG_ERR_NON_ASCII_CHARACTER FlexCat_Strings[62]
#define MSG_ERR_NO_CAT_REVISION FlexCat_Strings[63]

#endif
