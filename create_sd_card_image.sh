#!/bin/bash -e

# This is an adaptation on the Using image creator script:
#    https://github.com/renesas-rz/rzg2_bsp_scripts/tree/master/image_creator
#
# Usage ./create_sd_card_image.sh [Yocto-Deploy-Dir - defaults to 'build/tmp/deploy/images/rzv2m/']
#

YOCTO_DEPLOY_DIR="${1:-build/tmp/deploy/images/rzv2m/}"

usage_check() {
	if [ -z "$1" ] || [ ! -d "$1" ] ; then
		echo "Usage: create_usb.sh [Yocto-Deploy-Dir - defaults to 'build/tmp/deploy/images/rzv2m/']"
		exit 1
	fi
}

BIN_DIR="binaries"

if [ ! -d "$BIN_DIR" ] ; then
	echo "There should be a folder near this script, called '${BIN_DIR}' containing: core1_firmware.bin"
	exit 1
fi

for req_file in core1_firmware.bin ; do
	if [ ! -f "${BIN_DIR}/${req_file}" ] ; then
		echo "File '$req_file' not found in '${BIN_DIR}/' directory"
		exit 1
	fi
done

usage_check "${YOCTO_DEPLOY_DIR}"

# Tune these as needed
SZ_SUFFIX=G                 # G=1024*3 ; GB=1000*3 (choose your favorite)
FAT_SIZE=128M
TOTAL_DISK_SIZE=8
EXT_TYPE=ext4
FAT_PART_LABEL=RZ_FAT
ROOTFS_PART_LABEL=RZ_ext

IMG_FILENAME="sd_card.img"

WORK_DIR="${YOCTO_DEPLOY_DIR}/build/sd_card"

ROOTFS_IMG_FILE="$YOCTO_DEPLOY_DIR/core-image-bsp-rzv2m.tar.gz"

uncompress_rootfs_img_file() {
	tar -xf $1 -C $2
}

format_partition() {
	LOOP_DEV="$1"
	TYPE="$2"
	PART_LABEL="$3"
	SRC_DIR="$4"

	TMP="/tmp"

	# format as ext3/ext4
	if [ "$TYPE" = "vfat" ] ; then
		sudo mkfs.vfat -F 32 -n $PART_LABEL $LOOP_DEV
	else
		sudo mkfs.${TYPE} -L $PART_LABEL $LOOP_DEV
	fi

	# mount this loop device (partition) so we can copy files into it
	mkdir -p ${TMP}/loop_mount/${PART_LABEL}
	sudo mount $LOOP_DEV ${TMP}/loop_mount/${PART_LABEL}
	if [ -d "$SRC_DIR" ] ; then
		sudo cp -r $SRC_DIR/* ${TMP}/loop_mount/${PART_LABEL}
	fi
	sudo sync
	sudo umount ${TMP}/loop_mount/${PART_LABEL}
}

create_sdcard_image_gz() {
	IMG_FILE="${WORK_DIR}/${IMG_FILENAME}"

	# Create empty disk
	dd if=/dev/zero of=$IMG_FILE bs=1${SZ_SUFFIX} count=$TOTAL_DISK_SIZE

	# Create 2 rootfs partitions + 1 User partition with remainder of disk
	echo -e "n\np\n1\n\n+${FAT_SIZE}\n" \
		"n\np\n2\n\n\n" \
		"t\n1\n6\n" \
		"p\nw\n" | fdisk -u $IMG_FILE

	LOOP_DEVICE=$(sudo losetup --partscan --show --verbose --find $IMG_FILE)

	format_partition ${LOOP_DEVICE}p1 vfat $FAT_PART_LABEL $FAT_DIR
	format_partition ${LOOP_DEVICE}p2 ${EXT_TYPE} $ROOTFS_PART_LABEL $ROOTFS_DIR

	# Release the loop device
	sudo losetup -d $LOOP_DEVICE

	mv "${IMG_FILE}" .
}

rm -rf $WORK_DIR
mkdir -p $WORK_DIR

FAT_DIR="$WORK_DIR/fat"
ROOTFS_DIR="$WORK_DIR/rootfs"
mkdir -p $FAT_DIR
mkdir -p $ROOTFS_DIR

cp $(readlink -f ${BIN_DIR}/core1_firmware.bin) $FAT_DIR/core1_firmware.bin
cp $(readlink -f $YOCTO_DEPLOY_DIR/Image) $FAT_DIR/Image
cp $(readlink -f $YOCTO_DEPLOY_DIR/r9a09g011gbg-evaluation-board.dtb) $FAT_DIR/r9a09g011gbg-evaluation-board.dtb
uncompress_rootfs_img_file "$ROOTFS_IMG_FILE" "$ROOTFS_DIR"

create_sdcard_image_gz
