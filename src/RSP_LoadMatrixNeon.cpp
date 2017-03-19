#include <algorithm>
#include <cstring>
#include "Debug.h"
#include "RSP.h"
#include "RDP.h"
#include "N64.h"
#include "F3D.h"
#include "Turbo3D.h"
#include "VI.h"
#include "Combiner.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "FrameBufferInfo.h"
#include "GBI.h"
#include "PluginAPI.h"
#include "Config.h"
#include "TextureFilterHandler.h"
#include "DisplayWindow.h"
#include "arm_neon.h"

using namespace std;

void RSP_LoadMatrix( f32 mtx[4][4], u32 address )
{
    f32 recip = 1.5258789e-05f;

    struct _N64Matrix
    {
        SHORT integer[4][4];
        WORD fraction[4][4];
    } *n64Mat = (struct _N64Matrix *)&RDRAM[address];

    // Load recip
    float32_t _recip = recip;

    // Load integer
    int16x4_t _integer0_s16 = vld1_s16(n64Mat->integer[0]);
    int16x4_t _integer1_s16 = vld1_s16(n64Mat->integer[1]);
    int16x4_t _integer2_s16 = vld1_s16(n64Mat->integer[2]);
    int16x4_t _integer3_s16 = vld1_s16(n64Mat->integer[3]);

    // Load fraction
    uint16x4_t _fraction0_u16 = vld1_u16(n64Mat->fraction[0]);
    uint16x4_t _fraction1_u16 = vld1_u16(n64Mat->fraction[1]);
    uint16x4_t _fraction2_u16 = vld1_u16(n64Mat->fraction[2]);
    uint16x4_t _fraction3_u16 = vld1_u16(n64Mat->fraction[3]);

    // Reverse bytes --> n^1
    _integer0_s16 = vrev32_s16 (_integer0_s16); 
    _integer1_s16 = vrev32_s16 (_integer1_s16); 
    _integer2_s16 = vrev32_s16 (_integer2_s16); 
    _integer3_s16 = vrev32_s16 (_integer3_s16); 
    _fraction0_u16 = vrev32_u16 (_fraction0_u16); 
    _fraction1_u16 = vrev32_u16 (_fraction1_u16); 
    _fraction2_u16 = vrev32_u16 (_fraction2_u16); 
    _fraction3_u16 = vrev32_u16 (_fraction3_u16);
    
    // Expand to 32Bit int/uint
    int32x4_t _integer0_s32 = vmovl_s16(_integer0_s16);
    int32x4_t _integer1_s32 = vmovl_s16(_integer1_s16);
    int32x4_t _integer2_s32 = vmovl_s16(_integer2_s16);
    int32x4_t _integer3_s32 = vmovl_s16(_integer3_s16);
    uint32x4_t _fraction0_u32 = vmovl_u16(_fraction0_u16);
    uint32x4_t _fraction1_u32 = vmovl_u16(_fraction1_u16);
    uint32x4_t _fraction2_u32 = vmovl_u16(_fraction2_u16);
    uint32x4_t _fraction3_u32 = vmovl_u16(_fraction3_u16);
    
    // Convert to Float
    float32x4_t _integer0_f32 = vcvtq_f32_s32 (_integer0_s32); 
    float32x4_t _integer1_f32 = vcvtq_f32_s32 (_integer1_s32); 
    float32x4_t _integer2_f32 = vcvtq_f32_s32 (_integer2_s32); 
    float32x4_t _integer3_f32 = vcvtq_f32_s32 (_integer3_s32); 
    float32x4_t _fraction0_f32 = vcvtq_f32_u32 (_fraction0_u32); 
    float32x4_t _fraction1_f32 = vcvtq_f32_u32 (_fraction1_u32); 
    float32x4_t _fraction2_f32 = vcvtq_f32_u32 (_fraction2_u32); 
    float32x4_t _fraction3_f32 = vcvtq_f32_u32 (_fraction3_u32); 

    // Multiply and add
    _integer0_f32 = vmlaq_n_f32(_integer0_f32,_fraction0_f32,_recip); 
    _integer1_f32 = vmlaq_n_f32(_integer1_f32,_fraction1_f32,_recip);
    _integer2_f32 = vmlaq_n_f32(_integer2_f32,_fraction2_f32,_recip);
    _integer3_f32 = vmlaq_n_f32(_integer3_f32,_fraction3_f32,_recip);

    // Store in mtx
    vst1q_f32(mtx[0], _integer0_f32);
    vst1q_f32(mtx[1], _integer1_f32);
    vst1q_f32(mtx[2], _integer2_f32);
    vst1q_f32(mtx[3], _integer3_f32);
}
