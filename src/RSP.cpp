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

using namespace std;

RSPInfo		RSP;

void RSP_CheckDLCounter()
{
	if (RSP.count != -1) {
		--RSP.count;
		if (RSP.count == 0) {
			RSP.count = -1;
			--RSP.PCi;
			DebugMsg( DEBUG_LOW | DEBUG_HANDLED, "End of DL\n" );
		}
	}
}

void RSP_ProcessDList()
{
	if (ConfigOpen || dwnd().isResizeWindow()) {
		*REG.MI_INTR |= MI_INTR_DP;
		CheckInterrupts();
		return;
	}
	if (*REG.VI_ORIGIN != VI.lastOrigin) {
		VI_UpdateSize();
		dwnd().updateScale();
	}

	RSP.PC[0] = *(u32*)&DMEM[0x0FF0];
	RSP.PCi = 0;
	RSP.count = -1;

	RSP.halt = FALSE;
	RSP.busy = TRUE;

	gSP.matrix.stackSize = min( 32U, *(u32*)&DMEM[0x0FE4] >> 6 );
	if (gSP.matrix.stackSize == 0)
		gSP.matrix.stackSize = 32;
	gSP.matrix.modelViewi = 0;
	gSP.status[0] = gSP.status[1] = gSP.status[2] = gSP.status[3] = 0;
	gSP.changed |= CHANGED_MATRIX;
	gDP.changed &= ~CHANGED_CPU_FB_WRITE;
	gDPSetTexturePersp(G_TP_PERSP);

	u32 uc_start = *(u32*)&DMEM[0x0FD0];
	u32 uc_dstart = *(u32*)&DMEM[0x0FD8];
	u32 uc_dsize = *(u32*)&DMEM[0x0FDC];

	if ((uc_start != RSP.uc_start) || (uc_dstart != RSP.uc_dstart))
		gSPLoadUcodeEx(uc_start, uc_dstart, uc_dsize);

	depthBufferList().setNotCleared();

	if (GBI.getMicrocodeType() == Turbo3D)
		RunTurbo3D();
	else {
		while (!RSP.halt) {
			if ((RSP.PC[RSP.PCi] + 8) > RDRAMSize) {
#ifdef DEBUG
				switch (Debug.level)
				{
					case DEBUG_LOW:
					DebugMsg( DEBUG_LOW | DEBUG_ERROR, "ATTEMPTING TO EXECUTE RSP COMMAND AT INVALID RDRAM LOCATION\n" );
					break;
					case DEBUG_MEDIUM:
					DebugMsg( DEBUG_MEDIUM | DEBUG_ERROR, "Attempting to execute RSP command at invalid RDRAM location\n" );
					break;
					case DEBUG_HIGH:
					DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to execute RSP command at invalid RDRAM location\n" );
					break;
				}
#endif
				break;
			}

			RSP.w0 = *(u32*)&RDRAM[RSP.PC[RSP.PCi]];
			RSP.w1 = *(u32*)&RDRAM[RSP.PC[RSP.PCi] + 4];
			RSP.cmd = _SHIFTR(RSP.w0, 24, 8);

#ifdef DEBUG
			DebugRSPState( RSP.PCi, RSP.PC[RSP.PCi], _SHIFTR( RSP.w0, 24, 8 ), RSP.w0, RSP.w1 );
			DebugMsg( DEBUG_LOW | DEBUG_HANDLED, "0x%08lX: CMD=0x%02lX W0=0x%08lX W1=0x%08lX\n", RSP.PC[RSP.PCi], _SHIFTR( RSP.w0, 24, 8 ), RSP.w0, RSP.w1 );
#endif

			RSP.PC[RSP.PCi] += 8;
			u32 pci = RSP.PCi;
			if (RSP.count == 1)
				--pci;
			RSP.nextCmd = _SHIFTR(*(u32*)&RDRAM[RSP.PC[pci]], 24, 8);

			GBI.cmd[RSP.cmd](RSP.w0, RSP.w1);
			RSP_CheckDLCounter();
		}
	}

	if (config.frameBufferEmulation.copyDepthToRDRAM != Config::cdDisable) {
		if ((config.generalEmulation.hacks & hack_rectDepthBufferCopyCBFD) != 0) {
			; // do nothing
		} else if ((config.generalEmulation.hacks & hack_rectDepthBufferCopyPD) != 0) {
			if (rectDepthBufferCopyFrame == dwnd().getBuffersSwapCount())
				FrameBuffer_CopyDepthBuffer(gDP.colorImage.address);
		} else if (!FBInfo::fbInfo.isSupported())
			FrameBuffer_CopyDepthBuffer(gDP.colorImage.address);
	}

	RSP.busy = FALSE;
	gDP.changed |= CHANGED_COLORBUFFER;
}

