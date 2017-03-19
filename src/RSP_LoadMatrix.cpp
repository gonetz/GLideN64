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

void RSP_LoadMatrix( f32 mtx[4][4], u32 address )
{
	f32 recip = 1.5258789e-05f;
#ifdef WIN32_ASM
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
#else // WIN32_ASM
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
#else // X86_ASM
	struct _N64Matrix
	{
		SHORT integer[4][4];
		WORD fraction[4][4];
	} *n64Mat = (struct _N64Matrix *)&RDRAM[address];

	int i, j;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			mtx[i][j] = (f32)(n64Mat->integer[i][j^1]) + (f32)(n64Mat->fraction[i][j^1]) * recip;
#endif // !X86_ASM
#endif // WIN32_ASM
}