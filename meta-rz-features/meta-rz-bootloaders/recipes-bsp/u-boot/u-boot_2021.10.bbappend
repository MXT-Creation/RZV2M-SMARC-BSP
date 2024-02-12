FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI_append_rzv2ma = " \
	file://0001-usb-phy-support-v2ma.patch \
"

SRC_URI_append_rzv2m = " \
	file://0001-usb-phy-support-v2m.patch \
"
