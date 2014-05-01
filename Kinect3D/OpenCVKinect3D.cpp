// OpenCVWebcam.cpp : Defines the entry point for the console application.
//
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "XnCppWrapper.h"
#include "XnVNite.h"
#include "XnOpenNI.h"
#include "XnHash.h"
#include "XnLog.h"
#include <queue>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <iostream>

#define GESTURE_TO_USE "Click"

using namespace cv;
using namespace std;
using namespace xn;


#define DEFAULT_PORT "3490"

// OpenNI objects
xn::Context g_Context;
xn::ScriptNode g_ScriptNode;
xn::DepthGenerator g_DepthGenerator;
xn::ImageGenerator g_ImageGenerator;
xn::HandsGenerator g_HandsGenerator;
xn::GestureGenerator g_GestureGenerator;
xn::DepthMetaData depthMD;
xn::ImageMetaData imageMD;

SOCKET ClientSocket;

// Hand Objects, history
queue<const XnPoint3D> hand1;
queue<const XnPoint3D> hand2;
double oldDistance = -1.0;
XnUserID hand1ID = -1;
XnUserID hand2ID = -1;
double oldZoom = 1;
double oldAngle = 0.0;


void XN_CALLBACK_TYPE handCreate(HandsGenerator &generator, XnUserID user, const XnPoint3D *pPosition, XnFloat fTime, void *pCookie);
void XN_CALLBACK_TYPE handUpdate(HandsGenerator &generator, XnUserID user, const XnPoint3D *pPosition, XnFloat fTime, void *pCookie);
void XN_CALLBACK_TYPE handDestroy(HandsGenerator &generator, XnUserID user, XnFloat fTime, void *pCookie);
void XN_CALLBACK_TYPE Gesture_Process(xn::GestureGenerator& generator, const XnChar* strGesture, const XnPoint3D* pPosition, XnFloat fProgress, void* pCookie);
void XN_CALLBACK_TYPE Gesture_Recognized(xn::GestureGenerator& generator, const XnChar* strGesture, const XnPoint3D* pIDPosition, const XnPoint3D* pEndPosition, void* pCookie);
void colorizeDepth(Mat& map, Mat& valid, Mat& image);
double getHandsDistance();
double getZoom();
double getAngle();
double getAngle3D();
Mat getScaleMatrix(double scaleFactor);
Mat getRotationMatrix(double degrees);

