#ifndef _FAT32_H
#define _FAT32_H

#include "../stdlib.h"

// FAT32 structures
typedef struct __attribute__((packed)) {
  u8 jump[3];
  char oem[8];
  u16 bytes_per_sector;
  u8 sectors_per_cluster;
  u16 reserved_sectors;
  u8 fat_count;
  u16 root_dir_entries;
  u16 total_sectors_16;
  u8 media_descriptor;
  u16 sectors_per_fat_16;
  u16 sectors_per_track;
  u16 head_count;
  u32 hidden_sectors;
  u32 total_sectors_32;

  // Extended FAT32 fields
  u32 sectors_per_fat;
  u16 flags;
  u16 version;
  u32 root_cluster;
  u16 fs_info_sector;
  u16 backup_boot_sector;
  u8 reserved[12];
  u8 drive_number;
  u8 reserved1;
  u8 signature;
  u32 volume_id;
  char volume_label[11];
  char fs_type[8];
} fat32_bpb_t;

typedef struct __attribute__((packed)) {
  char name[8];
  char ext[3];
  u8 attributes;
  u8 reserved;
  u8 creation_time_tenths;
  u16 creation_time;
  u16 creation_date;
  u16 last_access_date;
  u16 cluster_high;
  u16 modification_time;
  u16 modification_date;
  u16 cluster_low;
  u32 file_size;
} fat32_dir_entry_t;

// File/directory attributes
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_NAME 0x0F

// Special entries
#define DIR_ENTRY_FREE 0xE5
#define DIR_ENTRY_END 0x00

// Functions
typedef struct {
  fat32_bpb_t bpb;
  u32 fat_start;
  u32 data_start;
  u32 current_cluster;
  u32 root_dir_cluster;
} fat32_fs_t;

// Disk I/O interface
typedef struct {
  int (*read_sectors)(u32 lba, u32 count, void *buffer);
  int (*write_sectors)(u32 lba, u32 count, void *buffer);
} disk_ops_t;

// Initialization
int fat32_init(disk_ops_t *ops);
fat32_fs_t *fat32_get_fs(void);

// File operations
int fat32_read_file(const char *path, void *buffer, u32 max_size);
int fat32_list_dir(const char *path);
int fat32_file_exists(const char *path);

#endif // _FAT32_H
