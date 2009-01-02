/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 Neil Cafferkey
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

#ifndef ALLOCDATABUFFER_H
#define ALLOCDATABUFFER_H

VOID freeDataBuffer(struct DataBuffer * DB);
BOOL doAllocDataBuffer(struct DataBuffer * DB, int size);

static inline BOOL allocDataBuffer(struct DataBuffer * DB, int size)
{
  if (DB->db_Size < size)
    return doAllocDataBuffer(DB, size);
  else
    return TRUE;
}

#endif /* ALLOCDATABUFFER_H */
