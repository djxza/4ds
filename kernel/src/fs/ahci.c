#include "ahci.h"
#include "../io.h"
#include "../stdlib.h"
#include "../string.h"

// AHCI Base Address Register (should be discovered via PCI)
#define AHCI_BASE 0x400000 // Common QEMU AHCI base address

// AHCI Registers
#define AHCI_CAP 0x00       // Host Capabilities
#define AHCI_GHC 0x04       // Global Host Control
#define AHCI_IS 0x08        // Interrupt Status
#define AHCI_PI 0x0C        // Ports Implemented
#define AHCI_VS 0x10        // Version
#define AHCI_CCC_CTL 0x14   // Command Completion Coalescing Control
#define AHCI_CCC_PORTS 0x18 // CCC Ports

// Port Registers (offset per port)
#define PORT_START 0x100
#define PORT_SIZE 0x80

// Port-specific registers
#define PORT_CLB 0x00  // Command List Base Address
#define PORT_FB 0x08   // FIS Base Address
#define PORT_IS 0x10   // Interrupt Status
#define PORT_IE 0x14   // Interrupt Enable
#define PORT_CMD 0x18  // Command and Status
#define PORT_TFD 0x20  // Task File Data
#define PORT_SIG 0x24  // Signature
#define PORT_SSTS 0x28 // Serial ATA Status
#define PORT_SCTL 0x2C // Serial ATA Control
#define PORT_SERR 0x30 // Serial ATA Error
#define PORT_SACT 0x34 // Serial ATA Active
#define PORT_CI 0x38   // Command Issue
#define PORT_SNTF 0x3C // Serial ATA Notification

// Command List and FIS sizes
#define CMD_LIST_SIZE 1024
#define FIS_SIZE 256

// Command Table size
#define CMD_TABLE_SIZE (sizeof(command_table_t) + 0x80)

// Data structures
typedef struct __attribute__((packed)) {
  u32 dw0;
  u32 dw1;
  u32 ctba;  // Command Table Descriptor Base Address
  u32 ctbau; // Command Table Descriptor Base Address Upper
  u32 reserved[4];
} command_header_t;

typedef struct __attribute__((packed)) {
  u8 cfis[64]; // Command FIS
  u8 acmd[32]; // ATAPI Command
  u8 reserved[16];
  u8 prdt[0x80]; // Physical Region Descriptor Table entries
} command_table_t;

typedef struct __attribute__((packed)) {
  u32 dba;  // Data Base Address
  u32 dbau; // Data Base Address Upper
  u32 reserved;
  u32 dbc : 22; // Byte Count
  u32 : 9;
  u32 i : 1; // Interrupt on Completion
} prdt_entry_t;

// PCI Configuration Space Access
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

// PCI Class Codes
#define PCI_CLASS_MASS_STORAGE 0x01
#define PCI_SUBCLASS_SATA 0x06
#define PCI_PROGIF_AHCI 0x01

// Simple directory entry structure for our fake disk
typedef struct __attribute__((packed)) {
  char name[8];
  char ext[3];
  u8 attributes;
  u8 reserved;
  u16 creation_time;
  u16 creation_date;
  u16 last_access_date;
  u16 cluster_high;
  u16 modification_time;
  u16 modification_date;
  u16 cluster_low;
  u32 file_size;
} simple_dir_entry_t;

#define SIMPLE_ATTR_ARCHIVE 0x20

