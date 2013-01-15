/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef RASPI_AUX_H
#define RASPI_AUX_H

#include <exec/types.h>
#include <stdint.h>

#define RASPIAUX_PHYSBASE (IPTR)0x20215000

struct raspiaux {
    uint32_t padding1;
    uint32_t enables;
    uint32_t padding2[14];
    uint32_t mu_io_reg;
    uint32_t mu_ier_reg;
    uint32_t mu_iir_reg;
    uint32_t mu_lcr_reg;
    uint32_t mu_mcr_reg;
    uint32_t mu_lsr_reg;
    uint32_t mu_msr_reg;
    uint32_t mu_scratch;
    uint32_t mu_cntl_reg;
    uint32_t mu_stat_reg;
    uint32_t mu_baud_reg;
}__attribute__((packed));;

#endif /* RASPI_AUX */

