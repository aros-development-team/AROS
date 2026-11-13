#include "debug.h"

#include "cdceth.class.h"
#include "cdceth_encap.h"

#include <hardware/cdc/cdc_eem.h>
#include <hardware/cdc/cdc_ncm.h>

#define CDCETH_EEM_DEFAULT_BUFSIZE  4096
#define CDCETH_EEM_HEADER_LEN       2
#define CDCETH_EEM_ALIGN            4

#define CDCETH_NCM_ALIGN            4
#define CDCETH_NCM_DEFAULT_NTB      16384

static ULONG cdceth_align(ULONG val, ULONG align)
{
    return (val + (align - 1)) & ~(align - 1);
}

BOOL cdceth_encap_configure(struct NepClassEth *ncp)
{
    struct Library *ps = ncp->ncp_Base;
    IPTR subcls = CDC_SUBCLASS_ETHERNET;

    KPRINTF(5, ("ncp =  0x%p, ps = 0x%p\n", ncp, ps));

    ncp->ncp_EncapType = CDCETH_ENCAP_ECM;
    ncp->ncp_ReadBufSize = ETHER_MAX_LEN;
    ncp->ncp_WriteBufSize = ETHER_MAX_LEN;
    ncp->ncp_NcmMaxSegment = ETHER_MAX_LEN;
    ncp->ncp_NcmSeq = 0;
    ncp->ncp_NtbMaxSize = ETHER_MAX_LEN;

    if(ncp->ncp_ControlInterface)
    {
        psdGetAttrs(PGA_INTERFACE, ncp->ncp_ControlInterface,
                    IFA_SubClass, &subcls,
                    TAG_END);
    }

    KPRINTF(5, ("subclass %u\n", subcls));
    switch(subcls)
    {
        case CDC_SUBCLASS_EEM:
            ncp->ncp_EncapType = CDCETH_ENCAP_EEM;
            ncp->ncp_ReadBufSize = CDCETH_EEM_DEFAULT_BUFSIZE;
            ncp->ncp_WriteBufSize = CDCETH_EEM_DEFAULT_BUFSIZE;
            break;

        case CDC_SUBCLASS_NCM:
        {
            struct PsdDescriptor *pdd;
            struct UsbCDCNcmFunctionalDesc *ncm = NULL;

            ncp->ncp_EncapType = CDCETH_ENCAP_NCM;
            ncp->ncp_NtbMaxSize = CDCETH_NCM_DEFAULT_NTB;
            ncp->ncp_ReadBufSize = CDCETH_NCM_DEFAULT_NTB;
            ncp->ncp_WriteBufSize = CDCETH_NCM_DEFAULT_NTB;

            if(ncp->ncp_ControlInterface)
            {
                pdd = psdFindDescriptor(ncp->ncp_Device, NULL,
                                        DDA_Interface, ncp->ncp_ControlInterface,
                                        DDA_DescriptorType, UDT_CS_INTERFACE,
                                        DDA_CS_SubType, CDC_NCM_FUNC_DESC_SUBTYPE,
                                        TAG_END);
                if(pdd)
                {
                    psdGetAttrs(PGA_DESCRIPTOR, pdd,
                                DDA_DescriptorData, &ncm,
                                TAG_END);
                    if(ncm)
                    {
                        UWORD seg = AROS_LE2WORD(ncm->wMaxSegmentSize);

                        if(seg)
                        {
                            ncp->ncp_NcmMaxSegment = seg;
                        }
                    }
                }
            }
            break;
        }

        case CDC_SUBCLASS_ETHERNET:
        default:
            ncp->ncp_EncapType = CDCETH_ENCAP_ECM;
            break;
    }

    KPRINTF(5, ("CDC encapsulation configured type %ld rx %lu tx %lu maxseg %u\n",
                (LONG) ncp->ncp_EncapType,
                ncp->ncp_ReadBufSize,
                ncp->ncp_WriteBufSize,
                (unsigned) ncp->ncp_NcmMaxSegment));

    return(TRUE);
#undef ps
}

