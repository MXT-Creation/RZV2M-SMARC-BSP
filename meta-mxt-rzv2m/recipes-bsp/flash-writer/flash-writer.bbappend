
do_compile () {
	cd ${S}
	cp stdint.h ${RECIPE_SYSROOT_NATIVE}/usr/lib/aarch64-poky-linux/gcc/aarch64-poky-linux/9.3.0/include/
	oe_runmake BOARD=RZV2M
}
