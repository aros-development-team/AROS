/*
 * ntfs.handler - New Technology FileSystem handler
 *
 * Copyright © 2012 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id $
 */

#include <aros/macros.h>
#include <exec/errors.h>
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/uuid.h>
#include <proto/codesets.h>

#include <clib/macros.h>

#include <string.h>   
#include <ctype.h>

#include "ntfs_fs.h"
#include "ntfs_protos.h"

//#define DEBUG_MFT
#include "debug.h"

extern struct Globals *glob;

ULONG PostProcessMFTRecord(struct FSData *fs_data, struct MFTRecordEntry *record, int len, UBYTE *magic)
{
    UWORD seqarray_len, seqnum;
    UBYTE *seqarray, *buf;

    buf = (UBYTE *)record;

    D(bug("[NTFS]: %s(%.4s)\n", __PRETTY_FUNCTION__, magic));

    /* Perform post-read MST fixup by applying the sequence array to acquired blocks */

    D(bug("[NTFS] %s: FSData @ 0x%p\n", __PRETTY_FUNCTION__, fs_data));
    D(bug("[NTFS] %s: MFTRecordEntry @ 0x%p\n", __PRETTY_FUNCTION__, record));

    if (memcmp(record->header.magic, magic, 4))
    {
	D(
	    bug("[NTFS] %s: record magic mismatch (got '%.4s')\n", __PRETTY_FUNCTION__, record->header.magic);
	 )
	return ERROR_OBJECT_WRONG_TYPE ;
    }

    seqarray_len = AROS_LE2WORD(record->header.usa_count) - 1;

    if (seqarray_len != len)
    {
	D(bug("[NTFS] %s: fixup error - sequence array size != record size\n", __PRETTY_FUNCTION__));
	return  ERROR_NOT_IMPLEMENTED;
    }

    seqarray = (char *)record + AROS_LE2WORD(record->header.usa_offset);
    seqnum = AROS_LE2WORD(*((UWORD*)seqarray));

    D(bug("[NTFS] %s: update sequence = %u (usa_offset %u)\n", __PRETTY_FUNCTION__, seqnum, AROS_LE2WORD(record->header.usa_offset)));

    while (seqarray_len > 0)
    {
	buf += fs_data->sectorsize;
	seqarray += 2;
	if (AROS_LE2WORD(*((UWORD*)(buf - 2))) != seqnum)
	{
	    D(bug("[NTFS] %s: update sequence mismatch  @ 0x%p (%u != %u)\n", __PRETTY_FUNCTION__, buf, AROS_LE2WORD(*((UWORD*)buf)), seqnum));
	    return ERROR_NOT_IMPLEMENTED;
	}

	*((UWORD*)(buf - 2)) = *((UWORD*)seqarray);
	seqarray_len--;
    }

    D(bug("[NTFS] %s: record fixup complete\n", __PRETTY_FUNCTION__));

    return 0;
}

ULONG PreProcessMFTRecord(struct FSData *fs_data, struct MFTRecordEntry *record, int len)
{
    D(bug("[NTFS]: %s(MFTRecordEntry @ 0x%p)\n", __PRETTY_FUNCTION__, record));

    /* Perform pre-write MST fixup.  set the sequence numbers of blocks */

    return 0;
}

struct MFTAttr *GetMappingPairPos(UBYTE *mappos, int nn, UQUAD *val, int sig)
{
    UQUAD pos = 0;
    UQUAD mask = 1;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    while (nn--)
    {
	pos += mask * (*mappos);
	mappos += 1;
	mask <<= 8;
    }

    if ((sig) && (pos & (mask >> 1)))
	pos -= mask;

    *val = pos;
    return (struct MFTAttr *)mappos;
}

IPTR ReadNTFSRunList(struct NTFSRunLstEntry * rle)
{
    int len, offs;
    UQUAD val;
    struct MFTAttr *mappos = (struct MFTAttr *)rle->mappingpair;

    D(bug("[NTFS]: %s(mappos @ 0x%p)\n", __PRETTY_FUNCTION__, mappos));

retry:
    len = (*(UBYTE *)mappos & 0xF);
    offs = (*(UBYTE *)mappos >> 4);

    D(bug("[NTFS] %s: len = %u\n", __PRETTY_FUNCTION__, len));
    D(bug("[NTFS] %s: offs = %u\n", __PRETTY_FUNCTION__, offs));

    if (!len)
    {
	D(bug("[NTFS] %s: !len\n", __PRETTY_FUNCTION__));
	if ((rle->attr) && (rle->attr->flags & AF_ALST))
	{
	    D(bug("[NTFS] %s: AF_ALST\n", __PRETTY_FUNCTION__));

	    mappos = FindMFTAttrib(rle->attr, *(UBYTE *)rle->attr->attr_cur);

	    if (mappos)
	    {
		D(bug("[NTFS] %s: 'RUN'\n", __PRETTY_FUNCTION__));
		if (mappos->residentflag == ATTR_RESIDENT_FORM)
		{
		    D(bug("[NTFS] %s: $DATA should be non-resident\n", __PRETTY_FUNCTION__));
		    return ~0;
		}

		mappos = (struct MFTAttr *)((IPTR)mappos + AROS_LE2WORD(mappos->data.non_resident.mapping_pairs_offset));
		rle->curr_lcn = 0;
		goto retry;
	    }
	}
	D(bug("[NTFS] %s: run list overflow\n", __PRETTY_FUNCTION__));
	return ~0;
    }
    // current VCN  length
    mappos = GetMappingPairPos((UBYTE *)mappos + 1, len, &val, 0);
    rle->curr_vcn = rle->next_vcn;
    rle->next_vcn = rle->next_vcn + val;

    D(bug("[NTFS] %s: curr_vcn = %u, next_vcn = %u, val = %u\n", __PRETTY_FUNCTION__, (unsigned int)rle->curr_vcn, (unsigned int)rle->next_vcn, (unsigned int)val));

    // previous LCN offset
    mappos = GetMappingPairPos((UBYTE *)mappos, offs, &val, 1);
    rle->curr_lcn = rle->curr_lcn + val;

    D(bug("[NTFS] %s: curr_lcn = %u\n", __PRETTY_FUNCTION__, (unsigned int)rle->curr_lcn));

    if (val == 0)
	rle->flags |= RLEFLAG_SPARSE;
    else
	rle->flags &= ~RLEFLAG_SPARSE;

    rle->mappingpair = (UBYTE *)mappos;

    return 0;
}

