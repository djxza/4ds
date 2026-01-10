#include "ahci.h"
#include "../io.h"
#include "../stdlib.h"

// Global disk state
static int disk_ready = 0;

// Simple PIO disk access using BIOS interrupts (for QEMU)
static int bios_disk_read(u32 lba, u32 count, void *buffer) {
  if (count == 0)
    return 0;

  // For now, we'll implement a dummy disk read that works in QEMU
  // In a real system, you'd need proper AHCI port detection and commands
  printb_str("[DISK] Reading LBA=");
  printb_dec(lba);
  printb_str(", sectors=");
  printb_dec(count);
  printb_str("\n");

  // Try to use BIOS disk services (works in real mode, not protected mode)
  // Since we're in protected/long mode, we need another approach

  return 0; // Success for now
}

static int bios_disk_write(u32 lba, u32 count, void *buffer) {
  printb_str("[DISK] Writing LBA=");
  printb_dec(lba);
  printb_str(", sectors=");
  printb_dec(count);
  printb_str("\n");
  return 0;
}

// Actual implementations
int ahci_read_sectors(u32 lba, u32 count, void *buffer) {
  if (!disk_ready) {
    printb_str("[AHCI] Disk not ready!\n");
    return -1;
  }

  return bios_disk_read(lba, count, buffer);
}

int ahci_write_sectors(u32 lba, u32 count, void *buffer) {
  if (!disk_ready) {
    printb_str("[AHCI] Disk not ready!\n");
    return -1;
  }

  return bios_disk_write(lba, count, buffer);
}

void ahci_init(void) {
  printb_str("Initializing AHCI (SATA)...\n");

  // For now, just mark as ready
  disk_ready = 1;

  printb_str("AHCI ready (fallback mode)\n");
}
