#ifndef CONFIG_H
#define CONFIG_H

#include "ConfigDecl.h"

#define hack_Ogre64					(1<<0)  //Ogre Battle 64 background copy
#define hack_noDepthFrameBuffers	(1<<1)  //Do not use depth buffers as texture
#define hack_blurPauseScreen		(1<<2)  //Game copies frame buffer to depth buffer area, CPU blurs it. That image is used as background for pause screen.
#define hack_clearAloneDepthBuffer	(1<<3)  //Force clear depth buffer if there is no frame buffer for it. Multiplayer in GE and PD.
#define hack_StarCraftBackgrounds	(1<<4)  //StarCraft special check for frame buffer usage.
#define hack_texrect_shade_alpha	(1<<5)  //Set vertex alpha to 1 when texrect alpha combiner uses shade. Pokemon Stadium 2
#define hack_subscreen				(1<<6)  //Fix subscreen delay in Zelda OOT and Doubutsu no Mori
#define hack_blastCorps				(1<<7)  //Blast Corps black polygons
#define hack_rectDepthBufferCopyPD	(1<<8)  //Copy depth buffer only when game need it. Optimized for PD
#define hack_rectDepthBufferCopyCBFD (1<<9) //Copy depth buffer only when game need it. Optimized for CBFD
#define hack_WinBack				(1<<10) //Hack for WinBack to remove gray rectangle in HLE mode
#define hack_ZeldaMM				(1<<11) //Special hacks for Zelda MM
#define hack_ModifyVertexXyInShader	(1<<12) //Pass screen coordinates provided in gSPModifyVertex to vertes shader.
#define hack_LodeRunner				(1<<13) //Hack for Lode runner VI issues.
#define hack_doNotResetOtherModeH	(1<<14) //Don't reset othermode.h after dlist end. Quake and Quake 2
#define hack_doNotResetOtherModeL	(1<<15) //Don't reset othermode.l after dlist end. Quake
#define hack_LoadDepthTextures		(1<<16) //Load textures for depth buffer
#define hack_Snap					(1<<17) //Frame buffer settings for camera detection in Pokemon Snap. Copy aux buffers at fullsync
#define hack_MK64					(1<<18) //Hack for load MK64 HD textures properly.
#define hack_RE2					(1<<19) //RE2 hacks.
#define hack_ZeldaMonochrome		(1<<20) //Hack for Zeldas monochrome effects.
#define hack_TonyHawk				(1<<21) //Hack for Tony Hawk blend mode.
#define hack_WCWNitro				(1<<22) //Hack for WCW Nitro backgrounds.

extern Config config;

void Config_LoadConfig();
#ifndef MUPENPLUSAPI
void Config_DoConfig(/*HWND hParent*/);
#endif

bool isHWLightingAllowed();

#endif // CONFIG_H