void FreeMFTAttrib(struct NTFSMFTAttr *at)
{
    D(bug("[NTFS]: %s(NTFSMFTAttr @ 0x%p)\n", __PRETTY_FUNCTION__, at));

    FreeVec(at->edat_buf);
    at->edat_buf = NULL;
    if (at->emft_buf)
    {
	FreeMem(at->emft_buf, at->mft->data->mft_size << SECTORSIZE_SHIFT);
	at->emft_buf = NULL;
    }
    if (at->sbuf)
    {
	FreeMem(at->sbuf, COM_LEN);
	at->sbuf = NULL;
    }
}

IPTR ReadMFTAttribData(struct NTFSMFTAttr *at, struct MFTAttr *attrentry, UBYTE *dest, UQUAD ofs, ULONG len, int cached)
{
    D(UQUAD vcn);
    struct NTFSRunLstEntry runlist_entry, *rle;

    D(
    bug("[NTFS]: %s(ofs = %u; len = %u)\n", __PRETTY_FUNCTION__, (IPTR)ofs, len);

    bug("[NTFS] %s: NTFSMFTAttr @ 0x%p\n", __PRETTY_FUNCTION__, at);
    bug("[NTFS] %s: MFTAttr @ 0x%p, dest @ 0x%p\n", __PRETTY_FUNCTION__, attrentry, dest);
    )

    if (len == 0)
	return 0;

    memset (&runlist_entry, 0, sizeof(struct NTFSRunLstEntry));
    rle = &runlist_entry;
    rle->attr = at;

    if (AROS_LE2LONG(attrentry->data.resident.value_offset) > AROS_LE2LONG(attrentry->length))
    {
	D(bug("[NTFS] %s: error - corrupt attribute\n", __PRETTY_FUNCTION__));
	return ~0;	
    }

    if (attrentry->residentflag == ATTR_RESIDENT_FORM)
    {
	D(bug("[NTFS] %s: ATTR_RESIDENT_FORM\n", __PRETTY_FUNCTION__));

	if ((ofs + len) > AROS_LE2LONG(attrentry->data.resident.value_length))
	{
	    D(bug("[NTFS] %s: error - read out of range\n", __PRETTY_FUNCTION__));
	    return ~0;
	}
	CopyMem(attrentry + AROS_LE2LONG(attrentry->data.resident.value_offset) + ofs, dest, len);
	return 0;
    }

    if (AROS_LE2WORD(attrentry->attrflags) & FLAG_COMPRESSED)
    {
	rle->flags |= RLEFLAG_COMPR;
    }
    else
    {
	rle->flags &= ~RLEFLAG_COMPR;
    }
    rle->mappingpair = (UBYTE *)((IPTR)attrentry + AROS_LE2WORD(attrentry->data.non_resident.mapping_pairs_offset));

    bug("[NTFS] %s: mappingpair @ 0x%p\n", __PRETTY_FUNCTION__, rle->mappingpair);
    
    if (rle->flags & RLEFLAG_COMPR)
    {
	D(bug("[NTFS] %s: ## Compressed\n", __PRETTY_FUNCTION__));
	if (!cached)
	{
	    D(bug("[NTFS] %s: error - attribute cannot be compressed\n", __PRETTY_FUNCTION__));
	    return ~0;
	}

	if (at->sbuf)
	{
	    if ((ofs & (~(COM_LEN - 1))) == at->save_pos)
	    {
		UQUAD n;

		n = COM_LEN - (ofs - at->save_pos);
		if (n > len)
		    n = len;

		CopyMem(at->sbuf + ofs - at->save_pos, dest, n);
		if (n == len)
		    return 0;

		dest += n;
		len -= n;
		ofs += n;
	    }
	}
	else
	{
	    at->sbuf = AllocMem(COM_LEN, MEMF_ANY);
	    if (at->sbuf == NULL)
	    {
		D(bug("[NTFS] %s: error - failed to allocate sbuf\n", __PRETTY_FUNCTION__));
		return ERROR_NO_FREE_STORE;
	    }
	    at->save_pos = 1;
	}

	D(vcn =) rle->target_vcn = (ofs >> COM_LOG_LEN) * (COM_SEC / at->mft->data->cluster_sectors);
	rle->target_vcn &= ~0xF;
    }
    else
    {
	rle->target_vcn = (ofs >> SECTORSIZE_SHIFT) / at->mft->data->cluster_sectors;
	D(vcn = rle->target_vcn);
    }

    rle->next_vcn = AROS_LE2QUAD(attrentry->data.non_resident.lowest_vcn);
    rle->curr_lcn = 0;

    D(bug("[NTFS] %s: vcn = %u\n", __PRETTY_FUNCTION__, vcn));

    while (rle->next_vcn <= rle->target_vcn)
    {
	D(bug("[NTFS] %s: next_vcn = %u, target_vcn = %u\n", __PRETTY_FUNCTION__, (IPTR)rle->next_vcn, (IPTR)rle->target_vcn));
	if (ReadNTFSRunList(rle))
	{
	    D(bug("[NTFS] %s: read_run_list failed\n", __PRETTY_FUNCTION__));
	    return ~0;
	}
    }

    D(bug("[NTFS] %s: next_vcn = %u\n", __PRETTY_FUNCTION__, (IPTR)rle->next_vcn));

    if (at->flags & AF_GPOS)
    {
	UQUAD st0, st1, m;

	D(bug("[NTFS] %s: AF_GPOS\n", __PRETTY_FUNCTION__));

	m = (ofs >> SECTORSIZE_SHIFT) % at->mft->data->cluster_sectors;

	st0 =
	(rle->target_vcn - rle->curr_vcn + rle->curr_lcn) * at->mft->data->cluster_sectors + m;
	st1 = st0 + 1;

	if (st1 ==
	  (rle->next_vcn - rle->curr_vcn + rle->curr_lcn) * at->mft->data->cluster_sectors)
	{
	    if (ReadNTFSRunList(rle))
	    {
		D(bug("[NTFS] %s: read_run_list failed\n", __PRETTY_FUNCTION__));
		return ~0;
	    }
	    st1 = rle->curr_lcn * at->mft->data->cluster_sectors;
	}
	*((ULONG *)dest) = AROS_LONG2LE(st0);
	*((ULONG *)(dest + 4)) = AROS_LONG2LE(st1);
	return 0;
    }

    if (!(rle->flags & RLEFLAG_COMPR))
    {
	D(bug("[NTFS] %s: ## Uncompressed\n", __PRETTY_FUNCTION__));

	if (!(at->mft->data->cluster_sectors & 0x1))
	{
	    unsigned int sectbits_shift = ilog2(at->mft->data->cluster_sectors);
	    ULONG blocksize = 1 << (sectbits_shift + SECTORSIZE_SHIFT);
	    UBYTE *buf = dest;

	    UQUAD i, blockcnt = ((len + ofs) + blocksize - 1) >> (sectbits_shift + SECTORSIZE_SHIFT);

	    D(
		bug("[NTFS] %s: blockcnt = %u\n", __PRETTY_FUNCTION__, (IPTR)blockcnt);
		bug("[NTFS] %s: blocksize = %u\n", __PRETTY_FUNCTION__, blocksize);
	    )

	    for (i = ofs >> (sectbits_shift + SECTORSIZE_SHIFT); i < blockcnt; i++)
	    {
		UQUAD blockstart;
		UQUAD blockoff = ofs & (blocksize - 1);
		UQUAD blockend = blocksize;
		UQUAD skipfirst = 0;

		D(
		    bug("[NTFS] %s: blockoff = %u\n", __PRETTY_FUNCTION__, (IPTR)blockoff);
		    bug("[NTFS] %s: blockend = %u\n", __PRETTY_FUNCTION__, (IPTR)blockend);
		)

		if (i >= rle->next_vcn)
		{
		    if (ReadNTFSRunList(rle))
		    {
			D(bug("[NTFS] %s: failed to read run list!\n", __PRETTY_FUNCTION__));
			return -1;
		    }

		    blockstart = rle->curr_lcn;
		}
		else
		{
		    blockstart = (rle->flags & RLEFLAG_SPARSE) ? 0 : (i - rle->curr_vcn + rle->curr_lcn);
		}

		blockstart = blockstart << sectbits_shift;

		/* Last block.  */
		if (i == (blockcnt - 1))
		{
		    D(bug("[NTFS] %s: last block.. \n", __PRETTY_FUNCTION__));

		    blockend = (len + ofs) & (blocksize - 1);

		    /* The last portion is exactly blocksize.  */
		    if (! blockend)
			blockend = blocksize;
		}

		/* First block.  */
		if (i == (ofs >> (sectbits_shift + SECTORSIZE_SHIFT)))
		{
		    D(bug("[NTFS] %s: first block.. \n", __PRETTY_FUNCTION__));

		    skipfirst = blockoff;
		    blockend -= skipfirst;
		}

		/* If the block number is 0 this block is not stored on disk but is zero filled instead.  */

		D(
		    bug("[NTFS] %s: blockstart = %u\n", __PRETTY_FUNCTION__, (IPTR)blockstart);
		    bug("[NTFS] %s: blockend = %u\n", __PRETTY_FUNCTION__, (IPTR)blockend);
		    bug("[NTFS] %s: skipfirst = %u\n", __PRETTY_FUNCTION__, (IPTR)skipfirst);
		)

		if (blockstart)
		{
		    UQUAD blocknr, skipblocks = skipfirst >> SECTORSIZE_SHIFT, lastblock = ((blockend + skipfirst) >> SECTORSIZE_SHIFT);
		    APTR blockbuf, bufstart = buf;
		    IPTR copysize;

		    if (lastblock == 0)
			lastblock = 1;

		    for (blocknr = skipblocks; blocknr < lastblock; blocknr++)
		    {
			D(bug("[NTFS] %s: block %u\n", __PRETTY_FUNCTION__, (IPTR)blocknr));
			if (blocknr >= (skipfirst >> SECTORSIZE_SHIFT))
			{
			    D(bug("[NTFS] %s: reading ..\n", __PRETTY_FUNCTION__));

			    if ((at->mft->cblock = Cache_GetBlock(at->mft->data->cache, at->mft->data->first_device_sector + blockstart + blocknr, &at->mft->cbuf)) == NULL)
			    {
				D(bug("[NTFS] %s: read failed\n", __PRETTY_FUNCTION__));
				return IoErr();
			    }

			    D(bug("[NTFS] %s: cbuf @ 0x%p\n", __PRETTY_FUNCTION__, at->mft->cbuf));
			    
			    if (blocknr == (lastblock - 1) && (blockend & (at->mft->data->sectorsize - 1)))
				copysize = (blockend & (at->mft->data->sectorsize - 1));
			    else
				copysize = at->mft->data->sectorsize;

			    if ((blocknr << SECTORSIZE_SHIFT) < skipfirst)
			    {
				blockbuf = at->mft->cbuf + (skipfirst & (at->mft->data->sectorsize - 1));
				
				copysize -= (skipfirst & (at->mft->data->sectorsize - 1));
			    }
			    else
			    {
				blockbuf = at->mft->cbuf;
			    }

			    if (copysize > 0)
			    {
				D(bug("[NTFS] %s: copying %u bytes from 0x%p -> 0x%p\n", __PRETTY_FUNCTION__, copysize, blockbuf, bufstart));
				CopyMem(blockbuf, bufstart, copysize);
			    }
			    bufstart +=copysize;

			    Cache_FreeBlock(at->mft->data->cache, at->mft->cblock);
			    at->mft->cblock = NULL;
			}
		    }
		}
		else
		    memset (buf, 0, blockend);

		buf += blocksize - skipfirst;
	    }

	}
	return 0;
    }

    /* Warning : TODO - decompress block */
    D(bug("[NTFS] %s: cannot decompress\n", __PRETTY_FUNCTION__));
    return ~0;
}

