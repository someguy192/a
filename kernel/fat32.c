// kernel/fat32.c
// Complete File - Using corrected Fat32VolumeInfo names from header. Readably formatted.

#include "fat32.h"
#include "io.h"
#include "string.h"
#include <stddef.h>
#include <stdint.h>

// --- Module State ---
static Fat32VolumeInfo volume_info; // Uses the corrected struct definition from fat32.h
static int is_initialized = 0;
#define MAX_CLUSTER_BUF_SIZE (64 * 512)
static uint8_t cluster_buffer[MAX_CLUSTER_BUF_SIZE];
static uint32_t current_directory_cluster = 0;

// --- Basic Memory Copy Helper ---
void* memcpy(void* dest, const void* src, size_t n) {
    char* dp = (char*)dest; const char* sp = (const char*)src;
    for (size_t i = 0; i < n; i++) dp[i] = sp[i];
    return dest;
}

// --- Filesystem Initialization ---
int fat32_init(uint32_t partition_start_lba) {
    if (is_initialized) { return 0; }
    // Use full member names matching the corrected struct
    volume_info.partition_start_lba = partition_start_lba;

    term_writestring("DEBUG: Reading BPB sector at LBA "); term_print_dec(volume_info.partition_start_lba); term_putchar('\n');
    if (read_sectors(volume_info.partition_start_lba, 1, cluster_buffer) != 0) {
        term_writestring("Error: fat32_init: Failed read BPB LBA "); term_print_dec(volume_info.partition_start_lba); term_putchar('\n');
        return -1;
    }
    memcpy(&volume_info.bpb, cluster_buffer, sizeof(Fat32BiosParameterBlock));

    term_writestring("DEBUG: Validating BPB...\n");
    if (volume_info.bpb.bytes_per_sector != 512 && volume_info.bpb.bytes_per_sector != 1024 &&
        volume_info.bpb.bytes_per_sector != 2048 && volume_info.bpb.bytes_per_sector != 4096) {
        term_writestring("Error: Unsupported sector size: "); term_print_dec(volume_info.bpb.bytes_per_sector); term_putchar('\n'); return -2;
    }
    if (volume_info.bpb.root_entry_count != 0 || volume_info.bpb.sectors_per_fat_fat16 != 0) {
        term_writestring("Error: Not FAT32 (BPB fields).\n"); return -3;
    }
    if (volume_info.bpb.num_fats == 0 || volume_info.bpb.sectors_per_cluster == 0 || (volume_info.bpb.sectors_per_cluster & (volume_info.bpb.sectors_per_cluster - 1)) != 0 ) {
        term_writestring("Error: Invalid BPB values (fats/spc).\n"); return -4;
    }
    if (*(uint16_t*)&cluster_buffer[510] != 0xAA55) { term_writestring("Warn: Boot sig missing.\n"); }

    // Calculations using full member names
    volume_info.sectors_per_fat = volume_info.bpb.sectors_per_fat_fat32;
    volume_info.total_sectors = volume_info.bpb.total_sectors_long;
    volume_info.bytes_per_cluster = (uint32_t)volume_info.bpb.bytes_per_sector * volume_info.bpb.sectors_per_cluster;
    volume_info.fat_start_lba = volume_info.partition_start_lba + volume_info.bpb.reserved_sectors;
    volume_info.data_start_lba = volume_info.fat_start_lba + ((uint32_t)volume_info.bpb.num_fats * volume_info.sectors_per_fat); // Use sectors_per_fat
    uint32_t data_sectors = volume_info.total_sectors - (volume_info.data_start_lba - volume_info.partition_start_lba);
    if (volume_info.bpb.sectors_per_cluster == 0) { return -4; }
    volume_info.total_clusters = data_sectors / volume_info.bpb.sectors_per_cluster; // Use total_clusters
    if (volume_info.total_clusters < 65525) {
        term_writestring("Error: Cluster count low: "); term_print_dec(volume_info.total_clusters); term_writestring(" (need >= 65525 for FAT32)\n"); return -5; // Use total_clusters
    }

    // Print Parsed Values using full member names
    term_writestring("FAT32: Initialized. Root Clus: "); term_print_dec(volume_info.bpb.root_dir_cluster);
    term_writestring(", Data LBA: "); term_print_dec(volume_info.data_start_lba); // Use data_start_lba
    term_writestring(", Total Clus: "); term_print_dec(volume_info.total_clusters); term_putchar('\n'); // Use total_clusters

    current_directory_cluster = volume_info.bpb.root_dir_cluster;
    if (current_directory_cluster < 2) {
        term_writestring("Error: Invalid root clus:"); term_print_dec(current_directory_cluster); term_putchar('\n'); return -6;
    }
    is_initialized = 1; return 0;
}

