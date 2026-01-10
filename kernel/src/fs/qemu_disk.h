#ifndef _QEMU_DISK_H
#define _QEMU_DISK_H

#include "../stdlib.h"

int qemu_disk_read_sectors(u32 lba, u32 count, void *buffer);
int qemu_disk_write_sectors(u32 lba, u32 count, void *buffer);
void qemu_disk_init(void);

#endif // _QEMU_DISK_H
