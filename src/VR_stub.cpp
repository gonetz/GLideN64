#include "VR.h"

float VR_ORIENTATION_MAT[4][4] = {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}};
float VR_TRANSFORM_MAT[4][4] = {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}};
bool VR_LEFT_EYE = true;
bool VR_CURRENTLY_RENDERING = false;
bool VR_HAS_CLEARED_SCREEN = false;

int VRSetupSensor() { return 1; }
bool VRRemapCoordinateSystem(float inR[4][4], const int X, const int Y, float outR[4][4])  { return false; }
int VRPollForSensorData() { return 1; }
void VRUpdateTransform() {}
int VRDestroySensor() { return 1; }
