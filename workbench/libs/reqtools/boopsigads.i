
BUTTON_MAGIC_LONGWORD	equ		'Oops'

	STRUCTURE InitData,0
		APTR idata_Gadget				* NULL for images not within gadgets
		APTR idata_TextAttr
		APTR idata_Label
		APTR idata_VisualInfo
		ULONG idata_Underscore
		LABEL idata_SIZEOF			* MUST BE MULTIPLE OF LONGWORD!!!

	STRUCTURE localobjdata,0
		STRUCT lod_IData,idata_SIZEOF
		UWORD lod_UnderOff
		UWORD lod_UnderWidth
		UWORD lod_UnderY
		UWORD lod_RestLen
		LABEL lod_SIZEOF

