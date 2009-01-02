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

#include <conf.h>

#include <exec/types.h>

#include <sys/malloc.h>

#include <api/amiga_api.h>
#include <api/allocdatabuffer.h>

BOOL doAllocDataBuffer(struct DataBuffer * DB, int size)
{
  if (DB->db_Addr)
    bsd_free(DB->db_Addr, M_TEMP);
  
  if ((DB->db_Addr = bsd_malloc(size, M_TEMP, M_WAITOK)) == NULL) {
    DB->db_Size = 0;
    return FALSE;
  }
  DB->db_Size = size;
  return TRUE;
}

VOID freeDataBuffer(struct DataBuffer * DB)
{
  if (DB->db_Addr)
    bsd_free(DB->db_Addr, M_TEMP);
  DB->db_Size = 0;
  DB->db_Addr = NULL;
}

