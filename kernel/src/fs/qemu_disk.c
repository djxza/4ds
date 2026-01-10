#include "../io.h"
#include "../stdlib.h"
#include "../string.h"
#include "qemu_disk.h"

// QEMU IDE PIO disk driver
#define IDE_PORT_BASE 0x1F0
#define IDE_STATUS (IDE_PORT_BASE + 7)

#define IDE_STATUS_BSY 0x80
#define IDE_STATUS_RDY 0x40
#define IDE_STATUS_DRQ 0x08
#define IDE_STATUS_ERR 0x01

static int disk_ready = 0;

// Wait for IDE to be ready (with timeout)
static int ide_wait(int check_error) {
  int timeout = 100000;

  while (timeout-- > 0) {
    u8 status = inb(IDE_STATUS);

    if (status & IDE_STATUS_BSY)
      continue;

    if (check_error && (status & IDE_STATUS_ERR))
      return -1;

    if (status & IDE_STATUS_RDY)
      return 0;
  }

  return -1;
}

// Simple disk read for QEMU
int qemu_disk_read_sectors(u32 lba, u32 count, void *buffer) {
  if (!disk_ready) {
    printb_str("[DISK] Disk not ready!\n");
    return -1;
  }

  if (count == 0 || !buffer) {
    return -1;
  }

  printb_str("[DISK] Reading LBA=");
  printb_dec(lba);
  printb_str(", sectors=");
  printb_dec(count);
  printb_str("\n");

  u16 *dest = (u16 *)buffer;

  for (u32 sector = 0; sector < count; sector++) {
    u32 current_lba = lba + sector;

    // Wait for controller
    if (ide_wait(1) < 0) {
      printb_str("[DISK] Controller not ready\n");
      return -1;
    }

    // Send LBA and command
    outb(IDE_PORT_BASE + 2, 1);                          // Sector count
    outb(IDE_PORT_BASE + 3, current_lba & 0xFF);         // LBA low
    outb(IDE_PORT_BASE + 4, (current_lba >> 8) & 0xFF);  // LBA mid
    outb(IDE_PORT_BASE + 5, (current_lba >> 16) & 0xFF); // LBA high
    outb(IDE_PORT_BASE + 6,
         0xE0 | ((current_lba >> 24) & 0x0F)); // Drive select
    outb(IDE_PORT_BASE + 7, 0x20);             // READ SECTORS command

    // Wait for data
    if (ide_wait(1) < 0) {
      printb_str("[DISK] Error waiting for data\n");
      return -1;
    }

    // Check if data is ready
    u8 status = inb(IDE_STATUS);
    if (!(status & IDE_STATUS_DRQ)) {
      printb_str("[DISK] Data not ready, status=0x");
      printb_hex(status);
      printb_str("\n");
      return -1;
    }

    // Read 256 words (512 bytes)
    for (int i = 0; i < 256; i++) {
      dest[i] = inw(IDE_PORT_BASE);
    }

    dest += 256;
  }

  printb_str("[DISK] Read successful\n");
  return 0;
}

int qemu_disk_write_sectors(u32 lba, u32 count, void *buffer) {
  printb_str("[DISK] Write not implemented - LBA=");
  printb_dec(lba);
  printb_str(", sectors=");
  printb_dec(count);
  printb_str("\n");
  return 0;
}

void qemu_disk_init(void) {
  printb_str("Initializing QEMU IDE disk...\n");

  // Simple initialization - just try to read sector 0
  u8 sector_buffer[512];

  // Try a simple read to test the disk
  int result = qemu_disk_read_sectors(0, 1, sector_buffer);

  if (result == 0) {
    disk_ready = 1;
    printb_str("QEMU disk initialized successfully\n");

    // Print first few bytes of MBR for verification
    printb_str("MBR first 16 bytes: ");
    for (int i = 0; i < 16; i++) {
      printb_hex(sector_buffer[i]);
      printb_str(" ");
    }
    printb_str("\n");
  } else {
    printb_str("QEMU disk initialization failed - using dummy driver\n");
    disk_ready = 0; // Will use dummy driver
  }
}
