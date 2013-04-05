#ifndef __LINUX__
# include <windows.h>
#else
# include "winlnxdefs.h"
#endif
#include <math.h>
#include "glN64.h"
#include "OpenGL.h"
#include "Debug.h"
#include "RSP.h"
#include "RDP.h"
#include "N64.h"
#include "F3D.h"
#include "3DMath.h"
#include "VI.h"
#include "Combiner.h"
//#include "textures.h"
//#include "Config.h"
#include "FrameBuffer.h"
#include "DepthBuffer.h"
#include "GBI.h"

RSPInfo		RSP;

void RSP_LoadMatrix( f32 mtx[4][4], u32 address )
{
	f32 recip = 1.5258789e-05f;
#ifndef __LINUX__
	__asm {
		mov		esi, dword ptr [RDRAM];
		add		esi, dword ptr [address];
		mov		edi, dword ptr [mtx];

		mov		ecx, 4
LoadLoop:
		fild	word ptr [esi+02h]
		movzx	eax, word ptr [esi+22h]
		mov		dword ptr [edi], eax
		fild	dword ptr [edi]
		fmul	dword ptr [recip]
		fadd
		fstp	dword ptr [edi]

		fild	word ptr [esi+00h]
		movzx	eax, word ptr [esi+20h]
		mov		dword ptr [edi+04h], eax
		fild	dword ptr [edi+04h]
		fmul	dword ptr [recip]
		fadd
		fstp	dword ptr [edi+04h]

		fild	word ptr [esi+06h]
		movzx	eax, word ptr [esi+26h]
		mov		dword ptr [edi+08h], eax
		fild	dword ptr [edi+08h]
		fmul	dword ptr [recip]
		fadd
		fstp	dword ptr [edi+08h]

		fild	word ptr [esi+04h]
		movzx	eax, word ptr [esi+24h]
		mov		dword ptr [edi+0Ch], eax
		fild	dword ptr [edi+0Ch]
		fmul	dword ptr [recip]
		fadd
		fstp	dword ptr [edi+0Ch]

		add		esi, 08h
		add		edi, 10h
		loop	LoadLoop
	}
#else // !__LINUX__
# ifdef X86_ASM
	__asm__ __volatile__(
	".intel_syntax noprefix"					"\n\t"
	"LoadLoop:"									"\n\t"
	"	fild	word ptr [esi+0x02]"			"\n\t"
	"	movzx	eax, word ptr [esi+0x22]"		"\n\t"
	"	mov		dword ptr [edi], eax"			"\n\t"
	"	fild	dword ptr [edi]"				"\n\t"
	"	fmul	%0"								"\n\t"
	"	fadd"									"\n\t"
	"	fstp	dword ptr [edi]"				"\n\t"

	"	fild	word ptr [esi+0x00]"			"\n\t"
	"	movzx	eax, word ptr [esi+0x20]"		"\n\t"
	"	mov		dword ptr [edi+0x04], eax"		"\n\t"
	"	fild	dword ptr [edi+0x04]"			"\n\t"
	"	fmul	%0"								"\n\t"
	"	fadd"									"\n\t"
	"	fstp	dword ptr [edi+0x04]"			"\n\t"

	"	fild	word ptr [esi+0x06]"			"\n\t"
	"	movzx	eax, word ptr [esi+0x26]"		"\n\t"
	"	mov		dword ptr [edi+0x08], eax"		"\n\t"
	"	fild	dword ptr [edi+0x08]"			"\n\t"
	"	fmul	%0"								"\n\t"
	"	fadd"									"\n\t"
	"	fstp	dword ptr [edi+0x08]"			"\n\t"

	"	fild	word ptr [esi+0x04]"			"\n\t"
	"	movzx	eax, word ptr [esi+0x24]"		"\n\t"
	"	mov		dword ptr [edi+0x0C], eax"		"\n\t"
	"	fild	dword ptr [edi+0x0C]"			"\n\t"
	"	fmul	%0"								"\n\t"
	"	fadd"									"\n\t"
	"	fstp	dword ptr [edi+0x0C]"			"\n\t"

	"	add		esi, 0x08"						"\n\t"
	"	add		edi, 0x10"						"\n\t"
	"	loop	LoadLoop"						"\n\t"
	".att_syntax prefix"						"\n\t"
	: /* no output */
	: "f"(recip), "S"((int)RDRAM+address), "D"(mtx), "c"(4)
	: "memory" );
# else // X86_ASM
	struct _N64Matrix
	{
		SHORT integer[4][4];
		WORD fraction[4][4];
	} *n64Mat = (struct _N64Matrix *)&RDRAM[address];
	int i, j;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			mtx[i][j] = (GLfloat)(n64Mat->integer[i][j^1]) + (GLfloat)(n64Mat->fraction[i][j^1]) * recip;
# endif // !X86_ASM
#endif // __LINUX__
}

