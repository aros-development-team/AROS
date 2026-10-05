/*
 * ntfs.handler - New Technology FileSystem handler
 *
 * Copyright (C) 2012-2026 The AROS Development Team
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

#include <stddef.h>
#include <string.h>
#include <ctype.h>

#include "ntfs_fs.h"
#include "ntfs_constants.h"
#include "ntfs_protos.h"

#if defined(__has_builtin)
#  if __has_builtin(__builtin_popcountl)
#    define HAVE_BUILTIN_POPCOUNTL 1
#  endif
#endif

#ifndef HAVE_BUILTIN_POPCOUNTL
#define NTFSFS_USEBCHACK
#endif

//#define DEBUG_MFT
#include "debug.h"

extern struct Globals *glob;

/* Validation helper */
static BOOL ValidateMFTRecord(struct MFTRecordEntry *record, ULONG expected_size)
{
    if (record == NULL || expected_size < sizeof(*record))
        return FALSE;

    if (memcmp(record->header.magic, "FILE", 4) != 0)
        return FALSE;

    if (AROS_LE2LONG(record->bytes_in_use) > expected_size ||
            AROS_LE2LONG(record->bytes_allocated) > expected_size)
        return FALSE;

    if (AROS_LE2LONG(record->bytes_in_use) < sizeof(*record) ||
        AROS_LE2WORD(record->attrs_offset) < offsetof(struct MFTRecordEntry, reserved) ||
        AROS_LE2WORD(record->attrs_offset) >= AROS_LE2LONG(record->bytes_in_use))
        return FALSE;

    return TRUE;
}

static BOOL NTFSAttributeFits(struct NTFSMFTAttr *at, struct MFTAttr *attr)
{
    UBYTE *base;
    ULONG record_size, used, offset, length, header_size, value_offset;
    UQUAD address;
    if (at == NULL || at->mft == NULL || at->mft->data == NULL || attr == NULL)
        return FALSE;
    record_size = at->mft->data->mft_size << SECTORSIZE_SHIFT;
    address = (UQUAD)(IPTR)attr;
    base = at->mft->buf;
    if (base == NULL || address < (UQUAD)(IPTR)base ||
        address - (UQUAD)(IPTR)base >= record_size)
        base = (UBYTE *)at->emft_buf;
    if (base == NULL || address < (UQUAD)(IPTR)base ||
        address - (UQUAD)(IPTR)base >= record_size ||
        !ValidateMFTRecord((struct MFTRecordEntry *)base, record_size))
        return FALSE;
    used = AROS_LE2LONG(((struct MFTRecordEntry *)base)->bytes_in_use);
    offset = address - (UQUAD)(IPTR)base;
    if (offset > used || used - offset < 0x10)
        return FALSE;
    length = AROS_LE2LONG(attr->length);
    header_size = attr->residentflag == ATTR_RESIDENT_FORM ? 0x18 : 0x40;
    if (attr->residentflag > ATTR_NONRESIDENT_FORM ||
        length < header_size || length > used - offset)
        return FALSE;
    if (attr->attrname_length != 0 &&
        (AROS_LE2WORD(attr->attrname_offset) < header_size ||
         AROS_LE2WORD(attr->attrname_offset) > length ||
         2 * (ULONG)attr->attrname_length > length - AROS_LE2WORD(attr->attrname_offset)))
        return FALSE;
    if (attr->residentflag == ATTR_RESIDENT_FORM)
    {
        value_offset = AROS_LE2WORD(attr->data.resident.value_offset);
        if (value_offset < header_size || value_offset > length ||
            AROS_LE2LONG(attr->data.resident.value_length) > length - value_offset)
            return FALSE;
    }
    else
    {
        value_offset = AROS_LE2WORD(attr->data.non_resident.mapping_pairs_offset);
        if (value_offset < header_size || value_offset >= length)
            return FALSE;
    }
    return TRUE;
}

static ULONG NTFSListEntryLength(struct MFTAttr *entry, struct MFTAttr *end)
{
    UBYTE *p = (UBYTE *)entry;
    ULONG length;
    if (p >= (UBYTE *)end || (UBYTE *)end - p < 0x1a)
        return 0;
    length = AROS_LE2WORD(*(UWORD *)(p + 4));
    if (length < 0x1a || length > (UBYTE *)end - p ||
        (p[6] != 0 && (p[7] < 0x1a || p[7] > length || 2 * (ULONG)p[6] > length - p[7])))
        return 0;
    return length;
}

ULONG PostProcessMFTRecord(struct FSData *fs_data, struct MFTRecordEntry *record, int len, UBYTE *magic)
{
    UWORD seqarray_len, seqnum;
    ULONG bytes, offset, count;
    UBYTE *seqarray, *buf;

    buf = (UBYTE *)record;

    D(bug("[NTFS]: %s(%.4s)\n", __func__, magic));

    if (record == NULL || magic == NULL || fs_data == NULL) {
        D(bug("[NTFS] %s: NULL pointer passed\n", __func__));
        return ERROR_REQUIRED_ARG_MISSING;
    }

    if (len <= 0 || fs_data->sectorsize < sizeof(struct MFTRecordMSH) ||
        (ULONG)len > 0xffffffffUL / fs_data->sectorsize)
        return ERROR_OBJECT_WRONG_TYPE;
    bytes = (ULONG)len * fs_data->sectorsize;
    offset = AROS_LE2WORD(record->header.usa_offset);
    count = AROS_LE2WORD(record->header.usa_count);
    if (offset < sizeof(struct MFTRecordMSH) || (offset & 1) || offset > bytes ||
        count < 2 || count > (bytes - offset) / sizeof(UWORD))
        return ERROR_OBJECT_WRONG_TYPE;

    /* Perform post-read MST fixup by applying the sequence array to acquired blocks */

    D(bug("[NTFS] %s: FSData @ 0x%p\n", __func__, fs_data));
    D(bug("[NTFS] %s: MFTRecordEntry @ 0x%p\n", __func__, record));

    if (memcmp(record->header.magic, magic, 4)) {
        D(
            bug("[NTFS] %s: record magic mismatch (got '%.4s')\n", __func__, record->header.magic);
        )
        return ERROR_OBJECT_WRONG_TYPE ;
    }

    seqarray_len = AROS_LE2WORD(record->header.usa_count) - 1;

    if (seqarray_len != len) {
        D(bug("[NTFS] %s: fixup error - sequence array size (%d) != record size (%d)\n",
              __func__, seqarray_len, len));
        return ERROR_NOT_IMPLEMENTED;
    }

    if (seqarray_len == 0) {
        D(bug("[NTFS] %s: fixup error - sequence array size != record size\n", __func__));
        return  ERROR_NOT_IMPLEMENTED;
    }

    seqarray = (char *)record + AROS_LE2WORD(record->header.usa_offset);
    seqnum = AROS_LE2WORD(*((UWORD*)seqarray));

    D(bug("[NTFS] %s: update sequence = %u (usa_offset %u)\n", __func__, seqnum, AROS_LE2WORD(record->header.usa_offset)));

    while (seqarray_len > 0) {
        buf += fs_data->sectorsize;
        seqarray += 2;
        if (AROS_LE2WORD(*((UWORD*)(buf - 2))) != seqnum) {
            D(bug("[NTFS] %s: update sequence mismatch  @ 0x%p (%u != %u)\n", __func__, buf, AROS_LE2WORD(*((UWORD*)buf)), seqnum));
            return ERROR_NOT_IMPLEMENTED;
        }

        *((UWORD*)(buf - 2)) = *((UWORD*)seqarray);
        seqarray_len--;
    }

    D(bug("[NTFS] %s: record fixup complete\n", __func__));

    return 0;
}