int main(int argc, char* argv[])
{
	WSADATA wsaData;

	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	struct addrinfo *result1 = NULL, *ptr = NULL, hints;

	ZeroMemory(&hints, sizeof (hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result1);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	SOCKET ListenSocket = INVALID_SOCKET;


	// Create a SOCKET for the server to listen for client connections

	ListenSocket = socket(result1->ai_family, result1->ai_socktype, result1->ai_protocol);

	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result1);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind( ListenSocket, result1->ai_addr, (int)result1->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result1);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result1);

	cout << "Listening\n";
	if ( listen( ListenSocket, SOMAXCONN ) == SOCKET_ERROR ) {
		printf( "Listen failed with error: %ld\n", WSAGetLastError() );
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	ClientSocket = INVALID_SOCKET;

	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	cout << "Client Accepted\n";


	// Start 
    VideoCapture capture (CV_CAP_OPENNI);
    if(!capture.isOpened())
    {
        int error = -1;
        return 1;
    }

    namedWindow( "Color Image", 1 );
    Mat view;
    bool blink = false;


	// NITE + openni
	
	XnStatus rc = XN_STATUS_OK;
	
	Context context;
	rc = context.Init();
	rc = g_GestureGenerator.Create(context);
	rc = g_HandsGenerator.Create(context);
	XnCallbackHandle hcb1,hcb2; 
	g_GestureGenerator.RegisterGestureCallbacks(Gesture_Recognized, Gesture_Process, NULL, hcb1);
	g_HandsGenerator.RegisterHandCallbacks(handCreate, handUpdate, handDestroy, NULL, hcb2);
	rc = context.StartGeneratingAll();
	rc = g_GestureGenerator.AddGesture(GESTURE_TO_USE, NULL);

	double d = 1.0;
	double angle = 0.0;
	double angleZ = 0.0;

    while( capture.isOpened() )
    {
		rc = context.WaitAndUpdateAll();
		d = getZoom();
		angle = getAngle();
		angleZ = getAngle3D();
		if (abs(d - oldZoom) > 0.009) {
			oldAngle += angle;

			//convert information to floats
			long double sendZoom = d;
			long double sendAngle = angle;
			long double sendAngleZ = angleZ;

			//create string stream to pack floats in string || INEFFICIENT
			/*ostringstream ss;
			ss << sendZoom;
			ss << " ";
			ss << sendAngle;
			ss << " ";
			ss << sendAngleZ;
			string s(ss.str());*/

			// to_string doesn't work for floats or doubles in VS2010! Ridiculous...
			// Passing in long doubles
			string k = to_string(sendZoom);
			k += " ";
			k += to_string(sendAngle);
			k += " ";
			k += to_string(sendAngleZ);
			

			//convert string to const char which is what send takes as an arg
			const char * c = k.c_str();

			//send message to client
			send(ClientSocket, c, k.length(), 0);

			//warp image to apply transformation
			oldZoom = d;
			
		}
		
        Mat bgrImage;
        capture.grab();

		capture.retrieve( bgrImage, CV_CAP_OPENNI_BGR_IMAGE );

		if (hand1ID != -1) {
			circle(bgrImage,Point(hand1.back().X + bgrImage.rows/2, bgrImage.cols/2 - hand1.back().Y),2,CV_RGB(0,255,0),3);
		
		}
		if (hand2ID != -1) {
			circle(bgrImage,Point(hand2.back().X + bgrImage.rows/2, bgrImage.cols/2 - hand2.back().Y),2,CV_RGB(0,255,0),3);
		}

		flip(bgrImage,bgrImage,1);
        imshow("Color Image", bgrImage);
	
        if(waitKey(33) == 'q')
        {
            break;
        }
    }
	context.Shutdown();
	WSACleanup();
    return 0;
}

double getHandsDistance() {
	if (hand1.size() > 0 && hand2.size() > 0) {
		const XnPoint3D h1 = hand1.front();
		const XnPoint3D h2 = hand2.front();
		return (double)sqrt((double)pow((h1.X - h2.X),2) + (double)pow((h1.Y - h2.Y),2));

	} else {
		return -1.0;
	}

}

double getZoom() {
	double zoom = 1.0;
	double resultingZoom = oldZoom;
	double d = getHandsDistance();
	if (d == -1.0) {
		return oldZoom;
	}
	
	if (oldDistance != -1.0) {
		zoom = d /oldDistance;
	}

	oldDistance = d;

	if (zoom < 1.0) {
		zoom = 1 - zoom;
		resultingZoom = oldZoom - zoom;
	} else if (zoom > 1.0) {
		zoom = zoom - 1;
		resultingZoom = oldZoom + zoom;
	}
	
	return resultingZoom;
}

double getAngle() {
	if(hand1.size() > 1 && hand2.size() > 1) {

		double dot = ((hand1.front().X - hand2.front().X) *(hand1.back().X - hand2.back().X) + (hand1.front().Y - hand2.front().Y) *(hand1.back().Y - hand2.back().Y));
		double magnitudeSum = sqrt(pow((hand1.front().X - hand2.front().X),2)+ pow((hand1.front().Y - hand2.front().Y),2)) * sqrt(pow((hand1.back().X - hand2.back().X),2)+ pow((hand1.back().Y - hand2.back().Y),2));
		if (magnitudeSum == 0 || dot/magnitudeSum > 1 || dot/magnitudeSum < -1)
		{
			return 0;
		}
		double angle1 = acos( dot / magnitudeSum);

		if (hand1.front().X > hand2.front().X && hand1.front().Y < hand1.back().Y) {
			angle1 *= -1;
		} else if (hand1.front().X < hand2.front().X && hand2.front().Y < hand2.back().Y) {
			angle1 *= -1;
		}

		
		
		return (angle1);
	} else {
		return 0.0;
	}
}

