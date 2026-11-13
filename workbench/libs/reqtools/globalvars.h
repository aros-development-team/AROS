#ifdef __RT_DEFINEVARS
#define extern
#endif
extern struct ReqToolsBase  	*ReqToolsBase;
extern struct Device 		*ConsoleDevice;
extern struct IOStdReq		iorequest;
extern Class			*ButtonImgClass;

/* On AROS don't define the libbases because that will prevent autoinit */
#if !defined(__RT_DEFINEVARS) || !defined(__AROS__)
extern struct ExecBase 	    	*SysBase;
extern struct DosLibrary 	*DOSBase;
extern struct UtilityBase 	*UtilityBase;
extern struct IntuitionBase	*IntuitionBase;
extern struct GfxBase 		*GfxBase;
extern struct LocaleBase 	*LocaleBase;
extern struct Library 		*LayersBase;
extern struct Library 		*GadToolsBase;
#endif

#ifdef __RT_DEFINEVARS
#undef extern
#endif
