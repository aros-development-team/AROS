#ifndef VERSION_H
#define VERSION_H 1

/*************************************************************************/

#define VERSION  1
#define REVISION 23
#define DAY      02
#define MONTH    04
#define YEAR     2011

#define APPLICATIONNAME "GiggleDisk"
#define AUTHORNAME "Guido Mersmann"                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    


/*
** macros to create version string
*/

#ifdef __MORPHOS__
#define VERSION_EXT "- MorphOS PPC"
#elif __amigaos4__
#define VERSION_EXT "- AmigaOS PPC"
#elif __AROS__
#define VERSION_EXT "- AROS X86"
#elif AMITHLON
#define VERSION_EXT "- Amithlon 68K"
#elif AMITHLONXL
#define VERSION_EXT "- AmithlonXL 68K"
#elif UAE
#define VERSION_EXT "- UAE 68K"
#else
#define VERSION_EXT "- Amiga 68K"
#endif

#ifdef __GNUC__
#define __entry
#endif

#define I2S(x) I2S2(x)
#define I2S2(x) #x

#ifndef VERSION_EXT
#define VERSION_EXT ""
#endif

#ifndef VERSION_LABEL
#define VERSION_LABEL app_versionstring
#endif

/*
** externals
*/

extern char          app_appname[];


/*************************************************************************/

#endif /* VERSION_H */

