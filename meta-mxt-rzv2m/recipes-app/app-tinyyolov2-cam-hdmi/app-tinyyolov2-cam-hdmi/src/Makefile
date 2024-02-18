INCPATH=${SDKTARGETSYSROOT}/usr/include/comctl/

all:
	${CXX} -std=gnu++14 sample_app_tinyyolov2_cam_hdmi.cpp box.cpp \
	-lpthread -O2 -ldl -lcomctl -I${INCPATH} \
	-o sample_app_tinyyolov2_cam_hdmi

clean:
	rm -rf sample_app_tinyyolov2_cam_hdmi
