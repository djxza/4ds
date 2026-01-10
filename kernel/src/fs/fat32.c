#include "fat32.h"
#include "../io.h"
#include "../string.h"

// Global filesystem structure
static fat32_fs_t fs;
static disk_ops_t *disk = NULL;

// Helper functions
static u32 cluster_to_lba(u32 cluster) {
  return fs.data_start + (cluster - 2) * fs.bpb.sectors_per_cluster;
}

static u32 read_fat_entry(u32 cluster) {
  u32 fat_offset = cluster * 4;
  u32 fat_sector = fs.fat_start + (fat_offset / fs.bpb.bytes_per_sector);
  u32 ent_offset = fat_offset % fs.bpb.bytes_per_sector;

  u32 buffer[128]; // Enough for a sector
  if (disk->read_sectors(fat_sector, 1, buffer) != 0) {
    printb_str("[FAT32] Error reading FAT sector\n");
    return 0x0FFFFFFF; // End of chain marker
  }

  u32 entry = buffer[ent_offset / 4] & 0x0FFFFFFF;
  return entry;
}

static int read_cluster(u32 cluster, void *buffer) {
  if (cluster < 2) {
    printb_str("[FAT32] Invalid cluster: ");
    printb_dec(cluster);
    printb_str("\n");
    return -1;
  }

  u32 lba = cluster_to_lba(cluster);
  u32 sectors = fs.bpb.sectors_per_cluster;

  printb_str("[FAT32] Reading cluster ");
  printb_dec(cluster);
  printb_str(" at LBA ");
  printb_dec(lba);
  printb_str(" (");
  printb_dec(sectors);
  printb_str(" sectors)\n");

  return disk->read_sectors(lba, sectors, buffer);
}

// Initialize FAT32 filesystem
int fat32_init(disk_ops_t *ops) {
  disk = ops;

  // Read boot sector
  if (disk->read_sectors(0, 1, &fs.bpb) != 0) {
    printb_str("[FAT32] Failed to read boot sector\n");
    return -1;
  }

  // Basic validation - be more flexible with sector size
  if (fs.bpb.bytes_per_sector != 512 && fs.bpb.bytes_per_sector != 0) {
    printb_str("[FAT32] Warning: Unusual sector size: ");
    printb_dec(fs.bpb.bytes_per_sector);
    printb_str("\n");
    // Continue anyway
  }

  // Print FS type for debugging
  printb_str("[FAT32] FS type: ");
  printb_str(fs.bpb.fs_type);
  printb_str("\n");

  // Calculate important offsets
  fs.fat_start = fs.bpb.reserved_sectors;

  // Use FAT32-specific field if available
  if (fs.bpb.sectors_per_fat > 0) {
    fs.data_start = fs.fat_start + (fs.bpb.fat_count * fs.bpb.sectors_per_fat);
  } else {
    // Fallback to FAT16 field
    fs.data_start =
        fs.fat_start + (fs.bpb.fat_count * fs.bpb.sectors_per_fat_16);
  }

  fs.root_dir_cluster = fs.bpb.root_cluster;

  printb_str("[FAT32] Initialized\n");
  printb_str("  FAT start: ");
  printb_dec(fs.fat_start);
  printb_str("\n");
  printb_str("  Data start: ");
  printb_dec(fs.data_start);
  printb_str("\n");
  printb_str("  Root cluster: ");
  printb_dec(fs.root_dir_cluster);
  printb_str("\n");

  return 0;
}

// Find file in directory
static fat32_dir_entry_t *find_file_in_dir(u32 cluster, const char *name) {
  u8 sector_buffer[512 *
                   4]; // Enough for a cluster (assuming 4 sectors/cluster)
  u32 sectors_per_cluster = fs.bpb.sectors_per_cluster;

  // Read directory cluster
  if (read_cluster(cluster, sector_buffer) != 0) {
    printb_str("[FAT32] Error reading directory cluster\n");
    return NULL;
  }

  // Search through directory entries
  fat32_dir_entry_t *entry = (fat32_dir_entry_t *)sector_buffer;
  u32 entries_per_cluster = (sectors_per_cluster * fs.bpb.bytes_per_sector) /
                            sizeof(fat32_dir_entry_t);

  for (u32 i = 0; i < entries_per_cluster; i++) {
    if (entry[i].name[0] == DIR_ENTRY_END) {
      break; // End of directory
    }

    if (entry[i].name[0] == DIR_ENTRY_FREE) {
      continue; // Free entry
    }

    // Skip long name entries
    if (entry[i].attributes == ATTR_LONG_NAME) {
      continue;
    }

    // Create 8.3 filename
    char short_name[13];
    int pos = 0;

    // Copy name
    for (int j = 0; j < 8 && entry[i].name[j] != ' '; j++) {
      short_name[pos++] = entry[i].name[j];
    }

    // Add extension if present
    if (entry[i].ext[0] != ' ') {
      short_name[pos++] = '.';
      for (int j = 0; j < 3 && entry[i].ext[j] != ' '; j++) {
        short_name[pos++] = entry[i].ext[j];
      }
    }

    short_name[pos] = '\0';

    // Compare with requested name
    if (strcmp(short_name, name) == 0) {
      // Found it!
      printb_str("[FAT32] Found file: ");
      printb_str(short_name);
      printb_str(", size: ");
      printb_dec(entry[i].file_size);
      printb_str(" bytes\n");

      // Allocate memory for the entry to return
      static fat32_dir_entry_t found_entry;
      memcpy(&found_entry, &entry[i], sizeof(fat32_dir_entry_t));
      return &found_entry;
    }
  }

  return NULL;
}

