#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# Script Arguments (passed from Makefile)
OS_IMAGE=${1:-os-image.bin}
BOOT_BIN=${2:-boot/boot.bin}
KERNEL_BIN=${3:-kernel.bin}
IMAGE_SECTORS=${4:-65536}   # Default 32 MiB
PART_START_SECTOR=${5:-2048} # Default start sector

# Constants
SECTOR_SIZE=512
PART_OFFSET=$((${PART_START_SECTOR} * ${SECTOR_SIZE}))
TEMP_IMG="boot_plus_kernel.img" # Temp file for combined boot+kernel

echo ">>> Creating blank disk image (${OS_IMAGE}, ${IMAGE_SECTORS} sectors)..."
dd if=/dev/zero of=${OS_IMAGE} bs=${SECTOR_SIZE} count=${IMAGE_SECTORS} status=none

echo ">>> Creating MBR partition table on ${OS_IMAGE}..."
# Use fdisk with heredoc - works reliably in a standalone script
fdisk ${OS_IMAGE} << EOF > /dev/null
o
n
p
1
${PART_START_SECTOR}

t
c
a
w
EOF
echo "Partition table created."
# Optional: Verify partition table
# echo "--- Verifying Partition Table ---"
# fdisk -l ${OS_IMAGE}
# echo "-------------------------------"


echo ">>> Formatting partition on ${OS_IMAGE}... (Requires sudo)"
# Find a free loop device
LOOP_DEV=$(sudo losetup -f)
if [ -z "${LOOP_DEV}" ]; then
    echo "Error: No free loop device found. Cannot format."
    exit 1
fi
echo "Using loop device: ${LOOP_DEV}"

# Attach the partition within the image file to the loop device
sudo losetup -o ${PART_OFFSET} ${LOOP_DEV} ${OS_IMAGE}

echo "Formatting ${LOOP_DEV} as FAT32..."
# Format the loop device (which points to the partition)
sudo mkfs.fat -F 32 ${LOOP_DEV} > /dev/null

# Detach the loop device
sudo losetup -d ${LOOP_DEV}
echo "Partition formatted."


echo ">>> Writing bootloader and kernel to ${OS_IMAGE}..."
# Combine bootloader and kernel into a temporary file
if [ ! -f "${BOOT_BIN}" ]; then echo "Error: Bootloader '${BOOT_BIN}' not found."; exit 1; fi
if [ ! -f "${KERNEL_BIN}" ]; then echo "Error: Kernel binary '${KERNEL_BIN}' not found."; exit 1; fi
cat ${BOOT_BIN} ${KERNEL_BIN} > ${TEMP_IMG}

# Write the combined bootloader+kernel to the start of the image file (Sector 0)
# conv=notrunc is CRUCIAL to avoid shrinking the image file!
dd if=${TEMP_IMG} of=${OS_IMAGE} bs=${SECTOR_SIZE} conv=notrunc status=none

# Clean up the temporary combined file
rm ${TEMP_IMG}

echo ">>> OS Image created successfully: ${OS_IMAGE}"
echo "--- NOTE: Formatting/losetup steps required sudo privileges. ---"

exit 0