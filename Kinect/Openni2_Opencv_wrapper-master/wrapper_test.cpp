#include "openni2_wrapper.h"
#include <OpenNI.h>

using namespace cv;
using namespace openni;

int main() {

	Device device;
	Status rc = STATUS_OK;
	VideoWrapper video_wrapper, video_wrapper2;
    VideoWrapper video_cam;

	rc = openni::OpenNI::initialize();    // Initialize OpenNI
    if(rc != STATUS_OK){
        std::cout << "OpenNI initialization failed" << std::endl;
        openni::OpenNI::shutdown();
    }
    else
        std::cout << "OpenNI initialization successful" << std::endl;
    
    rc = device.open(openni::ANY_DEVICE); // Open the Device
    if(rc != STATUS_OK){
        std::cout << "Device initialization failed" << std::endl;
        device.close();
    }

    video_wrapper.open(device, NI_SENSOR_DEPTH);
    video_wrapper2.open(device, NI_SENSOR_COLOR);
    //video_cam.open(0);

    namedWindow("color", 1);
    namedWindow("ir",1);
    namedWindow("cam", 1);
    Mat frame, frame2;
    Mat frame_cam;
    while(true){
    	video_wrapper.readFrame(frame);
        video_wrapper2.readFrame(frame2);
    	//video_cam.readFrame(frame_cam);
        imshow("ir", frame);
        imshow("color", frame2);
        //imshow("cam", frame_cam);
    	char key = waitKey(30);
            if(key==27) break;  
    }
    video_wrapper2.close();
    video_wrapper.close();
    device.close();                         // Close the PrimeSense Device

}