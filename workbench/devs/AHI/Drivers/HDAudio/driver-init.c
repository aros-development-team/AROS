/*
The contents of this file are subject to the AROS Public License Version 1.1 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
http://www.aros.org/license.html

Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
ANY KIND, either express or implied. See the License for the specific language governing rights and
limitations under the License.

(C) Copyright xxxx-2009 Davy Wentzler.
(C) Copyright 2009-2010 Stephen Jones.

The Initial Developer of the Original Code is Davy Wentzler.

All Rights Reserved.
*/

#include <exec/memory.h>

#include <proto/expansion.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <clib/alib_protos.h>
#ifdef __AROS__
#include <aros/debug.h>
struct DosLibrary* DOSBase;
#endif
#include <stdlib.h>

#include "library.h"
#include "version.h"
#include "pci_wrapper.h"
#include "misc.h"



struct DriverBase* AHIsubBase;


struct VendorDevice
{
    UWORD vendor;
    UWORD device;
};


struct VendorDevice *vendor_device_list = NULL;
static int vendor_device_list_size = 0;

static void parse_config_file(void);
static int hex_char_to_int(char c);
#define MAX_DEVICE_VENDORS 512


/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/

BOOL DriverInit(struct DriverBase* ahisubbase)
{
    struct HDAudioBase* card_base = (struct HDAudioBase*) ahisubbase;
    struct PCIDevice *dev;
    int card_no;
    int i;

    D(bug("[HDAudio] %s()\n"), __PRETTY_FUNCTION__);
    AHIsubBase = ahisubbase;

    DOSBase = (struct DosLibrary *) OpenLibrary(DOSNAME, 37);

    if (DOSBase == NULL)
    {
        Req("Unable to open 'dos.library' version 37.\n");
        return FALSE;
    }

    if (!ahi_pci_init(ahisubbase))
    {
        return FALSE;
    }

    InitSemaphore(&card_base->semaphore);

    /*** Count cards ***********************************************************/

    vendor_device_list = (struct VendorDevice *) AllocVec(sizeof(struct VendorDevice) * MAX_DEVICE_VENDORS, MEMF_PUBLIC | MEMF_CLEAR);

    // Add Intel ICH7 (used in iMica)
    vendor_device_list[0].vendor = 0x8086;
    vendor_device_list[0].device = 0x27D8;
    vendor_device_list_size++;
    
    // Then parse the hdaudio.config file, if available in ENV:
    parse_config_file();
    
    D(bug("[HDAudio] vendor_device_list_size = %ld\n", vendor_device_list_size));
    
    card_base->cards_found = 0;
    dev = NULL;

    for (i = 0; i < vendor_device_list_size; i++)
    {
        dev = ahi_pci_find_device(vendor_device_list[i].vendor, vendor_device_list[i].device, dev);
        
        if (dev != NULL)
        {
            D(bug("[HDAudio] Found device with vendor ID = %x, device ID = %x!(i = %d)\n", vendor_device_list[i].vendor, vendor_device_list[i].device, i));
            ++card_base->cards_found;
            break; // stop at first found controller
        }
    }
    
    
    FreeVec(vendor_device_list);

    // Fail if no hardware (prevents the audio modes from being added to
    // the database if the driver cannot be used).

    if (card_base->cards_found == 0)
    {
        D(bug("[HDAudio] No HDaudio controller found! :-(\n"));
        return FALSE;
    }


    /*** Allocate and init all cards *******************************************/

    card_base->driverdatas = (struct HDAudioChip **) AllocVec(sizeof(*card_base->driverdatas) * card_base->cards_found, MEMF_PUBLIC);

    if (card_base->driverdatas == NULL)
    {
        D(bug("[HDAudio] Out of memory.\n"));
        return FALSE;
    }

    card_no = 0;

    if (dev)
    {
        card_base->driverdatas[card_no] = AllocDriverData(dev, AHIsubBase);
        if (card_base->driverdatas[card_no] == NULL)
        {
            FreeVec(card_base->driverdatas);
            return FALSE;
        }

        ++card_no;
    }

    D(bug("[HDAudio] exit init\n"));
    return TRUE;
}


/******************************************************************************
** Custom driver clean-up *****************************************************
******************************************************************************/

