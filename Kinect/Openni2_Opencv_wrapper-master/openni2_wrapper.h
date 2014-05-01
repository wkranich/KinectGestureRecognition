#include "OpenNI.h"
#include "opencv2/core/core.hpp"
#include "opencv/cv.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include <string>
#include <iostream>

enum CameraMode {NI_SENSOR_DEPTH, NI_SENSOR_COLOR, CV_CAMERA};
/*enum Parameters {CV_CAP_PROP_POS_MSEC, CV_CAP_PROP_POS_FRAMES, CV_CAP_PROP_POS_AVI_RATIO, CV_CAP_PROP_FRAME_WIDTH, CV_CAP_PROP_FRAME_HEIGHT,
                 CV_CAP_PROP_FPS, CV_CAP_PROP_FOURCC, CV_CAP_PROP_FRAME_COUNT, CV_CAP_PROP_FORMAT, CV_CAP_PROP_MODE, CV_CAP_PROP_BRIGHTNESS, 
                 CV_CAP_PROP_CONTRAST, CV_CAP_PROP_SATURATION, CV_CAP_PROP_HUE, CV_CAP_PROP_GAIN, CV_CAP_PROP_EXPOSURE, CV_CAP_PROP_CONVERT_RGB,
                 CV_CAP_PROP_WHITE_BALANCE, CV_CAP_PROP_RECTIFICATION};
*/

/* VideoWrapper allows to translate an OpenNI VideoStream into an OpenCV stream.
 * Use open() to create either a openni::VideoStream or a cv::VideoCapture.
 * Call readFrame() to obtain the next frame in the stream.
 * In case of openni::VideoStream the frame is converted into a cv::Mat frame.
 */
class VideoWrapper {
public:
			
	bool open(openni::Device &device, CameraMode cm);
	bool open(std::string input_video);
	bool open(int val);
	double get(int id);
	bool set(int id, double value);
	bool setUndistortParameters(std::string path);
	
	bool isOpened() { return is_opened_; }
	bool readFrame(cv::Mat &frame);
	bool readFrameUndistorted(cv::Mat &frame);
	void operator>>(cv::Mat &frame) { readFrame(frame); }
	void close();

private:
	cv::Mat camera_matrix_, dist_coeffs_;
	cv::Mat map1_, map2_;
	bool distortion_parameters_set_;
	bool is_opened_;

	CameraMode cm_;
	cv::VideoCapture video_capture_;
	openni::VideoStream video_stream_;	
};

bool distortion_parameters_set_ = false;
bool is_opened_ = false;

using namespace cv;
using namespace openni;

double VideoWrapper::get(int id){

	if(cm_!= CV_CAMERA) {

		return video_capture_.get(id);
	}
	else {
			VideoMode video = video_stream_.getVideoMode();

		switch(id) {

			case(CV_CAP_PROP_FRAME_WIDTH ):
				return video.getResolutionX();
			case(CV_CAP_PROP_FRAME_HEIGHT):
				return video.getResolutionY();
			case(CV_CAP_PROP_FPS):
				return video.getFps();
			default:
				std::cout << "Invalid parameter for kinect.\n CV_CAP_PROP_FRAME_WIDTH, CV_CAP_PROP_FRAME_HEIGHT, CV_CAP_PROP_FPS are allowed. " << std::endl;
				return -1;
		}
	}
}

bool VideoWrapper::set(int id, double value){

	if(cm_!= CV_CAMERA) {

		return video_capture_.set(id, value);
	}
	else {
		VideoMode video = video_stream_.getVideoMode();
		double x,y;

		switch(id) {

			case(CV_CAP_PROP_FRAME_WIDTH ):
				y = video.getResolutionY();
				video.setResolution(value, y);
				return video_stream_.setVideoMode(video);
			case(CV_CAP_PROP_FRAME_HEIGHT):
				x = video.getResolutionX();
				video.setResolution(x, value);
				return video_stream_.setVideoMode(video);
			case(CV_CAP_PROP_FPS):
				video.setFps(value);
				return video_stream_.setVideoMode(video);
			default:
				std::cout << "Invalid parameter for kinect.\n CV_CAP_PROP_FRAME_WIDTH, CV_CAP_PROP_FRAME_HEIGHT, CV_CAP_PROP_FPS are allowed. " << std::endl;
				return 1;
		}
	}
}








void VideoWrapper::close(){

	if(cm_ != CV_CAMERA) {
		video_stream_.stop();                            
    	video_stream_.destroy(); 
	}

}