// Read a file
int fat32_read_file(const char *path, void *buffer, u32 max_size) {
  char filename[32];
  strcpy(filename, path);

  printb_str("[FAT32] Looking for file: ");
  printb_str(filename);
  printb_str("\n");

  // Search in root directory
  fat32_dir_entry_t *entry = find_file_in_dir(fs.root_dir_cluster, filename);
  if (!entry) {
    printb_str("[FAT32] File not found\n");
    return -1; // File not found
  }

  // Get file size
  u32 file_size = entry->file_size;
  if (file_size > max_size) {
    printb_str("[FAT32] Buffer too small\n");
    return -2; // Buffer too small
  }

  printb_str("[FAT32] Reading file, size: ");
  printb_dec(file_size);
  printb_str(" bytes\n");

  // Get starting cluster
  u32 cluster = (entry->cluster_high << 16) | entry->cluster_low;
  u8 *dest = (u8 *)buffer;
  u32 bytes_read = 0;
  u32 cluster_size = fs.bpb.sectors_per_cluster * fs.bpb.bytes_per_sector;

  printb_str("[FAT32] Starting cluster: ");
  printb_dec(cluster);
  printb_str("\n");

  // Read file cluster by cluster
  while (cluster < 0x0FFFFFF8) { // Not end of chain
    printb_str("[FAT32] Reading cluster ");
    printb_dec(cluster);
    printb_str("\n");

    if (read_cluster(cluster, dest) != 0) {
      printb_str("[FAT32] Error reading cluster\n");
      return -3; // Read error
    }

    bytes_read += cluster_size;

    if (bytes_read >= file_size) {
      // We've read enough
      printb_str("[FAT32] Read complete: ");
      printb_dec(bytes_read);
      printb_str(" bytes\n");
      break;
    }

    // Move destination pointer
    dest += cluster_size;

    // Get next cluster
    cluster = read_fat_entry(cluster);

    if (cluster == 0x0FFFFFFF || cluster == 0xFFFFFFFF) {
      printb_str("[FAT32] Invalid FAT entry\n");
      return -4; // FAT error
    }
  }

  return file_size;
}

// List directory contents
int fat32_list_dir(const char *path) {
  u8 sector_buffer[512 * 4];

  // Read root directory
  if (read_cluster(fs.root_dir_cluster, sector_buffer) != 0) {
    return -1;
  }

  fat32_dir_entry_t *entry = (fat32_dir_entry_t *)sector_buffer;
  u32 sectors_per_cluster = fs.bpb.sectors_per_cluster;
  u32 entries_per_cluster = (sectors_per_cluster * fs.bpb.bytes_per_sector) /
                            sizeof(fat32_dir_entry_t);

  printb_str("[FAT32] Directory listing:\n");

  for (u32 i = 0; i < entries_per_cluster; i++) {
    if (entry[i].name[0] == DIR_ENTRY_END) {
      break;
    }

    if (entry[i].name[0] == DIR_ENTRY_FREE) {
      continue;
    }

    if (entry[i].attributes == ATTR_LONG_NAME) {
      continue;
    }

    // Create filename
    char name[13];
    int pos = 0;

    for (int j = 0; j < 8 && entry[i].name[j] != ' '; j++) {
      name[pos++] = entry[i].name[j];
    }

    if (entry[i].ext[0] != ' ') {
      name[pos++] = '.';
      for (int j = 0; j < 3 && entry[i].ext[j] != ' '; j++) {
        name[pos++] = entry[i].ext[j];
      }
    }

    name[pos] = '\0';

    // Print entry
    printb_str("  ");
    printb_str(name);
    printb_str(" (");
    printb_dec(entry[i].file_size);
    printb_str(" bytes)\n");
  }

  return 0;
}

// Check if file exists
int fat32_file_exists(const char *path) {
  char filename[32];
  strcpy(filename, path);

  fat32_dir_entry_t *entry = find_file_in_dir(fs.root_dir_cluster, filename);
  return entry != NULL ? 1 : 0;
}

fat32_fs_t *fat32_get_fs(void) { return &fs; }