VOID DriverCleanup(struct DriverBase* AHIsubBase)
{
    struct HDAudioBase* card_base = (struct HDAudioBase*) AHIsubBase;
    int i;

    for(i = 0; i < card_base->cards_found; ++i)
    {
        FreeDriverData(card_base->driverdatas[i], AHIsubBase);
    }

    FreeVec(card_base->driverdatas); 

    ahi_pci_exit();

    if (DOSBase)
    {
        CloseLibrary((struct Library*) DOSBase);
    }
}


static void parse_config_file(void)
{
    BPTR config_file;
    BPTR handle;
    
    handle = Lock("ENV:hdaudio.config", SHARED_LOCK);
  
    if (handle == 0)
    {
        bug("No handle found\n");
        return;
    }
    
    UnLock(handle);
    
    config_file = Open("ENV:hdaudio.config", MODE_OLDFILE);

    if (config_file)
    {   
        BOOL Continue = TRUE;
        bug("Opened config file\n");
        
        while (Continue)
        {
            char *line = (char *) AllocVec(512, MEMF_CLEAR);
            char *ret;
            
            ret = FGets(config_file, line, 512);

            if (ret == NULL)
            {
                FreeVec(line);
                break;
            }
            
            if (ret[0] == '0' &&
                ret[1] == 'x' &&
                ret[6] == ',' &&
                ret[7] == ' ' &&
                ret[8] == '0' &&
                ret[9] == 'x' &&
                ret[15] == '\0')
            {
                int value;
                UWORD vendor, device;
                char *tmp = (char *) AllocVec(16, MEMF_CLEAR);
                char *tmp2 = (char *) AllocVec(4, MEMF_CLEAR);
                
                CopyMem(line + 2, tmp, 4);
                tmp[4] = '\0';
                
                // convert hex to decimal
                value = hex_char_to_int(tmp[0]) * 16 * 16 * 16 + hex_char_to_int(tmp[1]) * 16 * 16 + hex_char_to_int(tmp[2]) * 16 + hex_char_to_int(tmp[3]); 
                vendor = (UWORD) value;
                
                CopyMem(line + 10, tmp, 4);
                value = hex_char_to_int(tmp[0]) * 16 * 16 * 16 + hex_char_to_int(tmp[1]) * 16 * 16 + hex_char_to_int(tmp[2]) * 16 + hex_char_to_int(tmp[3]);
                device = (UWORD) value;
                //bug("Adding vendor = %x, device = %x to list, size = %ld\n", vendor, device, vendor_device_list_size);
                
                vendor_device_list[vendor_device_list_size].vendor = vendor;
                vendor_device_list[vendor_device_list_size].device = device;
                vendor_device_list_size++;
                
                if (vendor_device_list_size >= MAX_DEVICE_VENDORS)
                {
                    bug("Exceeded MAX_DEVICE_VENDORS\n");
                    break;
                }
            
                FreeVec(tmp);
                FreeVec(tmp2);
            }
            else if (ret[0] == 'Q' && ret[1] == 'U' && ret[2] == 'E' && ret[3] == 'R' && ret[4] == 'Y')
            {
                bug("QUERY found!\n");
                setForceQuery();
                
                if (ret[5] == 'D') // debug
                {
                    setDumpAll();
                }
            }
            else if (Strnicmp(ret, "speaker=0x", 10) == 0)
            {
                int speaker = 0;
                char *tmp = (char *) AllocVec(16, MEMF_CLEAR);
                
                CopyMem(line + 10, tmp, 2);
                tmp[2] = '\0';
                
                // convert hex to decimal
                speaker = hex_char_to_int(tmp[0]) * 16 + hex_char_to_int(tmp[1]);
                
                bug("Speaker in config = %x!\n", speaker);
                
                setForceSpeaker(speaker);
            }
            
            FreeVec(line);
        }
        
        Close(config_file);
    }
    else
    {
        bug("Couldn't open config file!\n");
    }
}


static int hex_char_to_int(char c)
{
    if (c >= '0' && c <= '9')
    {
        return (c - '0');
    }
    else if (c >= 'A' && c <= 'F')
    {
        return 10 + (c - 'A');
    }
    else if (c >= 'a' && c <= 'f')
    {
        return 10 + (c - 'a');
    }
    else
    {
        bug("Error in hex_char_to_int: char was %c\n", c);
        return 0;
    }
}