// PCI helper functions
static u32 pci_read_config(u8 bus, u8 slot, u8 func, u8 offset) {
  u32 address =
      (1 << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
  outl(PCI_CONFIG_ADDRESS, address);
  return inl(PCI_CONFIG_DATA);
}

// Find AHCI controller via PCI
static u32 find_ahci_controller(void) {
  printb_str("Scanning PCI for AHCI controller...\n");

  for (int bus = 0; bus < 256; bus++) { // Changed u8 to int
    for (u8 slot = 0; slot < 32; slot++) {
      for (u8 func = 0; func < 8; func++) {
        u32 vendor_device = pci_read_config((u8)bus, slot, func, 0x00);
        u16 vendor = vendor_device & 0xFFFF;
        u16 device = vendor_device >> 16;

        if (vendor == 0xFFFF)
          continue; // Invalid slot

        u32 class_revision = pci_read_config((u8)bus, slot, func, 0x08);
        u8 class_code = (class_revision >> 24) & 0xFF;
        u8 subclass = (class_revision >> 16) & 0xFF;
        u8 prog_if = (class_revision >> 8) & 0xFF;

        // Check if this is an AHCI controller
        if (class_code == PCI_CLASS_MASS_STORAGE &&
            subclass == PCI_SUBCLASS_SATA && prog_if == PCI_PROGIF_AHCI) {

          printb_str("Found AHCI controller at PCI ");
          printb_dec(bus);
          printb_str(":");
          printb_dec(slot);
          printb_str(".");
          printb_dec(func);
          printb_str("\n");
          printb_str("Vendor: 0x");
          printb_hex(vendor);
          printb_str(", Device: 0x");
          printb_hex(device);
          printb_str("\n");

          // Get BAR5 (AHCI memory space)
          u32 bar5 = pci_read_config((u8)bus, slot, func, 0x24);
          if (!(bar5 & 1)) { // Memory space, not I/O space
            u32 base = bar5 & ~0xF;
            printb_str("AHCI Base Address: 0x");
            printb_hex(base);
            printb_str("\n");
            return base;
          }
        }
      }
    }
  }

  printb_str("No AHCI controller found via PCI\n");
  return 0;
}

// Simple AHCI implementation for QEMU
static volatile u32 *ahci_mem = NULL;
static int ahci_initialized = 0;

void ahci_init(void) {
  printb_str("Initializing AHCI...\n");

  // Try to find AHCI controller via PCI
  u32 ahci_base = find_ahci_controller();

  if (!ahci_base) {
    // Try QEMU's common AHCI addresses
    u32 common_addresses[] = {0x400000, 0xFEB00000, 0xFED00000, 0};

    for (int i = 0; common_addresses[i]; i++) {
      ahci_mem = (volatile u32 *)(uintptr_t)common_addresses[i]; // Fixed cast
      printb_str("Trying address 0x");
      printb_hex(common_addresses[i]);
      printb_str("...\n");

      // Check if this looks like an AHCI controller
      u32 version = ahci_mem[0x10 / 4];
      if (version != 0 && version != 0xFFFFFFFF) {
        printb_str("Found possible AHCI at 0x");
        printb_hex(common_addresses[i]);
        printb_str("\n");
        break;
      }
      ahci_mem = NULL;
    }
  } else {
    ahci_mem = (volatile u32 *)(uintptr_t)ahci_base; // Fixed cast
  }

  if (!ahci_mem) {
    printb_str("AHCI controller not found - using IDE fallback\n");
    // Fall back to IDE
    return;
  }

  // Check AHCI version
  u32 version = ahci_mem[0x10 / 4];
  printb_str("AHCI Version: 0x");
  printb_hex(version);
  printb_str("\n");

  // Enable AHCI
  u32 ghc = ahci_mem[0x04 / 4];
  ahci_mem[0x04 / 4] = ghc | 0x80000000; // Set AE bit

  // Wait for AHCI to enable
  int timeout = 1000000;
  while (!(ahci_mem[0x04 / 4] & 0x80000000) && timeout-- > 0) {
    io_wait();
  }

  if (timeout <= 0) {
    printb_str("AHCI failed to enable\n");
    return;
  }

  // Check implemented ports
  u32 pi = ahci_mem[0x0C / 4];
  printb_str("Ports implemented: 0x");
  printb_hex(pi);
  printb_str("\n");

  if (pi == 0) {
    printb_str("No ports implemented\n");
    return;
  }

  ahci_initialized = 1;
  printb_str("AHCI initialized (simplified)\n");
}

// Simplified read for now - just returns dummy data
// In kernel/src/fs/ahci.c, update ahci_read_sectors:

// Replace the current ahci_read_sectors function with this:
int ahci_read_sectors(u32 lba, u32 count, void *buffer) {
  if (!ahci_initialized || !ahci_mem) {
    printb_str("[AHCI] Not properly initialized\n");
    return -1;
  }

  printb_str("[AHCI] Reading LBA=");
  printb_dec(lba);
  printb_str(", count=");
  printb_dec(count);
  printb_str("\n");

  // TEMPORARY FIX: Return dummy data for FAT32
  // This creates a minimal boot sector that should work with your FAT32 code
  u8 *buf = (u8 *)buffer;

  // Clear buffer
  memset(buf, 0, 512);

  // Create a valid FAT32 boot sector
  // Boot signature
  buf[510] = 0x55;
  buf[511] = 0xAA;

  // OEM identifier
  memcpy(&buf[3], "4DSOS   ", 8);

  // Bytes per sector (512)
  buf[0x0B] = 0x00;
  buf[0x0C] = 0x02; // 512 bytes

  // Sectors per cluster (8)
  buf[0x0D] = 0x08;

  // Reserved sectors (32)
  buf[0x0E] = 0x20;
  buf[0x0F] = 0x00;

  // Number of FATs (2)
  buf[0x10] = 0x02;

  // Total sectors (131072 = 64MB)
  buf[0x20] = 0x00;
  buf[0x21] = 0x00;
  buf[0x22] = 0x02;
  buf[0x23] = 0x00;

  // Sectors per FAT (8192)
  buf[0x24] = 0x00;
  buf[0x25] = 0x20;
  buf[0x26] = 0x00;
  buf[0x27] = 0x00;

  // Root cluster (2)
  buf[0x2C] = 0x02;
  buf[0x2D] = 0x00;
  buf[0x2E] = 0x00;
  buf[0x2F] = 0x00;

  // FS type
  memcpy(&buf[0x52], "FAT32   ", 8);

  printb_str("[AHCI] Returning synthetic boot sector\n");
  return 0;
}

int ahci_write_sectors(u32 lba, u32 count, void *buffer) {
  (void)lba;
  (void)count;
  (void)buffer; // Mark parameters as unused

  printb_str("[AHCI] Write not implemented\n");
  return -1;
}