ULONG PreProcessMFTRecord(struct FSData *fs_data, struct MFTRecordEntry *record, int len)
{
    D(bug("[NTFS]: %s(MFTRecordEntry @ 0x%p)\n", __func__, record));

    /* Perform pre-write MST fixup.  set the sequence numbers of blocks */

    return 0;
}

struct MFTAttr *GetMappingPairPos(UBYTE *mappos, int nn, UQUAD *val, int sig)
{
    UQUAD pos = 0;

    D(bug("[NTFS]: %s()\n", __func__));

    if (mappos == NULL || val == NULL || nn < 0 || nn > (int)sizeof(UQUAD)) {
        D(bug("[NTFS] %s: invalid parameters\n", __func__));
        return NULL;
    }

    {
        int i;
        for (i = 0; i < nn; i++)
            pos |= (UQUAD)mappos[i] << (8 * i);
        if (sig && nn != 0 && nn < (int)sizeof(UQUAD) && (mappos[nn - 1] & 0x80))
            pos |= (~(UQUAD)0) << (8 * nn);
        mappos += nn;
    }

    *val = pos;
    return (struct MFTAttr *)mappos;
}

IPTR ReadNTFSRunList(struct NTFSRunLstEntry * rle)
{
    int len, offs;
    UQUAD val;
    struct MFTAttr *mappos;

    if (rle == NULL || rle->mappingpair == NULL) {
        D(bug("[NTFS] %s: NULL runlist entry or mapping pair\n", __func__));
        return ~0;
    }

    mappos = (struct MFTAttr *)rle->mappingpair;
    D(bug("[NTFS]: %s(mappos @ 0x%p)\n", __func__, mappos));

retry:
    if (rle->mappingend == NULL || (UBYTE *)mappos >= rle->mappingend)
        return ~0;
    len = (*(UBYTE *)mappos & 0xF);
    offs = (*(UBYTE *)mappos >> 4);

    D(bug("[NTFS] %s: len = %u\n", __func__, len));
    D(bug("[NTFS] %s: offs = %u\n", __func__, offs));

    if (!len) {
        D(bug("[NTFS] %s: !len\n", __func__));
        if ((rle->attr) && (rle->attr->flags & AF_ALST)) {
            D(bug("[NTFS] %s: AF_ALST\n", __func__));

            mappos = FindMFTAttrib(rle->attr, *(UBYTE *)rle->attr->attr_cur);

            if (mappos) {
                D(bug("[NTFS] %s: 'RUN'\n", __func__));
                if (mappos->residentflag == ATTR_RESIDENT_FORM) {
                    D(bug("[NTFS] %s: $DATA should be non-resident\n", __func__));
                    return ~0;
                }

                if (!NTFSAttributeFits(rle->attr, mappos))
                    return ~0;
                rle->mappingend = (UBYTE *)mappos + AROS_LE2LONG(mappos->length);
                mappos = (struct MFTAttr *)((UBYTE *)mappos + AROS_LE2WORD(mappos->data.non_resident.mapping_pairs_offset));
                rle->curr_lcn = 0;
                goto retry;
            }
        }
        D(bug("[NTFS] %s: run list overflow\n", __func__));
        return ~0;
    }
    if (len > (int)sizeof(UQUAD) || offs > (int)sizeof(UQUAD) ||
        (ULONG)(1 + len + offs) > (ULONG)(rle->mappingend - (UBYTE *)mappos))
        return ~0;

    // current VCN  length
    mappos = GetMappingPairPos((UBYTE *)mappos + 1, len, &val, 0);
    if (mappos == NULL) {
        D(bug("[NTFS] %s: GetMappingPairPos failed for VCN\n", __func__));
        return ~0;
    }
    if (val == 0 || val > ~(UQUAD)0 - rle->next_vcn)
        return ~0;
    rle->curr_vcn = rle->next_vcn;
    rle->next_vcn = rle->next_vcn + val;

    D(bug("[NTFS] %s: curr_vcn = %u, next_vcn = %u, val = %u\n", __func__, (unsigned int)rle->curr_vcn, (unsigned int)rle->next_vcn, (unsigned int)val));

    // previous LCN offset
    mappos = GetMappingPairPos((UBYTE *)mappos, offs, &val, 1);
    if (mappos == NULL) {
        D(bug("[NTFS] %s: GetMappingPairPos failed for LCN\n", __func__));
        return ~0;
    }
    if (val & ((UQUAD)1 << 63)) {
        UQUAD distance = (UQUAD)0 - val;
        if (distance > rle->curr_lcn)
            return ~0;
        rle->curr_lcn -= distance;
    } else {
        if (val > ~(UQUAD)0 - rle->curr_lcn)
            return ~0;
        rle->curr_lcn += val;
    }

    D(bug("[NTFS] %s: curr_lcn = %u\n", __func__, (unsigned int)rle->curr_lcn));

    if (offs == 0)
        rle->flags |= RLEFLAG_SPARSE;
    else
        rle->flags &= ~RLEFLAG_SPARSE;

    rle->mappingpair = (UBYTE *)mappos;

    return 0;
}

void FreeMFTAttrib(struct NTFSMFTAttr *at)
{
    D(bug("[NTFS]: %s(NTFSMFTAttr @ 0x%p)\n", __func__, at));

    if (at == NULL)
        return;

    FreeVec(at->edat_buf);
    at->edat_buf = NULL;
    if (at->emft_buf) {
        FreeMem(at->emft_buf, at->mft->data->mft_size << SECTORSIZE_SHIFT);
        at->emft_buf = NULL;
    }
    if (at->sbuf) {
        FreeMem(at->sbuf, COM_LEN);
        at->sbuf = NULL;
    }
}

