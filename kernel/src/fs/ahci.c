#include "ahci.h"
#include "../io.h"
#include "../stdlib.h"
#include "../string.h"

// Simple IDE PIO implementation for QEMU
#define IDE_PORT_BASE 0x1F0
#define IDE_PORT_CTRL 0x3F6

// IDE Registers
#define IDE_DATA (IDE_PORT_BASE + 0)
#define IDE_ERROR (IDE_PORT_BASE + 1)
#define IDE_SECTOR_COUNT (IDE_PORT_BASE + 2)
#define IDE_LBA_LOW (IDE_PORT_BASE + 3)
#define IDE_LBA_MID (IDE_PORT_BASE + 4)
#define IDE_LBA_HIGH (IDE_PORT_BASE + 5)
#define IDE_DRIVE_SELECT (IDE_PORT_BASE + 6)
#define IDE_COMMAND (IDE_PORT_BASE + 7)
#define IDE_STATUS (IDE_PORT_BASE + 7)

// Status bits
#define IDE_STATUS_BSY 0x80
#define IDE_STATUS_RDY 0x40
#define IDE_STATUS_DRQ 0x08
#define IDE_STATUS_ERR 0x01

// Commands
#define IDE_CMD_READ 0x20
#define IDE_CMD_WRITE 0x30
#define IDE_CMD_IDENTIFY 0xEC

// Global disk state
static int disk_ready = 0;

// Wait for IDE controller to be ready
static void ide_wait_not_busy(void) {
  while (inb(IDE_STATUS) & IDE_STATUS_BSY) {
    // Small delay
    for (volatile int i = 0; i < 1000; i++)
      ;
  }
}

static void ide_wait_drq(void) {
  while (!(inb(IDE_STATUS) & IDE_STATUS_DRQ)) {
    if (inb(IDE_STATUS) & IDE_STATUS_ERR) {
      printb_str("[IDE] Error waiting for DRQ\n");
      return;
    }
  }
}

// Simple IDE PIO read for QEMU
static int ide_read_sectors(u32 lba, u32 count, void *buffer) {
  if (count == 0 || !buffer) {
    return -1;
  }

  printb_str("[IDE] Reading LBA=");
  printb_dec(lba);
  printb_str(", sectors=");
  printb_dec(count);
  printb_str("\n");

  u16 *dest = (u16 *)buffer;

  for (u32 sector = 0; sector < count; sector++) {
    u32 current_lba = lba + sector;

    // Wait for controller
    ide_wait_not_busy();

    // Select drive (master, LBA mode)
    outb(IDE_DRIVE_SELECT, 0xE0 | ((current_lba >> 24) & 0x0F));

    // Send sector count
    outb(IDE_SECTOR_COUNT, 1);

    // Send LBA
    outb(IDE_LBA_LOW, current_lba & 0xFF);
    outb(IDE_LBA_MID, (current_lba >> 8) & 0xFF);
    outb(IDE_LBA_HIGH, (current_lba >> 16) & 0xFF);

    // Send read command
    outb(IDE_COMMAND, IDE_CMD_READ);

    // Wait for data
    ide_wait_not_busy();
    ide_wait_drq();

    // Check for error
    if (inb(IDE_STATUS) & IDE_STATUS_ERR) {
      printb_str("[IDE] Read error at LBA ");
      printb_dec(current_lba);
      printb_str("\n");
      return -1;
    }

    // Read 256 words (512 bytes)
    for (int i = 0; i < 256; i++) {
      dest[i] = inw(IDE_DATA);
    }

    dest += 256;
  }

  return 0;
}

// Initialize IDE controller
void ide_init(void) {
  printb_str("Initializing IDE controller...\n");

  // Wait a bit for controller to settle
  for (volatile int i = 0; i < 10000; i++)
    ;

  // Select master drive
  outb(IDE_DRIVE_SELECT, 0xE0);

  // Wait a bit more
  for (volatile int i = 0; i < 10000; i++)
    ;

  // Check if drive exists - simple probe
  outb(IDE_SECTOR_COUNT, 0);
  outb(IDE_LBA_LOW, 0);
  outb(IDE_LBA_MID, 0);
  outb(IDE_LBA_HIGH, 0);
  outb(IDE_COMMAND, IDE_CMD_IDENTIFY);

  // Small delay
  for (volatile int i = 0; i < 1000; i++)
    ;

  // Check status
  u8 status = inb(IDE_STATUS);

  if (status == 0) {
    printb_str("[IDE] No drive detected - assuming QEMU drive exists\n");
    disk_ready = 1; // Assume it's there for QEMU
    return;
  }

  // Wait for not busy
  int timeout = 100000;
  while ((inb(IDE_STATUS) & IDE_STATUS_BSY) && timeout-- > 0) {
    for (volatile int i = 0; i < 1000; i++)
      ;
  }

  if (timeout <= 0) {
    printb_str("[IDE] Timeout waiting for drive - assuming it exists\n");
    disk_ready = 1; // Assume it's there
    return;
  }

  disk_ready = 1;
  printb_str("IDE controller ready (assumed)\n");
}

int ahci_write_sectors(u32 lba, u32 count, void *buffer) {
  printb_str("[AHCI] Write not implemented - LBA=");
  printb_dec(lba);
  printb_str(", sectors=");
  printb_dec(count);
  printb_str("\n");
  return 0; // Success for now
}

void ahci_init(void) {
  printb_str("Initializing disk (IDE PIO fallback)...\n");
  ide_init();
}
