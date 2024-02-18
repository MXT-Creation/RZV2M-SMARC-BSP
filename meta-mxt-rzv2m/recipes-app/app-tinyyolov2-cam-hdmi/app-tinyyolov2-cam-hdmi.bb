SUMMARY = "RZ/V AI Evaluation Software - TinyYOLOv2 from MIPI cam to HDMI"
SECTION = "app"
LICENSE = "MIT&Apache&BSD-3-Clause"
LIC_FILES_CHKSUM = " \
    file://../licenses/onnx_tutorials/LICENSE.txt;md5=c594e50eac2ce59950729028841eb01c \
    file://../licenses/pytorch/LICENSE.txt;md5=dfcca3a12d86473cf25a596ffe56a645 \
    file://../licenses/pytorch_vision/LICENSE.txt;md5=2a5939a3a1f3f85f890e8b0488f8d426 \
"

inherit systemd

SRC_URI = "file://src/ \
	   file://licenses/ \
	   file://${BPN}.service \
	   file://tinyyolov2_cam.tar.bz2 \
"

DEPENDS += " comctl drpai "
RDEPENDS_${PN} += " comctl "

APP_INSTALL_DIRECTORY ?= "/home/root/${BPN}"

S = "${WORKDIR}/src"

export SDKTARGETSYSROOT="${STAGING_DIR_TARGET}"

INSANE_SKIP_${PN} = "ldflags"
INSANE_SKIP_${PN}-dev = "ldflags"

do_install() {
	install -d ${D}${APP_INSTALL_DIRECTORY}/licenses/
	cp -rf ${WORKDIR}/licenses/* ${D}${APP_INSTALL_DIRECTORY}/licenses/

	install -d ${D}${APP_INSTALL_DIRECTORY}/tinyyolov2_cam/
	cp -rf ${WORKDIR}/tinyyolov2_cam/* ${D}${APP_INSTALL_DIRECTORY}/tinyyolov2_cam/

	install -m 0755 ${S}/sample_app_tinyyolov2_cam_hdmi ${D}${APP_INSTALL_DIRECTORY}/

	install -m 0755 -d ${D}${systemd_unitdir}/system
	install -m 0644 ${WORKDIR}/${BPN}.service ${D}${systemd_unitdir}/system/
}

SYSTEMD_SERVICE_${PN} = "${BPN}.service"

FILES_${PN} = " \
	${systemd_unitdir}/* \
	${APP_INSTALL_DIRECTORY}/* \
"