BOOL cdceth_prepare_tx(struct NepClassEth *ncp, ULONG payload_len, ULONG buf_size,
                       ULONG *payload_offset, ULONG *total_len)
{
    ULONG offset = 0;
    ULONG total = 0;

    if(!payload_offset || !total_len)
    {
        return(FALSE);
    }

    switch(ncp->ncp_EncapType)
    {
        case CDCETH_ENCAP_EEM:
        {
            if(payload_len > CDC_EEM_HDR_LEN_MASK)
            {
                return(FALSE);
            }
            offset = CDCETH_EEM_HEADER_LEN;
            total = cdceth_align(offset + payload_len, CDCETH_EEM_ALIGN);
            break;
        }

        case CDCETH_ENCAP_NCM:
        {
            ULONG nth_len = sizeof(struct UsbCDCNcmNtb16Header);
            ULONG ndp_len = sizeof(struct UsbCDCNcmNdp16) +
                            (sizeof(struct UsbCDCNcmDatagramPointer16) * 2);
            ULONG ndp_offset = cdceth_align(nth_len, CDCETH_NCM_ALIGN);
            offset = cdceth_align(ndp_offset + ndp_len, CDCETH_NCM_ALIGN);
            total = cdceth_align(offset + payload_len, CDCETH_NCM_ALIGN);
            break;
        }

        case CDCETH_ENCAP_ECM:
        default:
            offset = 0;
            total = payload_len;
            break;
    }

    if((payload_len > 0) && (total > buf_size))
    {
        return(FALSE);
    }

    *payload_offset = offset;
    *total_len = total;
    return(TRUE);
}

void cdceth_finalize_tx(struct NepClassEth *ncp, UBYTE *buf, ULONG payload_offset,
                        ULONG payload_len, ULONG total_len)
{
    switch(ncp->ncp_EncapType)
    {
        case CDCETH_ENCAP_EEM:
        {
            UWORD header = (UWORD) (payload_len & CDC_EEM_HDR_LEN_MASK);
            UWORD *hdr = (UWORD *) buf;

            *hdr = AROS_WORD2LE(header);
            if(total_len > payload_offset + payload_len)
            {
                memset(buf + payload_offset + payload_len, 0,
                       total_len - (payload_offset + payload_len));
            }
            break;
        }

        case CDCETH_ENCAP_NCM:
        {
            ULONG nth_len = sizeof(struct UsbCDCNcmNtb16Header);
            ULONG ndp_len = sizeof(struct UsbCDCNcmNdp16) +
                            (sizeof(struct UsbCDCNcmDatagramPointer16) * 2);
            ULONG ndp_offset = cdceth_align(nth_len, CDCETH_NCM_ALIGN);
            struct UsbCDCNcmNtb16Header *nth = (struct UsbCDCNcmNtb16Header *) buf;
            struct UsbCDCNcmNdp16 *ndp = (struct UsbCDCNcmNdp16 *) (buf + ndp_offset);
            struct UsbCDCNcmDatagramPointer16 *dpe =
                (struct UsbCDCNcmDatagramPointer16 *) ((UBYTE *) ndp + sizeof(*ndp));

            memset(buf, 0, payload_offset);

            nth->dwSignature = AROS_LONG2LE(CDC_NCM_NTH16_SIGNATURE);
            nth->wHeaderLength = AROS_WORD2LE((UWORD) nth_len);
            nth->wSequence = AROS_WORD2LE(ncp->ncp_NcmSeq++);
            nth->wBlockLength = AROS_WORD2LE((UWORD) total_len);
            nth->wNdpIndex = AROS_WORD2LE((UWORD) ndp_offset);

            ndp->dwSignature = AROS_LONG2LE(CDC_NCM_NDP16_SIGNATURE_NCM0);
            ndp->wLength = AROS_WORD2LE((UWORD) ndp_len);
            ndp->wNextNdpIndex = AROS_WORD2LE(0);

            dpe[0].wDatagramIndex = AROS_WORD2LE((UWORD) payload_offset);
            dpe[0].wDatagramLength = AROS_WORD2LE((UWORD) payload_len);
            dpe[1].wDatagramIndex = 0;
            dpe[1].wDatagramLength = 0;
            break;
        }

        case CDCETH_ENCAP_ECM:
        default:
            break;
    }
}

static BOOL cdceth_handle_eem_rx(struct NepClassEth *ncp, UBYTE *pktptr, ULONG pktlen)
{
    ULONG offset = 0;
    BOOL delivered = FALSE;

    while(offset + sizeof(UWORD) <= pktlen)
    {
        UWORD header = AROS_LE2WORD(*(UWORD *) (pktptr + offset));
        UWORD length = (UWORD) (header & CDC_EEM_HDR_LEN_MASK);
        BOOL is_command = (header & CDC_EEM_HDR_TYPE_COMMAND) != 0;
        BOOL has_crc = (header & CDC_EEM_HDR_CRC) != 0;

        offset += sizeof(UWORD);

        if(length == 0)
        {
            offset = cdceth_align(offset, CDCETH_EEM_ALIGN);
            continue;
        }

        if(offset + length > pktlen)
        {
            ncp->ncp_DeviceStats.BadData++;
            break;
        }

        if(!is_command)
        {
            ULONG frame_len = length;
            if(has_crc && frame_len >= 4)
            {
                frame_len -= 4;
            }
            if(frame_len >= sizeof(struct EtherPacketHeader))
            {
                delivered |= nReadPacket(ncp, pktptr + offset, frame_len);
            }
            else
            {
                ncp->ncp_DeviceStats.BadData++;
            }
        }

        offset += length;
        offset = cdceth_align(offset, CDCETH_EEM_ALIGN);
    }

    return delivered;
}

