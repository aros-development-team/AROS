/*

Copyright (C) 2001-2020 Neil Cafferkey

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.

*/

#ifndef ETHERNET_H
#define ETHERNET_H


/* General */

#define ETH_ADDRESSSIZE 6
#define ETH_HEADERSIZE 14
#define ETH_MTU 1500
#define ETH_MAXPACKETSIZE (ETH_HEADERSIZE + ETH_MTU)

#define ETH_PACKET_DEST 0
#define ETH_PACKET_SOURCE 6
#define ETH_PACKET_TYPE 12
#define ETH_PACKET_IEEELEN 12
#define ETH_PACKET_DATA 14

/* SNAP Frames */

#define SNAP_HEADERSIZE 8

#define SNAP_FRM_TYPE 6


#endif
