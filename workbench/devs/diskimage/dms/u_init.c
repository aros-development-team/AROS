/*
 *     xDMS  v1.3  -  Portable DMS archive unpacker  -  Public Domain
 *     Written by     Andre Rodrigues de la Rocha  <adlroc@usa.net>
 *
 *     Decruncher reinitialization
 *
 */

#include "xdms.h"

void Init_Decrunchers (struct xdms_data *xdms) {
	xdms->quick_text_loc = 251;
	xdms->medium_text_loc = 0x3fbe;
	xdms->heavy_text_loc = 0;
	xdms->deep_text_loc = 0x3fc4;
	xdms->init_deep_tabs = 1;
	memset(xdms->text,0,0x3fc8);
}
