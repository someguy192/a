// kernel/fat32.h
// Complete File - Corrected Fat32VolumeInfo member names. Readably formatted.

#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include <stddef.h>

// --- FAT32 On-Disk Structures ---
typedef struct __attribute__((packed)) { // Fat32BiosParameterBlock
    uint8_t jmp[3]; char oem_identifier[8]; uint16_t bytes_per_sector; uint8_t sectors_per_cluster;
    uint16_t reserved_sectors; uint8_t num_fats; uint16_t root_entry_count; uint16_t total_sectors_short;
    uint8_t media_descriptor; uint16_t sectors_per_fat_fat16; uint16_t sectors_per_track; uint16_t num_heads;
    uint32_t hidden_sectors; uint32_t total_sectors_long; uint32_t sectors_per_fat_fat32; uint16_t flags;
    uint16_t fat_version; uint32_t root_dir_cluster; uint16_t fsinfo_sector; uint16_t backup_boot_sector;
    uint8_t reserved[12]; uint8_t drive_number; uint8_t reserved1; uint8_t boot_signature;
    uint32_t volume_id; char volume_label[11]; char fs_type[8];
} Fat32BiosParameterBlock;

typedef struct __attribute__((packed)) { // Fat32DirectoryEntry
     union { char short_name[11]; struct { uint8_t sequence_number; uint16_t name1[5]; uint8_t attributes; uint8_t type; uint8_t checksum; uint16_t name2[6]; uint16_t first_cluster_low; uint16_t name3[2]; } lfn; };
    uint8_t attributes; uint8_t reserved_nt; uint8_t creation_time_tenths; uint16_t creation_time; uint16_t creation_date;
    uint16_t last_access_date; uint16_t first_cluster_high; uint16_t last_write_time; uint16_t last_write_date;
    uint16_t first_cluster_low; uint32_t file_size;
} Fat32DirectoryEntry;

// Directory Entry Attributes
#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10 // Correct definition was present
#define ATTR_ARCHIVE    0x20
#define ATTR_LONG_NAME  (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)

// --- Assumed Disk Driver Interface ---
int read_sectors(uint32_t lba, uint16_t count, void* buffer);
int write_sectors(uint32_t lba, uint16_t count, const void* buffer);

// --- FAT32 Driver State ---
// *** CORRECTED Member Names ***
typedef struct {
    uint32_t partition_start_lba;   // LBA partition start sector
    Fat32BiosParameterBlock bpb;    // Parsed BPB
    uint32_t fat_start_lba;         // LBA of first sector of first FAT
    uint32_t data_start_lba;        // LBA of first data cluster (Cluster #2)
    uint32_t sectors_per_fat;       // Sectors occupied by one FAT
    uint32_t total_sectors;         // Total sectors in the partition
    uint32_t total_clusters;        // Total data clusters in the partition
    uint32_t bytes_per_cluster;     // Bytes per allocation cluster
} Fat32VolumeInfo;

// --- Function Prototypes ---
int fat32_init(uint32_t partition_start_lba);
const Fat32VolumeInfo* fat32_get_volume_info(void); // Corrected name usage needed here too if accessed directly
uint32_t fat32_cluster_to_lba(uint32_t cluster);
uint32_t fat32_get_next_cluster(uint32_t current_cluster);
typedef void (*fat32_dir_entry_callback)(Fat32DirectoryEntry *entry, const char* short_name, void *user_data);
int fat32_read_directory(uint32_t directory_cluster, fat32_dir_entry_callback callback, void *user_data);
uint32_t fat32_get_current_directory_cluster(void);
void fat32_set_current_directory_cluster(uint32_t cluster);

#endif // FAT32_H