bool VideoWrapper::open(Device &device, CameraMode cm){
	Status rc = STATUS_OK;
	cm_ = cm;


	switch(cm){

	case(NI_SENSOR_DEPTH) :

    	rc = video_stream_.create(device, SENSOR_DEPTH);    // Create the VideoStream for IR
    	if(rc != STATUS_OK){
        	std::cout << "Ir sensor creation failed" << std::endl;
        	video_stream_.destroy();
        	return false;
    	}
	    else
	      	std::cout << "Ir sensor creation successful" << std::endl;
	    
	    rc = video_stream_.start();                      // Start the IR VideoStream
	    if(rc != STATUS_OK){
	        std::cout << "Ir activation failed" << std::endl;
	        video_stream_.destroy();
	        return false;
	    }
	    else
	        std::cout << "Ir activation successful" << std::endl;
	    is_opened_ = true;
	    return true;

	case(NI_SENSOR_COLOR) :

		rc = video_stream_.create(device, SENSOR_COLOR);    // Create the VideoStream for Color
	    if(rc != STATUS_OK){
	        std::cout << "Color sensor creation failed" << std::endl;
	        video_stream_.destroy();
	        return false;
	    }
	    else
	        std::cout << "Color sensor creation successful" << std::endl;
	    
	    rc = video_stream_.start();                      // Start the Color VideoStream
	    if(rc != STATUS_OK){
	        std::cout << "Color sensor activation failed" << std::endl;
	        video_stream_.destroy();
	        return false;
	    }
	    else
	        std::cout << "Color sensor activation successful" << std::endl;

	    is_opened_ = true;
	    return true;

	default:
		std::cout << "Invalid parameters!" << std::endl;
		return false;
	}
}


bool VideoWrapper::open(std::string input_video){
	cm_ = CV_CAMERA;
	video_capture_.open(input_video);
	if(!video_capture_.isOpened()) {
        std::cout << "OpenCV video NOT opened!" << std::endl;
        return false;
    }
    
    is_opened_ = true;
	return true;
}

bool VideoWrapper::open(int val) {
	cm_ = CV_CAMERA;
	video_capture_.open(val);
	if(!video_capture_.isOpened()) {
        std::cout << "OpenCV device NOT opened!" << std::endl;
        return false;
    }
	return true;
}


bool VideoWrapper::readFrame(cv::Mat &frame){
	Status rc = STATUS_OK;

	VideoFrameRef irf; 
	int hIr, wIr;

	VideoFrameRef colorf; 
	int hColor, wColor;

	switch(cm_){
		
		case(CV_CAMERA) :
			return video_capture_.read(frame);

		case(NI_SENSOR_DEPTH) :
			
			rc = video_stream_.readFrame(&irf);
			if(irf.isValid()){

				const uint16_t* imgBufIr = (const uint16_t*)irf.getData();
				hIr=irf.getHeight();
        		wIr=irf.getWidth();
        		frame.create(hIr, wIr, CV_16U);
        		memcpy(frame.data, imgBufIr, hIr * wIr * sizeof(uint16_t));
        		frame.convertTo(frame, CV_8U);
				return true;
			}
			std::cout << "Frame not valid!" << std::endl;
			return false;

		case(NI_SENSOR_COLOR):
			
			rc = video_stream_.readFrame(&colorf);
			if(colorf.isValid()){
				const openni::RGB888Pixel* imgBufColor = (const openni::RGB888Pixel*)colorf.getData();
				hColor=colorf.getHeight();
        		wColor=colorf.getWidth();
        		frame.create(hColor, wColor, CV_8UC3);
        		memcpy(frame.data, imgBufColor,  3 * hColor * wColor * sizeof(uint8_t));
        		cvtColor(frame, frame, CV_BGR2RGB);
        		return true;
			}	
			std::cout << "Frame not valid!" << std::endl;
			return false;

		default:
			std::cout << "No frame to be read! Object not initialize!" << std::endl;
			return false;
	}
}

bool VideoWrapper::setUndistortParameters(std::string path){

	FileStorage fs(path,FileStorage::READ);
    if(!fs.isOpened()) {
    	std::cout << "Could not open configuration file " <<path << std::endl;
    	return false;
    }

    fs["Camera_Matrix"] >> camera_matrix_;
    fs["Distortion_Coefficients"] >> dist_coeffs_;
    fs.release();
    /*
    //Size imageSize(get(CV_CAP_PROP_FRAME_WIDTH), get(CV_CAP_PROP_FRAME_HEIGHT));
    
    initUndistortRectifyMap(camera_matrix_, dist_coeffs_, Mat(), camera_matrix_, imageSize, CV_16SC2, map1_, map2_);
    */
    distortion_parameters_set_ = true;
    return true;

}


bool VideoWrapper::readFrameUndistorted(cv::Mat &frame){
	if(distortion_parameters_set_) {
		Mat tmp;
		readFrame(tmp);
		undistort(tmp, frame, camera_matrix_, dist_coeffs_);
		//remap(tmp, frame, map1_, map2_, INTER_LINEAR);
		return true;
	}
	else{
		std::cout << "camera configuration file not loaded. Use setUndistortParameters(std::string path) to load." << std::endl;
		return false;
	}
}