static
void RSP_SetDefaultState()
{
	memset(&gSP, 0, sizeof(gSPInfo));

	gSPTexture(1.0f, 1.0f, 0, 0, TRUE);
	gDP.loadTile = &gDP.tiles[7];
	gSP.textureTile[0] = &gDP.tiles[0];
	gSP.textureTile[1] = &gDP.tiles[1];
	gSP.lookat[0].y = gSP.lookat[1].x = 1.0f;
	gSP.lookatEnable = true;

	gSP.objMatrix.A = 1.0f;
	gSP.objMatrix.B = 0.0f;
	gSP.objMatrix.C = 0.0f;
	gSP.objMatrix.D = 1.0f;
	gSP.objMatrix.X = 0.0f;
	gSP.objMatrix.Y = 0.0f;
	gSP.objMatrix.baseScaleX = 1.0f;
	gSP.objMatrix.baseScaleY = 1.0f;
	gSP.objRendermode = 0;

	for (int i = 0; i < 4; i++)
	for (int j = 0; j < 4; j++)
		gSP.matrix.modelView[0][i][j] = 0.0f;

	gSP.matrix.modelView[0][0][0] = 1.0f;
	gSP.matrix.modelView[0][1][1] = 1.0f;
	gSP.matrix.modelView[0][2][2] = 1.0f;
	gSP.matrix.modelView[0][3][3] = 1.0f;

	gDP.otherMode._u64 = 0U;
}

u32 DepthClearColor = 0xfffcfffc;

static
void setDepthClearColor()
{
	if (strstr(RSP.romname, (const char *)"Elmo's") != nullptr)
		DepthClearColor = 0xFFFFFFFF;
	else if (strstr(RSP.romname, (const char *)"Taz Express") != nullptr)
		DepthClearColor = 0xFFBCFFBC;
	else if (strstr(RSP.romname, (const char *)"NFL QBC 2000") != nullptr || strstr(RSP.romname, (const char *)"NFL Quarterback Club") != nullptr || strstr(RSP.romname, (const char *)"Jeremy McGrath Super") != nullptr)
		DepthClearColor = 0xFFFDFFFC;
	else
		DepthClearColor = 0xFFFCFFFC;
}

