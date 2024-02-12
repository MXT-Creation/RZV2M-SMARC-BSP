IMAGE_FEATURES_remove = " ssh-server-dropbear"
IMAGE_FEATURES_append = " ssh-server-openssh"

IMAGE_INSTALL_append = " \
	u-boot-tools \
	kernel-image \
	kernel-devicetree \
	htop \
	iperf3 \
	app-usbcam-http \
	app-usbcam-client \
"

