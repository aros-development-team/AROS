/* ata_pthru.c - ATA pass through for ata.mod.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/ata.h>
#include <grub/disk.h>
#include <grub/dl.h>
#include <grub/mm.h>


/* ATA pass through support, used by hdparm.mod.  */
static grub_err_t
grub_ata_pass_through (grub_disk_t disk,
		       struct grub_disk_ata_pass_through_parms *parms)
{
  if (disk->dev->id != GRUB_DISK_DEVICE_ATA_ID)
    return grub_error (GRUB_ERR_BAD_DEVICE,
		       "device not accessed via ata.mod");

  struct grub_ata_device *dev = (struct grub_ata_device *) disk->data;

  if (! (parms->size == 0 || parms->size == GRUB_DISK_SECTOR_SIZE))
    return grub_error (GRUB_ERR_NOT_IMPLEMENTED_YET,
		       "ATA multi-sector read and DATA OUT not implemented");

  grub_dprintf ("ata", "ata_pass_through: cmd=0x%x, features=0x%x, sectors=0x%x\n",
		parms->taskfile[GRUB_ATA_REG_CMD],
		parms->taskfile[GRUB_ATA_REG_FEATURES],
		parms->taskfile[GRUB_ATA_REG_SECTORS]);
  grub_dprintf ("ata", "lba_high=0x%x, lba_mid=0x%x, lba_low=0x%x, size=%d\n",
	        parms->taskfile[GRUB_ATA_REG_LBAHIGH],
	        parms->taskfile[GRUB_ATA_REG_LBAMID],
	        parms->taskfile[GRUB_ATA_REG_LBALOW], parms->size);

  /* Set registers.  */
  grub_ata_regset (dev, GRUB_ATA_REG_DISK, 0xE0 | dev->device << 4
		   | (parms->taskfile[GRUB_ATA_REG_DISK] & 0xf));
  if (grub_ata_check_ready (dev))
    return grub_errno;

  int i;
  for (i = GRUB_ATA_REG_FEATURES; i <= GRUB_ATA_REG_LBAHIGH; i++)
    grub_ata_regset (dev, i, parms->taskfile[i]);

  /* Start command. */
  grub_ata_regset (dev, GRUB_ATA_REG_CMD, parms->taskfile[GRUB_ATA_REG_CMD]);

  /* Wait for !BSY.  */
  if (grub_ata_wait_not_busy (dev, GRUB_ATA_TOUT_DATA))
    return grub_errno;

  /* Check status.  */
  grub_int8_t sts = grub_ata_regget (dev, GRUB_ATA_REG_STATUS);
  grub_dprintf ("ata", "status=0x%x\n", sts);

  /* Transfer data.  */
  if ((sts & (GRUB_ATA_STATUS_DRQ | GRUB_ATA_STATUS_ERR)) == GRUB_ATA_STATUS_DRQ)
    {
      if (parms->size != GRUB_DISK_SECTOR_SIZE)
        return grub_error (GRUB_ERR_READ_ERROR, "DRQ unexpected");
      grub_ata_pio_read (dev, parms->buffer, GRUB_DISK_SECTOR_SIZE);
    }

  /* Return registers.  */
  for (i = GRUB_ATA_REG_ERROR; i <= GRUB_ATA_REG_STATUS; i++)
    parms->taskfile[i] = grub_ata_regget (dev, i);

  grub_dprintf ("ata", "status=0x%x, error=0x%x, sectors=0x%x\n",
	        parms->taskfile[GRUB_ATA_REG_STATUS],
	        parms->taskfile[GRUB_ATA_REG_ERROR],
		parms->taskfile[GRUB_ATA_REG_SECTORS]);

  if (parms->taskfile[GRUB_ATA_REG_STATUS]
      & (GRUB_ATA_STATUS_DRQ | GRUB_ATA_STATUS_ERR))
    return grub_error (GRUB_ERR_READ_ERROR, "ATA passthrough failed");

  return GRUB_ERR_NONE;
}



GRUB_MOD_INIT(ata_pthru)
{
  /* Register ATA pass through function.  */
  grub_disk_ata_pass_through = grub_ata_pass_through;
}

GRUB_MOD_FINI(ata_pthru)
{
  if (grub_disk_ata_pass_through == grub_ata_pass_through)
    grub_disk_ata_pass_through = NULL;
}
