/*
The contents of this file are subject to the AROS Public License Version 1.1 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
http://www.aros.org/license.html

Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
ANY KIND, either express or implied. See the License for the specific language governing rights and
limitations under the License.

(C) Copyright 2017 The AROS Dev Team.
(C) Copyright xxxx-2009 Davy Wentzler.
(C) Copyright 2009-2010 Stephen Jones.

The Initial Developer of the Original Code is Davy Wentzler.

All Rights Reserved.
*/

#include <config.h>

#include <exec/memory.h>
#include <proto/expansion.h>

#include <proto/dos.h>
#ifdef __AROS__
#include <aros/debug.h>
#endif
#include <math.h>

#include "library.h"
#include "regs.h"
#include "interrupt.h"
#include "misc.h"
#include "pci_wrapper.h"


/* Public functions in main.c */
static void perform_controller_specific_settings(struct HDAudioChip *card);
int card_init(struct HDAudioChip *card);
void card_cleanup(struct HDAudioChip *card);
static BOOL allocate_corb(struct HDAudioChip *card);
static BOOL allocate_rirb(struct HDAudioChip *card);
static BOOL allocate_pos_buffer(struct HDAudioChip *card);
static BOOL alloc_streams(struct HDAudioChip *card);
static BOOL perform_codec_specific_settings(struct HDAudioChip *card);
static void determine_frequencies(struct HDAudioChip *card);
static void set_frequency_info(struct Freq *freq, UWORD bitnr);
static BOOL reset_chip(struct HDAudioChip *card);
static ULONG get_response(struct HDAudioChip *card);
static BOOL perform_realtek_specific_settings(struct HDAudioChip *card, UWORD device);
static BOOL perform_via_specific_settings(struct HDAudioChip *card, UWORD device);
static BOOL perform_idt_specific_settings(struct HDAudioChip *card, UWORD device);
static void set_gpio(UBYTE mask, struct HDAudioChip *card);
static BOOL interrogate_unknown_chip(struct HDAudioChip *card);
static UBYTE find_widget(struct HDAudioChip *card, UBYTE type, UBYTE pin_type);
static BOOL power_up_all_nodes(struct HDAudioChip *card);

struct Device *TimerBase = NULL;
struct timerequest *TimerIO = NULL;
struct MsgPort *replymp = NULL;
static BOOL forceQuery = FALSE;
static BOOL dumpAll = FALSE;
static int force_speaker_nid = 0;
//void AddResetHandler(struct HDAudioChip *card);


#ifdef __AROS__
#define DebugPrintF bug
INTGW(static, void,  playbackinterrupt, PlaybackInterrupt);
INTGW(static, void,  recordinterrupt,   RecordInterrupt);
INTGW(static, ULONG, cardinterrupt,  CardInterrupt);
#endif

void micro_delay(unsigned int val)
{
    replymp = (struct MsgPort *) CreateMsgPort();
    if (!replymp)
    {
      D(bug("[HDAudio] Could not create the reply port!\n"));
      return;
    }
    
    TimerIO = (struct timerequest *) CreateIORequest(replymp, sizeof(struct timerequest));

    if (TimerIO == NULL)
    {
        D(bug("[HDAudio] Out of memory.\n"));
        return;
    }
    
    if (OpenDevice((CONST_STRPTR) "timer.device", UNIT_MICROHZ, (struct IORequest *) TimerIO, 0) != 0)
    {
        D(bug("[HDAudio] Unable to open 'timer.device'.\n"));
        return;
    }
    else
    {
        TimerBase = (struct Device *) TimerIO->tr_node.io_Device;
    }
    
    TimerIO->tr_node.io_Command = TR_ADDREQUEST; /* Add a request.   */
    TimerIO->tr_time.tv_secs = 0;                /* 0 seconds.      */
    TimerIO->tr_time.tv_micro = val;             /* 'val' micro seconds. */
    DoIO((struct IORequest *) TimerIO);
    CloseDevice((struct IORequest *) TimerIO);
    DeleteIORequest((struct IORequest *) TimerIO);
    TimerIO = NULL;
    
    if (replymp)
    {
        DeleteMsgPort(replymp);
    }
}


/******************************************************************************
** DriverData allocation ******************************************************
******************************************************************************/

struct HDAudioChip* AllocDriverData(APTR dev, struct DriverBase* AHIsubBase)
{
    struct HDAudioChip* card;
    UWORD command_word;
    BOOL success = TRUE;

    card = (struct HDAudioChip *) AllocVec(sizeof(struct HDAudioChip), MEMF_PUBLIC | MEMF_CLEAR);

    if (card == NULL)
    {
        Req("Unable to allocate driver structure.");
        return NULL;
    }

    card->ahisubbase = AHIsubBase;

    card->interrupt.is_Node.ln_Type = IRQTYPE;
    card->interrupt.is_Node.ln_Pri  = 0;
    card->interrupt.is_Node.ln_Name = (char *) LibName;
#ifdef __AROS__
    card->interrupt.is_Code         = (void(*)(void)) &cardinterrupt;
#else
    card->interrupt.is_Code         = (void(*)(void)) CardInterrupt;
#endif
    card->interrupt.is_Data         = (APTR) card;

    card->playback_interrupt.is_Node.ln_Type = IRQTYPE;
    card->playback_interrupt.is_Node.ln_Pri  = 0;
    card->playback_interrupt.is_Node.ln_Name = (char *) LibName;
#ifdef __AROS__
    card->playback_interrupt.is_Code         = (APTR)&playbackinterrupt;
#else
    card->playback_interrupt.is_Code         = PlaybackInterrupt;
#endif
    card->playback_interrupt.is_Data         = (APTR) card;

    card->record_interrupt.is_Node.ln_Type = IRQTYPE;
    card->record_interrupt.is_Node.ln_Pri  = 0;
    card->record_interrupt.is_Node.ln_Name = (char *) LibName;
#ifdef __AROS__
    card->record_interrupt.is_Code         = (APTR)&recordinterrupt;
#else
    card->record_interrupt.is_Code         = RecordInterrupt;
#endif
    card->record_interrupt.is_Data         = (APTR) card;

    command_word = inw_config(PCI_COMMAND, dev);
    command_word |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
    outw_config(PCI_COMMAND, command_word, dev);

    card->pci_dev = dev;
    card->pci_master_enabled = TRUE;

    card->iobase  = ahi_pci_get_base_address(0, dev);
    card->length  = ahi_pci_get_base_size(0, dev);
    card->chiprev = inb_config(PCI_REVISION_ID, dev);
    card->model   = inb_config(PCI_SUBSYSTEM_ID, dev);

    perform_controller_specific_settings(card);

    ahi_pci_add_intserver(&card->interrupt, dev);

    /* Initialize chip */
    if (card_init(card) < 0)
    {
        D(bug("[HDAudio] Unable to initialize Card subsystem.\n"));

        success = FALSE;
    }

    card->interrupt_added = TRUE;

    card->card_initialized = TRUE;
    card->input          = 0;
    card->output         = 0;
    card->monitor_volume = (unsigned long) (0x10000 * pow (10.0, -6.0 / 20.0)); // -6 dB
    card->input_gain     = 0x10000; // 0dB
    card->output_volume  = 0x10000; // 0dB
    
    if (success)
    {
        set_monitor_volumes(card, -6.0); // -6dB monitor volume
    }

    if (!success)
    {
        FreeDriverData(card, AHIsubBase);
        card = NULL;
    }

    return card;
}


/******************************************************************************
** DriverData deallocation ****************************************************
******************************************************************************/

