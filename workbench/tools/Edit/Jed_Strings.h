#ifndef JED_STRINGS_H
#define JED_STRINGS_H


/****************************************************************************/


/* This file was created automatically by CatComp.
 * Do NOT edit by hand!
 */


#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifdef CATCOMP_ARRAY
#undef CATCOMP_NUMBERS
#undef CATCOMP_STRINGS
#define CATCOMP_NUMBERS
#define CATCOMP_STRINGS
#endif

#ifdef CATCOMP_BLOCK
#undef CATCOMP_STRINGS
#define CATCOMP_STRINGS
#endif


/****************************************************************************/


#ifdef CATCOMP_NUMBERS

#define MSG_PROJECTTITLE 0
#define MSG_NEWEMPTYFILE 1
#define MSG_OPENNEWFILE 2
#define MSG_LOADINPRJ 3
#define MSG_SAVEFILE 4
#define MSG_SAVEFILEAS 5
#define MSG_SAVECHANGES 6
#define MSG_PRINTFILE 7
#define MSG_INFORMATION 8
#define MSG_CLOSE 9
#define MSG_QUIT 10
#define MSG_EDITTITLE 11
#define MSG_CUT 12
#define MSG_COPY 13
#define MSG_PASTE 14
#define MSG_MARK 15
#define MSG_MARKCOLUMN 16
#define MSG_SELECTALL 17
#define MSG_DELLINE 18
#define MSG_SUBTOOLS 19
#define MSG_INDENT 20
#define MSG_UNINDENT 21
#define MSG_UPPERCASE 22
#define MSG_LOWERCASE 23
#define MSG_TOGGLECASE 24
#define MSG_INSERTFILE 25
#define MSG_UNDO 26
#define MSG_REDO 27
#define MSG_SEARCHTITLE 28
#define MSG_FIND 29
#define MSG_REPLACE 30
#define MSG_SUBFIND 31
#define MSG_NEXTFIND 32
#define MSG_PREVFIND 33
#define MSG_NEXTREPLACE 34
#define MSG_PREVPAGE 35
#define MSG_NEXTPAGE 36
#define MSG_GOTOLINE 37
#define MSG_FINDBRACKET 38
#define MSG_LASTMODIF 39
#define MSG_BEGOFLINE 40
#define MSG_ENDOFLINE 41
#define MSG_MACROTITLE 42
#define MSG_STARTRECORD 43
#define MSG_STOPRECORD 44
#define MSG_PLAYMACRO 45
#define MSG_REPEATMACRO 46
#define MSG_ENVTITLE 47
#define MSG_SCRMODE 48
#define MSG_FONTS 49
#define MSG_PREFS 50
#define MSG_LOADPREFS 51
#define MSG_SAVEPREFS 52
#define MSG_SAVEPREFSAS 53
#define ERR_BADOS 54
#define ERR_NOASLREQ 55
#define ERR_NOWRTACCESS 56
#define ERR_NOMEM 57
#define ERR_NOGUI 58
#define ERR_NONAME 59
#define ERR_WRITECLIP 60
#define ERR_OPENCLIP 61
#define ERR_LOADFILE 62
#define ERR_NOTXTINCLIP 63
#define ERR_WRONG_TYPE 64
#define ERR_READCLIP 65
#define ERR_NOBRACKET 66
#define ERR_NOT_FOUND 67
#define ERR_LOADFONT 68
#define ERR_NOPREFEDITOR 69
#define ERR_BADPREFSFILE 70
#define ERR_FILEMODIFIED 71
#define ERR_SLC 72
#define ERR_NOSEARCHTEXT 73
#define ERR_FILEEXISTS 74
#define ERR_OC 75
#define WARN_RECORD 76
#define WARN_RECORDED 77
#define WARN_REC 78
#define MSG_ABOUT 79
#define MSG_FORMATINFO 80
#define MSG_CONTINUE 81
#define MSG_BYTE 82
#define MSG_BYTES 83
#define MSG_LINE 84
#define MSG_LINES 85
#define MSG_SEARCHWINDOW 86
#define MSG_REPLACEWINDOW 87
#define MSG_SEARCHSTRING 88
#define MSG_REPLACESTRING 89
#define MSG_OPTCASE 90
#define MSG_OPTWORDS 91
#define MSG_BUTTONREPLACE 92
#define MSG_BUTTONREPALL 93
#define MSG_BUTTONSEARCH 94
#define MSG_NEXTSEARCH 95
#define MSG_USESEARCH 96
#define MSG_CANCELSEARCH 97
#define MSG_REPLACEALL 98
#define MSG_OCCURENCY 99
#define MSG_OCCURENCIES 100

#endif /* CATCOMP_NUMBERS */


/****************************************************************************/


#ifdef CATCOMP_STRINGS

