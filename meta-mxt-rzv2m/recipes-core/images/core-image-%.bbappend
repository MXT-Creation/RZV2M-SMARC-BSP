IMAGE_FEATURES_remove = " ssh-server-dropbear"
IMAGE_FEATURES_append = " ssh-server-openssh"

IMAGE_INSTALL_append = " \
	devmem2 \
	phytool \
	u-boot-tools \
	kernel-image \
	kernel-devicetree \
	htop \
	iperf3 \
	app-usbcam-http \
	app-usbcam-client \
	app-hrnet-cam-hdmi \
	app-hrnet-pre-yolov3-cam-hdmi \
	app-tinyyolov2-cam-hdmi \
"

