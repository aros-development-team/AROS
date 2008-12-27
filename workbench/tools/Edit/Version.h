/*** Constants identifying the prog: ***/
#define	APPNAME		"JanoEditor"
#define	DATE			"29.11.2001"
#define	VERSION		"1.1"

/*** Processor target type ***/
#if defined(__GNUC__) || defined(__SASC__)
	#if   defined (mc68060)
	#define ARCH  "mc68060"
	#elif defined (mc68040)
	#define ARCH  "mc68040"
	#elif defined (mc68030)
	#define ARCH  "mc68030"
	#elif defined (mc68020)
	#define ARCH  "mc68020"
	#else
	#define ARCH  "mc68000"
	#endif
#else

	/** INSERT OTHER COMPILER **/
	#define ARCH "mc68000"

#endif

#ifdef __AROS__
#define TARGET "AROS"
#else
#define	TARGET		"CBM-AmigaDOS-" ARCH
#endif

#define	SVERID		APPNAME " " VERSION
#define	SVER			"$VER: " APPNAME " " VERSION " (" DATE ")"