#define MSG_PROJECTTITLE_STR "Project"
#define MSG_NEWEMPTYFILE_STR "New"
#define MSG_OPENNEWFILE_STR "Open file..."
#define MSG_LOADINPRJ_STR "Load file..."
#define MSG_SAVEFILE_STR "Save"
#define MSG_SAVEFILEAS_STR "Save as..."
#define MSG_SAVECHANGES_STR "Save changes"
#define MSG_PRINTFILE_STR "Print"
#define MSG_INFORMATION_STR "Information..."
#define MSG_CLOSE_STR "Close"
#define MSG_QUIT_STR "Quit"
#define MSG_EDITTITLE_STR "Edit"
#define MSG_CUT_STR "Cut"
#define MSG_COPY_STR "Copy"
#define MSG_PASTE_STR "Paste"
#define MSG_MARK_STR "Mark"
#define MSG_MARKCOLUMN_STR "Mark columnar"
#define MSG_SELECTALL_STR "Select all"
#define MSG_DELLINE_STR "Del line"
#define MSG_SUBTOOLS_STR "Tools"
#define MSG_INDENT_STR "Indent"
#define MSG_UNINDENT_STR "Unindent"
#define MSG_UPPERCASE_STR "Upper case"
#define MSG_LOWERCASE_STR "Lower case"
#define MSG_TOGGLECASE_STR "Toggle case"
#define MSG_INSERTFILE_STR "Insert file..."
#define MSG_UNDO_STR "Undo"
#define MSG_REDO_STR "Redo"
#define MSG_SEARCHTITLE_STR "Search"
#define MSG_FIND_STR "Find..."
#define MSG_REPLACE_STR "Replace..."
#define MSG_SUBFIND_STR "Find"
#define MSG_NEXTFIND_STR "Next"
#define MSG_PREVFIND_STR "Previous"
#define MSG_NEXTREPLACE_STR "Replace"
#define MSG_PREVPAGE_STR "Prev page"
#define MSG_NEXTPAGE_STR "Next page"
#define MSG_GOTOLINE_STR "Goto line..."
#define MSG_FINDBRACKET_STR "Matching bracket"
#define MSG_LASTMODIF_STR "Last modification"
#define MSG_BEGOFLINE_STR "Start of line"
#define MSG_ENDOFLINE_STR "End of line"
#define MSG_MACROTITLE_STR "Macro"
#define MSG_STARTRECORD_STR "Start recording"
#define MSG_STOPRECORD_STR "Stop recording"
#define MSG_PLAYMACRO_STR "Execute macro"
#define MSG_REPEATMACRO_STR "Repeat macro..."
#define MSG_ENVTITLE_STR "Environment"
#define MSG_SCRMODE_STR "Screen mode..."
#define MSG_FONTS_STR "Fonts..."
#define MSG_PREFS_STR "General prefs..."
#define MSG_LOADPREFS_STR "Load prefs..."
#define MSG_SAVEPREFS_STR "Save prefs"
#define MSG_SAVEPREFSAS_STR "Save prefs as..."
#define ERR_BADOS_STR "Obsolete OS. Install v2.0 or better."
#define ERR_NOASLREQ_STR "Cannot alloc ASL requester!"
#define ERR_NOWRTACCESS_STR "Disk is protected on writing"
#define ERR_NOMEM_STR "Out of memory!"
#define ERR_NOGUI_STR "Main interface failed to open!"
#define ERR_NONAME_STR "No title"
#define ERR_WRITECLIP_STR "Write error into clipboard device"
#define ERR_OPENCLIP_STR "Unable to open clipboard!"
#define ERR_LOADFILE_STR "Unable to load requested file"
#define ERR_NOTXTINCLIP_STR "No text in clipboard"
#define ERR_WRONG_TYPE_STR "Object is a directory!"
#define ERR_READCLIP_STR "Read operation incomplete due to probably a lack of memory"
#define ERR_NOBRACKET_STR "No parenthesis under cursor!"
#define ERR_NOT_FOUND_STR "Pattern `%s' not found"
#define ERR_LOADFONT_STR "Can't open desired font!"
#define ERR_NOPREFEDITOR_STR "Can't find preference editor!"
#define ERR_BADPREFSFILE_STR "Bad preference file format."
#define ERR_FILEMODIFIED_STR "%s has been modified.\nAre you sure want to close it?"
#define ERR_SLC_STR "Save|Close|Cancel"
#define ERR_NOSEARCHTEXT_STR "No text to search for!"
#define ERR_FILEEXISTS_STR "This file already exists. Overwrite it?"
#define ERR_OC_STR "Yes|No!"
#define WARN_RECORD_STR "Start reccording macro"
#define WARN_RECORDED_STR "Macro recorded"
#define WARN_REC_STR "[REC]"
#define MSG_ABOUT_STR "Information"
#define MSG_FORMATINFO_STR "%s for %s\nDesigned by T.Pierron and C.Guillaume © 2000\n\nFile edited : %s\nLength : %ld %s, %ld %s."
#define MSG_CONTINUE_STR "Continue"
#define MSG_BYTE_STR "byte"
#define MSG_BYTES_STR "bytes"
#define MSG_LINE_STR "line"
#define MSG_LINES_STR "lines"
#define MSG_SEARCHWINDOW_STR "Search pattern"
#define MSG_REPLACEWINDOW_STR "Search & Replace"
#define MSG_SEARCHSTRING_STR "Locate:"
#define MSG_REPLACESTRING_STR "Replace:"
#define MSG_OPTCASE_STR "Match case"
#define MSG_OPTWORDS_STR "Whole words"
#define MSG_BUTTONREPLACE_STR "_Replace"
#define MSG_BUTTONREPALL_STR "Replace _All"
#define MSG_BUTTONSEARCH_STR "Search"
#define MSG_NEXTSEARCH_STR "_Previous"
#define MSG_USESEARCH_STR "_Use"
#define MSG_CANCELSEARCH_STR "_Cancel"
#define MSG_REPLACEALL_STR "%d %s of `%s' replaced"
#define MSG_OCCURENCY_STR "occurency"
#define MSG_OCCURENCIES_STR "occurencies"

#endif /* CATCOMP_STRINGS */


/****************************************************************************/


struct LocaleInfo
{
    APTR li_LocaleBase;
    APTR li_Catalog;
};



#endif /* JED_STRINGS_H */