#ifdef RSPTHREAD
DWORD WINAPI RSP_ThreadProc( LPVOID lpParameter )
{
	RSP_Init();

	SetEvent( RSP.threadFinished );
#ifndef _DEBUG
	__try
	{
#endif
		while (TRUE)
		{
			switch (WaitForMultipleObjects( 6, RSP.threadMsg, FALSE, INFINITE ))
			{
				case (WAIT_OBJECT_0 + RSPMSG_PROCESSDLIST):
					RSP_ProcessDList();
					break;
				case (WAIT_OBJECT_0 + RSPMSG_UPDATESCREEN):
					VI_UpdateScreen();
					break;
				case (WAIT_OBJECT_0 + RSPMSG_CLOSE):
					OGL_Stop();
					SetEvent( RSP.threadFinished );
					return 1;
				case (WAIT_OBJECT_0 + RSPMSG_DESTROYTEXTURES):
					Combiner_Destroy();
					FrameBuffer_Destroy();
					TextureCache_Destroy();
					break;
				case (WAIT_OBJECT_0 + RSPMSG_INITTEXTURES):
					FrameBuffer_Init();
					TextureCache_Init();
					Combiner_Init();
					gSP.changed = gDP.changed = 0xFFFFFFFF;
					break;
				case (WAIT_OBJECT_0 + RSPMSG_CAPTURESCREEN):
					OGL_SaveScreenshot();
					break;
			}
			SetEvent( RSP.threadFinished );
		}
#ifndef _DEBUG
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		char exception[256];
		sprintf( exception, "Win32 exception 0x%08X occured in glN64", GetExceptionCode() );
		MessageBox( NULL, exception, pluginName, MB_OK | MB_ICONERROR );

		GBI_Destroy();
		DepthBuffer_Destroy();
		OGL_Stop();
	}
#endif
	RSP.thread = NULL;
	return 0;
}
#endif // RSPTHREAD