double getAngle3D() {
	if(hand1.size() > 1 && hand2.size() > 1) {

		double dot = ((hand1.front().X - hand2.front().X) *(hand1.back().X - hand2.back().X) + (hand1.front().Z - hand2.front().Z) *(hand1.back().Z - hand2.back().Z));
		double magnitudeSum = sqrt(pow((hand1.front().X - hand2.front().X),2)+ pow((hand1.front().Z - hand2.front().Z),2)) * sqrt(pow((hand1.back().X - hand2.back().X),2)+ pow((hand1.back().Z - hand2.back().Z),2));
		if (magnitudeSum == 0 || dot/magnitudeSum > 1 || dot/magnitudeSum < -1)
		{
			return 0;
		}
		double angle1 = acos( dot / magnitudeSum);

		if (hand1.front().X > hand2.front().X && hand1.front().Z < hand1.back().Z) {
			angle1 *= -1;
		} else if (hand1.front().X < hand2.front().X && hand2.front().Z < hand2.back().Z) {
			angle1 *= -1;
		}

		return (angle1);

	} else {
		return 0.0;
	}
}

Mat getScaleMatrix(double scaleFactor)
{
	double alpha = scaleFactor;
	Mat scale = Mat::eye(Size(3,3), CV_64FC1);
	scale.at<double>(0, 0) = alpha;
	scale.at<double>(1, 1) = alpha;
	return scale;
}

Mat getTranslationMatrix(Point2f offset)
{
	double x = (double)offset.x;
	double y = (double)offset.y;
	Mat translate = Mat::eye(Size(3,3), CV_64FC1);
	translate.at<double>(0,2) = x;
	translate.at<double>(1,2) = y;
	return translate;
}


void XN_CALLBACK_TYPE handCreate(HandsGenerator &generator, XnUserID user, const XnPoint3D *pPosition, XnFloat fTime, void *pCookie){
	printf("handCreate: x %f y %f z %f \n", pPosition->X, pPosition->Y, pPosition->Z);
	
	if (hand1ID == -1) {
		hand1ID = user;
		hand1.push(*pPosition);
	} else if (hand2ID == -1) {
		hand2ID = user;
		hand2.push(*pPosition);
	}
	
	//printf("handCreate: x %f y %f z %f \n", pPosition->X, pPosition->Y, pPosition->Z);
}
void XN_CALLBACK_TYPE handUpdate(HandsGenerator &generator, XnUserID user, const XnPoint3D *pPosition, XnFloat fTime, void *pCookie){

	if (hand1ID == user) {
		if (hand1.size() > 2) {
			hand1.pop();
		}
		hand1.push(*pPosition);
	} else if (hand2ID == user) {
		
		if (hand2.size() > 2) {
			hand2.pop();
		}
		hand2.push(*pPosition);
	}

	//printf("handUpdate: x %f y %f z %f \n", pPosition->X, pPosition->Y, pPosition->Z);
}
void XN_CALLBACK_TYPE handDestroy(HandsGenerator &generator, XnUserID user, XnFloat fTime, void *pCookie){
	printf("hand destroy \n");
	send(ClientSocket, "destroy", 7, 0);
	if (hand1ID == user) {
		hand1ID = -1;
		while (hand1.size() > 0) {
			hand1.pop();
		}
	} else if (hand2ID == user) {
		hand2ID = -1;
		while (hand2.size() > 0) {
			hand2.pop();
		}
	}
	oldZoom = 1;
	oldAngle = 0;
	g_GestureGenerator.AddGesture(GESTURE_TO_USE, NULL);
}

void XN_CALLBACK_TYPE 
Gesture_Recognized(xn::GestureGenerator& generator,
                   const XnChar* strGesture,
                   const XnPoint3D* pIDPosition,
                   const XnPoint3D* pEndPosition, void* pCookie)
{
    printf("Gesture recognized: %s\n", strGesture);
    //g_GestureGenerator.RemoveGesture(strGesture);
    g_HandsGenerator.StartTracking(*pEndPosition);
}
void XN_CALLBACK_TYPE
Gesture_Process(xn::GestureGenerator& generator,
                const XnChar* strGesture,
                const XnPoint3D* pPosition,
                XnFloat fProgress,
                void* pCookie)
{}
