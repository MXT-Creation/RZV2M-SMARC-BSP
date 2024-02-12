#
# This library controls special RTOS commands by Libopen_amp and Libmetal for inter-core communication.
#

SUMMARY = "This library controls special RTOS commands by Libopen_amp and Libmetal for inter-core communication."
SECTION = "libs"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/BSD;md5=3775480a712fc46a69647678acb234cb"

DEPENDS = "libmetal open-amp"
PACKAGE_ARCH = "${MACHINE_ARCH}"
PACKAGES = "${PN}"
PROVIDES = "${PN}"

inherit systemd

SYSTEMD_AUTO_ENABLE = "enable"
SYSTEMD_SERVICE_${PN} = "comctld.service"

PR = "r1"

FILES_${PN} = " \
    ${libdir}/libcomctl.so \
    ${bindir}/comctld \
    ${bindir}/comctld_rtolreq \
    ${includedir}/comctl/comctl_if.h \
    ${includedir}/comctl/b_std.h \
    ${includedir}/comctl/b2sdk_if.h \
    ${includedir}/comctl/b_image_api.h \
    ${includedir}/comctl/b_image_movie_rec_api.h \
    ${includedir}/comctl/b_image_movie_play_api.h \
    ${includedir}/comctl/b_image_iqtt_api.h \
    ${includedir}/comctl/b_obj_api.h \
    ${includedir}/comctl/b_graphic_api.h \
    ${includedir}/comctl/b_sys_api.h \
    ${includedir}/comctl/b_io_api.h \
    ${includedir}/comctl/b_sound_api.h \
    ${includedir}/comctl/b_sound_stream_api.h \
    ${systemd_unitdir}/system/comctld.service \
    ${systemd_unitdir}/system/automount_rtol.sh \
    /var/custom/mmc0 \
    /var/custom/mmc1 \
"

INHIBIT_PACKAGE_DEBUG_SPLIT = "1"

TARGET_CC_ARCH += "${LDFLAGS}"

SRC_URI = "file://."

S = "${WORKDIR}"

do_install() {
    install -d ${D}/${libdir}
    install -d ${D}/${bindir}
    install -d ${D}/${includedir}/comctl
    install -d ${D}/var/custom/mmc0
    install -d ${D}/var/custom/mmc1
    install -d ${D}/${systemd_unitdir}/system
    
    install -m 0755 ${S}/install/bin/libcomctl.so ${D}${libdir}
    install -m 0755 ${S}/install/bin/comctld ${D}${bindir}
    install -m 0755 ${S}/install/bin/comctld_rtolreq ${D}${bindir}
    cp -r ${S}/install/include/public/*.h ${D}/${includedir}/comctl
    install -m 0644 ${S}/install/service/comctld.service ${D}/${systemd_unitdir}/system
    install -m 0644 ${S}/install/service/automount_rtol.sh ${D}/${systemd_unitdir}/system
}