void RSP_ProcessDList()
{
	VI_UpdateSize();
	OGL_UpdateScale();

	RSP.PC[0] = *(u32*)&DMEM[0x0FF0];
	RSP.PCi = 0;
	RSP.count = 0;

	RSP.halt = FALSE;
	RSP.busy = TRUE;

	gSP.matrix.stackSize = min( 32, *(u32*)&DMEM[0x0FE4] >> 6 );
	gSP.matrix.modelViewi = 0;
	gSP.changed |= CHANGED_MATRIX;

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			gSP.matrix.modelView[0][i][j] = 0.0f;

	gSP.matrix.modelView[0][0][0] = 1.0f;
	gSP.matrix.modelView[0][1][1] = 1.0f;
	gSP.matrix.modelView[0][2][2] = 1.0f;
	gSP.matrix.modelView[0][3][3] = 1.0f;

	u32 uc_start = *(u32*)&DMEM[0x0FD0];
	u32 uc_dstart = *(u32*)&DMEM[0x0FD8];
	u32 uc_dsize = *(u32*)&DMEM[0x0FDC];

	if ((uc_start != RSP.uc_start) || (uc_dstart != RSP.uc_dstart))
		gSPLoadUcodeEx( uc_start, uc_dstart, uc_dsize );

	gDPSetAlphaCompare( G_AC_NONE );
	gDPSetDepthSource( G_ZS_PIXEL );
	gDPSetRenderMode( 0, 0 );
	gDPSetAlphaDither( G_AD_DISABLE );
	gDPSetColorDither( G_CD_DISABLE );
	gDPSetCombineKey( G_CK_NONE );
	gDPSetTextureConvert( G_TC_FILT );
	gDPSetTextureFilter( G_TF_POINT );
	gDPSetTextureLUT( G_TT_NONE );
	gDPSetTextureLOD( G_TL_TILE );
	gDPSetTextureDetail( G_TD_CLAMP );
	gDPSetTexturePersp( G_TP_PERSP );
	gDPSetCycleType( G_CYC_1CYCLE );
	gDPPipelineMode( G_PM_NPRIMITIVE );

	while (!RSP.halt)
	{
		if ((RSP.PC[RSP.PCi] + 8) > RDRAMSize)
		{
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

//		printf( "!!!!!! RDRAM = 0x%8.8x\n", RDRAM );//RSP.PC[RSP.PCi] );
/*		{
			static u8 *lastRDRAM = 0;
			if (lastRDRAM == 0)
				lastRDRAM = RDRAM;
			if (RDRAM != lastRDRAM)
			{
				__asm__( "int $3" );
			}
		}*/
		u32 w0 = *(u32*)&RDRAM[RSP.PC[RSP.PCi]];
		u32 w1 = *(u32*)&RDRAM[RSP.PC[RSP.PCi] + 4];
		RSP.cmd = _SHIFTR( w0, 24, 8 );

#ifdef DEBUG
		DebugRSPState( RSP.PCi, RSP.PC[RSP.PCi], _SHIFTR( w0, 24, 8 ), w0, w1 );
		DebugMsg( DEBUG_LOW | DEBUG_HANDLED, "0x%08lX: CMD=0x%02lX W0=0x%08lX W1=0x%08lX\n", RSP.PC[RSP.PCi], _SHIFTR( w0, 24, 8 ), w0, w1 );
#endif

		RSP.PC[RSP.PCi] += 8;
		RSP.nextCmd = _SHIFTR( *(u32*)&RDRAM[RSP.PC[RSP.PCi]], 24, 8 );

		GBI.cmd[RSP.cmd]( w0, w1 );
	}

/*	if (OGL.frameBufferTextures && gDP.colorImage.changed)
	{
		FrameBuffer_SaveBuffer( gDP.colorImage.address, gDP.colorImage.size, gDP.colorImage.width, gDP.colorImage.height );
		gDP.colorImage.changed = FALSE;
	}*/

	RSP.busy = FALSE;
	RSP.DList++;
	gSP.changed |= CHANGED_COLORBUFFER;
}

void RSP_Init()
{
	u8 test;
	u32 testAddress;

	// Calculate RDRAM size by intentionally causing an access violation
#ifndef __LINUX__
	__try
	{

		testAddress = 0;
		while (TRUE)
		{
			test = RDRAM[testAddress];
			testAddress++;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		RDRAMSize = testAddress;
	}
#else // !__LINUX__
	RDRAMSize = 1024 * 1024 * 8;
#endif // __LINUX__

	RSP.DList = 0;
	RSP.uc_start = RSP.uc_dstart = 0;

	gDP.loadTile = &gDP.tiles[7];
	gSP.textureTile[0] = &gDP.tiles[0];
	gSP.textureTile[1] = &gDP.tiles[1];
	DepthBuffer_Init();
	GBI_Init();
	OGL_Start();
}
