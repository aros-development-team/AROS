/*
 * mmu.h
 *
 *  Created on: 08.04.2015
 *      Author: michal
 */

#ifndef _MMU_H_
#define _MMU_H_

#include <stdint.h>

void mmu_init();
void mmu_load();
void mmu_unmap_section(uint32_t virt, uint32_t length);
void mmu_map_section(uint32_t phys, uint32_t virt, uint32_t length, int b, int c, int ap, int tex);

#endif /* _MMU_H_ */
