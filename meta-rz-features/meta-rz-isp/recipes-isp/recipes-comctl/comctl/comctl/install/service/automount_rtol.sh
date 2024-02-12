#!/bin/sh
mkdir -p /var/custom/mmc0
mkdir -p /var/custom/mmc1

#### Mount point for SD card and eMMC device
mp_sd=/var/custom/mmc0
mp_emmc=/var/custom/mmc1

#### SD card and eMMC device file info
device_sd=/dev/mmcblk0p2
device_emmc=/dev/mmcblk1p2

custom_sd=/var/custom/mmc0
custom_emmc=/var/custom/mmc1
tmp_custom=/mnt/tmp_custom

findmnt |egrep -o mmcblk0p2 > /dev/null
if [ $? = 0 ]; then
    mountpoint ${mp_emmc} > /dev/null
    if [ $? = 1 ]; then
        mkdir -p ${tmp_custom}
        mount ${device_emmc} ${tmp_custom}
        if [ $? = 0 ]; then
            mkdir -p ${tmp_custom}${mp_emmc}
            mount -B ${tmp_custom}${mp_emmc} ${custom_emmc}
            umount ${tmp_custom}
        fi
    else
        echo "########No mount eMMC with SD-boot.#########"
    fi
else
    findmnt |egrep -o mmcblk1p2 > /dev/null
    if [ $? = 0 ]; then
        mountpoint ${mp_sd} > /dev/null
        if [ $? = 1 ]; then
            echo "######## mounted SD with eMMC-boot. ######"
            mkdir -p ${tmp_custom}
            mount ${device_sd} ${tmp_custom}
            if [ $? = 0 ]; then
                mkdir -p ${tmp_custom}${mp_sd}
                mount -B ${tmp_custom}${mp_sd} ${custom_sd}
                umount ${tmp_custom}
            fi
        else
            echo "########No mount SD with eMMC-boot.#########"
        fi
    else
        mountpoint ${mp_sd} > /dev/null
        if [ $? = 1 ]; then
            echo "###### mounted SD with NFS-boot.########"
            mkdir -p ${tmp_custom}
            mount ${device_sd} ${tmp_custom}
            if [ $? = 0 ]; then
                mkdir -p ${tmp_custom}${mp_sd}
                mount -B ${tmp_custom}${mp_sd} ${custom_sd}
                umount ${tmp_custom}
            fi
        else
            echo "########No mount eMMC with SD-boot.#########"
        fi
        mountpoint ${mp_emmc} > /dev/null
        if [ $? = 1 ]; then
            echo "####### mounted eMMC with NFS-boot.########"
            mkdir -p ${tmp_custom}
            mount ${device_emmc} ${tmp_custom}
            if [ $? = 0 ]; then
                mkdir -p ${tmp_custom}${mp_emmc}
                mount -B ${tmp_custom}${mp_emmc} ${custom_emmc}
                umount ${tmp_custom}
            fi
        else
            echo "########No mount SD with eMMC-boot.#########"
        fi
    fi
fi