IPTR ReadMFTAttrib(struct NTFSMFTAttr *at, UBYTE *dest, UQUAD ofs, ULONG len, int cached)
{
    struct MFTAttr *save_cur;
    UBYTE attr;
    struct MFTAttr *attrentry;
    IPTR ret;

    D(bug("[NTFS]: %s(NTFSMFTAttr @ 0x%p; ofs = %d; len = %d)\n", __PRETTY_FUNCTION__, at, (IPTR)ofs, len));

    save_cur = at->attr_cur;
    at->attr_nxt = at->attr_cur;
    attr = *(UBYTE *)at->attr_nxt;
    if (at->flags & AF_ALST)
    {
	UQUAD vcn;

	D(bug("[NTFS] %s: AF_ALST\n", __PRETTY_FUNCTION__));

	vcn = ofs / (at->mft->data->cluster_sectors << SECTORSIZE_SHIFT);
	attrentry = (struct MFTAttr *)((IPTR)at->attr_nxt + AROS_LE2WORD(at->attr_nxt->length));
	while (attrentry < at->attr_end)
	{
	    if (*(UBYTE *)attrentry != attr)
		break;
	    if (AROS_LE2LONG(*((ULONG *)(attrentry + 8))) > vcn)
		break;
	    at->attr_nxt = attrentry;
	    attrentry = (struct MFTAttr *)((IPTR)attrentry + AROS_LE2WORD(attrentry->length));
	}
    }
    attrentry = FindMFTAttrib(at, attr);
    if (attrentry)
	ret = ReadMFTAttribData(at, attrentry, dest, ofs, len, cached);
    else
    {
	D(bug("[NTFS] %s: attribute %u not found\n", __PRETTY_FUNCTION__, attr));
	ret = ~0;
    }
    at->attr_cur = save_cur;
    return ret;
}

