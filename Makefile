# Makefile (Using external script for image creation)

# Cross-compiler prefix
PREFIX=i686-elf-
CC=$(PREFIX)gcc
AS=nasm
LD=$(PREFIX)ld
OBJCOPY=$(PREFIX)objcopy

# Flags
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -Ikernel -nostdlib -fno-builtin
LDFLAGS = -T kernel/linker.ld -nostdlib
ASFLAGS = -f elf32

# Objects
K_OBJS = kernel/start.o kernel/kernel.o kernel/io.o kernel/kbd.o kernel/string.o kernel/fat32.o kernel/ide.o
# Note: Removed kernel/fs.o if you aren't using the old ramdisk fs.c

# Output files
BOOT_BIN = boot/boot.bin
KERNEL_ELF = kernel.elf
KERNEL_BIN = kernel.bin
OS_IMAGE = os-image.bin
IMAGE_SCRIPT = ./create_image.sh

# Image/Partition Settings (Passed to script)
IMAGE_SECTORS = 65536 # 32 MiB
PART_START_SECTOR = 2048

# Default target
all: $(OS_IMAGE)

# --- Rule to create the final OS disk image ---
# Depends on the script and the compiled binaries
$(OS_IMAGE): $(BOOT_BIN) $(KERNEL_BIN) $(IMAGE_SCRIPT)
# --- TAB below ---
	# Execute the script, passing necessary parameters
	# Needs sudo because the script uses sudo internally for losetup/mkfs.fat
	sudo $(IMAGE_SCRIPT) $(OS_IMAGE) $(BOOT_BIN) $(KERNEL_BIN) $(IMAGE_SECTORS) $(PART_START_SECTOR)


# --- Other build rules ---

$(BOOT_BIN): boot/boot.asm
# --- TAB below ---
	$(AS) $< -f bin -o $@

$(KERNEL_BIN): $(KERNEL_ELF)
# --- TAB below ---
	$(OBJCOPY) -O binary $< $@

$(KERNEL_ELF): $(K_OBJS) kernel/linker.ld
# --- TAB below ---
	$(LD) $(LDFLAGS) -o $@ $(K_OBJS)

kernel/%.o: kernel/%.c kernel/*.h
# --- TAB below ---
	$(CC) $(CFLAGS) -c $< -o $@

kernel/start.o: kernel/start.asm
# --- TAB below ---
	$(AS) $(ASFLAGS) $< -o $@

# --- Clean Rule ---
clean:
# --- TAB below ---
	rm -f $(OS_IMAGE) $(BOOT_BIN) $(KERNEL_ELF) $(KERNEL_BIN) $(K_OBJS) boot_plus_kernel.img # Keep temp file name consistent
# --- TAB below ---
	@echo "Cleaned build files."

# --- Run Rules ---
run: $(OS_IMAGE)
# --- TAB below ---
	qemu-system-i386 -hda $(OS_IMAGE)

run-hd: $(OS_IMAGE)
# --- TAB below ---
	qemu-system-i386 -drive format=raw,file=$(OS_IMAGE),index=0,if=ide,media=disk

.PHONY: all clean run run-hd
