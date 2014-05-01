#define GESTURE_TO_USE "Wave"
#include "XnCppWrapper.h"

using namespace xn;

xn::GestureGenerator g_GestureGenerator;
xn::HandsGenerator g_HandsGenerator;

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

void XN_CALLBACK_TYPE
Hand_Create(xn::HandsGenerator& generator,
            XnUserID nId, const XnPoint3D* pPosition,
            XnFloat fTime, void* pCookie)
{
  printf("New Hand: %d @ (%f,%f,%f)\n", nId,
         pPosition->X, pPosition->Y, pPosition->Z);
}
void XN_CALLBACK_TYPE
	Hand_Update(xn::HandsGenerator& generator,
	XnUserID nId, const XnPoint3D* pPosition,
	XnFloat fTime, void* pCookie)
{
	printf("handUpdate: nId %d\n", nId);
}
void XN_CALLBACK_TYPE
Hand_Destroy(xn::HandsGenerator& generator,
             XnUserID nId, XnFloat fTime,
             void* pCookie)
{
  printf("Lost Hand: %d\n", nId);
  g_GestureGenerator.AddGesture(GESTURE_TO_USE, NULL);
}

void main()
{

  XnStatus nRetVal = XN_STATUS_OK;

  Context context;
  nRetVal = context.Init();
  // TODO: check error code

  // Create the gesture and hands generators
  nRetVal = g_GestureGenerator.Create(context);
  nRetVal = g_HandsGenerator.Create(context);
  // TODO: check error code

  // Register to callbacks
  XnCallbackHandle h1, h2;
  g_GestureGenerator.RegisterGestureCallbacks(Gesture_Recognized,
                                              Gesture_Process,
                                              NULL, h1);
  g_HandsGenerator.RegisterHandCallbacks(Hand_Create, Hand_Update,
                                         Hand_Destroy, NULL, h2);

  // Start generating
  nRetVal = context.StartGeneratingAll();
  // TODO: check error code

  nRetVal = g_GestureGenerator.AddGesture(GESTURE_TO_USE, NULL);

  while (TRUE)
  {
    // Update to next frame
    nRetVal = context.WaitAndUpdateAll();
    // TODO: check error code
  }

  // Clean up
  context.Shutdown();
}