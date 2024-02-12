FILESEXTRAPATHS_prepend := "${THISDIR}/2018.10:"
SRCBRANCH ?= "main"
SRCREV ?= "c5763fc0950455cb72c7d452bc1115f6904b6ca0"
LIC_FILES_CHKSUM ?= "file://LICENSE.md;md5=a8d8cf662ef6bf9936a1e1413585ecbf"

SRC_URI_append = " \
  file://0001-fix-allocate-id.patch \
  "

include open-amp.inc
