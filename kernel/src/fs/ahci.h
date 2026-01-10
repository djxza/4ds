#ifndef _AHCI_H
#define _AHCI_H

#include "../stdlib.h"

// Disk operations for AHCI (SATA)
int ahci_read_sectors(u32 lba, u32 count, void *buffer);
int ahci_write_sectors(u32 lba, u32 count, void *buffer);
void ahci_init(void);

#endif // _AHCI_H
