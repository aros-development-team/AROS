/*
 * Copyright 1992 by Jutta Degener and Carsten Bormann, Technische
 * Universitaet Berlin.  See the accompanying file "COPYRIGHT" for
 * details.  THERE IS ABSOLUTELY NO WARRANTY FOR THIS SOFTWARE.
 */

/* $Header: /cvsroot/sox/sox/src/gsm/gsm_destroy.c,v 1.1 2001/11/14 03:31:26 cbagwell Exp $ */

#include "gsm.h"

#	include	<stdlib.h>

void gsm_destroy (gsm S)
{
	if (S) free((char *)S);
}
