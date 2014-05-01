// OpenCVWebcam.cpp : Defines the entry point for the console application.
//
#include <OpenNI.h>
#include "NiTE.h"
#include "Openni2_Opencv_wrapper-master\openni2_wrapper.h"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"


using namespace cv;
using namespace std;
using namespace nite; 
using namespace openni;


void colorizeDepth(Mat& map, Mat& valid, Mat& image);

int main(int argc, char* argv[])
{
    /*VideoCapture capture (CV_CAP_OPENNI);
    if(!capture.isOpened())
    {
        int error = -1;
        return 1;
    }*/

	// From wrapper_test
	Device device;
	openni::Status rc = openni::STATUS_OK;
	VideoWrapper video_wrapper, video_wrapper2;
    VideoWrapper video_cam;

	rc = openni::OpenNI::initialize();    // Initialize OpenNI
    if(rc != nite::STATUS_OK){
        std::cout << "OpenNI initialization failed" << std::endl;
        openni::OpenNI::shutdown();
    }
    else
        std::cout << "OpenNI initialization successful" << std::endl;
    
    rc = device.open(openni::ANY_DEVICE); // Open the Device
    if(rc != nite::STATUS_OK){
        std::cout << "Device initialization failed" << std::endl;
        device.close();
    }

    video_wrapper.open(device, NI_SENSOR_DEPTH);
    video_wrapper2.open(device, NI_SENSOR_COLOR);
    //video_cam.open(0);


    namedWindow( "Color Image", 1 );
	namedWindow( "Depth Map", 1);
    Mat view;
    bool blink = false;


	nite::Status niteRc;
	HandTracker handTracker;
	niteRc = NiTE::initialize();
	if (niteRc != nite::STATUS_OK)
	{
		printf("NiTE initialization failed\n");
		return 1;
	}

	niteRc = handTracker.create();
	if (niteRc != nite::STATUS_OK)
	{
		printf("Couldn't create user tracker\n");
		return 3;
	}

	handTracker.startGestureDetection(GESTURE_WAVE);
	HandTrackerFrameRef handTrackerFrame;

    while( TRUE )
    {
		niteRc = handTracker.readFrame(&handTrackerFrame);
		{
			printf("Get next frame failed\n");
			continue;
		}

		const nite::Array<GestureData>& gestures = handTrackerFrame.getGestures();
		for (int i = 0; i < gestures.getSize(); ++i)
		{
			if (gestures[i].isComplete())
			{
				HandId newId;
				handTracker.startHandTracking(gestures[i].getCurrentPosition(), &newId);
			}
		}

		const nite::Array<HandData>& hands = handTrackerFrame.getHands();
		for (int i = 0; i < hands.getSize(); ++i)
		{
			const HandData& hand = hands[i];
			if (hand.isTracking())
			{
				printf("%d. (%5.2f, %5.2f, %5.2f)\n", hand.getId(), hand.getPosition().x, hand.getPosition().y, hand.getPosition().z);
			}
		}

        Mat bgrImage;
		Mat depthMap;
		Mat valid;

		video_wrapper.readFrame(depthMap);
        video_wrapper2.readFrame(bgrImage);

        /*capture.grab();

	    capture.retrieve( depthMap, CV_CAP_OPENNI_DEPTH_MAP );
		capture.retrieve( valid, CV_CAP_OPENNI_VALID_DEPTH_MASK);
		capture.retrieve( bgrImage, CV_CAP_OPENNI_BGR_IMAGE );*/

		Mat depthImage(depthMap.rows, depthMap.cols, CV_8UC3);
		colorizeDepth(depthMap, valid, depthImage);

        imshow("Color Image", bgrImage);
        imshow("Depth Map", depthImage);
        if(waitKey(33) == 'q')
        {
            break;
        }
    }

	NiTE::shutdown();
    return 0;
}

void colorizeDepth(Mat& map, Mat& valid, Mat& image)
{
	double maxVal = 0;
	double minVal = 32768;
	double sumVal = 0;
	double normVal = 0;
	int count = 0;
	for (int row=0; row < map.rows; row++)
	{
		unsigned short* mapPtr = map.ptr<unsigned short>(row);
		unsigned char* validPtr = valid.ptr<unsigned char>(row);
		for(int col=0; col < map.cols; col++)
		{
			if(validPtr[col] > 0)
			{
				if(mapPtr[col] > maxVal)
				{
					maxVal = mapPtr[col];
				}
				if(mapPtr[col] < minVal)
				{
					minVal = mapPtr[col];
				}
				sumVal += mapPtr[col];
				count++;
			}
		}
	}
	normVal = (maxVal-minVal) / 4;

	for (int row=0; row < map.rows; row++)
	{
		unsigned short* mapPtr = map.ptr<unsigned short>(row);
		unsigned char* validPtr = valid.ptr<unsigned char>(row);
		unsigned char* imgPtr = image.ptr<unsigned char>(row);
		for(int col=0; col < map.cols; col++)
		{
			if(validPtr[col]==0)
			{
				imgPtr[col*3]=0;
				imgPtr[col*3+1]=0;
				imgPtr[col*3+2]=0;
				continue;
			}
			imgPtr[col*3] = std::min(135.0,((double)(mapPtr[col]-minVal)/normVal) * 135);
			imgPtr[col*3+1] = 255;
			imgPtr[col*3+2] = 224;
		}
	}
	cvtColor(image, image, CV_HSV2BGR);
}
