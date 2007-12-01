/*
**  openurl.library - universal URL display and browser
**  launcher library
**
**  Written by Troels Walsted Hansen <troels@thule.no>
**  Placed in the public domain.
**
**  Developed by:
**  - Alfonso Ranieri <alforan@tin.it>
**  - Stefan Kost <ensonic@sonicpulse.de>
**
**  Ported to OS4 by Alexandre Balaban <alexandre@balaban.name>
*/


/* init.c */
void freeBase ( void );
ULONG initBase ( void );

/* api.c */
ULONG LIBCALL URL_OpenA ( REG (a0 ,UBYTE *URL ), REG (a1 ,struct TagItem *attrs ));
struct URL_Prefs *LIBCALL URL_GetPrefsA ( REG (a0 ,struct TagItem *attrs ));
struct URL_Prefs *LIBCALL URL_OldGetPrefs ( void );
void LIBCALL URL_FreePrefsA ( REG (a0 ,struct URL_Prefs *p ), REG (a1 ,struct TagItem *attrs ));
void LIBCALL URL_OldFreePrefs ( REG (a0 ,struct URL_Prefs *p ));
ULONG LIBCALL URL_SetPrefsA ( REG (a0 ,struct URL_Prefs *p ), REG (a1 ,struct TagItem *attrs ));
ULONG LIBCALL URL_OldSetPrefs ( REG (a0 ,struct URL_Prefs *p ), REG (d0 ,ULONG save ));
struct URL_Prefs *LIBCALL URL_OldGetDefaultPrefs ( void );
ULONG LIBCALL URL_LaunchPrefsAppA ( REG (a0 ,struct TagItem *attrs ));
ULONG LIBCALL URL_OldLaunchPrefsApp ( void );
ULONG LIBCALL URL_GetAttr ( REG (d0 ,ULONG attr ), REG (a0 ,ULONG *storage ));
#ifdef __MORPHOS__
LONG dispatch ( void );
#else
LONG LIBCALL dispatch ( REG (a0 , struct RexxMsg *msg ) , REG (a1 , UBYTE **resPtr ));
#endif

#ifdef __MORPHOS__
/* morphos.c */
ULONG LIB_URL_OpenA ( void );
struct URL_Prefs *LIB_URL_GetPrefsA ( void );
struct URL_Prefs *LIB_URL_OldGetPrefs ( void );
void LIB_URL_FreePrefsA ( void );
void LIB_URL_OldFreePrefs ( void );
ULONG LIB_URL_SetPrefsA ( void );
ULONG LIB_URL_OldSetPrefs ( void );
struct URL_Prefs *LIB_URL_OldGetDefaultPrefs ( void );
ULONG LIB_URL_LaunchPrefsAppA ( void );
ULONG LIB_URL_OldLaunchPrefsApp ( void );
ULONG LIB_URL_GetAttr ( void );
#elif defined(__amigaos4__)
#include <interfaces/openurl.h>
ULONG              VARARGS68K OS4_URL_OpenA ( struct OpenURLIFace * Self, STRPTR url, struct TagItem *attrs );
ULONG              VARARGS68K OS4_URL_Open ( struct OpenURLIFace * Self, STRPTR url, ... );
struct URL_Prefs * VARARGS68K OS4_URL_GetPrefsA ( struct OpenURLIFace * Self, struct TagItem *attrs );
struct URL_Prefs * VARARGS68K OS4_URL_GetPrefs ( struct OpenURLIFace * Self, ... );
struct URL_Prefs * VARARGS68K OS4_URL_OldGetPrefs ( struct OpenURLIFace * Self );
void               VARARGS68K OS4_URL_FreePrefsA ( struct OpenURLIFace * Self, struct URL_Prefs *up, struct TagItem *attrs );
void               VARARGS68K OS4_URL_FreePrefs ( struct OpenURLIFace * Self, struct URL_Prefs *up, ... );
void               VARARGS68K OS4_URL_OldFreePrefs ( struct OpenURLIFace * Self, struct URL_Prefs *up );
ULONG              VARARGS68K OS4_URL_SetPrefsA ( struct OpenURLIFace * Self, struct URL_Prefs *p, struct TagItem *attrs );
ULONG              VARARGS68K OS4_URL_SetPrefs ( struct OpenURLIFace * Self, struct URL_Prefs *p, ... );
ULONG              VARARGS68K OS4_URL_OldSetPrefs ( struct OpenURLIFace * Self, struct URL_Prefs *p , ULONG permanent );
struct URL_Prefs * VARARGS68K OS4_URL_OldGetDefaultPrefs ( struct OpenURLIFace * Self );
ULONG              VARARGS68K OS4_URL_LaunchPrefsAppA ( struct OpenURLIFace * Self, struct TagItem *attrs );
ULONG              VARARGS68K OS4_URL_LaunchPrefsApp ( struct OpenURLIFace * Self, ... );
ULONG              VARARGS68K OS4_URL_OldLaunchPrefsApp ( struct OpenURLIFace * Self );
ULONG              VARARGS68K OS4_URL_GetAttr ( struct OpenURLIFace * Self, ULONG attr , ULONG *storage );
LONG               VARARGS68K OS4_dispatch ( struct OpenURLIFace * Self, struct RexxMsg *msg, UBYTE **resPtr );

#endif

/* handler.c */
#ifdef __MORPHOS__
void handler ( void );
#else
void SAVEDS handler ( void );
#endif

/* prefs.c */
struct URL_Prefs *copyPrefs ( struct URL_Prefs *old );
void initPrefs ( struct URL_Prefs *p );
void setDefaultPrefs ( struct URL_Prefs *up );
ULONG savePrefs ( UBYTE *filename , struct URL_Prefs *up );
ULONG loadPrefs ( struct URL_Prefs *p , ULONG mode );
struct URL_Prefs *loadPrefsNotFail ( void );

/* utils.c */
ULONG sendToBrowser ( UBYTE *URL , struct List *portlist , ULONG show , ULONG toFront , ULONG newWindow , ULONG launch , UBYTE *pubScreenName );
ULONG sendToFTP ( UBYTE *URL , struct List *portlist , ULONG show , ULONG toFront , ULONG newWindow , ULONG launch , UBYTE *pubScreenName );
ULONG sendToMailer ( UBYTE *URL , struct List *portlist , ULONG show , ULONG toFront , ULONG launch , UBYTE *pubScreenName );
ULONG copyList ( struct List *dst , struct List *src , ULONG size );
void freeList ( struct List *list , ULONG size );
ULONG isdigits ( UBYTE *str );
APTR allocPooled ( ULONG size );
void freePooled ( APTR mem , ULONG size );
APTR allocVecPooled ( ULONG size );
void freeVecPooled ( APTR mem );
#ifdef __MORPHOS__
#define msprintf(to, fmt, ...) ({ ULONG _tags[] = { __VA_ARGS__ }; RawDoFmt(fmt, _tags, (void (*)(void)) 0, to); })
#elif defined(__amigaos4__)
void VARARGS68K msprintf ( UBYTE *to , UBYTE *fmt , ...);
#else
void STDARGS msprintf ( UBYTE *to , UBYTE *fmt , ...);
#endif

