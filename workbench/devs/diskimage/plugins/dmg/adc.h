/*
** Copyright (C) 2007-2010 Fredrik Wikstrom
**
** This file is part of diskimage.device
**
** Diskimage.device is free software: you can redistribute it and/or modify it
** under the terms of the GNU Library General Public License as published by
** the Free Software Foundation, either version 3 of the License, or (at your
** option) any later version.
**
** Diskimage.device is distributed in the hope that it will be useful, but
** WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
** or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
** License for more details.
**
** You should have received a copy of the GNU Library General Public License
** along with diskimage.device.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ADC_H
#define ADC_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

LONG adc_decompress(APTR dst, LONG dst_len, APTR src, LONG src_len);

#endif
