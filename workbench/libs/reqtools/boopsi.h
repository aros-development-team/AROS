#ifdef __AROS__
#include <aros/asmcall.h>

AROS_UFP3(IPTR, myBoopsiDispatch,
	  AROS_UFPA(Class *, cl, A0),
	  AROS_UFPA(struct Image *, im, A2),
	  AROS_UFPA(Msg, msg, A1));
#else
IPTR myBoopsiDispatch(REGPARAM(a0, Class *, cl),
    	    	      REGPARAM(a2, struct Image *, im),
		      REGPARAM(a1, Msg, msg));
#endif