static BOOL cdceth_handle_ncm_rx(struct NepClassEth *ncp, UBYTE *pktptr, ULONG pktlen)
{
    const struct UsbCDCNcmNtb16Header *nth;
    ULONG block_len;
    ULONG ndp_index;
    BOOL delivered = FALSE;
    int guard = 0;

    if(pktlen < sizeof(*nth))
    {
        ncp->ncp_DeviceStats.BadData++;
        return(FALSE);
    }

    nth = (const struct UsbCDCNcmNtb16Header *) pktptr;
    if(AROS_LE2LONG(nth->dwSignature) != CDC_NCM_NTH16_SIGNATURE)
    {
        ncp->ncp_DeviceStats.BadData++;
        return(FALSE);
    }

    block_len = AROS_LE2WORD(nth->wBlockLength);
    if(block_len == 0 || block_len > pktlen)
    {
        block_len = pktlen;
    }

    ndp_index = AROS_LE2WORD(nth->wNdpIndex);

    while(ndp_index && guard++ < 4)
    {
        const struct UsbCDCNcmNdp16 *ndp;
        ULONG ndp_length;
        ULONG ndp_end;
        ULONG entry_offset;
        ULONG signature;

        if(ndp_index + sizeof(*ndp) > block_len)
        {
            ncp->ncp_DeviceStats.BadData++;
            break;
        }

        ndp = (const struct UsbCDCNcmNdp16 *) (pktptr + ndp_index);
        signature = AROS_LE2LONG(ndp->dwSignature);
        if(signature != CDC_NCM_NDP16_SIGNATURE_NCM0 &&
           signature != CDC_NCM_NDP16_SIGNATURE_NCM1)
        {
            ncp->ncp_DeviceStats.BadData++;
            break;
        }

        ndp_length = AROS_LE2WORD(ndp->wLength);
        if(ndp_length < sizeof(*ndp))
        {
            ncp->ncp_DeviceStats.BadData++;
            break;
        }

        ndp_end = ndp_index + ndp_length;
        if(ndp_end > block_len)
        {
            ncp->ncp_DeviceStats.BadData++;
            break;
        }

        entry_offset = ndp_index + sizeof(*ndp);
        while(entry_offset + sizeof(struct UsbCDCNcmDatagramPointer16) <= ndp_end)
        {
            const struct UsbCDCNcmDatagramPointer16 *entry;
            ULONG d_index;
            ULONG d_len;

            entry = (const struct UsbCDCNcmDatagramPointer16 *) (pktptr + entry_offset);
            d_index = AROS_LE2WORD(entry->wDatagramIndex);
            d_len = AROS_LE2WORD(entry->wDatagramLength);

            if(d_index == 0 || d_len == 0)
            {
                break;
            }

            if(d_index + d_len > block_len)
            {
                ncp->ncp_DeviceStats.BadData++;
                break;
            }

            if(d_len >= sizeof(struct EtherPacketHeader))
            {
                delivered |= nReadPacket(ncp, pktptr + d_index, d_len);
            }
            else
            {
                ncp->ncp_DeviceStats.BadData++;
            }

            entry_offset += sizeof(struct UsbCDCNcmDatagramPointer16);
        }

        ndp_index = AROS_LE2WORD(ndp->wNextNdpIndex);
    }

    return delivered;
}

BOOL cdceth_handle_rx(struct NepClassEth *ncp, UBYTE *pktptr, ULONG pktlen)
{
    switch(ncp->ncp_EncapType)
    {
        case CDCETH_ENCAP_EEM:
            return cdceth_handle_eem_rx(ncp, pktptr, pktlen);
        case CDCETH_ENCAP_NCM:
            return cdceth_handle_ncm_rx(ncp, pktptr, pktlen);
        case CDCETH_ENCAP_ECM:
        default:
            return nReadPacket(ncp, pktptr, pktlen);
    }
}