static IPTR ReadMFTRecord(struct NTFSMFTEntry *mft, UBYTE *buf, ULONG mft_id)
{
    D(bug("[NTFS]: %s(%d)\n", __PRETTY_FUNCTION__, mft_id));

    if (ReadMFTAttrib
      (&mft->data->mft.attr, buf, mft_id * ((UQUAD) mft->data->mft_size << SECTORSIZE_SHIFT),
       mft->data->mft_size << SECTORSIZE_SHIFT, 0))
    {
	D(bug("[NTFS] %s: failed to read MFT #%d\n", __PRETTY_FUNCTION__, mft_id));
	return ~0;
    }
#if defined(DEBUG_MFT)
    D(
	int dumpx;

	bug("[NTFS] %s: MFTRecord #%d Dump -:\n", __PRETTY_FUNCTION__, mft_id);
	bug("[NTFS] %s: MFTRecord #%d buf @ 0x%p, size %d x %d", __PRETTY_FUNCTION__, mft_id, buf, mft->data->mft_size, mft->data->sectorsize);

	for (dumpx = 0; dumpx < (mft->data->mft_size * mft->data->sectorsize) ; dumpx ++)
	{
	    if ((dumpx%16) == 0)
	    {
		bug("\n[NTFS] %s:\t%03x:", __PRETTY_FUNCTION__, dumpx);
	    }
	    bug(" %02x", ((UBYTE*)buf)[dumpx]);
	}
	bug("\n");
     )
#endif
    return PostProcessMFTRecord (mft->data, (struct MFTRecordEntry *)buf, mft->data->mft_size, "FILE");
}

struct MFTAttr *FindMFTAttrib(struct NTFSMFTAttr *at, UBYTE attr)
{
    D(bug("[NTFS]: %s(%u)\n", __PRETTY_FUNCTION__, attr));

    if (at->flags & AF_ALST)
    {
	D(bug("[NTFS] %s: AF_ALST\n", __PRETTY_FUNCTION__));
retry:
	while (at->attr_nxt < at->attr_end)
	{
	    at->attr_cur = at->attr_nxt;
	    at->attr_nxt = (struct MFTAttr *)((IPTR)at->attr_nxt + AROS_LE2WORD(at->attr_cur->length));

	    D(bug("[NTFS] %s: attr_cur @ 0x%p, attr_nxt @ 0x%p\n", __PRETTY_FUNCTION__, at->attr_cur, at->attr_nxt ));

	    if ((*(UBYTE *)at->attr_cur == attr) || (attr == 0))
	    {
		UBYTE *new_pos;

		D(bug("[NTFS] %s: attr %u found @ 0x%p\n", __PRETTY_FUNCTION__, attr, at->attr_cur));

		if (at->flags & AF_MMFT)
		{
		    D(bug("[NTFS] %s: AF_MMFT\n", __PRETTY_FUNCTION__));

		    if ((at->mft->cblock = Cache_GetBlock(at->mft->data->cache, at->mft->data->first_device_sector + AROS_LE2LONG(*((ULONG *)(at->attr_cur + 0x10))), &at->mft->cbuf)) == NULL)
		    {
			D(bug("[NTFS] %s: read failed\n", __PRETTY_FUNCTION__));
			return NULL;
		    }
		    CopyMem(at->mft->cbuf, at->emft_buf, at->mft->data->sectorsize);
		    Cache_FreeBlock(at->mft->data->cache, at->mft->cblock);
		    at->mft->cblock = NULL;

		    if ((at->mft->cblock = Cache_GetBlock(at->mft->data->cache, at->mft->data->first_device_sector + AROS_LE2LONG(*((ULONG *)(at->attr_cur + 0x14))), &at->mft->cbuf)) == NULL)
		    {
			D(bug("[NTFS] %s: read failed\n", __PRETTY_FUNCTION__));
			return NULL;
		    }
		    CopyMem(at->mft->cbuf, at->emft_buf + at->mft->data->sectorsize, at->mft->data->sectorsize);
		    Cache_FreeBlock(at->mft->data->cache, at->mft->cblock);
		    at->mft->cblock = NULL;

		    if (PostProcessMFTRecord
		      (at->mft->data, (struct MFTRecordEntry *)at->emft_buf, at->mft->data->mft_size,
		       "FILE"))
			return NULL;
		}
		else
		{
		    D(bug("[NTFS] %s: !AF_MMFT\n", __PRETTY_FUNCTION__));

		    if (ReadMFTRecord(at->mft, (UBYTE *)at->emft_buf,
				AROS_LE2LONG(at->attr_cur->data.resident.value_length)))
			return NULL;
		}

		new_pos = &((UBYTE *)at->emft_buf)[AROS_LE2WORD(at->emft_buf->data.resident.value_offset)];
		while ((UBYTE) *new_pos != 0xFF)
		{
		    if ((*new_pos ==
		       *(UBYTE *)at->attr_cur)
		      && (AROS_LE2WORD(*((UWORD *)(new_pos + 0xE))) == AROS_LE2WORD(*((UWORD *)(at->attr_cur + 0x18)))))
		    {
			return (struct MFTAttr *)new_pos;
		    }
		    new_pos += AROS_LE2WORD(*((UWORD *)(new_pos + 4)));
		}
		D(bug("[NTFS] %s: %u not found in attribute list!\n", __PRETTY_FUNCTION__, at->attr_cur));
		return NULL;
	    }
	}
	return NULL;
    }

    at->attr_cur = at->attr_nxt;

