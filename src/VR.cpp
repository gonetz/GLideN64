#define ASENSOR_TYPE_ROTATION_VECTOR 15
#include <android/looper.h>
#include <android/sensor.h>
#include <cstring>
#include "VR.h"
#include "3DMath.h"
#include "gSP.h"
#include "Config.h"

float VR_ORIENTATION_MAT[4][4] = {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}};
float VR_TRANSFORM_MAT[4][4] = {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}};
bool VR_LEFT_EYE = true;
bool VR_CURRENTLY_RENDERING = false;
bool VR_HAS_CLEARED_SCREEN = false;

static ASensorEventQueue* VR_SENSOR_QUEUE = NULL;
static ASensorRef VR_SENSOR = NULL;

int VRSetupSensor() {
    if (!config.vr.enable) return 1;

    if (VR_SENSOR_QUEUE != NULL) {
        LOGD("**************** VR_SENSOR_QUEUE Already Initialized!\n");
        return 1;
    }

    ASensorManager* sensor_manager =
            ASensorManager_getInstance();
    if (!sensor_manager) {
        LOGD("**************** Failed to get a sensor manager\n");
        return 1;
    }
    ASensorList sensor_list = NULL;
    int sensor_count = ASensorManager_getSensorList(sensor_manager, &sensor_list);
    LOGD("**************** Found %d supported sensors\n", sensor_count);
    for (int i = 0; i < sensor_count; i++) {
        LOGD("**************** HAL supports sensor %s\n", ASensor_getName(sensor_list[i]));
    }
    const int kLooperId = 1;
    VR_SENSOR_QUEUE = ASensorManager_createEventQueue(
            sensor_manager,
            ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS),
            kLooperId,
            NULL, /* no callback */
            NULL  /* no private data for a callback  */);
    if (!VR_SENSOR_QUEUE) {
        LOGD("**************** Failed to create a sensor event queue\n");
        return 1;
    }
    // Find the first sensor of the specified type that can be opened
    const int kTimeoutMicroSecs = 10000;
    bool sensor_found = false;
    for (int i = 0; i < sensor_count; i++) {
        ASensorRef sensor = sensor_list[i];
        if (ASensor_getType(sensor) != ASENSOR_TYPE_ROTATION_VECTOR)
            continue;
        if (ASensorEventQueue_enableSensor(VR_SENSOR_QUEUE, sensor) < 0)
            continue;
        if (ASensorEventQueue_setEventRate(VR_SENSOR_QUEUE, sensor, kTimeoutMicroSecs) < 0) {
            LOGD("**************** Failed to set the %s sample rate\n",
                 ASensor_getName(sensor));
            return 1;
        }
        // Found an equipped sensor of the specified type.
        sensor_found = true;
        VR_SENSOR = sensor;
        break;
    }
    if (!sensor_found) {
        LOGD("**************** No sensor of the specified type found\n");
        int ret = ASensorManager_destroyEventQueue(sensor_manager, VR_SENSOR_QUEUE);
        if (ret < 0)
            LOGD("**************** Failed to destroy event queue: %s\n", strerror(-ret));
        VR_SENSOR_QUEUE = NULL;
        return 1;
    }
    LOGD("\n**************** Sensor %s activated\n", ASensor_getName(VR_SENSOR));

    return 0;
}

const int AXIS_X = 1;
const int AXIS_Y = 2;
const int AXIS_Z = 3;
const int AXIS_MINUS_X = AXIS_X | 0x80;
const int AXIS_MINUS_Y = AXIS_Y | 0x80;
const int AXIS_MINUS_Z = AXIS_Z | 0x80;

bool VRRemapCoordinateSystem(float inR[4][4], const int X, const int Y, float outR[4][4])
{
    /*
     * X and Y define a rotation matrix 'r':
     *
     *  (X==1)?((X&0x80)?-1:1):0    (X==2)?((X&0x80)?-1:1):0    (X==3)?((X&0x80)?-1:1):0
     *  (Y==1)?((Y&0x80)?-1:1):0    (Y==2)?((Y&0x80)?-1:1):0    (Y==3)?((X&0x80)?-1:1):0
     *                              r[0] ^ r[1]
     *
     * where the 3rd line is the vector product of the first 2 lines
     *
     */
    if ((X & 0x7C)!=0 || (Y & 0x7C)!=0)
        return false;   // invalid parameter
    if (((X & 0x3)==0) || ((Y & 0x3)==0))
        return false;   // no axis specified
    if ((X & 0x3) == (Y & 0x3))
        return false;   // same axis specified
    // Z is "the other" axis, its sign is either +/- sign(X)*sign(Y)
    // this can be calculated by exclusive-or'ing X and Y; except for
    // the sign inversion (+/-) which is calculated below.
    int Z = X ^ Y;
    // extract the axis (remove the sign), offset in the range 0 to 2.
    const int x = (X & 0x3)-1;
    const int y = (Y & 0x3)-1;
    const int z = (Z & 0x3)-1;
    // compute the sign of Z (whether it needs to be inverted)
    const int axis_y = (z+1)%3;
    const int axis_z = (z+2)%3;
    if (((x^axis_y)|(y^axis_z)) != 0)
        Z ^= 0x80;
    const bool sx = (X>=0x80);
    const bool sy = (Y>=0x80);
    const bool sz = (Z>=0x80);
    // Perform R * r, in avoiding actual muls and adds.
    for (unsigned int j=0 ; j<3 ; j++) {
        for (unsigned int i=0 ; i<3 ; i++) {
            if (x==i)   outR[j][i] = sx ? -inR[j][0] : inR[j][0];
            if (y==i)   outR[j][i] = sy ? -inR[j][1] : inR[j][1];
            if (z==i)   outR[j][i] = sz ? -inR[j][2] : inR[j][2];
        }
    }
    outR[0][3] = outR[1][3] = outR[2][3] = outR[3][0] = outR[3][1] = outR[3][2] = 0;
    outR[3][3] = 1;
    return true;
}

