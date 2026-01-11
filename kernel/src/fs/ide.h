#ifndef _IDE_H
#define _IDE_H

#include "../stdlib.h"

void ide_init(void);
int ide_read_sectors(u32 lba, u32 count, void *buffer);
int ide_write_sectors(u32 lba, u32 count, void *buffer);

#endif // _IDE_H
