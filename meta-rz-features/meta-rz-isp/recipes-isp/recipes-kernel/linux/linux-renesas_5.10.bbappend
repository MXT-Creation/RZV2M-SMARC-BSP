#
# Copyright (C) 2020-2022 RenesasElectronics, Co, Ltd.
#
DESCRIPTION = "ISP package with kernel for the RZV2M based board"

FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI_append = " \
    file://0001-modified-rzv2m-device-tree.patch \
    file://0002-smp.c.patch \
    file://0003-uio_pdrv_genirq.c.patch \
    file://0004-irq-gic.patch \
    file://0005-add-v4l2-comctl-driver.patch \
"

KERNEL_FEATURES += " openamp_uio.scc"