// --- Get Volume Info ---
const Fat32VolumeInfo* fat32_get_volume_info(void) {
    if (!is_initialized) { return NULL; }
    return &volume_info;
}

// --- Cluster Number to LBA --- (Uses full member names from volume_info)
uint32_t fat32_cluster_to_lba(uint32_t cluster) {
    if (!is_initialized || cluster < 2 || cluster >= (volume_info.total_clusters + 2)) return 0;
    return volume_info.data_start_lba + (cluster - 2) * volume_info.bpb.sectors_per_cluster;
}

// --- Read FAT Entry --- (Uses full member names from volume_info)
uint32_t fat32_get_next_cluster(uint32_t current_cluster) {
    if (!is_initialized || current_cluster < 2 || current_cluster >= (volume_info.total_clusters + 2)) return 0x0FFFFFFF;
    uint32_t fat_offset = current_cluster * 4;
    uint32_t fat_sector_lba = volume_info.fat_start_lba + (fat_offset / volume_info.bpb.bytes_per_sector);
    uint32_t entry_offset_in_sector = fat_offset % volume_info.bpb.bytes_per_sector;

    if (read_sectors(fat_sector_lba, 1, cluster_buffer) != 0) {
        term_writestring("ERR: read FAT sec "); term_print_dec(fat_sector_lba); term_putchar('\n'); return 0x0FFFFFFF;
    }
    uint32_t next = *(uint32_t*)&cluster_buffer[entry_offset_in_sector];
    next &= 0x0FFFFFFF;
    if (next >= 0x0FFFFFF8) return 0;
    else if (next == 0x0FFFFFF7) { term_writestring("Warn: Bad clus mark\n"); return 0x0FFFFFF7; }
    else if (next == 0) { term_writestring("Warn: Free clus mark\n"); return 0; }
    else return next;
}

// --- Get/Set CWD ---
uint32_t fat32_get_current_directory_cluster(void) { return current_directory_cluster; }
void fat32_set_current_directory_cluster(uint32_t cluster) {
    if (cluster >= 2) { current_directory_cluster = cluster; }
    else { term_writestring("Warn: set CWD invalid clus "); term_print_dec(cluster); term_putchar('\n'); }
}

