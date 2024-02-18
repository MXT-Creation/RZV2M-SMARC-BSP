INCPATH=${SDKTARGETSYSROOT}/usr/include/comctl/

all:
	${CXX} -std=gnu++14 -O2 -c box.cpp -o box.o
	${CXX} -std=gnu++14 -O2 -I${INCPATH} -c sample_app_hrnet_pre-yolov3_cam_hdmi.cpp -o sample_app_hrnet_pre-yolov3_cam_hdmi.o
	${CXX} -std=gnu++14 -O2 -ldl -lpthread -lcomctl box.o sample_app_hrnet_pre-yolov3_cam_hdmi.o -o sample_app_hrnet_pre-yolov3_cam_hdmi

clean:
	rm -rf *.o sample_app_hrnet_pre-yolov3_cam_hdmi
