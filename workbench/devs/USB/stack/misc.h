#ifndef MISC_H_
#define MISC_H_

/*
    Copyright (C) 2006 by Michal Schulz
    $Id$

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Library General Public License as 
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this program; if not, write to the
    Free Software Foundation, Inc.,
    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <inttypes.h>
#include <aros/debug.h>
#include <usb/usb_core.h>
#include <devices/timer.h>

void DumpDescriptor(usb_descriptor_t *desc);
void USBDelay(struct timerequest *tr, uint32_t msec);
struct timerequest *USBCreateTimer();
void USBDeleteTimer(struct timerequest *tr);
uint32_t USBTimer(struct timerequest *tr, uint32_t msec);
void USBTimerDone(struct timerequest *tr);


#endif /*MISC_H_*/