void FreeDriverData(struct HDAudioChip* card, struct DriverBase*  AHIsubBase)
{
    if (card != NULL)
    {
        if (card->pci_dev != NULL)
        {
            if (card->card_initialized)
            {
                card_cleanup(card);
            }

            if (card->pci_master_enabled)
            {
                UWORD cmd;

                cmd = inw_config(PCI_COMMAND, card->pci_dev);
                cmd &= ~(PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
                outw_config(PCI_COMMAND, cmd, card->pci_dev);
            }
        }

        if (card->interrupt_added)
        {
            ahi_pci_rem_intserver(&card->interrupt, card->pci_dev);
        }

        FreeVec(card);
    }
}

#define CNT_VEN_ID_ATI_SB       0x437B1002
#define CNT_VEN_ID_ATI_SB2      0x43831002
#define CNT_VEN_ID_ATI_HUDSON   0x780D1022
#define CNT_VEN_ID_NVIDIA       0x10DE

static const UWORD intel_no_snoop_list[] =
{
    0x2668,
    0x27d8,
    0x269a,
    0x284b,
    0x293e,
    0x293f,
    0x3a3e,
    0x3a6e,
    0
};

/* This is the controller specific portion, for fixes to southbridge */
static void perform_controller_specific_settings(struct HDAudioChip *card)
{
    ULONG data, subsystem;
    ULONG mask = (1 << 16) - 1;
    UWORD i, vendor_id, product_id;
    BOOL snoop = TRUE;

    /* Get vendor/product/subsystem IDs */
    data = inl_config(0x0, card->pci_dev);
    vendor_id = inw_config(0x0, card->pci_dev);
    product_id = inw_config(0x2, card->pci_dev);
    D(bug("DEBUG: Controller Vendor ID: %x\n", data));
    subsystem = inl_config(0x2C, card->pci_dev);

    /* Check for Intel controllers that need snoop */
    if (vendor_id == 0x8086)
    {
        D(bug("[HDAudio] Intel controller detected, checking if snooping needed\n"));
        for (i = 0; intel_no_snoop_list[i] != 0; i++)
        {
            if (intel_no_snoop_list[i] == product_id)
            {
                snoop = FALSE;
            }
        }

        if (snoop)
        {
            D(bug("[HDAudio] Enabling snooping\n"));
            data = inw_config(0x78, card->pci_dev);
            data &= ~0x800;
            outw_config(0x78, data, card->pci_dev);
        }
    }

    /* Check for ATI Southbridge or AMD Hudson controller */
    if (data == CNT_VEN_ID_ATI_SB || data == CNT_VEN_ID_ATI_SB2 || data == CNT_VEN_ID_ATI_HUDSON)
    {
        D(bug("[HDAudio] ATI SB/AMD Hudson controller detected, setting snoop to on.\n"));
        data = inb_config(0x42, card->pci_dev);
        data &= ~0x07;
        data |= 0x02;
        outb_config(0x42, data, card->pci_dev);
    }

    /* Check for NVidia MCP controller */
    if ((data & mask) == CNT_VEN_ID_NVIDIA )
    {
        D(bug("[HDAudio] NVidia MCP controller detected, setting snoop to on.\n"));
        data = inb_config(0x4E, card->pci_dev);
        data |= 0x0F;
        outb_config(0x4E, data, card->pci_dev);

        data = inb_config(0x4D, card->pci_dev);
        data |= 0x01;
        outb_config(0x4D, data, card->pci_dev);

        data = inb_config(0x4C, card->pci_dev);
        data |= 0x01;
        outb_config(0x4C, data, card->pci_dev);
    }

    /* Check for HP Compaq nc6320 laptop */
    if (subsystem == 0x30aa103c)
    {
        D(bug("[HDAudio] HP Compaq nc6320 laptop detected, correcting IRQ\n"));
        data = inb_config(0x3C, card->pci_dev);
        if (data == 10)
            outb_config(0x3C, 11, card->pci_dev);
    }
}

int card_init(struct HDAudioChip *card)
{
    int i;

    if (reset_chip(card) == FALSE)
    {
        D(bug("[HDAudio] Reset chip failed\n"));
        return -1;
    }

    // 4.3 Codec discovery: 15 codecs can be connected, bits that are on indicate a codec
    card->codecbits = pci_inw(HD_STATESTS, card);

    if (card->codecbits == 0)
    {
        D(bug("[HDAudio] No codecs found!\n"));
        return -1;
    }

    if (alloc_streams(card) == FALSE)
    {
        D(bug("[HDAudio] Allocating streams failed!\n"));
        return -1;
    }

    if (allocate_corb(card) == FALSE)
    {
        D(bug("[HDAudio] Allocating CORB failed!\n"));
        return -1;
    }

    if (allocate_rirb(card) == FALSE)
    {
        D(bug("[HDAudio] Allocating RIRB failed!\n"));
        return -1;
    }

    if (allocate_pos_buffer(card) == FALSE)
    {
        D(bug("[HDAudio] Allocating position buffer failed!\n"));
        return -1;
    }

    // enable interrupts
    pci_outl(HD_INTCTL_CIE | HD_INTCTL_GLOBAL, HD_INTCTL, card);
    udelay(200);

    /* Find the first codec with an audio function group */
    for (i = 0; i < 16; i++)
    {
        if (card->codecbits & (1 << i))
        {
            card->codecnr = i;
            if (power_up_all_nodes(card))
                break;
        }
    }

    if (perform_codec_specific_settings(card) == FALSE)
    {
        return -1;
    }

    if (dumpAll)
    {
        codec_discovery(card);
    }

    D(bug("[HDAudio] card_init() was a success!\n"));

    return 0;
}


void card_cleanup(struct HDAudioChip *card)
{
}


static BOOL reset_chip(struct HDAudioChip *card)
{
    int counter = 0;
    UBYTE ubval = 0;
    UBYTE tcsel;

    /*
        Intel® HIgh Definition Audio Traffic Class Assignment (TCSEL), bits 0:2 -> 000 = TC0
        This register assigned the value to be placed in the TC field. CORB and RIRB data will always be
        assigned TC0.
    */
    #define TCSEL_PCIREG 0x44
    tcsel = inb_config(TCSEL_PCIREG, card->pci_dev);
    tcsel &= ~0x07;
    outb_config(TCSEL_PCIREG, tcsel, card->pci_dev);

    pci_outb(0, HD_CORBCTL, card);
    pci_outb(0, HD_RIRBCTL, card);

    // Clear STATESTS just to be sure. After reset, this register holds the ID's of the connected codecs
    pci_outb(0xFF, HD_STATESTS, card);

    // Transition to reset state
    outl_clearbits(1, HD_GCTL, card);

    // Wait for bit 0 to read 0
    for (counter = 0; counter < 1000; counter++)
    {
        ubval = pci_inb(HD_GCTL, card);

        if ((ubval & 0x1) == 0)
        {
            break;
        }

        udelay(100);
    }

    if (counter == 1000)
    {
        D(bug("[HDAudio] Couldn't reset chip!\n"));
        return FALSE;
    }

    udelay(100);
    // 4.2.2. Take controller out of reset
    outl_setbits(1, HD_GCTL, card);
  
   
    // Wait for bit 0 to read 1
    for (counter = 0; counter < 1000; counter++)
    {
        ubval = pci_inb(HD_GCTL, card);

        if ((ubval & 0x1) == 1)
        {
            D(bug("[HDAudio] Codec came out of reset!\n"));
            break;
        }

        udelay(100);
    }

    if (counter == 1000)
    {
        D(bug("[HDAudio] Couldn't reset chip!\n"));
        return FALSE;
    }

    // The software must wait 250 microseconds after reading CRST as 1, but it's suggested to wait longer
    udelay(1000);

    // do not accept unsolicited events for now (jack sense etc.)
    //outl_setbits((1 << 8), HD_GCTL, card); // accept unsolicited events

   return TRUE;
}


void codec_discovery(struct HDAudioChip *card)
{
    int i;

    ULONG subnode_count_response = get_parameter(card->function_group, VERB_GET_PARMS_NODE_COUNT, card);
    UBYTE subnode_count = subnode_count_response & 0xFF;
    UBYTE sub_starting_node = (subnode_count_response >> 16) & 0xFF;
    ULONG connections = 0;

    D(bug("[HDAudio] Subnode count = %d, sub_starting_node = %x\n", subnode_count, sub_starting_node));

    //D(bug("[HDAudio] Audio supported = %lx\n", get_parameter(card->function_group, 0xA, card)));
    //D(bug("[HDAudio] Sup streams = %lx\n", get_parameter(card->function_group, 0xB, card)));

    for (i = 0; i < subnode_count; i++) // widgets
    {
        const ULONG NID = i + sub_starting_node;
        ULONG widget_caps;

        widget_caps = get_parameter(NID, VERB_GET_PARMS_AUDIO_WIDGET_CAPS, card);

        //if ((NID == 0x18) || (NID == 0xB))
        {
            D(bug("[HDAudio] Subnode %x has caps %lx\n", NID, widget_caps));
            D(bug("[HDAudio] %xh: Supported PCM size/rate = %lx\n", NID, get_parameter(NID, VERB_GET_PARMS_SUPPORTED_PCM_SIZE_RATE, card)));

            if (AUDIO_WIDGET_CAPS(widget_caps) == 0x4) // pin complex
            {
                D(bug("[HDAudio] PIN: caps = %lx\n", get_parameter(NID, VERB_GET_PARMS_PIN_CAPS, card)));
                D(bug("[HDAudio] PIN: Connected = %s\n", is_jack_connected(card, NID) ? "TRUE" : "FALSE"));
            }

            D(bug("[HDAudio] %xh: Input Amp caps = %lx\n", NID, get_parameter(NID, 0xD, card)));
            D(bug("[HDAudio] %xh: Output Amp caps = %lx\n", NID, get_parameter(NID, 0x12, card)));

            connections = get_parameter(NID, 0xE, card);
            D(bug("[HDAudio] %xh: Conn list len = %lx\n", NID, connections));
            if (connections > 0) // print connections
            {
                ULONG entry = 0;

                for (entry = 0; entry < connections; entry+=4)
                {
                    ULONG connectedTo = send_command_12(card->codecnr, NID, VERB_GET_CONNECTION_LIST_ENTRY, entry, card);

                    bug("%lx, ", connectedTo);
                }
                bug("\n");
            }

            D(bug("[HDAudio] %xh: Supported power state = %lx\n", NID, get_parameter(NID, 0xF, card)));

            D(ULONG n);
            D(n = send_command_12(card->codecnr, NID, VERB_GET_CONNECTION_SELECT, 0, card));
            D(bug("[HDAudio] %xh: Connection selection = %lx\n", NID, n));

            D(n = send_command_4(card->codecnr, NID, 0xB, 0x8000, card));
            D(bug("[HDAudio] %xh: Output Amp gain = %lx\n", NID, n));
            D(n = send_command_4(card->codecnr, NID, 0xB, 0x0000, card));
            D(bug("[HDAudio] %xh: Input Amp gain = %lx\n", NID, n));
            D(n = send_command_4(card->codecnr, NID, 0xA, 0, card));
            D(bug("[HDAudio] %xh: Format = %lx\n", NID, n));
            D(n = send_command_12(card->codecnr, NID, 0xF05, 0, card));
            D(bug("[HDAudio] %xh: Power state = %lx\n", NID, n));
            D(n = send_command_12(card->codecnr, NID, 0xF06, 0, card));
            D(bug("[HDAudio] %xh: Stream = %lx\n", NID, n));
            D(n = send_command_12(card->codecnr, NID, 0xF07, 0, card));
            D(bug("[HDAudio] %xh: Pin widget control = %lx\n", NID, n));
            D(bug("[HDAudio] --------------------------------\n\n"));
        }
    }
}


static BOOL power_up_all_nodes(struct HDAudioChip *card)
{
    int i;
    ULONG node_count_response = get_parameter(0, VERB_GET_PARMS_NODE_COUNT, card);
    UBYTE node_count = node_count_response & 0xFF;
    UBYTE starting_node = (node_count_response >> 16) & 0xFF;
    BOOL audio_found = FALSE;
   
    D(bug("[HDAudio] power up\n"));
    send_command_12(card->codecnr, 1, VERB_SET_POWER_STATE , 0, card); // send function reset to audio node, this should power up all nodes
    udelay(20000);

    for (i = 0; i < node_count && !audio_found; i++)
    {
        ULONG function_group_response = get_parameter(starting_node + i, VERB_GET_PARMS_FUNCTION_GROUP_TYPE, card);
        UBYTE function_group = function_group_response & 0xFF;

        if (function_group == AUDIO_FUNCTION)
        {
            int j;

            ULONG subnode_count_response = get_parameter(starting_node + i, VERB_GET_PARMS_NODE_COUNT, card);
            UBYTE subnode_count = subnode_count_response & 0xFF;
            UBYTE sub_starting_node = (subnode_count_response >> 16) & 0xFF;

            audio_found = TRUE;
            card->function_group = starting_node + i;

            for (j = 0; j < subnode_count; j++) // widgets
            {
                const ULONG NID = j + sub_starting_node;
                ULONG widget_caps;

                widget_caps = get_parameter(NID, VERB_GET_PARMS_AUDIO_WIDGET_CAPS, card);

                {   
                    if (AUDIO_WIDGET_POWER_CONTROL(widget_caps) == 1) // power control
                    {
                        ULONG power_state = 0;
                        
                        power_state = send_command_12(card->codecnr, NID, VERB_GET_POWER_STATE, 0, card);
                        D(bug("[HDAudio] %xh: power state = %xh\n", NID, power_state));
                        
                        if (power_state != 0)
                        {
                            D(bug("[HDAudio] Setting power state to 0\n"));
                            send_command_12(card->codecnr, NID, VERB_SET_POWER_STATE, 0, card);
                        }
                    }
                }
            }
        }
    }

    return audio_found;
}


// allocates memory on the given boundary. Returns the aligned memory address and the non-aligned memory address
// in NonAlignedAddress, if not NULL.
void *pci_alloc_consistent(size_t size, APTR *NonAlignedAddress, unsigned int boundary)
{
    void* address;
    unsigned long a;

    address = (void *) AllocVec(size + boundary, MEMF_PUBLIC | MEMF_CLEAR);

    if (address != NULL)
    {
        a = (unsigned long) address;
        a = (a + boundary - 1) & ~(boundary - 1);
        address = (void *) a;
    }

    if (NonAlignedAddress)
    {
        *NonAlignedAddress = address;
    }

    return address;
}


void pci_free_consistent(void* addr)
{
    FreeVec(addr);
}


ULONG get_parameter(UBYTE node, UBYTE parameter, struct HDAudioChip *card)
{
    return send_command_12(card->codecnr, node, VERB_GET_PARMS, parameter, card);
}


ULONG send_command_4(UBYTE codec, UBYTE node, UBYTE verb, UWORD payload, struct HDAudioChip *card)
{
    UWORD wp = pci_inw(HD_CORBWP, card) & 0xFF;
    ULONG data = (codec << 28) | (node << 20) | (verb << 16) | payload;

    if (wp == card->corb_entries - 1)
    {
        wp = 0;
    }
    else
    {
        wp++;
    }

    //bug("Sending command %lx\n", data);

    card->corb[wp] = data;
    pci_outw(wp, HD_CORBWP, card);

    return get_response(card);
}


ULONG send_command_12(UBYTE codec, UBYTE node, UWORD verb, UBYTE payload, struct HDAudioChip *card)
{
    UWORD wp = pci_inw(HD_CORBWP, card) & 0xFF;
    ULONG data = (codec << 28) | (node << 20) | (verb << 8) | payload;

    if (wp == card->corb_entries - 1)
    {
        wp = 0;
    }
    else
    {
        wp++;
    }

    //bug("Sending command %lx\n", data);

    card->corb[wp] = data;
    pci_outw(wp, HD_CORBWP, card);

    return get_response(card);
}


ULONG get_response(struct HDAudioChip *card)
{
    int timeout = 10000;
    int i;
    UBYTE rirb_wp;

    udelay(20); // 
    
    // wait for interrupt
    for (i = 0; i < timeout; i++)
    {
        if (card->rirb_irq > 0)
        {
            card->rirb_irq--;
            break;
        }
        udelay(10);
    }
    
    if (i == timeout)
    {
        D(bug("[HDAudio] No IRQ!\n"));
    }
    
    for (i = 0; i < timeout; i++)
    {
        rirb_wp = pci_inb(HD_RIRBWP, card);
        
        if (rirb_wp == card->rirb_rp) // strange, we expect the wp to have increased
        {
            D(bug("[HDAudio] WP has not increased! rirb_wp = %u, rirb_rp = %lu\n", rirb_wp, card->rirb_rp));
            udelay(5000);
        }
        else
        {
            if ( ((rirb_wp > card->rirb_rp) &&
                  ((rirb_wp - card->rirb_rp) >= 2)) ||
                
                ((rirb_wp < card->rirb_rp) &&
                 ( ((int) rirb_wp) + card->rirb_entries) - card->rirb_rp >= 2))
            {
                D(bug("[HDAudio] Write pointer is more than 1 step ahead!\n"));
            }
            
            ULONG addr;
            ULONG response, response_ex; // 3.6.5 Response Input Ring Buffer
                
            card->rirb_rp = rirb_wp;
            addr = card->rirb_rp;
            addr *= 2; // 64-bit entries
                   
            response = card->rirb[addr];
            response_ex = card->rirb[addr + 1];
            if (response_ex & 0x10) // unsolicited
            {
                D(bug("[HDAudio] Unsolicited response! Skipping!\n"));
            }
            else
            {
                //bug("Response is %lx\n", response);
                return response;
            }
        }
    }
    
    D(bug("[HDAudio] ERROR in get_response() card->rirb_rp = %u!rirb_wp = %u\n", card->rirb_rp, rirb_wp));
    return 0;
}


static BOOL allocate_corb(struct HDAudioChip *card)
{
    UBYTE corbsize_reg;

    // 4.4.1.3 Initialize the CORB

    // stop the DMA
    outb_clearbits(HD_CORBRUN, HD_CORBCTL, card);

    // set CORB size
    corbsize_reg = pci_inb(HD_CORBSIZE, card);
    if (corbsize_reg & (1 << 6))
    {
        pci_outb(0x2, HD_CORBSIZE, card);
        card->corb_entries = 256;
    }
    else if (corbsize_reg & (1 << 5))
    {
        pci_outb(0x1, HD_CORBSIZE, card);
        card->corb_entries = 16;
    }
    else if (corbsize_reg & (1 << 4))
    {
        pci_outb(0x0, HD_CORBSIZE, card);
        card->corb_entries = 2;
    }
   
    // Allocate CORB memory
    card->corb = pci_alloc_consistent(4 * card->corb_entries, NULL, 128); // todo: virtual

    // Set CORB base
#if defined(__AROS__) && (__WORDSIZE==64)
    pci_outl((ULONG)((IPTR)card->corb & 0xFFFFFFFF), HD_CORB_LOW, card);
    pci_outl((ULONG)(((IPTR)card->corb >> 32) & 0xFFFFFFFF), HD_CORB_HIGH, card);
#else
    pci_outl((ULONG) card->corb, HD_CORB_LOW, card);
    pci_outl(0, HD_CORB_HIGH, card);
#endif

    //bug("Before reset rp: corbrp = %x\n", pci_inw(0x4A, card));

    // Reset read pointer: if we set this, the CORB will not work??
    //outw_setbits(HD_CORBRPRST, HD_CORBRP, card);

    //bug("After reset rp: corbrp = %x\n", pci_inw(0x4A, card));

    // Write a 0 to the write pointer to clear
    pci_outw(0, HD_CORBWP, card);

    // run it
    outb_setbits(HD_CORBRUN, HD_CORBCTL, card);

    if (card->corb)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


static BOOL allocate_rirb(struct HDAudioChip *card)
{
    UBYTE rirbsize_reg;

    // 4.4.2.2 Initialize the RIRB

    // stop the DMA
    outb_clearbits(HD_RIRBRUN, HD_RIRBCTL, card);

    // set rirb size
    rirbsize_reg = pci_inb(HD_RIRBSIZE, card);
    if (rirbsize_reg & (1 << 6))
    {
        pci_outb(0x2, HD_RIRBSIZE, card);
        card->rirb_entries = 256;
    }
    else if (rirbsize_reg & (1 << 5))
    {
        pci_outb(0x1, HD_RIRBSIZE, card);
        card->rirb_entries = 16;
    }
    else if (rirbsize_reg & (1 << 4))
    {
        pci_outb(0x0, HD_RIRBSIZE, card);
        card->rirb_entries = 2;
    }
    
    card->rirb_irq = 0;
   
    // Allocate rirb memory
    card->rirb = pci_alloc_consistent(4 * 2 * card->rirb_entries, NULL, 128); // todo: virtual
    card->rirb_rp = 0;

    // Set rirb base
#if defined(__AROS__) && (__WORDSIZE==64)
    pci_outl((ULONG)((IPTR)card->rirb & 0xFFFFFFFF), HD_RIRB_LOW, card);
    pci_outl((ULONG)(((IPTR)card->rirb >> 32) & 0xFFFFFFFF), HD_RIRB_HIGH, card);
#else
    pci_outl((ULONG) card->rirb, HD_RIRB_LOW, card);
    pci_outl(0, HD_RIRB_HIGH, card);
#endif

    // Reset read pointer: if we set this, it will not come out of reset??
    //outw_setbits(HD_RIRBWPRST, HD_RIRBWP, card);

    // Set N=1, which generates an interrupt for every response
    pci_outw(1, HD_RINTCNT, card);

    pci_outb(0x5, HD_RIRBSTS, card);

    // run it and enable IRQ
    outb_setbits(HD_RIRBRUN | HD_RINTCTL | 0x4, HD_RIRBCTL, card);

    if (card->rirb)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}



static BOOL allocate_pos_buffer(struct HDAudioChip *card)
{
    card->dma_position_buffer = pci_alloc_consistent(sizeof(APTR) * 36, NULL, 128);

//warning: DMA poition buffer is unused?
#if defined(__AROS__) && (__WORDSIZE==64)
    pci_outl(0, HD_DPLBASE, card);
    pci_outl(0, HD_DPUBASE, card);
#else
    //pci_outl((ULONG) card->dma_position_buffer | HD_DPLBASE_ENABLE, HD_DPLBASE, card);
    pci_outl(0, HD_DPLBASE, card);
    pci_outl(0, HD_DPUBASE, card);
#endif

    if (card->dma_position_buffer)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


static BOOL alloc_streams(struct HDAudioChip *card)
{
    int i;
    card->nr_of_input_streams = (pci_inw(HD_GCAP, card) & HD_GCAP_ISS_MASK) >> 8;
    card->nr_of_output_streams = (pci_inw(HD_GCAP, card) & HD_GCAP_OSS_MASK) >> 12;
    card->nr_of_streams = card->nr_of_input_streams + card->nr_of_output_streams;
    //bug("Streams in = %d, out = %d\n", card->nr_of_input_streams, card->nr_of_output_streams);

    card->streams = (struct Stream *) AllocVec(sizeof(struct Stream) * card->nr_of_streams, MEMF_PUBLIC | MEMF_CLEAR);

    for (i = 0; i < card->nr_of_streams; i++)
    {
        card->streams[i].bdl = NULL;
        card->streams[i].bdl_nonaligned_addresses = NULL;
        card->streams[i].sd_reg_offset = HD_SD_BASE_OFFSET + HD_SD_DESCRIPTOR_SIZE * i;
        card->streams[i].index = i;
        card->streams[i].tag = i + 1;
        card->streams[i].fifo_size = pci_inw(card->streams[i].sd_reg_offset + HD_SD_OFFSET_FIFO_SIZE, card);

        // clear the descriptor error, fifo error and buffer completion interrupt status flags
        pci_outb(HD_SD_STATUS_MASK, card->streams[i].sd_reg_offset + HD_SD_OFFSET_STATUS, card);
    }

    if (card->streams)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


/*static ULONG ResetHandler(struct ExceptionContext *ctx, struct ExecBase *pExecBase, struct HDAudioChip *card)
{
    struct PCIDevice *dev = card->pci_dev;

    return 0UL;
}


void AddResetHandler(struct HDAudioChip *card)
{
    static struct Interrupt interrupt;

    interrupt.is_Code = (void (*)())ResetHandler;
    interrupt.is_Data = (APTR) card;
    interrupt.is_Node.ln_Pri  = 0;
    interrupt.is_Node.ln_Type = NT_EXTINTERRUPT;
    interrupt.is_Node.ln_Name = "reset handler";

    AddResetCallback(&interrupt);
}*/


static BOOL perform_codec_specific_settings(struct HDAudioChip *card)
{
    BOOL configured = FALSE;
    ULONG vendor_device_id = get_parameter(0x0, VERB_GET_PARMS_VENDOR_DEVICE, card); // get vendor and device ID from root node
    UWORD vendor = (vendor_device_id >> 16);
    UWORD device = (vendor_device_id & 0xFFFF);
    
    card->frequencies = NULL;
    card->nr_of_frequencies = 0;
    card->selected_freq_index = 0;
    
    card->dac_min_gain = -64.0;
    card->dac_max_gain = 0;
    card->dac_step_gain = 1.0;
    card->speaker_nid = 0; // off
    card->headphone_nid = 0; // off
    card->speaker_active = FALSE;

    D(bug("[HDAudio] vendor = %x, device = %x\n", vendor, device));
    
    if (vendor == 0x10EC && forceQuery == FALSE) // Realtek
    {
        configured = perform_realtek_specific_settings(card, device);
    }
    else if (vendor == 0x1106 && forceQuery == FALSE) // VIA
    {
        configured = perform_via_specific_settings(card, device);
    }    
    else if (vendor == 0x111d || (vendor == 0x8384 && forceQuery == FALSE)) // IDT
    {
        configured = perform_idt_specific_settings(card, device);
    }

    if (!configured) // default: fall-back 
    {
        if (interrogate_unknown_chip(card) == FALSE)
        {
            return FALSE;
        }
    }
    
    determine_frequencies(card);
    return TRUE;
}


static BOOL perform_realtek_specific_settings(struct HDAudioChip *card, UWORD device)
{
    D(bug("[HDAudio] Found Realtek codec\n"));
        
    if (!(device == 0x662
        || device == 0x663
        || device == 0x268
        || device == 0x269
        || device == 0x888))
    {
        D(bug("[HDAudio] Unknown Realtek codec.\n"));
        return FALSE;
    }

    card->dac_nid = 0x2;
    card->dac_volume_nids[0] = 0x2;
    card->dac_volume_count = 1;
    card->adc_nid = 0x8;
        
    card->adc_mixer_nid = 0x23;
    card->line_in_nid = 0x1A;
    card->mic1_nid = 0x18;
    card->mic2_nid = 0x19;
    card->cd_nid = 0x1C;
        
    card->adc_mixer_is_mux = FALSE;
    
    // FRONT pin (0x14)
    send_command_4(card->codecnr, 0x14, VERB_SET_AMP_GAIN, OUTPUT_AMP_GAIN | AMP_GAIN_LR, card); // set amplifier gain: unmute output of FRONT (Port-D)
        
    send_command_12(card->codecnr, 0x14, VERB_SET_PIN_WIDGET_CONTROL, 0x40, card); // output enabled
    card->speaker_active = TRUE;
        
    // MIC1 pin (0x18) as input
    send_command_12(card->codecnr, card->mic1_nid, VERB_SET_PIN_WIDGET_CONTROL, 0x20, card); // input enabled
        
    send_command_4(card->codecnr, card->mic1_nid, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR | 0, card); // set amplifier gain: unmute input and set boost to +10dB
        
        
    // device specific support
        if (device == 0x662 || device == 0x663) // Realtek ALC662/663
        {
            D(bug("[HDAudio] Adding ALC662/663 specific support\n"));
            
            card->adc_mixer_indices[0] = 2; // line in
            card->adc_mixer_indices[1] = 0; // mic1
            card->adc_mixer_indices[2] = 1; // mic2
            card->adc_mixer_indices[3] = 4; // cd
            card->adc_mixer_indices[4] = 8; // mon mixer
            
            card->adc_min_gain = -13.5;
            card->adc_max_gain = 33.0;
            card->adc_step_gain = 1.5;
            
            // LINE2 pin (0x1B) as second front output (duplicates sound of 0xC (front DAC))
            send_command_4(card->codecnr, 0x1B, VERB_SET_AMP_GAIN, OUTPUT_AMP_GAIN | AMP_GAIN_LR, card); // set amplifier gain: unmute output of LINE2 (Port-E)
        
            send_command_12(card->codecnr, 0x1B, VERB_SET_PIN_WIDGET_CONTROL, 0x40, card); // output enabled
            
            // Monitor mixer (0xB): set the first 3 inputs to 0dB and unmute them
            send_command_4(card->codecnr, 0xB, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR | 23 | (0 << 8), card); // set input amplifier gain and unmute (index 0 is MIC1), 23 is 0dB
            
            send_command_4(card->codecnr, 0xB, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR | 23 | (1 << 8), card); // set input amplifier gain and unmute (index 2 is MIC2), 23 is 0dB
            
            send_command_4(card->codecnr, 0xB, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR | 23 | (2 << 8), card); // set input amplifier gain and unmute (index 2 is LINE1), 23 is 0dB

            // Front DAC (0xC)
            send_command_4(card->codecnr, 0xC, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR, card); // unmute PCM at index 0
            
            send_command_4(card->codecnr, 0xC, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR | (1 << 8), card); // unmute monitor mixer at index 1
            
            // LINE1 pin (0x1A) as input
            send_command_12(card->codecnr, card->line_in_nid, VERB_SET_PIN_WIDGET_CONTROL, 0x20, card); // input enabled
        
            // MIC2 pin (0x19) as input
            send_command_12(card->codecnr, card->mic2_nid, VERB_SET_PIN_WIDGET_CONTROL, 0x20, card); // input enabled
            
            send_command_4(card->codecnr, card->adc_mixer_nid, VERB_SET_AMP_GAIN, OUTPUT_AMP_GAIN | AMP_GAIN_LR | 11, card); // set amplifier gain: unmute and set to 0dB
        }
        else if (device == 0x268)
        {
            D(bug("[HDAudio] Adding ALC268 specific support\n"));
            
            card->speaker_nid = 0x14;
            card->headphone_nid = 0x15;

            card->adc_mixer_indices[0] = 2; // line in
            card->adc_mixer_indices[1] = 0; // mic1
            card->adc_mixer_indices[2] = 5; // mic2
            card->adc_mixer_indices[3] = 3; // cd
            card->adc_mixer_indices[4] = 255; // no mon mixer
            
            card->adc_min_gain = -16.5;
            card->adc_max_gain = 30.0;
            card->adc_step_gain = 1.5;
            
            card->adc_mixer_is_mux = TRUE;
            
            // sum widget before output (0xF)
            send_command_4(card->codecnr, 0xF, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR, card); // unmute
            
            // sum widget before headphone output (0x10)
            send_command_4(card->codecnr, 0x10, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR | (2 << 8), card); // unmute
            
            // HP-OUT pin (0x15)
            send_command_4(card->codecnr, 0x15, VERB_SET_AMP_GAIN, OUTPUT_AMP_GAIN | AMP_GAIN_LR, card); // set amplifier gain: unmute output of HP-OUT (Port-A)
        
            send_command_12(card->codecnr, 0x15, VERB_SET_PIN_WIDGET_CONTROL, 0x40, card); // output enabled
            
            send_command_12(card->codecnr, 0x14, VERB_SET_EAPD, 0x2, card); // enable EAPD (external power amp)
            
            send_command_12(card->codecnr, 0x15, VERB_SET_EAPD, 0x2, card); // enable EAPD (external power amp)
        }
        else if (device == 0x269) // Dell mini etc.
        {
            D(bug("[HDAudio] Adding ALC269 specific support\n"));
            
            card->speaker_nid = 0x14;
            card->headphone_nid = 0x15;

            card->adc_mixer_indices[0] = 2; // line in
            card->adc_mixer_indices[1] = 0; // mic1
            card->adc_mixer_indices[2] = 1; // mic2
            card->adc_mixer_indices[3] = 4; // cd
            card->adc_mixer_indices[4] = 6; // mon mixer
            
            card->adc_min_gain = -17;
            card->adc_max_gain = 29.0;
            card->adc_step_gain = 1.0;
            
            card->adc_mixer_is_mux = TRUE;
            
            // Front DAC (0xC)
            send_command_4(card->codecnr, 0xC, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR, card); // unmute PCM at index 0
            
            send_command_4(card->codecnr, 0xC, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR | (1 << 8), card); // unmute monitor mixer at index 1
            
            // sum widget before output (0xF)
            send_command_4(card->codecnr, 0xF, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR, card); // unmute
            
            // sum widget before headphone output (0x10)
            send_command_4(card->codecnr, 0x10, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR | (2 << 8), card); // unmute
            
            // HP-OUT pin (0x15)
            send_command_4(card->codecnr, 0x15, VERB_SET_AMP_GAIN, OUTPUT_AMP_GAIN | AMP_GAIN_LR, card); // set amplifier gain: unmute output of HP-OUT (Port-A)
        
            send_command_12(card->codecnr, 0x15, VERB_SET_PIN_WIDGET_CONTROL, 0x40, card); // output enabled
            
            send_command_12(card->codecnr, 0x14, VERB_SET_EAPD, 0x2, card); // enable EAPD (external power amp)
            
            send_command_12(card->codecnr, 0x15, VERB_SET_EAPD, 0x2, card); // enable EAPD (external power amp)
        }
        else if (device == 0x888) // ALC888
        {
            D(bug("[HDAudio] Adding ALC888 specific support\n"));
            
            card->adc_mixer_indices[0] = 2; // line in
            card->adc_mixer_indices[1] = 0; // mic1
            card->adc_mixer_indices[2] = 1; // mic2
            card->adc_mixer_indices[3] = 4; // cd
            card->adc_mixer_indices[4] = 10; // mon mixer
            
            card->adc_min_gain = -16.5;
            card->adc_max_gain = 30.0;
            card->adc_step_gain = 1.5;
            
            card->dac_min_gain = -46.5;
            card->dac_max_gain = 0;
            card->dac_step_gain = 1.5;
            
            card->dac_volume_nids[0] = 0xC;
            card->dac_volume_count = 1;
            
            send_command_4(card->codecnr, 0xC, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR, card); // unmute PCM at index 0
            send_command_4(card->codecnr, 0xC, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR | (1 << 8), card); // unmute monitor mixer at index 1
        }

    return TRUE;
}


static BOOL perform_via_specific_settings(struct HDAudioChip *card, UWORD device)
{
    D(bug("[HDAudio] Found VIA codec\n"));
        
    if (!(device == 0xE721 || device == 0x0397))
    {
        D(bug("[HDAudio] Unknown VIA codec.\n"));
        return FALSE;
    }

    card->dac_nid = 0x10;
    card->adc_nid = 0x13;
        
    card->adc_mixer_nid = 0x17;
    card->line_in_nid = 0x1B;
    card->mic1_nid = 0x1A;
    card->mic2_nid = 0x1E;
    card->cd_nid = 0x1F;
        
    card->adc_mixer_is_mux = TRUE;
        
    // FRONT pin (0x1C)
    send_command_4(card->codecnr, 0x1C, VERB_SET_AMP_GAIN, OUTPUT_AMP_GAIN | AMP_GAIN_LR | 0x1B, card); // set amplifier gain: unmute output and set to 0dB of FRONT (Port-D)
    send_command_12(card->codecnr, 0x1C, VERB_SET_PIN_WIDGET_CONTROL, 0x40, card); // output enabled
        
    // MIC1 pin as input
    send_command_12(card->codecnr, card->mic1_nid, VERB_SET_PIN_WIDGET_CONTROL, 0x20, card); // input enabled        
    send_command_4(card->codecnr, card->mic1_nid, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR, card); // set amplifier gain: unmute input
        
        
    // device specific support
    if (device == 0xE721) // VIA VT1708B
    {
        D(bug("[HDAudio] Adding VIA VT1708B specific support\n"));
            
        card->adc_mixer_indices[0] = 3; // line in
        card->adc_mixer_indices[1] = 2; // mic1
        card->adc_mixer_indices[2] = 4; // mic2
        card->adc_mixer_indices[3] = 1; // cd
        card->adc_mixer_indices[4] = 255; // mon mixer
            
        card->adc_min_gain = -13.5;
        card->adc_max_gain = 33.0;
        card->adc_step_gain = 1.5;
    }

    return TRUE;
}

/*
  IDT specific settings

  information: http://www.idt.com/document/92hd75b-datasheet-92hd75-being-discontinued-see-pdn-notice

  TODO: input

*/
static BOOL perform_idt_specific_settings(struct HDAudioChip *card, UWORD device)
{
    D(bug("[HDAudio] Found IDT codec\n"));
    
    if (device == 0x76a0)
    {
        D(bug("[HDAudio] STAC9205 detected.\n"));

        card->eapd_gpio_mask = 0x1;
        set_gpio(card->eapd_gpio_mask, card);

        return FALSE;
    }

    if (!(device == 0x7608))
    {
        D(bug("[HDAudio] Unknown IDT codec.\n"));
        return FALSE;
    }

    card->dac_nid = 0x10;
    card->adc_nid = 0x12;
    card->adc_mixer_nid = 0x1C;
    card->dac_volume_nids[0] = 0x10;
    card->dac_volume_count = 1;
   
    card->speaker_nid = 0x0D;
    card->headphone_nid = 0x0A;

    card->line_in_nid = 0x0B;
    card->mic1_nid = 0x0B;
    card->mic2_nid = 0x0C;
    card->cd_nid = 0x0E; /* no cd but ...*/

    card->adc_mixer_is_mux = TRUE;

    /* to not to enable headphone and the speaker at the same time */
    card->speaker_active = TRUE;

    /* enable eapd. Specs says this is spdif out, but this is required */
    send_command_12(card->codecnr, 0x1f, VERB_SET_EAPD, 0x2, card);

    /* set connections */
    send_command_12 (card->codecnr, 0x0f, VERB_SET_CONNECTION_SELECT, 0, card); /* 48QFN specific */
    send_command_12 (card->codecnr, 0x0a, VERB_SET_CONNECTION_SELECT, 0, card); /* headset */
    send_command_12 (card->codecnr, 0x0d, VERB_SET_CONNECTION_SELECT, 0, card); /* speaker */

    /* set output gains */
    send_command_4 (card->codecnr, 0x0f, VERB_SET_AMP_GAIN, OUTPUT_AMP_GAIN | AMP_GAIN_LR, card);
    send_command_4 (card->codecnr, 0x0a, VERB_SET_AMP_GAIN, OUTPUT_AMP_GAIN | AMP_GAIN_LR, card);
    send_command_4 (card->codecnr, 0x0d, VERB_SET_AMP_GAIN, OUTPUT_AMP_GAIN | AMP_GAIN_LR, card);

    /* enable outputs */
    send_command_12(card->codecnr, 0x0f, VERB_SET_PIN_WIDGET_CONTROL, 0x40, card);
    send_command_12(card->codecnr, 0x0a, VERB_SET_PIN_WIDGET_CONTROL, 0x40, card);
    send_command_12(card->codecnr, 0x0d, VERB_SET_PIN_WIDGET_CONTROL, 0x40, card);
 
    if (device == 0x7608)
    {
        /* move 0x7608 specific stuff here */

        /* Not sure about indices */
        card->adc_mixer_indices[0] = 3;   // line in
        card->adc_mixer_indices[1] = 2;   // mic1
        card->adc_mixer_indices[2] = 4;   // mic2
        card->adc_mixer_indices[3] = 1;   // cd
        card->adc_mixer_indices[4] = 255; // no mon mixer

        card->adc_min_gain = 0.0;
        card->adc_max_gain = 22.5;
        card->adc_step_gain = 1.5;
        
        card->dac_min_gain = -95.25;
        card->dac_max_gain = 0.0;
        card->dac_step_gain = 0.75;
    }

    return TRUE;
}


static void update_gpio(UBYTE mask, BOOL on, struct HDAudioChip *card)
{
    ULONG gpio_data, gpio_enable, gpio_dir;

    gpio_enable = send_command_12(card->codecnr, card->function_group,
        VERB_GET_GPIO_ENABLE, 0, card);
    gpio_enable |= mask;
    send_command_12(card->codecnr, card->function_group, VERB_SET_GPIO_ENABLE,
        gpio_enable, card);

    gpio_dir = send_command_12(card->codecnr, card->function_group,
        VERB_GET_GPIO_DIR, 0, card);
    gpio_dir |= mask;
    send_command_12(card->codecnr, card->function_group, VERB_SET_GPIO_DIR,
        gpio_dir, card);

    gpio_data = send_command_12(card->codecnr, card->function_group,
        VERB_GET_GPIO_DATA, 0, card);
    if (on)
        gpio_data |= mask;
    else
        gpio_data &= ~mask;
    send_command_12(card->codecnr, card->function_group, VERB_SET_GPIO_DATA,
        gpio_data, card);
}


static void set_gpio(UBYTE mask, struct HDAudioChip *card)
{
    update_gpio(mask, TRUE, card);
}


static UBYTE get_connected_widget(UBYTE nid, UBYTE index,
    struct HDAudioChip *card)
{
    UBYTE input_nid = 0;
    ULONG connections, entry;

    connections = get_parameter(nid, 0xE, card);
    if (index < connections)
    {
        entry = send_command_12(card->codecnr, nid,
            VERB_GET_CONNECTION_LIST_ENTRY, index, card);
        input_nid = entry >> ((index % 4) * 8);
    }

    return input_nid;
}


static UBYTE get_selected_widget(UBYTE nid, struct HDAudioChip *card)
{
    UBYTE input_nid = 0;
    ULONG connections;
    ULONG index;

    connections = get_parameter(nid, 0xE, card);
    if (connections > 0)
    {
        if (connections == 1)
            index = 0;
        else
        {
            index = send_command_12(card->codecnr, nid,
                VERB_GET_CONNECTION_SELECT, 0, card);
        }

        input_nid = get_connected_widget(nid, index, card);
    }

    return input_nid;
}


/* Adds a widget to the array of volume controlling widgets if it has volume
 * control */
static void check_widget_volume(UBYTE nid, struct HDAudioChip *card)
{
    ULONG parm;

    parm = get_parameter(nid, VERB_GET_PARMS_AUDIO_WIDGET_CAPS, card);
    D(bug("[HDAudio] NID %xh: Audio widget caps = %lx\n", nid, parm));
    if (parm & 0x4) // OutAmpPre
    {
        D(bug("[HDAudio] NID %xh has volume control\n", nid));
        card->dac_volume_nids[card->dac_volume_count++] = nid;
    }
    else
    {
        D(bug("[HDAudio] NID %xh does not have volume control\n", nid));
    }
}


static BOOL interrogate_unknown_chip(struct HDAudioChip *card)
{
    int dac, adc, front, steps = 0, offset0dB = 0;
    double step_size = 0.25;
    ULONG parm, widget_caps, nid;
    UWORD connections, i;
    UBYTE before_front = 0;
    
    D(bug("[HDAudio] Unknown codec, interrogating chip...\n"));

    // find out the first PCM DAC
    dac = find_widget(card, 0, 0);
    D(bug("[HDAudio] DAC NID = %xh\n", dac));
	
    if (dac == 0)
    {
        bug("Didn't find DAC!\n");
        return FALSE;
    }
	
    card->dac_nid = dac;
    
    check_widget_volume(dac, card);

    // find FRONT pin
    front = find_widget(card, 4, 0);
    D(bug("[HDAudio] Front PIN = %xh\n", front));
	
    if (front == 0)
    {
        D(bug("[HDAudio] Didn't find jack/pin for line output!\n"));
    }
    else
    {
        send_command_4(card->codecnr, front, VERB_SET_AMP_GAIN, OUTPUT_AMP_GAIN | AMP_GAIN_LR, card); // set amplifier gain: unmute output of FRONT
        send_command_12(card->codecnr, front, VERB_SET_PIN_WIDGET_CONTROL, 0x40, card); // output enabled
        send_command_12(card->codecnr, front, VERB_SET_CONNECTION_SELECT, 0, card); // use first input
        check_widget_volume(front, card);
    }
    
    // find SPEAKER
    if (force_speaker_nid > 0)
    {
        D(bug("[HDAudio] Using speaker nid from config file"));
        card->speaker_nid = force_speaker_nid;
    }
    else
    {
        card->speaker_nid = find_widget(card, 4, 1);
    }
    D(bug("[HDAudio] Speaker NID = %xh\n", card->speaker_nid));
	
    if (card->speaker_nid != 0)
    {
        // check if there is a power amp and if so, enable it
        if (get_parameter(card->speaker_nid, VERB_GET_PARMS_PIN_CAPS, card) & PIN_CAPS_EAPD_CAPABLE)
        {
            D(bug("[HDAudio] Enabling power amp of speaker\n"));
            send_command_12(card->codecnr, card->speaker_nid, VERB_SET_EAPD, 0x2, card); // enable EAPD (external power amp)
        }
        
        // set amplifier gain: unmute output
        send_command_4(card->codecnr, card->speaker_nid, VERB_SET_AMP_GAIN,
            OUTPUT_AMP_GAIN | INPUT_AMP_GAIN | AMP_GAIN_LR | 0x0, card);

            D(bug("[HDAudio] Enabling speaker output\n"));
            send_command_12(card->codecnr, card->speaker_nid, VERB_SET_PIN_WIDGET_CONTROL, 0x40, card); // output enabled
            card->speaker_active = TRUE;
        send_command_12(card->codecnr, card->speaker_nid, VERB_SET_CONNECTION_SELECT, 0, card); // use first input
        
        check_widget_volume(card->speaker_nid, card);
    }
    else
    {
        D(bug("[HDAudio] No speaker pin found, continuing anyway!\n"));
    }
	
    // Find headphones socket
        card->headphone_nid = find_widget(card, 4, 2);
        
    D(bug("[HDAudio] Headphone NID = %xh\n", card->headphone_nid));

    if (card->headphone_nid != 0)
    {
        send_command_4(card->codecnr, card->headphone_nid, VERB_SET_AMP_GAIN, OUTPUT_AMP_GAIN | AMP_GAIN_LR, card); // set amplifier gain: unmute headphone
        send_command_12(card->codecnr, card->headphone_nid, VERB_SET_PIN_WIDGET_CONTROL, 0xC0, card); // output enabled and headphone enabled
        send_command_12(card->codecnr, card->headphone_nid, VERB_SET_CONNECTION_SELECT, 0, card); // use first input
        
        // check if there is a power amp and if so, enable it
        if (get_parameter(card->headphone_nid, VERB_GET_PARMS_PIN_CAPS, card) & PIN_CAPS_EAPD_CAPABLE)
        {
            D(bug("[HDAudio] Enabling power amp of headphone port\n"));
            send_command_12(card->codecnr, card->headphone_nid, VERB_SET_EAPD, 0x2, card); // enable EAPD (external power amp)
        }
        
        // unmute widget before headphone jack if it's not the DAC
   	    nid = send_command_12(card->codecnr, card->headphone_nid,
            VERB_GET_CONNECTION_LIST_ENTRY, 0, card);
        if (nid != dac)
        {
            D(bug("[HDAudio] Widget before headphone port is not the DAC\n"));
            connections = get_parameter(nid, 0xE, card);
            for (i = 0; i < connections; i++)
                send_command_4(card->codecnr, nid, VERB_SET_AMP_GAIN,
                    INPUT_AMP_GAIN | AMP_GAIN_LR | i << 8, card);
        }

        check_widget_volume(card->headphone_nid, card);
    }

    // find the node before the front, speaker or HP node
    if (front != 0)
        nid = front;
    else if (card->speaker_nid != 0)
        nid = card->speaker_nid;
    else if (card->headphone_nid != 0)
        nid = card->headphone_nid;
    else nid = 0;

    if (nid != 0)
        before_front = (UBYTE)send_command_12(card->codecnr, nid,
            VERB_GET_CONNECTION_LIST_ENTRY, 0, card);
	
  if (before_front != dac)
	 {
	    D(bug("[HDAudio] The widget before front/speaker/HP (%xh) is not equal to DAC!\n", before_front));
	    
	    send_command_4(card->codecnr, before_front, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR, card); // unmute PCM at index 0
	    D(bug("[HDAudio] Let's hope it was a mute that now got unmuted!\n"));
	    
        check_widget_volume(before_front, card);
	  }
	  else
	  {
	    D(bug("[HDAudio] The widget before front/speaker/HP is equal to DAC.\n"));
	  }
	
    for (i = 0; card->dac_volume_nids[i] != 0; i++)
    {
        parm = get_parameter(card->dac_volume_nids[i],
            VERB_GET_PARMS_AUDIO_WIDGET_CAPS, card);
        if ((parm & 0x8) != 0)
            parm = get_parameter(card->dac_volume_nids[i],
                VERB_GET_PARMS_OUTPUT_AMP_CAPS , card);
        else
            parm = get_parameter(card->function_group,
                VERB_GET_PARMS_OUTPUT_AMP_CAPS, card);
        D(bug("[HDAudio] NID %xh: Output amp caps = %lx\n", card->dac_volume_nids[i], parm));

        step_size = (((parm >> 16) & 0x7F) + 1) * 0.25;
        steps = ((parm >> 8) & 0x7F);
        offset0dB = (parm & 0x7F);

        if (steps != 0)
        {
            card->dac_min_gain = -(offset0dB * step_size);
            card->dac_max_gain = card->dac_min_gain + step_size * steps;
            card->dac_step_gain = step_size;
            D(bug("[HDAudio] Gain step size = %lu * 0.25 dB,"
                " min gain = %d, max gain = %d\n",
                (((parm >> 16) & 0x7F) + 1), (int) (card->dac_min_gain),
                (int) (card->dac_max_gain)));
        }
    }

    // find out the first PCM ADC
    adc = find_widget(card, 1, 0);
    D(bug("[HDAudio] ADC NID = %xh\n", adc));

    if (adc != 0)
    {
        card->adc_nid = adc;

        card->line_in_nid = find_widget(card, 4, 8);
        D(bug("[HDAudio] Line-in NID = %xh\n", card->line_in_nid));
        card->mic1_nid = find_widget(card, 4, 0xA);
        D(bug("[HDAudio] Mic1 NID = %xh\n", card->mic1_nid));

        nid = adc;
        while (nid != 0)
        {
            widget_caps = get_parameter(nid,
                VERB_GET_PARMS_AUDIO_WIDGET_CAPS, card);
            D(bug("[HDAudio] audio widget caps = %lx\n", widget_caps));

            if (((widget_caps >> 20) & 0xF) == 0x3)
            {
                card->adc_mixer_nid = nid;
                D(bug("[HDAudio] ADC mixer NID = %xh\n", card->adc_mixer_nid));
                card->adc_mixer_is_mux = TRUE;
            }

            // set amplifier gain and unmute
            send_command_4(card->codecnr, nid, VERB_SET_AMP_GAIN,
                OUTPUT_AMP_GAIN | INPUT_AMP_GAIN | AMP_GAIN_LR | 0x0, card);

            nid = get_selected_widget(nid, card);
        }

        if (card->line_in_nid != 0)
        {
            send_command_12(card->codecnr, card->line_in_nid,
                VERB_SET_PIN_WIDGET_CONTROL, 0x20, card); // input enabled
            send_command_4(card->codecnr, card->line_in_nid, VERB_SET_AMP_GAIN,
                OUTPUT_AMP_GAIN | INPUT_AMP_GAIN | AMP_GAIN_LR | 0x0, card);
        }
        if (card->mic1_nid != 0)
        {
            send_command_12(card->codecnr, card->mic1_nid,
                VERB_SET_PIN_WIDGET_CONTROL, 0x20, card); // input enabled
            send_command_4(card->codecnr, card->mic1_nid, VERB_SET_AMP_GAIN,
                OUTPUT_AMP_GAIN | INPUT_AMP_GAIN | AMP_GAIN_LR | 0x0, card);
        }
    }
    else
        bug("[HDAudio] Didn't find ADC!\n");
	
	return TRUE;
}


static UBYTE find_widget(struct HDAudioChip *card, UBYTE type, UBYTE pin_type)
{
    ULONG node_count_response = get_parameter(card->function_group,
        VERB_GET_PARMS_NODE_COUNT, card);
    UBYTE node_count = node_count_response & 0xFF;
    UBYTE starting_node = (node_count_response >> 16) & 0xFF;
    UBYTE i;
    ULONG config_default;

    for (i = 0; i < node_count; i++) // widgets
    {
        UBYTE nid = i + starting_node;
        ULONG widget_caps, pin_caps, connections;

        widget_caps = get_parameter(nid, VERB_GET_PARMS_AUDIO_WIDGET_CAPS,
            card);

        if (((widget_caps >> 20) & 0xF) == type)
        {
            BOOL ok;

            if (type == 4) // node is a pin widget
            {
                config_default = send_command_12(card->codecnr, nid,
                    VERB_GET_CONFIG_DEFAULT, 0, card);

                if (((config_default >> 20) & 0xF) == pin_type)
                {
                    D(bug("[HDAudio] Config default for NID %x = %x\n", nid,
                        config_default));
                    if ((widget_caps & 1 << 8) != 0)
                        connections = get_parameter(nid, 0xE, card) & 0x7f;
                    else
                        connections = 0;
                    pin_caps =
                        get_parameter(nid, VERB_GET_PARMS_PIN_CAPS, card);
                    switch (pin_type)
                    {
                    case 0x0:
                    case 0x1:
                    case 0x2:
                    case 0x4:
                    case 0x5:
                        ok = (pin_caps & PIN_CAPS_OUTPUT_CAPABLE) != 0
                            && connections != 0;
                        break;
                    default:
                        ok = (pin_caps & PIN_CAPS_INPUT_CAPABLE) != 0;
                    }

                    // check speaker connection type is internal
                    if (pin_type == 1 && (config_default >> 16 & 0xB) != 3)
                        ok = FALSE;

                    // check headphone connection type is mini-jack
                    if (pin_type == 2 && (config_default >> 16 & 0xF) != 1)
                        ok = FALSE;

                    // check microphone connection type is mini-jack
                    if (pin_type == 0xA && (config_default >> 16 & 0xF) != 1)
                        ok = FALSE;
                }  
            }
            else
                ok = (widget_caps & 0x1) == 1 && // stereo
                    ((widget_caps >> 9) & 0x1) == 0; // analogue

            if (ok)
                return nid;
        }
    }

    return 0;
}


static void determine_frequencies(struct HDAudioChip *card)
{
    ULONG verb = get_parameter(card->dac_nid, 0xA, card);
    UWORD samplerate_flags = verb & 0x0FFF;
    int i;
    ULONG freqs = 0;
    BOOL default_freq_found = FALSE;
  
    if (samplerate_flags == 0)
    {
        verb = get_parameter(0x1, 0xA, card);
        samplerate_flags = verb & 0x0FFF;
        D(bug("[HDAudio] dac_nid didn't have a list of sample rates, trying AFG node\n"));
    }
  
    // count number of frequencies
    for (i = 0; i < 12; i++)
    {
        if (samplerate_flags & (1 << i))
        {
            freqs++;
        }
    }
    
    D(bug("[HDAudio] Frequencies found = %lu\n", freqs));
    card->frequencies = (struct Freq *) AllocVec(sizeof(struct Freq) * freqs, MEMF_PUBLIC | MEMF_CLEAR);
    card->nr_of_frequencies = freqs;
    
    freqs = 0;
    for (i = 0; i < 12; i++)
    {
        if (samplerate_flags & (1 << i))
        {
            set_frequency_info(&(card->frequencies[freqs]), i);
            
            if (card->frequencies[freqs].frequency == 44100 && !default_freq_found)
            {
                card->selected_freq_index = freqs; // set default freq index to 44100 Hz
                default_freq_found = TRUE;
            }
            
            freqs++;
        }
    }
    
    if (default_freq_found == FALSE)
    {
        D(bug("[HDAudio] 44100 Hz is not supported!\n"));
        if (freqs > 0)
        {
            D(bug("[HDAudio] Setting default frequency to %lu\n", card->frequencies[0].frequency));
            card->selected_freq_index = 0;
        }
    }
}



static void set_frequency_info(struct Freq *freq, UWORD bitnr)
{
    switch (bitnr)
    {
        case 0: freq->frequency = 8000;
                freq->base44100 = 0;
                freq->mult = 0;
                freq->div = 5;
                break;
        
        case 1: freq->frequency = 11025;
                freq->base44100 = 1;
                freq->mult = 0;
                freq->div = 3;
                break;
                
        case 2: freq->frequency = 16000;
                freq->base44100 = 0;
                freq->mult = 0;
                freq->div = 2;
                break;
        
        case 3: freq->frequency = 22050;
                freq->base44100 = 1;
                freq->mult = 0;
                freq->div = 1;
                break;
        
        case 4: freq->frequency = 32000;
                freq->base44100 = 0;
                freq->mult = 0;
                freq->div = 2;
                break;
                
        case 5: freq->frequency = 44100;
                freq->base44100 = 1;
                freq->mult = 0;
                freq->div = 0;
                break;
        
        case 6: freq->frequency = 48000;
                freq->base44100 = 0;
                freq->mult = 0;
                freq->div = 0;
                break;
        
        case 7: freq->frequency = 88200;
                freq->base44100 = 1;
                freq->mult = 1;
                freq->div = 0;
                break;
        
        case 8: freq->frequency = 96000;
                freq->base44100 = 0;
                freq->mult = 1;
                freq->div = 0;
                break;
        
        case 9: freq->frequency = 176400;
                freq->base44100 = 1;
                freq->mult = 3;
                freq->div = 0;
                break;
        
        case 10: freq->frequency = 192000;
                freq->base44100 = 0;
                freq->mult = 3;
                freq->div = 0;
                break;
        
        default: 
                D(bug("[HDAudio] Unsupported frequency!\n"));
                 break;
    }
}


void set_monitor_volumes(struct HDAudioChip *card, double dB)
{
#if 0
    int i;
#endif
    int dB_steps = (int) ((dB + 34.5) / 1.5);

    if (dB_steps < 0)
    {
        dB_steps = 0;
    }
    else if (dB_steps > 31)
    {
        dB_steps = 31;
    }

#if 0
    for (i = 0; i < 9; i++)
    {
        if (i == 0 || i == 1 || i == 2 || i == 4)
        {
    	    send_command_4(card->codecnr, 0xB, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR | (i << 8) | dB_steps, card);
    	}
    	else // mute
    	{
    	    send_command_4(card->codecnr, 0xB, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR | (i << 8) | (1 << 7), card);
    	}
	}
#endif
}


void set_adc_input(struct HDAudioChip *card)
{
    int i;
    
    if (card->input >= INPUTS)
    {
        card->input = 0;
    }

    if (card->adc_mixer_is_mux == TRUE)
    {
        D(bug("[HDAudio] Selecting ADC input %d\n", card->adc_mixer_indices[card->input]));
        send_command_12(card->codecnr, card->adc_mixer_nid, VERB_SET_CONNECTION_SELECT,
      	                                card->adc_mixer_indices[card->input], card);
        return;
    }
    else
    {
        for (i = 0; i < INPUTS; i++)
        {
            if (card->adc_mixer_indices[i] != 255) // input is present
            {
                if (i == card->input) // unmute or select
                {    
                    D(bug("[HDAudio] Unmuting ADC input %d\n", card->adc_mixer_indices[i]));
                    send_command_4(card->codecnr, card->adc_mixer_nid, VERB_SET_AMP_GAIN,
                                   INPUT_AMP_GAIN | AMP_GAIN_LR | (card->adc_mixer_indices[i] << 8), card);
                }
                else // mute
                {
                    D(bug("[HDAudio] Muting ADC input %d\n", card->adc_mixer_indices[i]));
                    send_command_4(card->codecnr, card->adc_mixer_nid, VERB_SET_AMP_GAIN,
                                   INPUT_AMP_GAIN | AMP_GAIN_LR | (card->adc_mixer_indices[i] << 8) | (1 << 7), card);
            
                }
            }
        }
    }
}


void set_adc_gain(struct HDAudioChip *card, double dB)
{
    int dB_steps = (int) ( (dB - card->adc_min_gain) / card->adc_step_gain);

    if (dB_steps < 0)
    {
        dB_steps = 0;
    }

    if (card->adc_mixer_is_mux == TRUE)
    {
        send_command_4(card->codecnr, card->adc_mixer_nid, VERB_SET_AMP_GAIN, OUTPUT_AMP_GAIN | AMP_GAIN_LR | dB_steps, card);
    }
    else
    {
       send_command_4(card->codecnr, card->adc_nid, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR | dB_steps, card);
    }
}


void set_dac_gain(struct HDAudioChip *card, double dB)
{
    int i;
    int dB_steps = (int) ( (dB - card->dac_min_gain) / card->dac_step_gain);

    if (dB_steps < 0)
    {
        dB_steps = 0;
    }

    for (i = 0; i < card->dac_volume_count; i++)
        send_command_4(card->codecnr, card->dac_volume_nids[i],
            VERB_SET_AMP_GAIN, OUTPUT_AMP_GAIN | AMP_GAIN_LR | dB_steps, card);
}


void setForceQuery(void)
{
    forceQuery = TRUE;
}


void setDumpAll(void)
{
    dumpAll = TRUE;
}


void setForceSpeaker(int speaker_nid)
{
    force_speaker_nid = speaker_nid;
}



BOOL is_jack_connected(struct HDAudioChip *card, UBYTE NID)
{
    ULONG result;
    
    send_command_12(card->codecnr, NID, VERB_EXECUTE_PIN_SENSE, 0, card);
    udelay(2000);
    result = send_command_12(card->codecnr, NID, VERB_GET_PIN_SENSE, 0, card);
    
    if (result & 0x80000000)
    {
        D(bug("[HDAudio] jack connected\n"));
        return TRUE;
    }
    else
    {
        D(bug("[HDAudio] jack disconnected\n"));
        return FALSE;
    }
}



void detect_headphone_change(struct HDAudioChip *card)
{
    if (card->speaker_nid != 0 &&
        card->headphone_nid != 0)
    {
        if (card->speaker_active &&
            is_jack_connected(card, card->headphone_nid)) // disable speaker
        {
            send_command_12(card->codecnr, card->speaker_nid, VERB_SET_PIN_WIDGET_CONTROL, 0x0, card); // output disabled
            card->speaker_active = FALSE;
        }
        else if (card->speaker_active == FALSE &&
                 is_jack_connected(card, card->headphone_nid) == FALSE) // enable speaker
        {
            send_command_12(card->codecnr, card->speaker_nid, VERB_SET_PIN_WIDGET_CONTROL, 0x40, card); // output enabled
            card->speaker_active = TRUE;
        }
    }
}

