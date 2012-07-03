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


extern int z;

/* Public functions in main.c */
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
static int find_pin_widget_with_encoding(struct HDAudioChip *card, UBYTE encoding);
static BOOL interrogate_unknown_chip(struct HDAudioChip *card);
static int find_audio_output(struct HDAudioChip *card, UBYTE digital);
static int find_speaker_nid(struct HDAudioChip *card);
static int find_headphone_nid(struct HDAudioChip *card);
static BOOL power_up_all_nodes(struct HDAudioChip *card);

struct Device *TimerBase = NULL;
struct timerequest *TimerIO = NULL;
struct MsgPort *replymp = NULL;
static BOOL forceQuery = FALSE;
static BOOL dumpAll = FALSE;
static int force_speaker_nid = -1;
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
      bug("Could not create the reply port!\n");
      return;
    }
    
    TimerIO = (struct timerequest *) CreateIORequest(replymp, sizeof(struct timerequest));

    if (TimerIO == NULL)
    {
        DebugPrintF("Out of memory.\n");
        return;
    }
    
    if (OpenDevice((CONST_STRPTR) "timer.device", UNIT_MICROHZ, (struct IORequest *) TimerIO, 0) != 0)
    {
        DebugPrintF("Unable to open 'timer.device'.\n");
        return; 
    }
    else
    {
        TimerBase = (struct Device *) TimerIO->tr_node.io_Device;
    }
    
    if (TimerIO)
    {
        TimerIO->tr_node.io_Command = TR_ADDREQUEST; /* Add a request.   */
        TimerIO->tr_time.tv_secs = 0;                /* 0 seconds.      */
        TimerIO->tr_time.tv_micro = val;             /* 'val' micro seconds. */
        DoIO((struct IORequest *) TimerIO);
        DeleteIORequest((struct IORequest *) TimerIO);
        TimerIO = NULL;
        CloseDevice((struct IORequest *) TimerIO);
    }
    
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
    struct HDAudioBase* card_base = (struct HDAudioBase*) AHIsubBase;
    struct HDAudioChip* card;
    UWORD command_word;
    int i;
    unsigned short uval;
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
    card->playback_interrupt.is_Code         = &playbackinterrupt;
#else
    card->playback_interrupt.is_Code         = PlaybackInterrupt;
#endif
    card->playback_interrupt.is_Data         = (APTR) card;

    card->record_interrupt.is_Node.ln_Type = IRQTYPE;
    card->record_interrupt.is_Node.ln_Pri  = 0;
    card->record_interrupt.is_Node.ln_Name = (char *) LibName;