void RSP_Init()
{
#ifdef OS_WINDOWS
	// Calculate RDRAM size by intentionally causing an access violation
	u32 test;
	try
	{
		test = RDRAM[0x007FFFFF] + 1;
	}
	catch (...)
	{
		test = 0;
	}
	if (test > 0)
		RDRAMSize = 0x7FFFFF;
	else
		RDRAMSize = 0x3FFFFF;
#else // OS_WINDOWS
	RDRAMSize = 1024 * 1024 * 8 - 1;
#endif // OS_WINDOWS

	RSP.uc_start = RSP.uc_dstart = 0;
	RSP.bLLE = false;

	// get the name of the ROM
	char romname[21];
	for (int i = 0; i < 20; ++i)
		romname[i] = HEADER[(32 + i) ^ 3];
	romname[20] = 0;

	// remove all trailing spaces
	while (romname[strlen(romname) - 1] == ' ')
		romname[strlen(romname) - 1] = 0;

	if (strcmp(RSP.romname, romname) != 0)
		TFH.shutdown();

	strncpy(RSP.romname, romname, 21);
	setDepthClearColor();
	config.generalEmulation.hacks = 0;
	if (strstr(RSP.romname, (const char *)"OgreBattle64") != nullptr)
		config.generalEmulation.hacks |= hack_Ogre64;
	else if (strstr(RSP.romname, (const char *)"F1 POLE POSITION 64") != nullptr)
		config.generalEmulation.hacks |= hack_noDepthFrameBuffers;
	else if (strstr(RSP.romname, (const char *)"ROADSTERS TROPHY") != nullptr)
		config.generalEmulation.hacks |= hack_noDepthFrameBuffers;
	else if (strstr(RSP.romname, (const char *)"CONKER BFD") != nullptr)
		config.generalEmulation.hacks |= hack_blurPauseScreen | hack_rectDepthBufferCopyCBFD;
	else if (strstr(RSP.romname, (const char *)"MICKEY USA") != nullptr)
		config.generalEmulation.hacks |= hack_blurPauseScreen;
	else if (strstr(RSP.romname, (const char *)"MarioTennis64") != nullptr)
		config.generalEmulation.hacks |= hack_scoreboardJ;
	else if (strstr(RSP.romname, (const char *)"MarioTennis") != nullptr)
		config.generalEmulation.hacks |= hack_scoreboard;
	else if (strstr(RSP.romname, (const char *)"Pilot Wings64") != nullptr)
		config.generalEmulation.hacks |= hack_pilotWings;
	else if (strstr(RSP.romname, (const char *)"THE LEGEND OF ZELDA") != nullptr ||
			 strstr(RSP.romname, (const char *)"ZELDA MASTER QUEST") != nullptr ||
			 strstr(RSP.romname, (const char *)"DOUBUTSUNOMORI") != nullptr ||
			 strstr(RSP.romname, (const char *)"ANIMAL FOREST") != nullptr)
		config.generalEmulation.hacks |= hack_subscreen;
	else if (strstr(RSP.romname, (const char *)"LEGORacers") != nullptr)
		config.generalEmulation.hacks |= hack_legoRacers;
	else if (strstr(RSP.romname, (const char *)"Blast") != nullptr)
		config.generalEmulation.hacks |= hack_blastCorps;
	else if (strstr(RSP.romname, (const char *)"SPACE INVADERS") != nullptr)
		config.generalEmulation.hacks |= hack_ignoreVIHeightChange;
	else if (strstr(RSP.romname, (const char *)"MASK") != nullptr) // Zelda MM
		config.generalEmulation.hacks |= hack_ZeldaMM;
	else if (strstr(RSP.romname, (const char *)"Perfect Dark") != nullptr ||
			 strstr(RSP.romname, (const char *)"PERFECT DARK") != nullptr)
		config.generalEmulation.hacks |= hack_rectDepthBufferCopyPD;
	else if (strstr(RSP.romname, (const char *)"Jeremy McGrath Super") != nullptr)
		config.generalEmulation.hacks |= hack_ModifyVertexXyInShader;
	else if (strstr(RSP.romname, (const char *)"Quake") != nullptr ||
			 strstr(RSP.romname, (const char *)"QUAKE II") != nullptr)
		config.generalEmulation.hacks |= hack_doNotResetTLUTmode;
	else if (strstr(RSP.romname, (const char *)"quarterback_club_98") != nullptr)
		config.generalEmulation.hacks |= hack_LoadDepthTextures;
	else if (strstr(RSP.romname, (const char *)"WIN BACK") != nullptr ||
		strstr(RSP.romname, (const char *)"OPERATION WINBACK") != nullptr)
		config.generalEmulation.hacks |= hack_WinBack;
	else if (strstr(RSP.romname, (const char *)"POKEMON SNAP") != nullptr)
		config.generalEmulation.hacks |= hack_Snap;

	api().FindPluginPath(RSP.pluginpath);

	RSP_SetDefaultState();
}
