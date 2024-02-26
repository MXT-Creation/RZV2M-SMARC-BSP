FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI_append_r9a09g011gbg = " \
   file://fragment-01-mxt.cfg \
   file://fragment-02-usb-ethernet.cfg \
   file://patches/0065-phy_mscc_clock_en.patch \
   file://patches/0066-mscc-ethernet-phy-add-reset-gpio.patch \
   file://patches/0067-net-ravb-workaround-missing-phy-node.patch \
   file://patches/0071_sdhi_core_voltage_switch.patch \
"

KERNEL_MODULE_AUTOLOAD += " uvcvideo "