    while (*(UBYTE *)at->attr_cur != 0xFF)
    {
	at->attr_nxt = (struct MFTAttr *)((IPTR)at->attr_nxt + AROS_LE2WORD(at->attr_cur->length));

	D(bug("[NTFS] %s: attr_cur @ 0x%p, attr_nxt @ 0x%p (offset %u) \n", __PRETTY_FUNCTION__, at->attr_cur, at->attr_nxt, AROS_LE2WORD(at->attr_cur->length)));

	if (*(UBYTE *)at->attr_cur == AT_ATTRIBUTE_LIST)
	    at->attr_end = at->attr_cur;
	if ((*(UBYTE *)at->attr_cur == attr) || (attr == 0))
	{
	    D(bug("[NTFS] %s: returning attr_cur @ 0x%p\n", __PRETTY_FUNCTION__, at->attr_cur));
	    return at->attr_cur;
	}
	at->attr_cur = at->attr_nxt;
    }
    if (at->attr_end)
    {
	struct MFTAttr *attrentry;

	D(bug("[NTFS] %s: attr_end @ 0x%p\n", __PRETTY_FUNCTION__, at->attr_end));

	at->emft_buf = AllocMem(at->mft->data->mft_size << SECTORSIZE_SHIFT, MEMF_ANY);
	if (at->emft_buf == NULL)
	    return NULL;

	D(bug("[NTFS] %s: emft_buf allocated @ 0x%p\n", __PRETTY_FUNCTION__, at->emft_buf));

	attrentry = at->attr_end;

	if (attrentry->residentflag == ATTR_NONRESIDENT_FORM)
	{
	    int n;

	    n = ((AROS_LE2QUAD(attrentry->data.non_resident.data_size) + (512 - 1)) & (~(512 - 1)));
	    at->attr_cur = at->attr_end;
	    at->edat_buf = AllocVec(n, MEMF_ANY);
	    if (!at->edat_buf)
		return NULL;

	    D(bug("[NTFS] %s: edat_buf allocated @ 0x%p\n", __PRETTY_FUNCTION__, at->edat_buf));

	    if (ReadMFTAttribData(at, attrentry, (UBYTE *)at->edat_buf, 0, n, 0))
	    {
		D(bug("[NTFS] %s: failed to read non-resident attribute list!\n", __PRETTY_FUNCTION__));
	    }
	    at->attr_nxt = at->edat_buf;
	    at->attr_end = (struct MFTAttr *)((IPTR)at->edat_buf + AROS_LE2QUAD(attrentry->data.non_resident.data_size));
	}
	else
	{
	    at->attr_nxt = (struct MFTAttr *)((IPTR)at->attr_end + AROS_LE2WORD(attrentry->data.resident.value_offset));
	    at->attr_end = (struct MFTAttr *)((IPTR)at->attr_end + AROS_LE2LONG(attrentry->length));
	    D(bug("[NTFS] %s: attr_nxt @ 0x%p, attr_end @ 0x%p\n", __PRETTY_FUNCTION__, at->attr_nxt, at->attr_end));
	}
	at->flags |= AF_ALST;
	while (at->attr_nxt < at->attr_end)
	{
	    if ((*(UBYTE *)at->attr_nxt == attr) || (attr == 0))
		break;
	    at->attr_nxt = (struct MFTAttr *)((IPTR)at->attr_nxt + AROS_LE2WORD(at->attr_nxt->length));
	}
	if (at->attr_nxt >= at->attr_end)
	    return NULL;

	if ((at->flags & AF_MMFT) && (attr == AT_DATA))
	{
	    D(bug("[NTFS] %s: AT_DATA && AF_MMFT\n", __PRETTY_FUNCTION__));

	    at->flags |= AF_GPOS;
	    at->attr_cur = at->attr_nxt;
	    attrentry = at->attr_cur;
	    attrentry->data.resident.value_length = AROS_LONG2LE(at->mft->data->mft_start);
	    attrentry->data.resident.value_offset = AROS_WORD2LE(at->mft->data->mft_start + 1);
	    attrentry = (struct MFTAttr *)((IPTR)at->attr_nxt + AROS_LE2WORD(attrentry->length));
	    while (attrentry < at->attr_end)
	    {
		if (*(UBYTE *)attrentry != attr)
		    break;
		if (ReadMFTAttrib
		    (at, (UBYTE *)(attrentry + 0x10),
		    AROS_LE2LONG(attrentry->data.resident.value_length) * (at->mft->data->mft_size << SECTORSIZE_SHIFT),
		    at->mft->data->mft_size << SECTORSIZE_SHIFT, 0))
		    return NULL;
		attrentry = (struct MFTAttr *)((IPTR)attrentry + AROS_LE2WORD(attrentry->length));
	    }
	    at->attr_nxt = at->attr_cur;
	    at->flags &= ~AF_GPOS;
	}
	goto retry;
    }
    return NULL;
}

struct MFTAttr *MapMFTAttrib(struct NTFSMFTAttr *at, struct NTFSMFTEntry *mft, UBYTE attr)
{
    struct MFTAttr *attrentry;

    D(bug("[NTFS]: %s(%ld)\n", __PRETTY_FUNCTION__, attr));

    INIT_MFTATTRIB(at, mft);
    if ((attrentry = FindMFTAttrib(at, attr)) == NULL)
	return NULL;

    if ((at->flags & AF_ALST) == 0)
    {
	while (1)
	{
	    if ((attrentry = FindMFTAttrib(at, attr)) == NULL)
		break;
	    if (at->flags & AF_ALST)
		return attrentry;
	}
	FreeMFTAttrib(at);
	INIT_MFTATTRIB(at, mft);
	attrentry = FindMFTAttrib(at, attr);
    }
    return attrentry;
}

IPTR InitMFTEntry(struct NTFSMFTEntry *mft, ULONG mft_id)
{
    struct MFTRecordEntry *record;
    unsigned short flag;

    D(bug("[NTFS]: %s(%ld)\n", __PRETTY_FUNCTION__, mft_id));

    mft->buf_filled = 1;

    if (mft->buf != NULL)
    {
	D(bug("[NTFS] %s: NTFSMFTEntry @ 0x%p in use? (mft->buf != NULL)\n", __PRETTY_FUNCTION__, mft));
	return ~0;
    }

    mft->buf = AllocMem(mft->data->mft_size << SECTORSIZE_SHIFT, MEMF_ANY);
    if ((record = (struct MFTRecordEntry *)mft->buf) == NULL)
    {
	return ERROR_NO_FREE_STORE;
    }

    if (ReadMFTRecord(mft, mft->buf, mft_id))
    {
	D(bug("[NTFS] %s: failed to read MFT #%d\n", __PRETTY_FUNCTION__, mft_id));
	return ~0;
    }

    flag = AROS_LE2WORD(record->flags);
    if ((flag & FILERECORD_SEGMENT_IN_USE) == 0)
    {
	D(bug("[NTFS] %s: MFT not in use!\n", __PRETTY_FUNCTION__));
	return ~0;
    }

    if ((flag & FILERECORD_NAME_INDEX_PRESENT) == 0)
    {
	struct MFTAttr *attrentry;

	attrentry = MapMFTAttrib(&mft->attr, mft, AT_DATA);
	if (attrentry == NULL)
	{
	    D(bug("[NTFS] %s: No $DATA in MFT #%d\n", __PRETTY_FUNCTION__, mft_id));
	    return ~0;
	}

	if (attrentry->residentflag == ATTR_RESIDENT_FORM)
	{
	    mft->size = AROS_LE2LONG(*(ULONG *)((IPTR)attrentry + 0x10));
	}
	else
	{
	    mft->size = AROS_LE2QUAD(*(UQUAD *)((IPTR)attrentry + 0x30));
	}

	if ((mft->attr.flags & AF_ALST) == 0)
	    mft->attr.attr_end = 0;	/*  Don't jump to attribute list */
    }
    else
    {
	INIT_MFTATTRIB(&mft->attr, mft);
    }

    return 0;
}