int VRPollForSensorData() {
    if (!config.vr.enable) return 1;

    if (!VR_SENSOR_QUEUE || !VR_SENSOR) {
        LOGD("**************** Sensors not initialized\n");
        return 1;
    }

    ASensorEvent data[1];
    memset(data, 0, sizeof(data));
    ALooper_pollAll(
            0, // timeout
            NULL /* no output file descriptor */,
            NULL /* no output event */,
            NULL /* no output data */);
    if (ASensorEventQueue_getEvents(VR_SENSOR_QUEUE, data, 1) <= 0) {
        //LOGD("**************** Failed to read data from the sensor.\n");
        return 1;
    }

    float q1 = data[0].data[0];
    float q2 = data[0].data[1];
    float q3 = data[0].data[2];
    float q0 = data[0].data[3];

    float d = (float) sqrt(q1*q1+q2*q2+q3*q3+q0*q0);
    q1 /= d; q2 /= d; q3 /= d; q0 /= d;

    float sq_q1 = 2 * q1 * q1;
    float sq_q2 = 2 * q2 * q2;
    float sq_q3 = 2 * q3 * q3;
    float q1_q2 = 2 * q1 * q2;
    float q3_q0 = 2 * q3 * q0;
    float q1_q3 = 2 * q1 * q3;
    float q2_q0 = 2 * q2 * q0;
    float q2_q3 = 2 * q2 * q3;
    float q1_q0 = 2 * q1 * q0;

    float R[4][4] = {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}};

    R[0][0] = 1 - sq_q2 - sq_q3;
    R[0][1] = q1_q2 - q3_q0;
    R[0][2] = q1_q3 + q2_q0;
    R[1][0] = q1_q2 + q3_q0;
    R[1][1] = 1 - sq_q1 - sq_q3;
    R[1][2] = q2_q3 - q1_q0;
    R[2][0] = q1_q3 - q2_q0;
    R[2][1] = q2_q3 + q1_q0;
    R[2][2] = 1 - sq_q1 - sq_q2;

    VRRemapCoordinateSystem(R, AXIS_Y, AXIS_MINUS_X, VR_ORIENTATION_MAT);

    float rot_mat[4][4] = {{1,0,0,0}, {0,0,1,0}, {0,-1,0,0}, {0,0,0,1}};
    float res_mat[4][4] = {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}};
    MultMatrix(VR_ORIENTATION_MAT, rot_mat, res_mat);
    CopyMatrix(VR_ORIENTATION_MAT, res_mat);

    while (ASensorEventQueue_getEvents(VR_SENSOR_QUEUE, data, 1) > 0);

    return 0;
}

void VRUpdateTransform() {
    if (!config.vr.enable) return;

    float trans = -EYE_DISTANCE;
    if (VR_LEFT_EYE) trans *= -1;
    float trans_mat[4][4] = {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {trans,0,0,1}};
    float res_mat[4][4] = {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}};
    MultMatrix(trans_mat, VR_ORIENTATION_MAT, res_mat);

    MultMatrix(gSP.matrix.projection, res_mat, VR_TRANSFORM_MAT);

    // Hack around swashed viewport
    float change_aspect[4][4] = {{1,0,0,0}, {0,0.5,0,0}, {0,0,1,0}, {0,0,0,1}};
    MultMatrix(change_aspect, VR_TRANSFORM_MAT, res_mat);
    CopyMatrix(VR_TRANSFORM_MAT, res_mat);
}

int VRDestroySensor() {
    if (!config.vr.enable) return 1;

    ASensorManager* sensor_manager =
            ASensorManager_getInstance();
    if (!sensor_manager) {
        LOGD("**************** Failed to get a sensor manager\n");
        return 1;
    }

    int ret = ASensorEventQueue_disableSensor(VR_SENSOR_QUEUE, VR_SENSOR);
    if (ret < 0) {
        LOGD("**************** Failed to disable %s: %s\n",
             ASensor_getName(VR_SENSOR), strerror(-ret));
    }
    ret = ASensorManager_destroyEventQueue(sensor_manager, VR_SENSOR_QUEUE);
    if (ret < 0) {
        LOGD("**************** Failed to destroy event queue: %s\n", strerror(-ret));
        return 1;
    }

    return 0;
}