#ifdef __AROS__
    card->record_interrupt.is_Code         = &recordinterrupt;
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
    card->irq     = ahi_pci_get_irq(dev);
    card->chiprev = inb_config(PCI_REVISION_ID, dev);
    card->model   = inb_config(PCI_SUBSYSTEM_ID, dev);

    ahi_pci_add_intserver(&card->interrupt, dev);

    /* Initialize chip */
    if (card_init(card) < 0)
    {
        DebugPrintF("Unable to initialize Card subsystem.\n");

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

        //AddResetHandler(card);
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



int card_init(struct HDAudioChip *card)
{
    struct PCIDevice *dev = (struct PCIDevice *) card->pci_dev;
    UWORD uwval;
    unsigned char pval, byt;
    long *ptr;
    int i;

    if (reset_chip(card) == FALSE)
    {
        bug("Reset chip failed\n");
        return -1;
    }
    
    // 4.3 Codec discovery: 15 codecs can be connected, bits that are on indicate a codec
    card->codecbits = pci_inw(HD_STATESTS, card);

    if (card->codecbits == 0)
    {
        bug("No codecs found!\n");
        return -1;
    }

    if (alloc_streams(card) == FALSE)
    {
        bug("Allocating streams failed!\n");
        return -1;
    }

    if (allocate_corb(card) == FALSE)
    {
        bug("Allocating CORB failed!\n");
        return -1;
    }

    if (allocate_rirb(card) == FALSE)
    {
        bug("Allocating RIRB failed!\n");
        return -1;
    }

    if (allocate_pos_buffer(card) == FALSE)
    {
        bug("Allocating position buffer failed!\n");
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

    bug("card_init() was a success!\n");

    return 0;
}


void card_cleanup(struct HDAudioChip *card)
{
}


static BOOL reset_chip(struct HDAudioChip *card)
{
    int counter = 0;
    UBYTE ubval = 0;
    UWORD uwval = 0;
    int count;
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
        bug("Couldn't reset chip!\n");
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
            bug("Codec came out of reset!\n");
            break;
        }

        udelay(100);
    }

    if (counter == 1000)
    {
        bug("Couldn't reset chip!\n");
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
    ULONG node_count_response = get_parameter(0, VERB_GET_PARMS_NODE_COUNT, card);
    UBYTE node_count = node_count_response & 0xFF;
    UBYTE starting_node = (node_count_response >> 16) & 0xFF;
    bug("Node count = %x, starting_node = %x\n", node_count, starting_node);
   
    for (i = 0; i < node_count; i++)
    {
        ULONG function_group_response = get_parameter(starting_node + i, VERB_GET_PARMS_FUNCTION_GROUP_TYPE, card);
        UBYTE function_group = function_group_response & 0xFF;
        //bug("Function group = %x, UnSol cap = %x\n", function_group, (function_group_response >> 8) & 0x1);

        if (function_group == AUDIO_FUNCTION)
        {
            int j;

            ULONG subnode_count_response = get_parameter(starting_node + i, VERB_GET_PARMS_NODE_COUNT, card);
            UBYTE subnode_count = subnode_count_response & 0xFF;
            UBYTE sub_starting_node = (subnode_count_response >> 16) & 0xFF;
            ULONG connections = 0;
            bug("Subnode count = %d, sub_starting_node = %x\n", subnode_count, sub_starting_node);

            //bug("Audio supported = %lx\n", get_parameter(starting_node + i, 0xA, card));
            //bug("Sup streams = %lx\n", get_parameter(starting_node + i, 0xB, card));

            for (j = 0; j < subnode_count; j++) // widgets
            {
                const ULONG NID = j + sub_starting_node;
                ULONG widget_caps;

                widget_caps = get_parameter(NID, VERB_GET_PARMS_AUDIO_WIDGET_CAPS, card);

                //if ((NID == 0x18) || (NID == 0xB))
                {
                    bug("Subnode %x has caps %lx\n", NID, widget_caps);
                    bug("%xh: Supported PCM size/rate = %lx\n", NID, get_parameter(NID, VERB_GET_PARMS_SUPPORTED_PCM_SIZE_RATE, card));
                    
                    if (AUDIO_WIDGET_CAPS(widget_caps) == 0x4) // pin complex
                    {
                        ULONG config_default = 0;
                        
                        bug("PIN: caps = %lx\n", get_parameter(NID, VERB_GET_PARMS_PIN_CAPS, card));
                        config_default = send_command_12(card->codecnr, NID, VERB_GET_CONFIG_DEFAULT, 0, card);
                    
                        bug("PIN: Config default = %lx\n", config_default);
                        
                        bug("PIN: Connected = %s\n", is_jack_connected(card, NID) ? "TRUE" : "FALSE");
                    }
                    
                    bug("%xh: Input Amp caps = %lx\n", NID, get_parameter(NID, 0xD, card));
                    bug("%xh: Output Amp caps = %lx\n", NID, get_parameter(NID, 0x12, card));
                    
                    connections = get_parameter(NID, 0xE, card);
                    bug("%xh: Conn list len = %lx\n", NID, connections);
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
                    
                    bug("%xh: Supported power state = %lx\n", NID, get_parameter(NID, 0xF, card));
                    
                    bug("%xh: Connection selection = %lx\n", NID, send_command_12(card->codecnr, NID, VERB_GET_CONNECTION_SELECT, 0, card));
                    
                    bug("%xh: Output Amp gain = %lx\n", NID, send_command_4(card->codecnr, NID, 0xB, 0x8000, card));
                    bug("%xh: Input Amp gain = %lx\n", NID, send_command_4(card->codecnr, NID, 0xB, 0x0000, card));
                    bug("%xh: Format = %lx\n", NID, send_command_4(card->codecnr, NID, 0xA, 0, card));
                    bug("%xh: Power state = %lx\n", NID, send_command_12(card->codecnr, NID, 0xF05, 0, card));
                    bug("%xh: Stream = %lx\n", NID, send_command_12(card->codecnr, NID, 0xF06, 0, card));
                    bug("%xh: Pin widget control = %lx\n", NID, send_command_12(card->codecnr, NID, 0xF07, 0, card));
                    bug("--------------------------------\n\n");
                }
            }
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
   
    bug("power up\n");
    send_command_12(card->codecnr, 1, VERB_SET_POWER_STATE , 0, card); // send function reset to audio node, this should power up all nodes
    udelay(20000);

    for (i = 0; i < node_count; i++)
    {
        ULONG function_group_response = get_parameter(starting_node + i, VERB_GET_PARMS_FUNCTION_GROUP_TYPE, card);
        UBYTE function_group = function_group_response & 0xFF;

        if (function_group == AUDIO_FUNCTION)
        {
            int j;

            ULONG subnode_count_response = get_parameter(starting_node + i, VERB_GET_PARMS_NODE_COUNT, card);
            UBYTE subnode_count = subnode_count_response & 0xFF;
            UBYTE sub_starting_node = (subnode_count_response >> 16) & 0xFF;
            ULONG connections = 0;

            audio_found = TRUE;

            for (j = 0; j < subnode_count; j++) // widgets
            {
                const ULONG NID = j + sub_starting_node;
                ULONG widget_caps;

                widget_caps = get_parameter(NID, VERB_GET_PARMS_AUDIO_WIDGET_CAPS, card);

                {   
                    if (AUDIO_WIDGET_POWER_CONTROL(widget_caps) == 1) // power control
                    {
                        ULONG power_state = 0;
                        
                        power_state = send_command_12(card->codecnr, NID, VERB_GET_POWER_STATE , 0, card);
                        bug("%xh: power state = %xh\n", power_state);
                        
                        if (power_state != 0)
                        {
                            bug("Setting power state to 0\n");
                            send_command_12(card->codecnr, NID, VERB_SET_POWER_STATE , 0, card);
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
        bug("No IRQ!\n");
    }
    
    for (i = 0; i < timeout; i++)
    {
        rirb_wp = pci_inb(HD_RIRBWP, card);
        
        if (rirb_wp == card->rirb_rp) // strange, we expect the wp to have increased
        {
            bug("WP has not increased! rirb_wp = %u, rirb_rp = %lu\n", rirb_wp, card->rirb_rp);
            udelay(5000);
        }
        else
        {
            if ( ((rirb_wp > card->rirb_rp) &&
                  ((rirb_wp - card->rirb_rp) >= 2)) ||
                
                ((rirb_wp < card->rirb_rp) &&
                 ( ((int) rirb_wp) + card->rirb_entries) - card->rirb_rp >= 2))
            {
                bug("Write pointer is more than 1 step ahead!\n");
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
                bug("Unsolicited response! Skipping!\n");
            }
            else
            {
                //bug("Response is %lx\n", response);
                return response;
            }
        }
    }
    
    bug("ERROR in get_response() card->rirb_rp = %u!rirb_wp = %u\n", card->rirb_rp, rirb_wp);
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
    pci_outl((ULONG) card->corb, HD_CORB_LOW, card);
    pci_outl(0, HD_CORB_HIGH, card);

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
    pci_outl((ULONG) card->rirb, HD_RIRB_LOW, card);
    pci_outl(0, HD_RIRB_HIGH, card);

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
    //pci_outl((ULONG) card->dma_position_buffer | HD_DPLBASE_ENABLE, HD_DPLBASE, card);
    pci_outl(0, HD_DPUBASE, card);

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
    BOOL found = FALSE;
    ULONG vendor_device_id = get_parameter(0x0, VERB_GET_PARMS_VENDOR_DEVICE, card); // get vendor and device ID from root node
    UBYTE old;
    UWORD vendor = (vendor_device_id >> 16);
    UWORD device = (vendor_device_id & 0xFFFF);
    
    card->frequencies = NULL;
    card->nr_of_frequencies = 0;
    card->selected_freq_index = 0;
    
    card->dac_min_gain = -64.0;
    card->dac_max_gain = 0;
    card->dac_step_gain = 1.0;
    card->speaker_nid = 255; // off
    card->headphone_nid = 255; // off
    card->speaker_active = FALSE;
    bug("vendor = %x, device = %x\n", vendor, device);
    
    if (vendor == 0x10EC && forceQuery == FALSE) // Realtek
    {
        found = perform_realtek_specific_settings(card, device);
    }
    else if (vendor == 0x1106 && forceQuery == FALSE) // VIA
    {
        found = perform_via_specific_settings(card, device);
    }
    if (!found) // default: fall-back 
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
    bug("Found Realtek codec\n");
        
    if (!(device == 0x662
        || device == 0x663
        || device == 0x268
        || device == 0x269
        || device == 0x888))
    {
        bug("Unknown Realtek codec.\n");
        return FALSE;
    }

    card->dac_nid = 0x2;
    card->dac_volume_nid = 0x2;
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
            bug("Adding ALC662/663 specific support\n");
            
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
            bug("Adding ALC268 specific support\n");
            
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
            bug("Adding ALC269 specific support\n");
            
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
            bug("Adding ALC888 specific support\n");
            
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
            
            card->dac_volume_nid = 0xC;
            
            send_command_4(card->codecnr, 0xC, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR, card); // unmute PCM at index 0
            send_command_4(card->codecnr, 0xC, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR | (1 << 8), card); // unmute monitor mixer at index 1
        }

    return TRUE;
}


static BOOL perform_via_specific_settings(struct HDAudioChip *card, UWORD device)
{
    bug("Found VIA codec\n");
        
    if (!(device == 0xE721))
    {
        bug("Unknown VIA codec.\n");
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
        bug("Adding VIA VT1708B specific support\n");
            
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


static BOOL interrogate_unknown_chip(struct HDAudioChip *card)
{
    int dac, front, speaker = -1, steps = 0, offset0dB = 0;
    double step_size = 0.25;
    ULONG parm;
    
    bug("Unknown codec, interrogating chip...\n");
    
    card->dac_nid = 0x2;
    card->dac_volume_nid = 0;
    card->adc_nid = 0x8;
        
    card->adc_mixer_nid = 255;
    card->line_in_nid = 255;
    card->mic1_nid = 255;
    card->mic2_nid = 255;
    card->cd_nid = 255;
        
    card->adc_mixer_is_mux = FALSE;
            
    
    // find out the first PCM DAC
    dac = find_audio_output(card, 0); // analogue out
    bug("DAC NID = %xh\n", dac);
	
    if (dac == -1)
    {
        bug("Didn't find DAC!\n");
        return FALSE;
    }
	
    card->dac_nid = dac;
    
    parm = get_parameter(dac, VERB_GET_PARMS_AUDIO_WIDGET_CAPS, card);
    bug("audio widget caps for dac = %lx\n", parm);
    if (parm & 0x4) // OutAmpPre
    {
        card->dac_volume_nid = dac;
        bug("DAC seems to have volume control\n");
    }

    // find FRONT pin
    front = find_pin_widget_with_encoding(card, 0);
    bug("Front PIN = %xh\n", front);
	
    if (front == -1)
    {
        bug("Didn't find jack/pin for line output!\n");
    }
    else
    {
        send_command_4(card->codecnr, front, VERB_SET_AMP_GAIN, OUTPUT_AMP_GAIN | AMP_GAIN_LR, card); // set amplifier gain: unmute output of FRONT
        send_command_12(card->codecnr, front, VERB_SET_PIN_WIDGET_CONTROL, 0x40, card); // output enabled
    }
    
    
    // find SPEAKER
    if (force_speaker_nid >= 0)
    {
        bug("Using speaker nid from config file");
        speaker = force_speaker_nid;
    }
    else
    {
    	   speaker = find_speaker_nid(card);
    }
    bug("Speaker NID = %xh\n", speaker);
	
    if (speaker == -1)
    {
        bug("No speaker pin found, continuing anyway!\n");
    }
        card->speaker_nid = speaker;
        card->headphone_nid = find_headphone_nid(card);
        
        bug("Headphone NID = %xh\n", card->headphone_nid);
        
    if (speaker != -1)
    {
        // check if there is a power amp and if so, enable it
        if (get_parameter(speaker, VERB_GET_PARMS_PIN_CAPS, card) & PIN_CAPS_EAPD_CAPABLE)
        {
            bug("Enabling power amp of speaker\n");
            send_command_12(card->codecnr, speaker, VERB_SET_EAPD, 0x2, card); // enable EAPD (external power amp)
        }
        
        send_command_4(card->codecnr, speaker, VERB_SET_AMP_GAIN, OUTPUT_AMP_GAIN | AMP_GAIN_LR, card); // set amplifier gain: unmute output
    }

        send_command_4(card->codecnr, card->headphone_nid, VERB_SET_AMP_GAIN, OUTPUT_AMP_GAIN | AMP_GAIN_LR, card); // set amplifier gain: unmute headphone
        send_command_12(card->codecnr, card->headphone_nid, VERB_SET_PIN_WIDGET_CONTROL, 0xC0, card); // output enabled and headphone enabled
        
        // check if there is a power amp and if so, enable it
        if (get_parameter(card->headphone_nid, VERB_GET_PARMS_PIN_CAPS, card) & PIN_CAPS_EAPD_CAPABLE)
        {
            bug("Enabling power amp of headphone port\n");
            send_command_12(card->codecnr, card->headphone_nid, VERB_SET_EAPD, 0x2, card); // enable EAPD (external power amp)
        }
        
    if (speaker != -1)
    {
        if (card->headphone_nid != 255 && // headphone found NID and
            is_jack_connected(card, card->headphone_nid) == FALSE) // no headphone connected -> switch on the speaker
        {
            send_command_12(card->codecnr, speaker, VERB_SET_PIN_WIDGET_CONTROL, 0x40, card); // output enabled
            card->speaker_active = TRUE;
        
            if (card->dac_volume_nid == 0)
            {
                bug("Volume NID has not been set yet, so let's see if the speaker has volume control...\n");
                parm = get_parameter(speaker, VERB_GET_PARMS_AUDIO_WIDGET_CAPS, card);
                bug("Audio widget caps for speaker = %lx\n", parm);
                if (parm & 0x4) // OutAmpPre
                {
                    card->dac_volume_nid = speaker;
                    bug("Speaker seems to have volume control\n");
                }
                else
                {
                    bug("Speaker did not have volume control\n");
                }
            }
        }
        else if (card->headphone_nid != 255 && // headphone found NID and
                 is_jack_connected(card, card->headphone_nid) == TRUE) // headphone connected -> switch off the speaker
        {
            //send_command_4(card->codecnr, speaker, VERB_SET_AMP_GAIN, OUTPUT_AMP_GAIN | AMP_GAIN_LR | (1 << 7), card); // set amplifier gain: mute output
            send_command_12(card->codecnr, speaker, VERB_SET_PIN_WIDGET_CONTROL, 0x0, card); // output disabled
        }
    }
	
	
	   UBYTE before_front = 0;
    if (front != -1)
    {
        // now the hard part: see if the input of FRONT is equal to the DAC NID
   	    before_front = (UBYTE) send_command_12(card->codecnr, front, VERB_GET_CONNECTION_LIST_ENTRY, 0, card);
   	}
   	else if (speaker != -1)
   	{
   	    bug("setting before_front to before speaker\n");
   	    if (card->headphone_nid != 255 &&
            is_jack_connected(card, card->headphone_nid) == FALSE)
        {
   	        before_front = (UBYTE) send_command_12(card->codecnr, speaker, VERB_GET_CONNECTION_LIST_ENTRY, 0, card);
   	    }
   	    else // use headphone
   	    {
   	        before_front = (UBYTE) send_command_12(card->codecnr, card->headphone_nid, VERB_GET_CONNECTION_LIST_ENTRY, 0, card);
   	    }
   	}
	
  if (before_front != dac)
	 {
	    bug("The widget before front (%xh) is not equal to DAC!\n", before_front);
	    
	    send_command_4(card->codecnr, before_front, VERB_SET_AMP_GAIN, INPUT_AMP_GAIN | AMP_GAIN_LR, card); // unmute PCM at index 0
	    bug("Let's hope it was a mute that now got unmuted!\n");
	    
	    bug("audio widget caps for before_front= %lx\n", get_parameter(before_front, VERB_GET_PARMS_AUDIO_WIDGET_CAPS, card));
	    
	    if (card->dac_volume_nid == 0) // volume NID not set yet
	    {
	        bug("Checking if before_front has volume control\n");
	        parm = get_parameter(before_front, VERB_GET_PARMS_AUDIO_WIDGET_CAPS, card);
         if (parm & 0x4) // OutAmpPre
         {
             card->dac_volume_nid = before_front;
             bug("before_front seems to have volume control\n");
        }
        else
        {
             bug("Odd, before_front doesn't seem to have volume control\n");
        }
	    }
	  }
	  else
	  {
	    bug("The widget before front or speaker is equal to DAC.\n");
	  }
	
	parm = get_parameter(card->dac_volume_nid, VERB_GET_PARMS_OUTPUT_AMP_CAPS , card);
    if ((parm & 0x7fffffff) == 0)
        parm = get_parameter(0x1, VERB_GET_PARMS_OUTPUT_AMP_CAPS, card);
	bug("Output amp caps = %lx\n", parm);
	
	step_size = (((parm >> 16) & 0x7F) + 1) * 0.25;
	steps = ((parm >> 8) & 0x7F);
	offset0dB = (parm & 0x7F);
	
	card->dac_min_gain = -(offset0dB * step_size);
    card->dac_max_gain = card->dac_min_gain + step_size * steps;
    card->dac_step_gain = step_size;
    bug("Gain step size = %lu * 0.25 dB, max gain = %d\n", (((parm >> 16) & 0x7F) + 1), (int) (card->dac_max_gain));
	
	return TRUE;
}


static int find_pin_widget_with_encoding(struct HDAudioChip *card, UBYTE encoding)
{
    int i;
    ULONG node_count_response = get_parameter(0, VERB_GET_PARMS_NODE_COUNT, card);
    UBYTE node_count = node_count_response & 0xFF;
    UBYTE starting_node = (node_count_response >> 16) & 0xFF;
   
    for (i = 0; i < node_count; i++)
    {
        ULONG function_group_response = get_parameter(starting_node + i, VERB_GET_PARMS_FUNCTION_GROUP_TYPE, card);
        UBYTE function_group = function_group_response & 0xFF;
        //bug("Function group = %x, UnSol cap = %x\n", function_group, (function_group_response >> 8) & 0x1);

        if (function_group == AUDIO_FUNCTION)
        {
            int j;

            ULONG subnode_count_response = get_parameter(starting_node + i, VERB_GET_PARMS_NODE_COUNT, card);
            UBYTE subnode_count = subnode_count_response & 0xFF;
            UBYTE sub_starting_node = (subnode_count_response >> 16) & 0xFF;
            ULONG config_default;


            for (j = 0; j < subnode_count; j++) // widgets
            {
                const ULONG NID = j + sub_starting_node;
                ULONG widget_caps;

                widget_caps = get_parameter(NID, VERB_GET_PARMS_AUDIO_WIDGET_CAPS, card);
                
                if (((widget_caps >> 20) & 0xF) == 4) // node is a pin widget
                {
                    config_default = send_command_12(card->codecnr, NID, VERB_GET_CONFIG_DEFAULT, 0, card);
                    
                    if (((config_default >> 20) & 0xF) == encoding)
                    {
                       bug("Config default for NID %x = %x\n", NID, config_default);
                       return (int) (NID);
                    }  
                }
            }
        }
    }
    
    return -1;
}


static int find_speaker_nid(struct HDAudioChip *card)
{
    int i;
    ULONG node_count_response = get_parameter(0, VERB_GET_PARMS_NODE_COUNT, card);
    UBYTE node_count = node_count_response & 0xFF;
    UBYTE starting_node = (node_count_response >> 16) & 0xFF;
   
    for (i = 0; i < node_count; i++)
    {
        ULONG function_group_response = get_parameter(starting_node + i, VERB_GET_PARMS_FUNCTION_GROUP_TYPE, card);
        UBYTE function_group = function_group_response & 0xFF;
        //bug("Function group = %x, UnSol cap = %x\n", function_group, (function_group_response >> 8) & 0x1);

        if (function_group == AUDIO_FUNCTION)
        {
            int j;

            ULONG subnode_count_response = get_parameter(starting_node + i, VERB_GET_PARMS_NODE_COUNT, card);
            UBYTE subnode_count = subnode_count_response & 0xFF;
            UBYTE sub_starting_node = (subnode_count_response >> 16) & 0xFF;
            ULONG config_default;


            for (j = 0; j < subnode_count; j++) // widgets
            {
                const ULONG NID = j + sub_starting_node;
                ULONG widget_caps;

                widget_caps = get_parameter(NID, VERB_GET_PARMS_AUDIO_WIDGET_CAPS, card);
                
                if (AUDIO_WIDGET_CAPS(widget_caps) == 4) // node is a pin widget
                {
                    config_default = send_command_12(card->codecnr, NID, VERB_GET_CONFIG_DEFAULT, 0, card);
                    
                    if ( (((config_default >> 20) & 0xF) == 1) && // the default device/use is a speaker
                         (((config_default >> 16) & 0xB) == 3)) // connection type is internal
                    {
                       return (int) (NID);
                    }  
                    //else bug("Pin widget 0x%x not a speaker: config_default=0x%lx\n", NID, config_default);
                }
            }
        }
    }
    
    return -1;
}


static int find_headphone_nid(struct HDAudioChip *card)
{
    int i;
    ULONG node_count_response = get_parameter(0, VERB_GET_PARMS_NODE_COUNT, card);
    UBYTE node_count = node_count_response & 0xFF;
    UBYTE starting_node = (node_count_response >> 16) & 0xFF;
   
    for (i = 0; i < node_count; i++)
    {
        ULONG function_group_response = get_parameter(starting_node + i, VERB_GET_PARMS_FUNCTION_GROUP_TYPE, card);
        UBYTE function_group = function_group_response & 0xFF;
        //bug("Function group = %x, UnSol cap = %x\n", function_group, (function_group_response >> 8) & 0x1);

        if (function_group == AUDIO_FUNCTION)
        {
            int j;

            ULONG subnode_count_response = get_parameter(starting_node + i, VERB_GET_PARMS_NODE_COUNT, card);
            UBYTE subnode_count = subnode_count_response & 0xFF;
            UBYTE sub_starting_node = (subnode_count_response >> 16) & 0xFF;
            ULONG config_default;


            for (j = 0; j < subnode_count; j++) // widgets
            {
                const ULONG NID = j + sub_starting_node;
                ULONG widget_caps;

                widget_caps = get_parameter(NID, VERB_GET_PARMS_AUDIO_WIDGET_CAPS, card);
                
                if (AUDIO_WIDGET_CAPS(widget_caps) == 4) // node is a pin widget
                {
                    config_default = send_command_12(card->codecnr, NID, VERB_GET_CONFIG_DEFAULT, 0, card);
                    
                    if ( (((config_default >> 20) & 0xF) == 2) && // the default device/use is a headphone output
                         (((config_default >> 16) & 0xF) == 1) ) // connection type is mini-jack
                    {
                       return (int) (NID);
                    }  
                }
            }
        }
    }
    
    return -1;
}


static int find_audio_output(struct HDAudioChip *card, UBYTE digital)
{
    int i;
    ULONG node_count_response = get_parameter(0, VERB_GET_PARMS_NODE_COUNT, card);
    UBYTE node_count = node_count_response & 0xFF;
    UBYTE starting_node = (node_count_response >> 16) & 0xFF;
   
    for (i = 0; i < node_count; i++)
    {
        ULONG function_group_response = get_parameter(starting_node + i, VERB_GET_PARMS_FUNCTION_GROUP_TYPE, card);
        UBYTE function_group = function_group_response & 0xFF;
        //bug("Function group = %x, UnSol cap = %x\n", function_group, (function_group_response >> 8) & 0x1);

        if (function_group == AUDIO_FUNCTION)
        {
            int j;

            ULONG subnode_count_response = get_parameter(starting_node + i, VERB_GET_PARMS_NODE_COUNT, card);
            UBYTE subnode_count = subnode_count_response & 0xFF;
            UBYTE sub_starting_node = (subnode_count_response >> 16) & 0xFF;
            ULONG config_default;


            for (j = 0; j < subnode_count; j++) // widgets
            {
                const ULONG NID = j + sub_starting_node;
                ULONG widget_caps;

                widget_caps = get_parameter(NID, VERB_GET_PARMS_AUDIO_WIDGET_CAPS, card);
                
                if (((widget_caps >> 20) & 0xF) == 0) // audio output
                {
                    if ((widget_caps & 0x1) == 1 && // stereo
                        ((widget_caps >> 9) & 0x1) == digital)
                    {
                        return (int) (NID);
                    }  
                }
            }
        }
    }
    
    return -1;
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
        bug("dac_nid didn't have a list of sample rates, trying AFG node\n");
    }
  
    // count number of frequencies
    for (i = 0; i < 12; i++)
    {
        if (samplerate_flags & (1 << i))
        {
            freqs++;
        }
    }
    
    bug("Frequencies found = %lu\n", freqs);
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
        bug("44100 Hz is not supported!\n");
        if (freqs > 0)
        {
            bug("Setting default frequency to %lu\n", card->frequencies[0].frequency);
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
        
        default: bug("Unsupported frequency!\n");
                 break;
    }
}


void set_monitor_volumes(struct HDAudioChip *card, double dB)
{
    int i;
    int dB_steps = (int) ((dB + 34.5) / 1.5);

    if (dB_steps < 0)
    {
        dB_steps = 0;
    }
    else if (dB_steps > 31)
    {
        dB_steps = 31;
    }

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
                    send_command_4(card->codecnr, card->adc_mixer_nid, VERB_SET_AMP_GAIN,
                                   INPUT_AMP_GAIN | AMP_GAIN_LR | (card->adc_mixer_indices[i] << 8), card);
                }
                else // mute
                {
                    send_command_4(card->codecnr, card->adc_mixer_nid, VERB_SET_AMP_GAIN,
                                   INPUT_AMP_GAIN | AMP_GAIN_LR | (card->adc_mixer_indices[i] << 8) | (1 << 7), card);
            
                }
            }
        }
    }
}


void set_adc_gain(struct HDAudioChip *card, double dB)
{
    int i;
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

    send_command_4(card->codecnr, card->dac_volume_nid, VERB_SET_AMP_GAIN, OUTPUT_AMP_GAIN | AMP_GAIN_LR | dB_steps, card);
}


void switch_nid_to_input(struct HDAudioChip *card, UBYTE NID)
{
    ULONG data = send_command_12(card->codecnr, NID, VERB_GET_PIN_WIDGET_CONTROL , 0, card);
    
    data |= 0x20; // input enable
    data &= ~(0x40); // output disable
    send_command_12(card->codecnr, NID, VERB_SET_PIN_WIDGET_CONTROL, data, card);
}


void switch_nid_to_output(struct HDAudioChip *card, UBYTE NID)
{
    ULONG data = send_command_12(card->codecnr, NID, VERB_GET_PIN_WIDGET_CONTROL , 0, card);
    
    data &= ~(0x20); // input disable
    data |= 0x40; // output enable
    send_command_12(card->codecnr, NID, VERB_SET_PIN_WIDGET_CONTROL, data, card);
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
        //bug("jack connected\n");
        return TRUE;
    }
    else
    {
        //bug("jack disconnected\n");
        return FALSE;
    }
}



void detect_headphone_change(struct HDAudioChip *card)
{
    if (card->speaker_nid != 255 &&
        card->headphone_nid != 255)
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