LONG
ProcessFSEntry(struct NTFSMFTEntry *diro, struct DirEntry *de, ULONG **countptr)
{
    ULONG *count = NULL;
    UBYTE *np;
    int ns_len;

    D(bug("[NTFS]: %s(NTFSMFTEntry @ 0x%p)\n", __PRETTY_FUNCTION__, diro));

    if (countptr)
    {
	count = *countptr;
	D(bug("[NTFS] %s: counter @ 0x%p, val = %d\n", __PRETTY_FUNCTION__, count, *count));
    }

    while (1)
    {
	UBYTE ns_type;

	D(bug("[NTFS] %s: pos = 0x%p\n", __PRETTY_FUNCTION__, de->key->pos));
	
	
	if (de->key->pos >= de->key->indx + (diro->data->idx_size << SECTORSIZE_SHIFT))
	{
	    D(bug("[NTFS] %s: reached index record buffer end\n", __PRETTY_FUNCTION__));
	    de->key->pos = 0;
	    return 0;
	}

	if (de->key->pos[0xC] & INDEX_ENTRY_END)		/* end of index signature */
	{
	    D(bug("[NTFS] %s: reached index-record end signature\n", __PRETTY_FUNCTION__));
	    de->key->pos = 0;
	    return 0;
	}

	np = de->key->pos + 0x50;
	ns_len = (UBYTE) *(np++);
	ns_type = *(np++);

	D(bug("[NTFS] %s: np = 0x%p, ns_len = %d (type = %d)\n", __PRETTY_FUNCTION__, np, ns_len, ns_type));

	/*  ignore DOS namespace files (we want Win32 versions) */
	if ((ns_len) && (ns_type != 2))
	{
	    int i;

	    if (AROS_LE2WORD(*((UWORD *)(de->key->pos + 4))))
	    {
		D(bug("[NTFS] %s: **skipping** [64bit mft number]\n", __PRETTY_FUNCTION__));
		    return 0;
	    }

	    D(bug("[NTFS] %s: type = %d\n", __PRETTY_FUNCTION__, AROS_LE2LONG(*((ULONG *)(de->key->pos + 0x48)))));

	    if (de)
	    {
		FreeVec(de->entryname);
		de->entryname = NULL;
	    }

	    if (de && de->data)
	    {
		if (count)
		    *count += 1;

		if (!de->entry)
		    de->entry = AllocMem(sizeof (struct NTFSMFTEntry), MEMF_ANY|MEMF_CLEAR);

		if (!de->entry)
		{
		    D(bug("[NTFS] %s: failed to allocate NTFSMFTEntry\n", __PRETTY_FUNCTION__));
		    return ERROR_NO_FREE_STORE;
		}

		de->entry->data = diro->data;
		
		de->entry->mftrec_no = AROS_LE2LONG(*(ULONG *)de->key->pos);

		de->entrytype = AROS_LE2LONG(*((ULONG *)(de->key->pos + 0x48)));
		
		if ((de->entryname = AllocVec(ns_len + 1, MEMF_ANY)) == NULL)
		    return ERROR_NO_FREE_STORE;

		for (i = 0; i < ns_len; i++)
		{
		    de->entryname[i] = glob->from_unicode[AROS_LE2WORD(*((UWORD *)(np + (i * 2))))];
		}
		de->entryname[ns_len] = '\0';

		D(
		    bug("[NTFS] %s: ", __PRETTY_FUNCTION__);
		    if (count)
		    {
			bug("[#%d]", *count);
		    }
		    bug(" Label '%s'\n", de->entryname);
		)

		if ((!count) || ((count) && (*count == de->no)))
		    return 1;
	    }
	}
	de->key->pos += AROS_LE2WORD(*((UWORD *)(de->key->pos + 8)));
    }
    return 0;
}

int bitcount(ULONG n)
{ 
   register unsigned int tmp; 
   tmp = n - ((n >> 1) & 033333333333) 
           - ((n >> 2) & 011111111111); 
   return ((tmp + (tmp >> 3)) & 030707070707) % 63; 
} 

