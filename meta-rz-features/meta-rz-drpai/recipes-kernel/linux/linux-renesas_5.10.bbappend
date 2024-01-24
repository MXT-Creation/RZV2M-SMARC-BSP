#
# Copyright (C) 2022 RenesasElectronics, Co, Ltd.
#
DESCRIPTION = "DRP-AI Support Package with kernel for RZ/V2M Evaluation Board Kit"

FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}/:"

SRC_URI_append += "\
	file://0008-enable-drpai-drv.patch \
	file://0009-modify-device-tree-for-drpai.patch \
    file://1000-WA-available-v4l2-memory-userptr.patch \
"

KERNEL_FEATURES += " strict_devmem.scc"