// --- Read Directory --- (Uses full member names from volume_info)
int fat32_read_directory(uint32_t directory_cluster, fat32_dir_entry_callback callback, void *user_data) {
    if (!is_initialized || !callback || directory_cluster < 2) return -1;
    uint32_t current_cluster = directory_cluster;
    int err = 0; char namebuf[13]; int found_flag = 0;

    if (volume_info.bytes_per_cluster > MAX_CLUSTER_BUF_SIZE) { // Use full name
        term_writestring("ERR: clus size "); term_print_dec(volume_info.bytes_per_cluster); // Use full name
        term_writestring(" > buf "); term_print_dec(MAX_CLUSTER_BUF_SIZE); term_putchar('\n'); return -5;
    }

    term_writestring("DEBUG: Reading dir clus:"); term_print_dec(current_cluster); term_putchar('\n');

    while (current_cluster != 0 && current_cluster < 0x0FFFFFF7) {
        uint32_t lba = fat32_cluster_to_lba(current_cluster);
        term_writestring("DEBUG: Reading Cluster "); term_print_dec(current_cluster);
        term_writestring(" at LBA "); term_print_dec(lba); term_putchar('\n'); // Print correct LBA
        if (lba == 0) { err = -2; break; }

        if (read_sectors(lba, volume_info.bpb.sectors_per_cluster, cluster_buffer) != 0) { // Use full name
            term_writestring("ERR: read dir clus LBA "); term_print_dec(lba); term_putchar('\n'); err = -3; break;
        }
        term_writestring("DEBUG: Read Cluster "); term_print_dec(current_cluster);
        term_writestring(" Data(Hex): "); for (int k = 0; k < 16; ++k) { term_print_hex_byte(cluster_buffer[k]); term_putchar(' '); } term_putchar('\n');

        Fat32DirectoryEntry *entry = (Fat32DirectoryEntry*)cluster_buffer;
        uint32_t num_entries = volume_info.bytes_per_cluster / sizeof(Fat32DirectoryEntry); // Use full name

        for (uint32_t i = 0; i < num_entries; ++i, ++entry) {
            uint8_t fb = entry->short_name[0];
            // Debug print entry info
            term_writestring("DEBUG: Entry "); term_print_dec(i); term_writestring(": first_byte="); term_print_hex_byte(fb); term_writestring(", attrs="); term_print_hex_byte(entry->attributes); term_writestring(", name[0..2]='"); term_putchar(fb >= ' ' ? fb : '.'); term_putchar(entry->short_name[1] >= ' ' ? entry->short_name[1] : '.'); term_putchar(entry->short_name[2] >= ' ' ? entry->short_name[2] : '.'); term_writestring("'\n");

            if (fb == 0x00) { term_writestring("DEBUG: EOD marker (0x00)\n"); current_cluster = 0; goto end_loop; }
            if (fb == 0xE5) { /*Skip deleted*/ continue; }
            if (entry->attributes == ATTR_LONG_NAME) { /*Skip LFN*/ continue; }
            if (entry->attributes & ATTR_VOLUME_ID) { /*Skip VolID*/ continue; }

            // Process valid 8.3 entry
            found_flag = 1;
            int k = 0; for (int j = 0; j < 8 && entry->short_name[j] != ' '; ++j) namebuf[k++] = (j == 0 && fb == 0x05) ? 0xE5 : entry->short_name[j];
            if (entry->short_name[8] != ' ') { namebuf[k++] = '.'; for (int j = 8; j < 11 && entry->short_name[j] != ' '; ++j) namebuf[k++] = entry->short_name[j]; } namebuf[k] = '\0';
            //term_writestring("DEBUG: Calling CB for '"); term_writestring(namebuf); term_writestring("'\n"); // Optional
            callback(entry, namebuf, user_data);
        } // End entry loop

        current_cluster = fat32_get_next_cluster(current_cluster);
        term_writestring("DEBUG: Next clus "); term_print_hex(current_cluster); term_putchar('\n');
        if (current_cluster == 0x0FFFFFF7) { term_writestring("ERR: Bad clus chain\n"); err = -4; break; }
    end_loop:;
    } // End cluster loop

    // Final debug message
    if (found_flag == 0 && err == 0) { term_writestring("DEBUG: ReadDIR done: No valid entries found.\n"); }
    else if (err != 0) { term_writestring("DEBUG: ReadDIR done: Error "); term_print_dec(err); term_putchar('\n'); }
    else { term_writestring("DEBUG: ReadDIR done: Success.\n"); }

    // Pass flag back via user_data if caller provided pointer (for improved ls)
    if (user_data) { *((int*)user_data) = found_flag; }
    return err;
}

// --- TODO ---