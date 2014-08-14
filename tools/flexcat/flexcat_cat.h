/****************************************************************
   This file was created automatically by `FlexCat 2.4'
   from "Catalogs_Src/FlexCat.cd".

   Do NOT edit by hand!
****************************************************************/

/****************************************************************

   This file is a quick-hack-solution for all of you wanting
   to compile FlexCat on non-amiga platform. It creates
   necessary string tables (hardcoded ATM) and functions to
   make code work under any operating system.

   Carlos

****************************************************************/

#ifndef FlexCat_CAT_H
#define FlexCat_CAT_H

void OpenFlexCatCatalog( void );
void CloseFlexCatCatalog( void );

char * FlexCat_Strings[47] = {
    (char *)"Out of memory!",
    (char *)"%s, Line %d; warning: ",
    (char *)"Expected hex character (one of [0-9a-fA-F]).",
    (char *)"Expected octal character (one of [0-7]).",
    (char *)"Cannot open catalog description %s.",
    (char *)"LengthBytes > %d (sizeof long) not possible.",
    (char *)"Unknown catalog description command",
    (char *)"Unexpected blanks.",
    (char *)"Missing identifier.",
    (char *)"Missing '('.",
    (char *)"ID number used twice.",
    (char *)"Identifier redeclared.",
    (char *)"Expected MinLen (character '/').",
    (char *)"Expected MaxLen (character '/').",
    (char *)"Expected ')'.",
    (char *)"Extra characters at the end of the line.",
    (char *)"Unexpected end of file (missing catalog string).",
    (char *)"String too short.",
    (char *)"String too long.",
    (char *)"Cannot open catalog translation file %s.",
    (char *)"Missing catalog translation command. (Expected second '#'.)",
    (char *)"Unknown catalog translation command.",
    (char *)"Missing catalog translation version; use either ##version\n"\
	"or ##rcsid and ##name.",
    (char *)"Missing catalog translation language.",
    (char *)"Cannot open catalog file %s.",
    (char *)"Cannot create catalog translation file %s.",
    (char *)"%s missing in catalog description.",
    (char *)"Cannot open source description file %s.",
    (char *)"Cannot open source file %s.",
    (char *)"Unknown string type.",
    (char *)"Unexpected end of line. (Missing ')')",
    (char *)"  CDFILE        Catalog description file to scan\n"\
	"  CTFILE        Catalog translation file to scan\n"\
	"  CATALOG       Catalog file to create\n"\
	"  NEWCTFILE     Catalog translation file to create\n"\
	"  SOURCES       Sources to create; must be something like sfile=sdfile,\n"\
	"                where sfile is a sourcefile and sdfile is a source\n"\
	"                description file\n"\
	"  WARNCTGAPS    Warn symbols missing in CT file\n"\
	"  NOOPTIM       Do not skip unchanged strings (equal in both #?.cd and #?.ct)\n"\
	"  FILL          Use descriptor texts if translation are missing\n"\
	"  FLUSH         Flush memory when catalog is written\n"\
	"  NOBEEP        Suppress DisplayBeep()'s on error and warnings\n"\
	"  QUIET         Suppress warning messages",
    (char *)"Creating a catalog needs a catalog translation file as argument.\n",
    (char *)"Binary characters in stringtype None.",
    (char *)"ID %s missing in CT file.",
    (char *)"Catalog language declared twice.",
    (char *)"Catalog version declared twice.",
    (char *)"Incorrect rcs ID (must be similar to\n"\
	"'$Date$ $Revision$')",
    (char *)"Usage",
    (char *)"Error processing FlexCat.prefs variable, falling back to defaults.\n"\
	"Preferences template: ",
    (char *)"  NOLANGTOLOWER Prevents #language name from being lowercased\n"\
	"  NOBUFFEREDIO  Disables IO buffers\n"\
	"  MODIFIED      Creates the catalog only when #?.c(d|t) files were changed",
    (char *)"File %s is up to date",
    (char *)"Cannot get the datestamp of %s",
    (char *)"  COPYMSGNEW    Turns on copying ***NEW*** markers while updating #?.ct file\n"\
	"  OLDMSGNEW     If old #?.ct file is using other marker, specify it here",
    (char *)"Original string has a trailing ellipsis (...)",
    (char *)"Original string has a trailing spaces",
    (char *)"  NOSPACE       Removes the space being usually put between ';' and the comment"
};

#define msgMemoryError FlexCat_Strings[0]
#define msgWarning FlexCat_Strings[1]
#define msgExpectedHex FlexCat_Strings[2]
#define msgExpectedOctal FlexCat_Strings[3]
#define msgNoCatalogDescription FlexCat_Strings[4]
#define msgNoLengthBytes FlexCat_Strings[5]
#define msgUnknownCDCommand FlexCat_Strings[6]
#define msgUnexpectedBlanks FlexCat_Strings[7]
#define msgNoIdentifier FlexCat_Strings[8]
#define msgNoLeadingBracket FlexCat_Strings[9]
#define msgDoubleID FlexCat_Strings[10]
#define msgDoubleIdentifier FlexCat_Strings[11]
#define msgNoMinLen FlexCat_Strings[12]
#define msgNoMaxLen FlexCat_Strings[13]
#define msgNoTrailingBracket FlexCat_Strings[14]
#define msgExtraCharacters FlexCat_Strings[15]
#define msgNoString FlexCat_Strings[16]
#define msgShortString FlexCat_Strings[17]
#define msgLongString FlexCat_Strings[18]
#define msgNoCatalogTranslation FlexCat_Strings[19]
#define msgNoCTCommand FlexCat_Strings[20]
#define msgUnknownCTCommand FlexCat_Strings[21]
#define msgNoCTVersion FlexCat_Strings[22]
#define msgNoCTLanguage FlexCat_Strings[23]
#define msgNoCatalog FlexCat_Strings[24]
#define msgNoNewCTFile FlexCat_Strings[25]
#define msgUnknownIdentifier FlexCat_Strings[26]
#define msgNoSourceDescription FlexCat_Strings[27]
#define msgNoSource FlexCat_Strings[28]
#define msgUnknownStringType FlexCat_Strings[29]
#define msgNoTerminateBracket FlexCat_Strings[30]
#define msgUsage FlexCat_Strings[31]
#define msgNoCTArgument FlexCat_Strings[32]
#define msgNoBinChars FlexCat_Strings[33]
#define msgCTGap FlexCat_Strings[34]
#define msgDoubleCTLanguage FlexCat_Strings[35]
#define msgDoubleCTVersion FlexCat_Strings[36]
#define msgWrongRcsId FlexCat_Strings[37]
#define msgUsageHead FlexCat_Strings[38]
#define msgPrefsError FlexCat_Strings[39]
#define msgUsage_2 FlexCat_Strings[40]
#define msgUpToDate FlexCat_Strings[41]
#define msgCantCheckDate FlexCat_Strings[42]
#define msgUsage_3 FlexCat_Strings[43]
#define msgTrailingEllipsis FlexCat_Strings[44]
#define msgTrailingSpaces FlexCat_Strings[45]
#define msgUsage_4 FlexCat_Strings[46]

#endif
