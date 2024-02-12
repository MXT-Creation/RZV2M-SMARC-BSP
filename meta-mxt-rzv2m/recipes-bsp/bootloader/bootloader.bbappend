FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI_append_r9a09g011gbg += "\
  file://2000-hardware-intialization-carrier-stdby-card-power.patch \
  file://2001-ddr-parameter-adaption-for-used-SMARC-memory-module.patch \
"

