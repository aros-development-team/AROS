/*** Constants identifying the prog: ***/
#define	APPNAME		"JanoEditor"
#define	DATE			"nov 29, 2001"
#define	VERSION		"v1.01"

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

#define	TARGET		"CBM-AmigaDOS-" ARCH
#define	SVERID		APPNAME " " VERSION " http://perso.wanadoo.fr/cyrille.guillaume/"
#define	SVER			"$VER: " APPNAME " " VERSION " on " DATE " for " TARGET
