FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI_append_r9a09g011gbg += "\
  file://rzv2m_patch/1000-added-sd0-pwr-en-pin-adaption-for-rzv2ma-smarc.patch \
  file://rzv2m_patch/1001-updated-uboot-disable_pwm.patch \
  file://rzv2m_patch/1002-defconfig_enable_mscc_phy.patch \
  file://rzv2m_patch/1003-enable_mscc_phy_clock.patch \
"

