/**************************************************************
**** Print.h : procedures for printing                     ****
**** Free software under GNU license, started on 2/2/2012  ****
**** © AROS Team                                           ****
**************************************************************/

#ifndef PRINT_H
#define PRINT_H

#include "Memory.h"

/* Print a file
 */
BYTE print_file(LINE *svg, unsigned char eol);

/* Get/set current printer.device unit
 *   If unit < 0, gets current printer unit
 *   Otherwise, sets unit to the selected unit
 */
BYTE print_unit(BYTE unit);

#endif