LONG ReadBootSector(struct FSData *fs_data )
{
    struct DosEnvec *de = BADDR(glob->fssm->fssm_Environ);
    LONG err;
    ULONG bsize = de->de_SizeBlock * 4;
    struct NTFSBootSector *boot;
    BOOL invalid = FALSE;
    int i;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    boot = AllocMem(bsize, MEMF_ANY);
    if (!boot)
	return ERROR_NO_FREE_STORE;

    /*
     * Read the boot sector. We go direct because we don't have a cache yet,
     * and can't create one until we know the sector size, which is held in
     * the boot sector. In practice it doesn't matter - we're going to use
     * this once and once only.
     */
    fs_data->first_device_sector =
        de->de_BlocksPerTrack * de->de_Surfaces * de->de_LowCyl;

    D(bug("[NTFS] %s: trying bootsector at sector %ld (%ld bytes)\n", __PRETTY_FUNCTION__, fs_data->first_device_sector, bsize));

    if ((err = AccessDisk(FALSE, fs_data->first_device_sector, 1, bsize, (UBYTE *)boot)) != 0) {
        D(bug("[NTFS] %s: failed to read boot block (%ld)\n", __PRETTY_FUNCTION__, err));
	FreeMem(boot, bsize);
        return err;
    }

    /* check for  NTFS signature */
    if (boot->oem_name[0] != 'N' || boot->oem_name[1] != 'T' || boot->oem_name[2] != 'F' || boot->oem_name[3] != 'S')
        invalid = TRUE;

    if (invalid)
    {
        D(bug("[NTFS] %s: invalid NTFS bootsector\n", __PRETTY_FUNCTION__));
	FreeMem(boot, bsize);
        return ERROR_NOT_A_DOS_DISK;
    }
    
    D(bug("[NTFS] %s: NTFSBootsector:\n", __PRETTY_FUNCTION__));

    fs_data->sectorsize = AROS_LE2WORD(boot->bytes_per_sector);
    fs_data->sectorsize_bits = ilog2(fs_data->sectorsize);
    D(bug("[NTFS] %s:\tSectorSize = %ld\n", __PRETTY_FUNCTION__, fs_data->sectorsize));
    D(bug("[NTFS] %s:\tSectorSize Bits = %ld\n", __PRETTY_FUNCTION__, fs_data->sectorsize_bits));

    fs_data->cluster_sectors = boot->sectors_per_cluster;
    fs_data->clustersize = fs_data->sectorsize * fs_data->cluster_sectors;
    fs_data->clustersize_bits = ilog2(fs_data->clustersize);
    fs_data->cluster_sectors_bits = fs_data->clustersize_bits - fs_data->sectorsize_bits;

    D(bug("[NTFS] %s:\tSectorsPerCluster = %ld\n", __PRETTY_FUNCTION__, fs_data->cluster_sectors));
    D(bug("[NTFS] %s:\tClusterSize = %ld\n", __PRETTY_FUNCTION__, fs_data->clustersize));
    D(bug("[NTFS] %s:\tClusterSize Bits = %ld\n", __PRETTY_FUNCTION__, fs_data->clustersize_bits));
    D(bug("[NTFS] %s:\tCluster Sectors Bits = %ld\n", __PRETTY_FUNCTION__, fs_data->cluster_sectors_bits));

    fs_data->total_sectors = AROS_LE2QUAD(boot->number_of_sectors);

    D(bug("[NTFS] %s:\tVolumeSize in sectors = %ld\n", __PRETTY_FUNCTION__, fs_data->total_sectors));
    D(bug("[NTFS] %s:\t                in bytes = %ld\n", __PRETTY_FUNCTION__, fs_data->total_sectors * fs_data->sectorsize));
#if (0)
/* Warning : TODO - check the volume /drive can be properly accessed */
    if ((fs_data->first_device_sector + fs_data->total_sectors - 1 > end) && (glob->readcmd == CMD_READ))
    {
	D(bug("[NTFS] %s: volume is too large\n", __PRETTY_FUNCTION__));
	FreeMem(boot, bsize);
	return IOERR_BADADDRESS;
    }
#endif

    fs_data->cache = Cache_CreateCache(64, 64, fs_data->sectorsize);

    D(bug("[NTFS] %s: allocated cache @ 0x%p (64,64,%d)\n", __PRETTY_FUNCTION__, fs_data->cache, fs_data->sectorsize));

    if (boot->clusters_per_mft_record > 0)
	fs_data->mft_size = fs_data->cluster_sectors * boot->clusters_per_mft_record;
    else
	fs_data->mft_size = 1 << (-boot->clusters_per_mft_record - SECTORSIZE_SHIFT);

    D(bug("[NTFS] %s:\tMFTRecordSize = %ld (%ld clusters per record)\n", __PRETTY_FUNCTION__, fs_data->mft_size, boot->clusters_per_mft_record));

    if (boot->clusters_per_index_record > 0)
	fs_data->idx_size = fs_data->cluster_sectors * boot->clusters_per_index_record;
    else
	fs_data->idx_size = 1 << (-boot->clusters_per_index_record - SECTORSIZE_SHIFT);

    D(bug("[NTFS] %s:\tIndexRecordSize = %ld (%ld clusters per index)\n", __PRETTY_FUNCTION__, fs_data->idx_size, boot->clusters_per_index_record));

    fs_data->mft_start = AROS_LE2QUAD(boot->mft_lcn) * fs_data->cluster_sectors;

    D(bug("[NTFS] %s:\tMFTStart = %ld\n", __PRETTY_FUNCTION__, fs_data->mft_start));

    fs_data->mft.buf = AllocMem(fs_data->mft_size * fs_data->sectorsize, MEMF_ANY);
    if (!fs_data->mft.buf)
    {
	FreeMem(boot, bsize);
	return ERROR_NO_FREE_STORE;
    }

    /* Warning: todo - UUID stored in LE format- do we need to convert it? */
    boot->volume_serial_number = AROS_LE2QUAD(boot->volume_serial_number);
    UUID_Copy((const uuid_t *)&boot->volume_serial_number, (uuid_t *)&fs_data->uuid);

    D(
        char uuid_str[UUID_STRLEN + 1];
        uuid_str[UUID_STRLEN] = 0;

	/* convert UUID into human-readable format */
	UUID_Unparse(&fs_data->uuid, uuid_str);

        bug("[NTFS] %s:\tVolumeSerial = %s\n", __PRETTY_FUNCTION__, uuid_str);
    )

    for (i = 0; i < fs_data->mft_size; i++)
    {
	if ((fs_data->mft.cblock = Cache_GetBlock(fs_data->cache, fs_data->first_device_sector + fs_data->mft_start + i, &fs_data->mft.cbuf)) == NULL)
	{
	    err = IoErr();
	    D(bug("[NTFS] %s: failed to read MFT (error:%ld)\n", __PRETTY_FUNCTION__, err));
	    FreeMem(fs_data->mft.buf, fs_data->mft_size * fs_data->sectorsize);
	    FreeMem(boot, bsize);
	    return err;
	}
	CopyMem(fs_data->mft.cbuf, fs_data->mft.buf + (i * fs_data->sectorsize), fs_data->sectorsize);
	Cache_FreeBlock(fs_data->cache, fs_data->mft.cblock);
	fs_data->mft.cblock = NULL;
    }

#if defined(DEBUG_MFT)
    D(
	int dumpx;

	bug("[NTFS] %s: MFTRecord Dump -:\n", __PRETTY_FUNCTION__);
	bug("[NTFS] %s: MFTRecord buf @ 0x%p, size %d x %d", __PRETTY_FUNCTION__, fs_data->mft.buf, fs_data->mft_size, fs_data->sectorsize);

	for (dumpx = 0; dumpx < (fs_data->mft_size * fs_data->sectorsize) ; dumpx ++)
	{
	    if ((dumpx%16) == 0)
	    {
		bug("\n[NTFS] %s:\t%03x:", __PRETTY_FUNCTION__, dumpx);
	    }
	    bug(" %02x", ((UBYTE*)fs_data->mft.buf)[dumpx]);
	}
	bug("\n");
     )
#endif

    fs_data->mft.data = fs_data; 
    if (PostProcessMFTRecord (fs_data, (struct MFTRecordEntry *)fs_data->mft.buf, fs_data->mft_size, "FILE"))
    {
	FreeMem(fs_data->mft.buf, fs_data->mft_size * fs_data->sectorsize);
	FreeMem(boot, bsize);
	return ERROR_NO_FREE_STORE;
    }

#if defined(DEBUG_MFT)
    D(
	bug("[NTFS] %s: MFTRecord Dump (Post Processing) -:\n", __PRETTY_FUNCTION__);
	bug("[NTFS] %s: MFTRecord buf @ 0x%p, size %d x %d", __PRETTY_FUNCTION__, fs_data->mft.buf, fs_data->mft_size, fs_data->sectorsize);

	for (dumpx = 0; dumpx < (fs_data->mft_size * fs_data->sectorsize) ; dumpx ++)
	{
	    if ((dumpx%16) == 0)
	    {
		bug("\n[NTFS] %s:\t%03x:", __PRETTY_FUNCTION__, dumpx);
	    }
	    bug(" %02x", ((UBYTE*)fs_data->mft.buf)[dumpx]);
	}
	bug("\n");
     )
#endif

    if (!MapMFTAttrib(&fs_data->mft.attr, &fs_data->mft, AT_DATA))
    {
	D(bug("[NTFS] %s: no $DATA in MFT\n", __PRETTY_FUNCTION__));
	FreeMem(fs_data->mft.buf, fs_data->mft_size * fs_data->sectorsize);
	FreeMem(boot, bsize);
	return ERROR_NO_FREE_STORE;
    }

    struct DirHandle dh;
    dh.ioh.mft.buf = NULL;
    dh.ioh.mft.mftrec_no = FILE_ROOT;
    InitDirHandle(fs_data, &dh, FALSE);

    struct DirEntry dir_entry;
    memset(&dir_entry, 0, sizeof(struct DirEntry));
    dir_entry.data = fs_data;
    while ((err = GetDirEntry(&dh, dh.cur_no + 1, &dir_entry)) == 0) 
    {
	struct MFTAttr *attrentry;

	if (strcmp(dir_entry.entryname, "$MFT") == 0)
	{
	    D(bug("[NTFS] %s: ## found $MFT entry\n", __PRETTY_FUNCTION__));

	    INIT_MFTATTRIB(&dir_entry.entry->attr, dir_entry.entry);
	    attrentry = FindMFTAttrib(&dir_entry.entry->attr, AT_STANDARD_INFORMATION);
	    if ((attrentry) && (attrentry->residentflag == ATTR_RESIDENT_FORM) && (AROS_LE2LONG(attrentry->data.resident.value_length) > 0))
	    {
		attrentry = (struct MFTAttr *)((IPTR)attrentry + AROS_LE2WORD(attrentry->data.resident.value_offset));

		D(bug("[NTFS] %s: nfstime     = %d\n", __PRETTY_FUNCTION__, *(UQUAD *)attrentry));

		NTFS2DateStamp((UQUAD *)attrentry, &fs_data->volume.create_time);

		D(bug("[NTFS] %s:\tVolumeDate: %ld days, %ld, minutes, %ld ticks \n", __PRETTY_FUNCTION__, fs_data->volume.create_time.ds_Days, fs_data->volume.create_time.ds_Minute, fs_data->volume.create_time.ds_Tick));
	    }
	}
	else if (strcmp(dir_entry.entryname, "$Volume") == 0)
	{
	    D(bug("[NTFS] %s: ## found $Volume label entry\n", __PRETTY_FUNCTION__));

	    INIT_MFTATTRIB(&dir_entry.entry->attr, dir_entry.entry);
	    attrentry = FindMFTAttrib(&dir_entry.entry->attr, AT_VOLUME_NAME);
	    if ((attrentry) && (attrentry->residentflag == ATTR_RESIDENT_FORM) && (AROS_LE2LONG(attrentry->data.resident.value_length) > 0))
	    {
		int i;
		fs_data->volume.name[0] = (UBYTE)(AROS_LE2LONG(attrentry->data.resident.value_length) / 2) + 1;
		attrentry = (struct MFTAttr *)((IPTR)attrentry + AROS_LE2WORD(attrentry->data.resident.value_offset));

		if (fs_data->volume.name[0] > 30)
		    fs_data->volume.name[0] = 30;

		for (i = 0; i < fs_data->volume.name[0]; i++)
		{
		    fs_data->volume.name[i + 1] = glob->from_unicode[AROS_LE2WORD(*((UWORD *)((IPTR)attrentry + (i * 2))))];
		}
		fs_data->volume.name[fs_data->volume.name[0]] = '\0';

		D(bug("[NTFS] %s:\tVolumeLabel = '%s'\n", __PRETTY_FUNCTION__, &fs_data->volume.name[1]));
	    }
	}
	else if (strcmp(dir_entry.entryname, "$Bitmap") == 0)
	{
	    struct NTFSMFTAttr bitmapatrr;
	    UBYTE *MFTBitmap;
	    int i, allocated = 0;

	    D(bug("[NTFS] %s: ## found $Bitmap entry\n", __PRETTY_FUNCTION__));
	    D(bug("[NTFS] %s: ## size = %u\n", __PRETTY_FUNCTION__, dir_entry.entry->size));
	    
	    MFTBitmap = AllocVec(dir_entry.entry->size, MEMF_ANY);

	    INIT_MFTATTRIB(&bitmapatrr, dir_entry.entry);
	    if (MapMFTAttrib (&bitmapatrr, dir_entry.entry, AT_DATA))
	    {
		if (ReadMFTAttrib(&bitmapatrr, MFTBitmap, 0, dir_entry.entry->size, 0) == 0)
		{
		    D(bug("[NTFS] %s: read $Bitmap into buffer @ 0x%p\n", __PRETTY_FUNCTION__, MFTBitmap));
		    for (i = 0; i < (dir_entry.entry->size / 4); i++)
		    {
			allocated += bitcount(*(ULONG *)(MFTBitmap + (i * 4)));
		    }
		    D(bug("[NTFS] %s: allocated = %u\n", __PRETTY_FUNCTION__, allocated));
		    fs_data->used_sectors = allocated * fs_data->cluster_sectors;
		}
	    }
	}
    }

    if (fs_data->volume.name[0] == '\0')
    {
	char tmp[UUID_STRLEN + 1];
	int t = 0;
	UUID_Unparse(&fs_data->uuid, tmp);
	for (i = 0; i < UUID_STRLEN; i++)
	{
	    if (tmp[i] == '-')
	    {
		tmp[t++] = tmp[i + 1];
		i ++;
	    }
	    else
		t++;
	}
	CopyMem(tmp, fs_data->volume.name, 30);
	fs_data->volume.name[31] = '\0';
    }

    bug("[NTFS] %s: successfully detected NTFS Filesystem.\n", __PRETTY_FUNCTION__);

    FreeMem(boot, bsize);
    return 0;
}

void FreeBootSector(struct FSData *fs_data)
{
    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    D(bug("[NTFS] %s: removing NTFSBootsector from memory\n", __PRETTY_FUNCTION__));

    Cache_DestroyCache(fs_data->cache);
}
