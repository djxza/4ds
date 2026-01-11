#include "ide.h"
#include "../io.h"
#include "../stdlib.h"
#include "../string.h"

// IDE I/O Ports
#define IDE_PORT_BASE 0x1F0
#define IDE_PORT_CTRL 0x3F6

// Register offsets
#define IDE_DATA (IDE_PORT_BASE + 0)
#define IDE_ERROR (IDE_PORT_BASE + 1)
#define IDE_FEATURES (IDE_PORT_BASE + 1)
#define IDE_SECTOR_CNT (IDE_PORT_BASE + 2)
#define IDE_LBA_LOW (IDE_PORT_BASE + 3)
#define IDE_LBA_MID (IDE_PORT_BASE + 4)
#define IDE_LBA_HIGH (IDE_PORT_BASE + 5)
#define IDE_DRIVE_SEL (IDE_PORT_BASE + 6)
#define IDE_STATUS (IDE_PORT_BASE + 7)
#define IDE_COMMAND (IDE_PORT_BASE + 7)

// Alternate Status Register
#define IDE_ALT_STATUS (IDE_PORT_CTRL + 0)

// Status bits
#define IDE_STATUS_BSY 0x80 // Busy
#define IDE_STATUS_RDY 0x40 // Ready
#define IDE_STATUS_DRQ 0x08 // Data Request
#define IDE_STATUS_ERR 0x01 // Error

// Commands
#define IDE_CMD_READ 0x20     // Read Sectors with Retry
#define IDE_CMD_WRITE 0x30    // Write Sectors with Retry
#define IDE_CMD_IDENTIFY 0xEC // Identify Device

// Drive selection
#define IDE_DRIVE_MASTER 0xE0
#define IDE_DRIVE_SLAVE 0xF0

static int ide_ready = 0;

// Wait for IDE to be ready
static int ide_wait_ready(void) {
  int timeout = 100000;

  while (timeout-- > 0) {
    u8 status = inb(IDE_STATUS);
    if (!(status & IDE_STATUS_BSY)) {
      if (status & IDE_STATUS_RDY) {
        return 0; // Ready
      }
    }
    io_wait();
  }

  printb_str("[IDE] Timeout waiting for ready\n");
  return -1;
}

// Wait for data request
static int ide_wait_data(void) {
  int timeout = 100000;

  while (timeout-- > 0) {
    u8 status = inb(IDE_STATUS);
    if (status & IDE_STATUS_ERR) {
      printb_str("[IDE] Error while waiting for data\n");
      return -1;
    }
    if (status & IDE_STATUS_DRQ) {
      return 0; // Data ready
    }
    io_wait();
  }

  printb_str("[IDE] Timeout waiting for data\n");
  return -1;
}

// Initialize IDE controller
void ide_init(void) {
  printb_str("Initializing IDE controller...\n");

  // Disable interrupts
  outb(IDE_ALT_STATUS, 0x02);

  // Select master drive
  outb(IDE_DRIVE_SEL, IDE_DRIVE_MASTER);

  // Wait a bit
  for (volatile int i = 0; i < 1000; i++)
    ;

  // Check if drive exists
  outb(IDE_SECTOR_CNT, 0);
  outb(IDE_LBA_LOW, 0);
  outb(IDE_LBA_MID, 0);
  outb(IDE_LBA_HIGH, 0);
  outb(IDE_COMMAND, IDE_CMD_IDENTIFY);

  // Wait for status
  if (ide_wait_ready() < 0) {
    printb_str("[IDE] No response from drive - assuming it exists\n");
    ide_ready = 1; // Assume it's there for QEMU
    return;
  }

  // Check if command was accepted
  u8 status = inb(IDE_STATUS);
  if (status == 0) {
    printb_str("[IDE] No drive detected\n");
    return;
  }

  // Wait for data
  if (ide_wait_data() < 0) {
    printb_str("[IDE] No data from identify command\n");
    return;
  }

  // Read identify data (we don't need it, just clear the buffer)
  for (int i = 0; i < 256; i++) {
    inw(IDE_DATA);
  }

  ide_ready = 1;
  printb_str("IDE controller ready\n");
}

// Read sectors using IDE PIO
int ide_read_sectors(u32 lba, u32 count, void *buffer) {
  if (!ide_ready) {
    printb_str("[IDE] Controller not ready\n");
    return -1;
  }

  if (count == 0 || !buffer) {
    return -1;
  }

  // Only handle single sector reads for now
  if (count > 1) {
    printb_str("[IDE] Multi-sector read not implemented\n");
    return -1;
  }

  printb_str("[IDE] Reading LBA=");
  printb_dec(lba);
  printb_str("\n");

  // Wait for controller to be ready
  if (ide_wait_ready() < 0) {
    return -1;
  }

  // Select drive and LBA
  outb(IDE_DRIVE_SEL, IDE_DRIVE_MASTER | ((lba >> 24) & 0x0F));

  // Send sector count
  outb(IDE_SECTOR_CNT, 1);

  // Send LBA
  outb(IDE_LBA_LOW, lba & 0xFF);
  outb(IDE_LBA_MID, (lba >> 8) & 0xFF);
  outb(IDE_LBA_HIGH, (lba >> 16) & 0xFF);

  // Send read command
  outb(IDE_COMMAND, IDE_CMD_READ);

  // Wait for data
  if (ide_wait_data() < 0) {
    return -1;
  }

  // Read data (256 words = 512 bytes)
  u16 *dest = (u16 *)buffer;
  for (int i = 0; i < 256; i++) {
    dest[i] = inw(IDE_DATA);
  }

  // Verify boot sector signature
  u8 *buf8 = (u8 *)buffer;
  if (buf8[510] == 0x55 && buf8[511] == 0xAA) {
    printb_str("[IDE] Valid boot sector found\n");
  } else {
    printb_str("[IDE] Warning: No boot sector signature\n");
  }

  printb_str("[IDE] Read successful\n");
  return 0;
}

int ide_write_sectors(u32 lba, u32 count, void *buffer) {
  printb_str("[IDE] Write not implemented\n");
  return -1;
}