IPTR ReadMFTAttribData(struct NTFSMFTAttr *at, struct MFTAttr *attrentry, UBYTE *dest, UQUAD ofs, ULONG len, int cached)
{
    D(UQUAD vcn);
    struct NTFSRunLstEntry runlist_entry, *rle;

    D(
        bug("[NTFS]: %s(ofs = %u; len = %u)\n", __func__, (IPTR)ofs, len);

        bug("[NTFS] %s: NTFSMFTAttr @ 0x%p\n", __func__, at);
        bug("[NTFS] %s: MFTAttr @ 0x%p, dest @ 0x%p\n", __func__, attrentry, dest);
    )

    if (at == NULL || attrentry == NULL || dest == NULL) {
        D(bug("[NTFS] %s: NULL pointer parameter\n", __func__));
        return ~0;
    }

    if (len == 0)
        return 0;
    if (ofs > ~(UQUAD)0 - len)
        return ERROR_OBJECT_WRONG_TYPE;

    memset (&runlist_entry, 0, sizeof(struct NTFSRunLstEntry));
    rle = &runlist_entry;
    rle->attr = at;

    if (!NTFSAttributeFits(at, attrentry)) {
        D(bug("[NTFS] %s: error - invalid attribute length\n", __func__));
        return ~0;
    }

    if (attrentry->residentflag == ATTR_RESIDENT_FORM) {
        ULONG value_offset = AROS_LE2WORD(attrentry->data.resident.value_offset);
        ULONG value_length = AROS_LE2LONG(attrentry->data.resident.value_length);

        D(bug("[NTFS] %s: ATTR_RESIDENT_FORM\n", __func__));

        if (value_offset > AROS_LE2LONG(attrentry->length)) {
            D(bug("[NTFS] %s: error - value offset beyond attribute length\n", __func__));
            return ~0;
        }

        if (ofs > value_length || len > value_length - ofs) {
            D(bug("[NTFS] %s: error - read out of range\n", __func__));
            return ~0;
        }
        CopyMem((UBYTE *)attrentry + value_offset + ofs, dest, len);
        return 0;
    }

    if (AROS_LE2WORD(attrentry->attrflags) & FLAG_COMPRESSED) {
        rle->flags |= RLEFLAG_COMPR;
    } else {
        rle->flags &= ~RLEFLAG_COMPR;
    }
    rle->mappingpair = (UBYTE *)attrentry + AROS_LE2WORD(attrentry->data.non_resident.mapping_pairs_offset);
    rle->mappingend = (UBYTE *)attrentry + AROS_LE2LONG(attrentry->length);

    D(bug("[NTFS] %s: mappingpair @ 0x%p\n", __func__, rle->mappingpair));

    if (rle->flags & RLEFLAG_COMPR) {
        D(bug("[NTFS] %s: ## Compressed\n", __func__));
        if (!cached) {
            D(bug("[NTFS] %s: error - attribute cannot be compressed\n", __func__));
            return ~0;
        }

        if (at->sbuf) {
            if ((ofs & (~(COM_LEN - 1))) == at->save_pos) {
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
        } else {
            at->sbuf = AllocMem(COM_LEN, MEMF_ANY);
            if (at->sbuf == NULL) {
                D(bug("[NTFS] %s: error - failed to allocate sbuf\n", __func__));
                return ERROR_NO_FREE_STORE;
            }
            at->save_pos = 1;
        }

        D(vcn =) rle->target_vcn = (ofs >> COM_LOG_LEN) * (COM_SEC / at->mft->data->cluster_sectors);
        rle->target_vcn &= ~0xF;
    } else {
        rle->target_vcn = (ofs >> SECTORSIZE_SHIFT) / at->mft->data->cluster_sectors;
        D(vcn = rle->target_vcn);
    }

    rle->next_vcn = AROS_LE2QUAD(attrentry->data.non_resident.lowest_vcn);
    if (rle->target_vcn < rle->next_vcn)
        return ERROR_OBJECT_WRONG_TYPE;
    rle->curr_lcn = 0;

    D(bug("[NTFS] %s: vcn = %u\n", __func__, vcn));

    while (rle->next_vcn <= rle->target_vcn) {
        D(bug("[NTFS] %s: next_vcn = %u, target_vcn = %u\n", __func__, (IPTR)rle->next_vcn, (IPTR)rle->target_vcn));
        if (ReadNTFSRunList(rle)) {
            D(bug("[NTFS] %s: read_run_list failed\n", __func__));
            return ~0;
        }
    }

    D(bug("[NTFS] %s: next_vcn = %u\n", __func__, (IPTR)rle->next_vcn));

    if (at->flags & AF_GPOS) {
        UQUAD st0, st1, m;

        D(bug("[NTFS] %s: AF_GPOS\n", __func__));

        m = (ofs >> SECTORSIZE_SHIFT) % at->mft->data->cluster_sectors;

        st0 =
            (rle->target_vcn - rle->curr_vcn + rle->curr_lcn) * at->mft->data->cluster_sectors + m;
        st1 = st0 + 1;

        if (st1 ==
                (rle->next_vcn - rle->curr_vcn + rle->curr_lcn) * at->mft->data->cluster_sectors) {
            if (ReadNTFSRunList(rle)) {
                D(bug("[NTFS] %s: read_run_list failed\n", __func__));
                return ~0;
            }
            st1 = rle->curr_lcn * at->mft->data->cluster_sectors;
        }
        *((ULONG *)dest) = AROS_LONG2LE(st0);
        *((ULONG *)(dest + 4)) = AROS_LONG2LE(st1);
        return 0;
    }

    if (!(rle->flags & RLEFLAG_COMPR)) {
        ULONG remaining = len;
        while (remaining != 0) {
            UQUAD sector = ofs >> SECTORSIZE_SHIFT;
            UQUAD vcn = sector / at->mft->data->cluster_sectors;
            ULONG offset = ofs & (at->mft->data->sectorsize - 1);
            ULONG copy = at->mft->data->sectorsize - offset;
            if (copy > remaining)
                copy = remaining;
            while (vcn >= rle->next_vcn)
                if (ReadNTFSRunList(rle))
                    return ERROR_OBJECT_WRONG_TYPE;
            if (rle->flags & RLEFLAG_SPARSE) {
                memset(dest, 0, copy);
            } else {
                UQUAD cluster = vcn - rle->curr_vcn + rle->curr_lcn;
                UQUAD block = cluster * at->mft->data->cluster_sectors +
                    sector % at->mft->data->cluster_sectors;
                if (cluster >= at->mft->data->total_sectors / at->mft->data->cluster_sectors ||
                    block >= at->mft->data->total_sectors)
                    return ERROR_OBJECT_WRONG_TYPE;
                at->mft->cblock = Cache_GetBlock(at->mft->data->cache,
                    at->mft->data->first_device_sector + block, &at->mft->cbuf);
                if (at->mft->cblock == NULL)
                    return IoErr() ? IoErr() : ERROR_UNKNOWN;
                CopyMem(at->mft->cbuf + offset, dest, copy);
                Cache_FreeBlock(at->mft->data->cache, at->mft->cblock);
                at->mft->cblock = NULL;
            }
            dest += copy;
            ofs += copy;
            remaining -= copy;
        }
        return 0;
    }

    /* Warning : TODO - decompress block */
    D(bug("[NTFS] %s: cannot decompress\n", __func__));
    return ~0;
}

IPTR ReadMFTAttrib(struct NTFSMFTAttr *at, UBYTE *dest, UQUAD ofs, ULONG len, int cached)
{
    struct MFTAttr *save_cur;
    UBYTE attr;
    struct MFTAttr *attrentry;
    IPTR ret;

    D(bug("[NTFS]: %s(NTFSMFTAttr @ 0x%p; ofs = %d; len = %d)\n", __func__, at, (IPTR)ofs, len));

    if (at == NULL || dest == NULL || at->attr_cur == NULL) {
        D(bug("[NTFS] %s: NULL pointer parameter\n", __func__));
        return ~0;
    }

    save_cur = at->attr_cur;
    at->attr_nxt = at->attr_cur;
    attr = *(UBYTE *)at->attr_nxt;
    if (at->flags & AF_ALST) {
        UQUAD vcn;

        D(bug("[NTFS] %s: AF_ALST\n", __func__));

        vcn = ofs / (at->mft->data->cluster_sectors << SECTORSIZE_SHIFT);
        {
            ULONG step = NTFSListEntryLength(at->attr_nxt, at->attr_end);
            if (step == 0)
                return ERROR_OBJECT_WRONG_TYPE;
            attrentry = (struct MFTAttr *)((UBYTE *)at->attr_nxt + step);
            while ((UBYTE *)attrentry < (UBYTE *)at->attr_end) {
                step = NTFSListEntryLength(attrentry, at->attr_end);
                if (step == 0)
                    return ERROR_OBJECT_WRONG_TYPE;
                if (*(UBYTE *)attrentry != attr ||
                    AROS_LE2QUAD(*(UQUAD *)((UBYTE *)attrentry + 8)) > vcn)
                    break;
                at->attr_nxt = attrentry;
                attrentry = (struct MFTAttr *)((UBYTE *)attrentry + step);
            }
        }
    }
    attrentry = FindMFTAttrib(at, attr);
    if (attrentry)
        ret = ReadMFTAttribData(at, attrentry, dest, ofs, len, cached);
    else {
        D(bug("[NTFS] %s: attribute %u not found\n", __func__, attr));
        ret = ~0;
    }
    at->attr_cur = save_cur;
    return ret;
}

static IPTR ReadMFTRecord(struct NTFSMFTEntry *mft, UBYTE *buf, ULONG mft_id)
{
    IPTR err;

    D(bug("[NTFS]: %s(%d)\n", __func__, mft_id));

    if (mft == NULL || buf == NULL) {
        D(bug("[NTFS] %s: NULL pointer parameter\n", __func__));
        return ~0;
    }

    if (ReadMFTAttrib
            (&mft->data->mft.attr, buf, mft_id * ((UQUAD) mft->data->mft_size << SECTORSIZE_SHIFT),
             mft->data->mft_size << SECTORSIZE_SHIFT, 0)) {
        D(bug("[NTFS] %s: failed to read MFT #%d\n", __func__, mft_id));
        return ~0;
    }
#if defined(DEBUG_MFT)
    D(
        int dumpx;

        bug("[NTFS] %s: MFTRecord #%d Dump -:\n", __func__, mft_id);
        bug("[NTFS] %s: MFTRecord #%d buf @ 0x%p, size %d x %d", __func__, mft_id, buf, mft->data->mft_size, mft->data->sectorsize);

    for (dumpx = 0; dumpx < (mft->data->mft_size * mft->data->sectorsize) ; dumpx ++) {
    if ((dumpx%16) == 0) {
            bug("\n[NTFS] %s:\t%03x:", __func__, dumpx);
        }
        bug(" %02x", ((UBYTE*)buf)[dumpx]);
    }
    bug("\n");
    )
#endif
    err = PostProcessMFTRecord (mft->data, (struct MFTRecordEntry *)buf, mft->data->mft_size, "FILE");

    if (err == 0) {
        if (!ValidateMFTRecord((struct MFTRecordEntry *)buf, mft->data->mft_size << SECTORSIZE_SHIFT)) {
            D(bug("[NTFS] %s: MFT record validation failed\n", __func__));
            return ERROR_OBJECT_WRONG_TYPE;
        }
    }

    return err;
}

/* Bootstrap $MFT extension reads without following its attribute list. */
static struct MFTAttr *FindBaseMFTData(struct NTFSMFTAttr *at)
{
    UBYTE *base = at->mft->buf;
    UBYTE *end = base + AROS_LE2LONG(((struct MFTRecordEntry *)base)->bytes_in_use);
    struct MFTAttr *entry = at->attr_nxt;

    while ((UBYTE *)entry < end) {
        if ((ULONG)(end - (UBYTE *)entry) < sizeof(ULONG) ||
            AROS_LE2LONG(entry->type) == 0xffffffffUL ||
            !NTFSAttributeFits(at, entry))
            return NULL;
        if (AROS_LE2LONG(entry->type) == AT_DATA &&
            entry->residentflag == ATTR_NONRESIDENT_FORM &&
            AROS_LE2QUAD(entry->data.non_resident.lowest_vcn) == 0)
            return entry;
        entry = (struct MFTAttr *)((UBYTE *)entry + AROS_LE2LONG(entry->length));
    }
    return NULL;
}

struct MFTAttr *FindMFTAttrib(struct NTFSMFTAttr *at, UBYTE attr)
{
    struct MFTAttr *entry;
    ULONG record_size;
    UBYTE *base, *end;
    D(bug("[NTFS]: %s(attribute %u)\n", __func__, attr));
    if (at == NULL || at->mft == NULL || at->mft->data == NULL || at->mft->buf == NULL)
        return NULL;
    record_size = at->mft->data->mft_size << SECTORSIZE_SHIFT;
    base = at->mft->buf;
    if (!ValidateMFTRecord((struct MFTRecordEntry *)base, record_size))
        return NULL;
    end = base + AROS_LE2LONG(((struct MFTRecordEntry *)base)->bytes_in_use);

    if (!(at->flags & AF_ALST)) {
        while ((UBYTE *)at->attr_nxt >= base && (UBYTE *)at->attr_nxt < end) {
            entry = at->attr_nxt;
            if ((ULONG)(end - (UBYTE *)entry) < sizeof(ULONG))
                return NULL;
            if (AROS_LE2LONG(entry->type) == 0xffffffffUL)
                break;
            if (!NTFSAttributeFits(at, entry))
                return NULL;
            at->attr_cur = entry;
            at->attr_nxt = (struct MFTAttr *)((UBYTE *)entry + AROS_LE2LONG(entry->length));
            if (AROS_LE2LONG(entry->type) == AT_ATTRIBUTE_LIST)
                at->attr_end = entry;
            if (AROS_LE2LONG(entry->type) == attr || attr == 0)
                return entry;
        }
        entry = at->attr_end;
        if (entry == NULL || !NTFSAttributeFits(at, entry))
            return NULL;
        if (at->emft_buf == NULL)
            at->emft_buf = AllocMem(record_size, MEMF_ANY);
        if (at->emft_buf == NULL)
            return NULL;
        if (entry->residentflag == ATTR_NONRESIDENT_FORM) {
            UQUAD length = AROS_LE2QUAD(entry->data.non_resident.data_size);
            if (length == 0 || length > 256 * 1024)
                return NULL;
            at->edat_buf = AllocVec((ULONG)length, MEMF_ANY);
            if (at->edat_buf == NULL)
                return NULL;
            if (ReadMFTAttribData(at, entry, (UBYTE *)at->edat_buf, 0, (ULONG)length, 0))
                return NULL;
            at->attr_nxt = at->edat_buf;
            at->attr_end = (struct MFTAttr *)((UBYTE *)at->edat_buf + (ULONG)length);
        } else {
            at->attr_nxt = (struct MFTAttr *)((UBYTE *)entry + AROS_LE2WORD(entry->data.resident.value_offset));
            at->attr_end = (struct MFTAttr *)((UBYTE *)at->attr_nxt + AROS_LE2LONG(entry->data.resident.value_length));
        }
        at->flags |= AF_ALST;
    }
    while ((UBYTE *)at->attr_nxt < (UBYTE *)at->attr_end) {
        ULONG length = NTFSListEntryLength(at->attr_nxt, at->attr_end);
        UBYTE *list_entry = (UBYTE *)at->attr_nxt;
        UQUAD reference;
        if (length == 0)
            return NULL;
        at->attr_cur = at->attr_nxt;
        at->attr_nxt = (struct MFTAttr *)(list_entry + length);
        if (AROS_LE2LONG(*(ULONG *)list_entry) != attr && attr != 0)
            continue;
        reference = AROS_LE2QUAD(*(UQUAD *)(list_entry + 0x10)) & MFTREF_MASK;
        /* ReadMFTRecord's current API supports 32-bit MFT record numbers. */
        if (reference > 0xffffffffUL)
            return NULL;
        if (at->flags & AF_MMFT) {
            /* Bootstrap an extension record from the base MFT data extent.
             * Using this list again here would recursively read the same record. */
            struct NTFSMFTAttr base_at;
            struct MFTAttr *data;
            INIT_MFTATTRIB(&base_at, at->mft);
            data = FindBaseMFTData(&base_at);
            if (data == NULL)
                return NULL;
            if (ReadMFTAttribData(&base_at, data, (UBYTE *)at->emft_buf,
                reference * record_size, record_size, 0)) {
                FreeMFTAttrib(&base_at);
                return NULL;
            }
            FreeMFTAttrib(&base_at);
            if (PostProcessMFTRecord(at->mft->data,
                (struct MFTRecordEntry *)at->emft_buf, at->mft->data->mft_size, "FILE"))
                return NULL;
        } else if (ReadMFTRecord(at->mft, (UBYTE *)at->emft_buf, (ULONG)reference))
            return NULL;
        if (!ValidateMFTRecord((struct MFTRecordEntry *)at->emft_buf, record_size))
            return NULL;
        base = (UBYTE *)at->emft_buf;
        end = base + AROS_LE2LONG(((struct MFTRecordEntry *)base)->bytes_in_use);
        entry = (struct MFTAttr *)(base + AROS_LE2WORD(((struct MFTRecordEntry *)base)->attrs_offset));
        while ((UBYTE *)entry < end) {
            if ((ULONG)(end - (UBYTE *)entry) < sizeof(ULONG) || AROS_LE2LONG(entry->type) == 0xffffffffUL)
                break;
            if (!NTFSAttributeFits(at, entry))
                return NULL;
            if (AROS_LE2LONG(entry->type) == AROS_LE2LONG(*(ULONG *)list_entry) &&
                AROS_LE2WORD(entry->instance) == AROS_LE2WORD(*(UWORD *)(list_entry + 0x18)))
                return entry;
            entry = (struct MFTAttr *)((UBYTE *)entry + AROS_LE2LONG(entry->length));
        }
        return NULL;
    }
    return NULL;
}

struct MFTAttr *MapMFTAttrib(struct NTFSMFTAttr *at, struct NTFSMFTEntry *mft, UBYTE attr)
{
    struct MFTAttr *attrentry;

    D(bug("[NTFS]: %s(%ld)\n", __func__, attr));

    if (at == NULL || mft == NULL) {
        D(bug("[NTFS] %s: NULL pointer parameter\n", __func__));
        return NULL;
    }

    INIT_MFTATTRIB(at, mft);
    if ((attrentry = FindMFTAttrib(at, attr)) == NULL)
        return NULL;

    if ((at->flags & AF_ALST) == 0) {
        while (1) {
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

    D(bug("[NTFS]: %s(%ld)\n", __func__, mft_id));

    if (mft == NULL) {
        D(bug("[NTFS] %s: NULL MFT entry pointer\n", __func__));
        return ERROR_REQUIRED_ARG_MISSING;
    }

    mft->buf_filled = 1;

    if (mft->buf != NULL) {
        D(bug("[NTFS] %s: NTFSMFTEntry @ 0x%p in use? (mft->buf != NULL)\n", __func__, mft));
        return ~0;
    }

    mft->buf = AllocMem(mft->data->mft_size << SECTORSIZE_SHIFT, MEMF_ANY);
    if ((record = (struct MFTRecordEntry *)mft->buf) == NULL) {
        return ERROR_NO_FREE_STORE;
    }

    if (ReadMFTRecord(mft, mft->buf, mft_id)) {
        D(bug("[NTFS] %s: failed to read MFT #%d\n", __func__, mft_id));
        FreeMem(mft->buf, mft->data->mft_size << SECTORSIZE_SHIFT);
        mft->buf = NULL;
        return ~0;
    }

    flag = AROS_LE2WORD(record->flags);
    if ((flag & FILERECORD_SEGMENT_IN_USE) == 0) {
        D(bug("[NTFS] %s: MFT not in use!\n", __func__));
        FreeMem(mft->buf, mft->data->mft_size << SECTORSIZE_SHIFT);
        mft->buf = NULL;
        return ~0;
    }

    if ((flag & FILERECORD_NAME_INDEX_PRESENT) == 0) {
        struct MFTAttr *attrentry;

        attrentry = MapMFTAttrib(&mft->attr, mft, AT_DATA);
        if (attrentry == NULL) {
            D(bug("[NTFS] %s: No $DATA in MFT #%d\n", __func__, mft_id));
            FreeMem(mft->buf, mft->data->mft_size << SECTORSIZE_SHIFT);
            mft->buf = NULL;
            return ~0;
        }

        if (attrentry->residentflag == ATTR_RESIDENT_FORM) {
            mft->size = AROS_LE2LONG(*(ULONG *)((IPTR)attrentry + 0x10));
        } else {
            mft->size = AROS_LE2QUAD(*(UQUAD *)((IPTR)attrentry + 0x30));
        }

        if ((mft->attr.flags & AF_ALST) == 0)
            mft->attr.attr_end = 0;	/*  Don't jump to attribute list */
    } else {
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

    D(bug("[NTFS]: %s(NTFSMFTEntry @ %p)\n", __func__, (void *)diro));

    if (!diro || !de)
        return ERROR_REQUIRED_ARG_MISSING;

    if (countptr)
        count = *countptr;

    if (!de->key || !de->key->indx || !diro->data)
        return ERROR_INVALID_COMPONENT_NAME;

    UBYTE *idx_base = de->key->indx;
    size_t total_bytes = (size_t)diro->data->idx_size << SECTORSIZE_SHIFT;
    UBYTE *idx_end = idx_base + total_bytes;

    while (1) {
        if (!de->key->pos)
            return ERROR_INVALID_COMPONENT_NAME;

        if (de->key->pos >= idx_end || de->key->pos < idx_base) {
            de->key->pos = NULL;
            return 0;
        }

        if (de->key->pos[0xC] & INDEX_ENTRY_END) {
            de->key->pos = NULL;
            return 0;
        }

        if (de->key->pos + 8 + sizeof(UWORD) > idx_end)
            return ERROR_INVALID_COMPONENT_NAME;

        UWORD step_tmp;
        memcpy(&step_tmp, de->key->pos + 8, sizeof(UWORD));
        UWORD step = AROS_LE2WORD(step_tmp);
        if (!step)
            return ERROR_INVALID_COMPONENT_NAME;

        if (de->key->pos + 0x50 + 2 > idx_end) {
            de->key->pos += step;
            continue;
        }

        np = de->key->pos + 0x50;
        ns_len = (UBYTE)*np++;
        UBYTE ns_type = *np++;

        if (ns_len <= 0 || ns_len > 255) {
            de->key->pos += step;
            continue;
        }

        size_t name_bytes = (size_t)ns_len * 2;
        if (np + name_bytes > idx_end) {
            de->key->pos += step;
            continue;
        }

        if (ns_type != 2) {
            if (de->key->pos + 4 + sizeof(UWORD) > idx_end) {
                de->key->pos += step;
                continue;
            }

            UWORD mft_high_tmp;
            memcpy(&mft_high_tmp, de->key->pos + 4, sizeof(UWORD));
            if (AROS_LE2WORD(mft_high_tmp)) {
                de->key->pos += step;
                continue;
            }

            if (de->key->pos + 0x48 + sizeof(ULONG) > idx_end) {
                de->key->pos += step;
                continue;
            }

            ULONG entrytype_tmp;
            memcpy(&entrytype_tmp, de->key->pos + 0x48, sizeof(ULONG));
            ULONG entrytype = AROS_LE2LONG(entrytype_tmp);

            if (de->data) {
                if (count)
                    (*count)++;

                if (!de->entry)
                    de->entry = AllocMem(sizeof(struct NTFSMFTEntry), MEMF_ANY | MEMF_CLEAR);
                if (!de->entry)
                    return ERROR_NO_FREE_STORE;

                de->entry->data = diro->data;

                ULONG mftrec_tmp;
                memcpy(&mftrec_tmp, de->key->pos, sizeof(ULONG));
                de->entry->mftrec_no = AROS_LE2LONG(mftrec_tmp);
                de->entrytype = entrytype;

                char *entrynamestr = AllocVec(ns_len + 1, MEMF_ANY);
                if (!entrynamestr)
                    return ERROR_NO_FREE_STORE;

                if (de->entryname)
                    FreeVec(de->entryname);
                de->entryname = entrynamestr;

                for (int i = 0; i < ns_len; i++) {
                    UWORD code_tmp;
                    memcpy(&code_tmp, np + (i * 2), sizeof(UWORD));
                    UWORD unicode_char = AROS_LE2WORD(code_tmp);
                    if (glob && glob->from_unicode)
                        de->entryname[i] = glob->from_unicode[unicode_char];
                    else
                        de->entryname[i] = '?';
                }
                de->entryname[ns_len] = '\0';

                D(
                    bug("[NTFS] %s: ", __func__);
                    if (count)
                        bug("<#%lu>", (unsigned long)*count);
                    bug(" Label '%s'\n", de->entryname);
                )

                if ((!count) || (*count == de->no))
                    return 1;
            }
        }

        de->key->pos += step;
    }
    return 0;
}

static int bitcount(ULONG n)
{
#if defined(HAVE_BUILTIN_POPCOUNTL)
    return __builtin_popcount(n);
#else
#if defined(NTFSFS_USEBCHACK)
    unsigned long tmp;

    tmp = n - ((n >> 1) & 0x55555555UL)
            - ((n >> 2) & 0x11111111UL);
    return ((tmp + (tmp >> 3)) & 0x03070707UL) % 63;
#else
    int count = 0;
    while (n) {
        n &= (n - 1);
        count++;
    }
    return count;
#endif
#endif
}

static BOOL NTFSRecordSectors(BYTE encoded, ULONG cluster_sectors, ULONG *sectors)
{
    ULONG count;
    if (encoded > 0)
        count = cluster_sectors * (ULONG)encoded;
    else {
        int bits = -(int)encoded;
        if (bits < SECTORSIZE_SHIFT || bits >= 32)
            return FALSE;
        count = 1UL << (bits - SECTORSIZE_SHIFT);
    }
    if (count == 0 || count > (0xffffffffUL >> SECTORSIZE_SHIFT) ||
        (count & (count - 1)) != 0)
        return FALSE;
    *sectors = count;
    return TRUE;
}

LONG ReadBootSector(struct FSData *fs_data )
{
    struct DosEnvec *de = BADDR(glob->fssm->fssm_Environ);
    LONG err;
    ULONG bsize = de->de_SizeBlock * 4;
    struct NTFSBootSector *boot;
    UQUAD volserial;
    BOOL invalid = FALSE;
    int i;

    D(bug("[NTFS]: %s()\n", __func__));

    if (fs_data == NULL) {
        D(bug("[NTFS] %s: NULL fs_data pointer\n", __func__));
        return ERROR_REQUIRED_ARG_MISSING;
    }

    /* The rest of this handler currently uses fixed 512-byte sector offsets. */
    if (bsize != (1UL << SECTORSIZE_SHIFT))
        return ERROR_NOT_IMPLEMENTED;
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

    D(bug("[NTFS] %s: trying bootsector at sector %ld (%ld bytes)\n", __func__, fs_data->first_device_sector, bsize));

    if ((err = AccessDisk(FALSE, fs_data->first_device_sector, 1, bsize, (UBYTE *)boot)) != 0) {
        D(bug("[NTFS] %s: failed to read boot block (%ld)\n", __func__, err));
        goto cleanup;
    }

    /* check for  NTFS signature */
    if (boot->oem_name[0] != 'N' || boot->oem_name[1] != 'T' || boot->oem_name[2] != 'F' || boot->oem_name[3] != 'S')
        invalid = TRUE;

    if (invalid) {
        D(bug("[NTFS] %s: invalid NTFS bootsector\n", __func__));
        err = ERROR_NOT_A_DOS_DISK;
        goto cleanup;
    }

    D(bug("[NTFS] %s: NTFSBootsector:\n", __func__));

    fs_data->sectorsize = AROS_LE2WORD(boot->bytes_per_sector);
    if (fs_data->sectorsize != bsize) {
        D(bug("[NTFS] %s: invalid sector size %ld\n", __func__, fs_data->sectorsize));
        err = ERROR_NOT_A_DOS_DISK;
        goto cleanup;
    }

    fs_data->sectorsize_bits = ilog2(fs_data->sectorsize);
    D(bug("[NTFS] %s:\tSectorSize = %ld\n", __func__, fs_data->sectorsize));
    D(bug("[NTFS] %s:\tSectorSize Bits = %ld\n", __func__, fs_data->sectorsize_bits));

    fs_data->cluster_sectors = boot->sectors_per_cluster;
    if (fs_data->cluster_sectors == 0 || (fs_data->cluster_sectors & (fs_data->cluster_sectors - 1)) != 0) {
        D(bug("[NTFS] %s: invalid sectors per cluster\n", __func__));
        err = ERROR_NOT_A_DOS_DISK;
        goto cleanup;
    }

    fs_data->clustersize = fs_data->sectorsize * fs_data->cluster_sectors;
    fs_data->clustersize_bits = ilog2(fs_data->clustersize);
    fs_data->cluster_sectors_bits = fs_data->clustersize_bits - fs_data->sectorsize_bits;

    D(bug("[NTFS] %s:\tSectorsPerCluster = %ld\n", __func__, fs_data->cluster_sectors));
    D(bug("[NTFS] %s:\tClusterSize = %ld\n", __func__, fs_data->clustersize));
    D(bug("[NTFS] %s:\tClusterSize Bits = %ld\n", __func__, fs_data->clustersize_bits));
    D(bug("[NTFS] %s:\tCluster Sectors Bits = %ld\n", __func__, fs_data->cluster_sectors_bits));

    fs_data->total_sectors = AROS_LE2QUAD(boot->number_of_sectors);

    D(bug("[NTFS] %s:\tVolumeSize in sectors = %ld\n", __func__, fs_data->total_sectors));
    D(bug("[NTFS] %s:\t                in bytes = %ld\n", __func__, fs_data->total_sectors * fs_data->sectorsize));
    {
        UQUAD partition_sectors;
        if (de->de_HighCyl < de->de_LowCyl) {
            err = ERROR_NOT_A_DOS_DISK;
            goto cleanup;
        }
        partition_sectors = (UQUAD)de->de_BlocksPerTrack * de->de_Surfaces *
            ((UQUAD)de->de_HighCyl - de->de_LowCyl + 1);
        if (fs_data->total_sectors == 0 || fs_data->total_sectors > partition_sectors) {
            err = IOERR_BADADDRESS;
            goto cleanup;
        }
    }

    fs_data->cache = Cache_CreateCache(64, 64, fs_data->sectorsize);
    if (fs_data->cache == NULL) {
        D(bug("[NTFS] %s: failed to create cache\n", __func__));
        err = ERROR_NO_FREE_STORE;
        goto cleanup;
    }

    D(bug("[NTFS] %s: allocated cache @ 0x%p (64,64,%d)\n", __func__, fs_data->cache, fs_data->sectorsize));

    if (!NTFSRecordSectors(boot->clusters_per_mft_record, fs_data->cluster_sectors, &fs_data->mft_size) ||
        !NTFSRecordSectors(boot->clusters_per_index_record, fs_data->cluster_sectors, &fs_data->idx_size)) {
        err = ERROR_NOT_A_DOS_DISK;
        goto cleanup;
    }

    if (AROS_LE2QUAD(boot->mft_lcn) >= fs_data->total_sectors / fs_data->cluster_sectors) {
        err = IOERR_BADADDRESS;
        goto cleanup;
    }
    fs_data->mft_start = AROS_LE2QUAD(boot->mft_lcn) * fs_data->cluster_sectors;
    if (fs_data->mft_size > fs_data->total_sectors - fs_data->mft_start) {
        err = IOERR_BADADDRESS;
        goto cleanup;
    }

    D(bug("[NTFS] %s:\tMFTStart = %ld\n", __func__, fs_data->mft_start));

    fs_data->mft.buf = AllocMem(fs_data->mft_size * fs_data->sectorsize, MEMF_ANY);
    if (!fs_data->mft.buf) {
        err = ERROR_NO_FREE_STORE;
        goto cleanup;
    }

    volserial = AROS_LE2QUAD(boot->volume_serial_number);
    /* NTFS stores an eight-byte serial; a uuid_t is sixteen bytes. */
    memset(&fs_data->uuid, 0, sizeof(fs_data->uuid));
    CopyMem(&volserial, &fs_data->uuid, sizeof(volserial));

    D(
        char uuid_str[UUID_STRLEN + 1];
        uuid_str[UUID_STRLEN] = 0;

        /* convert UUID into human-readable format */
        UUID_Unparse(&fs_data->uuid, uuid_str);

        bug("[NTFS] %s:\tVolumeSerial = %s\n", __func__, uuid_str);
    )

    for (i = 0; i < fs_data->mft_size; i++) {
        if ((fs_data->mft.cblock = Cache_GetBlock(fs_data->cache, fs_data->first_device_sector + fs_data->mft_start + i, &fs_data->mft.cbuf)) == NULL) {
            err = IoErr();
            D(bug("[NTFS] %s: failed to read MFT (error:%ld)\n", __func__, err));
            goto cleanup;
        }
        CopyMem(fs_data->mft.cbuf, fs_data->mft.buf + (i * fs_data->sectorsize), fs_data->sectorsize);
        Cache_FreeBlock(fs_data->cache, fs_data->mft.cblock);
        fs_data->mft.cblock = NULL;
    }

#if defined(DEBUG_MFT)
    D(
        int dumpx;

        bug("[NTFS] %s: MFTRecord Dump -:\n", __func__);
        bug("[NTFS] %s: MFTRecord buf @ 0x%p, size %d x %d", __func__, fs_data->mft.buf, fs_data->mft_size, fs_data->sectorsize);

    for (dumpx = 0; dumpx < (fs_data->mft_size * fs_data->sectorsize) ; dumpx ++) {
    if ((dumpx%16) == 0) {
            bug("\n[NTFS] %s:\t%03x:", __func__, dumpx);
        }
        bug(" %02x", ((UBYTE*)fs_data->mft.buf)[dumpx]);
    }
    bug("\n");
    )
#endif

    fs_data->mft.data = fs_data;
    if ((err = PostProcessMFTRecord (fs_data, (struct MFTRecordEntry *)fs_data->mft.buf, fs_data->mft_size, "FILE")) != 0) {
        goto cleanup;
    }

#if defined(DEBUG_MFT)
    D(
        bug("[NTFS] %s: MFTRecord Dump (Post Processing) -:\n", __func__);
        bug("[NTFS] %s: MFTRecord buf @ 0x%p, size %d x %d", __func__, fs_data->mft.buf, fs_data->mft_size, fs_data->sectorsize);

    for (dumpx = 0; dumpx < (fs_data->mft_size * fs_data->sectorsize) ; dumpx ++) {
    if ((dumpx%16) == 0) {
            bug("\n[NTFS] %s:\t%03x:", __func__, dumpx);
        }
        bug(" %02x", ((UBYTE*)fs_data->mft.buf)[dumpx]);
    }
    bug("\n");
    )
#endif

    if (!ValidateMFTRecord((struct MFTRecordEntry *)fs_data->mft.buf,
        fs_data->mft_size << SECTORSIZE_SHIFT)) {
        err = ERROR_OBJECT_WRONG_TYPE;
        goto cleanup;
    }
    if (!MapMFTAttrib(&fs_data->mft.attr, &fs_data->mft, AT_DATA)) {
        D(bug("[NTFS] %s: no $DATA in MFT\n", __func__));
        err = ERROR_NO_FREE_STORE;
        goto cleanup;
    }

    struct DirHandle dh;
    dh.ioh.mft.buf = NULL;
    dh.ioh.mft.mftrec_no = FILE_ROOT;
    InitDirHandle(fs_data, &dh, FALSE);

    struct DirEntry dir_entry;
    memset(&dir_entry, 0, sizeof(struct DirEntry));
    dir_entry.data = fs_data;
    while ((err = GetDirEntry(&dh, dh.cur_no + 1, &dir_entry)) == 0) {
        struct MFTAttr *attrentry;

        if (strcmp(dir_entry.entryname, "$MFT") == 0) {
            D(bug("[NTFS] %s: ## found $MFT entry\n", __func__));

            INIT_MFTATTRIB(&dir_entry.entry->attr, dir_entry.entry);
            attrentry = FindMFTAttrib(&dir_entry.entry->attr, AT_STANDARD_INFORMATION);
            if ((attrentry) && (attrentry->residentflag == ATTR_RESIDENT_FORM) && (AROS_LE2LONG(attrentry->data.resident.value_length) > 0)) {
                UQUAD ntfstv;
                attrentry = (struct MFTAttr *)((IPTR)attrentry + AROS_LE2WORD(attrentry->data.resident.value_offset));
                ntfstv = *(UQUAD *)attrentry;

                D(bug("[NTFS] %s: nfstime     = %d\n", __func__, ntfstv));

                NTFS2DateStamp(&ntfstv, &fs_data->volume.create_time);

                D(bug("[NTFS] %s:\tVolumeDate: %ld days, %ld, minutes, %ld ticks \n", __func__, fs_data->volume.create_time.ds_Days, fs_data->volume.create_time.ds_Minute, fs_data->volume.create_time.ds_Tick));
            }
        } else if (strcmp(dir_entry.entryname, "$Volume") == 0) {
            D(bug("[NTFS] %s: ## found $Volume label entry\n", __func__));

            INIT_MFTATTRIB(&dir_entry.entry->attr, dir_entry.entry);
            attrentry = FindMFTAttrib(&dir_entry.entry->attr, AT_VOLUME_NAME);
            if ((attrentry) && (attrentry->residentflag == ATTR_RESIDENT_FORM) && (AROS_LE2LONG(attrentry->data.resident.value_length) > 0)) {
                int i;
                int name_len = AROS_LE2LONG(attrentry->data.resident.value_length) / 2;
                fs_data->volume.name[0] = (UBYTE)(name_len + 1);
                attrentry = (struct MFTAttr *)((IPTR)attrentry + AROS_LE2WORD(attrentry->data.resident.value_offset));

                if (fs_data->volume.name[0] > 30)
                    fs_data->volume.name[0] = 30;

                for (i = 0; i < fs_data->volume.name[0] - 1; i++) {
                    UWORD unicode_char = AROS_LE2WORD(*((UWORD *)((IPTR)attrentry + (i * 2))));
                    if (unicode_char <= 65535 && glob->from_unicode)
                        fs_data->volume.name[i + 1] = glob->from_unicode[unicode_char];
                    else
                        fs_data->volume.name[i + 1] = '?';
                }
                fs_data->volume.name[fs_data->volume.name[0]] = '\0';

                D(bug("[NTFS] %s:\tVolumeLabel = '%s'\n", __func__, &fs_data->volume.name[1]));
            }
        } else if (strcmp(dir_entry.entryname, "$Bitmap") == 0) {
            struct NTFSMFTAttr bitmapatrr;
            UBYTE *MFTBitmap;
            int i, allocated = 0;

            D(bug("[NTFS] %s: ## found $Bitmap entry\n", __func__));
            D(bug("[NTFS] %s: ## size = %u\n", __func__, dir_entry.entry->size));

            MFTBitmap = AllocVec(dir_entry.entry->size, MEMF_ANY);
            if (MFTBitmap == NULL) {
                D(bug("[NTFS] %s: failed to allocate bitmap buffer\n", __func__));
                continue;
            }

            INIT_MFTATTRIB(&bitmapatrr, dir_entry.entry);
            if (MapMFTAttrib (&bitmapatrr, dir_entry.entry, AT_DATA)) {
                if (ReadMFTAttrib(&bitmapatrr, MFTBitmap, 0, dir_entry.entry->size, 0) == 0) {
                    D(bug("[NTFS] %s: read $Bitmap into buffer @ 0x%p\n", __func__, MFTBitmap));
                    for (i = 0; i < (dir_entry.entry->size / 4); i++) {
                        allocated += bitcount(*(ULONG *)(MFTBitmap + (i * 4)));
                    }
                    D(bug("[NTFS] %s: allocated = %u\n", __func__, allocated));
                    fs_data->used_sectors = allocated * fs_data->cluster_sectors;
                }
            }
            FreeVec(MFTBitmap);
        }
    }

    if (fs_data->volume.name[0] == '\0') {
        char tmp[UUID_STRLEN + 1];
        int t = 0;
        UUID_Unparse(&fs_data->uuid, tmp);
        for (i = 0; i < UUID_STRLEN; i++) {
            if (tmp[i] == '-') {
                tmp[t++] = tmp[i + 1];
                i ++;
            } else
                t++;
        }
        tmp[t] = '\0';
        int copy_len = (t < 30) ? t : 30;
        CopyMem(tmp, fs_data->volume.name, copy_len);
        fs_data->volume.name[copy_len] = '\0';
    }

    bug("[NTFS] %s: successfully detected NTFS Filesystem.\n", __func__);

    FreeMem(boot, bsize);
    return 0;

cleanup:
    if (boot)
        FreeMem(boot, bsize);

    if (fs_data->mft.buf) {
        FreeMem(fs_data->mft.buf, fs_data->mft_size * fs_data->sectorsize);
        fs_data->mft.buf = NULL;
    }

    if (fs_data->cache) {
        Cache_DestroyCache(fs_data->cache);
        fs_data->cache = NULL;
    }

    return err;
}

void FreeBootSector(struct FSData *fs_data)
{
    D(bug("[NTFS]: %s()\n", __func__));

    if (fs_data == NULL)
        return;

    D(bug("[NTFS] %s: removing NTFSBootsector from memory\n", __func__));

    if (fs_data->cache) {
        Cache_DestroyCache(fs_data->cache);
        fs_data->cache = NULL;
    }
}
