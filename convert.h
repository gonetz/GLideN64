#ifndef CONVERT_H
#define CONVERT_H

#include "Types.h"

// inline asm in intel syntax and optimizations break things...
#ifdef X86_ASM
# define X86_ASM_DEFINED
# undef X86_ASM
#endif

#if !(defined(X86_ASM) && defined(__LINUX__))
const volatile unsigned char Five2Eight[32] =
{
	  0, // 00000 = 00000000
	  8, // 00001 = 00001000
	 16, // 00010 = 00010000
	 25, // 00011 = 00011001
	 33, // 00100 = 00100001
	 41, // 00101 = 00101001
	 49, // 00110 = 00110001
	 58, // 00111 = 00111010
	 66, // 01000 = 01000010
	 74, // 01001 = 01001010
	 82, // 01010 = 01010010
	 90, // 01011 = 01011010
	 99, // 01100 = 01100011
	107, // 01101 = 01101011
	115, // 01110 = 01110011
	123, // 01111 = 01111011
	132, // 10000 = 10000100
	140, // 10001 = 10001100
	148, // 10010 = 10010100
	156, // 10011 = 10011100
	165, // 10100 = 10100101
	173, // 10101 = 10101101
	181, // 10110 = 10110101
	189, // 10111 = 10111101
	197, // 11000 = 11000101
	206, // 11001 = 11001110
	214, // 11010 = 11010110
	222, // 11011 = 11011110
	230, // 11100 = 11100110
	239, // 11101 = 11101111
	247, // 11110 = 11110111
	255  // 11111 = 11111111
};

const volatile unsigned char Four2Eight[16] =
{
	  0, // 0000 = 00000000
	 17, // 0001 = 00010001
	 34, // 0010 = 00100010
	 51, // 0011 = 00110011
	 68, // 0100 = 01000100
	 85, // 0101 = 01010101
	102, // 0110 = 01100110
	119, // 0111 = 01110111
	136, // 1000 = 10001000
	153, // 1001 = 10011001
	170, // 1010 = 10101010
	187, // 1011 = 10111011
	204, // 1100 = 11001100
	221, // 1101 = 11011101
	238, // 1110 = 11101110
	255  // 1111 = 11111111
};

const volatile unsigned char Three2Four[8] =
{
	 0, // 000 = 0000
     2, // 001 = 0010
	 4, // 010 = 0100
	 6, // 011 = 0110
	 9, // 100 = 1001
	11, // 101 = 1011
    13, // 110 = 1101
	15, // 111 = 1111
};

const volatile unsigned char Three2Eight[8] =
{
	  0, // 000 = 00000000
     36, // 001 = 00100100
	 73, // 010 = 01001001
	109, // 011 = 01101101
	146, // 100 = 10010010
	182, // 101 = 10110110
    219, // 110 = 11011011
	255, // 111 = 11111111
};
const volatile unsigned char Two2Eight[4] =
{
	  0, // 00 = 00000000
	 85, // 01 = 01010101
	170, // 10 = 10101010
	255  // 11 = 11111111
};

const volatile unsigned char One2Four[2] =
{
	 0, // 0 = 0000
	15, // 1 = 1111
};

const volatile unsigned char One2Eight[2] =
{
	  0, // 0 = 00000000
	255, // 1 = 11111111
};
#else
__asm__(
	"	.align 32"						"\n\t"

	"	.type	Five2Eight,@object"		"\n\t"
	"	.size	Five2Eight,32"			"\n\t"
	"Five2Eight:"						"\n\t"
	"	.byte	0"						"\n\t"
	"	.byte	8"						"\n\t"
	"	.byte	16"						"\n\t"
	"	.byte	25"						"\n\t"
	"	.byte	33"						"\n\t"
	"	.byte	41"						"\n\t"
	"	.byte	49"						"\n\t"
	"	.byte	58"						"\n\t"
	"	.byte	66"						"\n\t"
	"	.byte	74"						"\n\t"
	"	.byte	82"						"\n\t"
	"	.byte	90"						"\n\t"
	"	.byte	99"						"\n\t"
	"	.byte	107"					"\n\t"
	"	.byte	115"					"\n\t"
	"	.byte	123"					"\n\t"
	"	.byte	-124"					"\n\t"
	"	.byte	-116"					"\n\t"
	"	.byte	-108"					"\n\t"
	"	.byte	-100"					"\n\t"
	"	.byte	-91"					"\n\t"
	"	.byte	-83"					"\n\t"
	"	.byte	-75"					"\n\t"
	"	.byte	-67"					"\n\t"
	"	.byte	-59"					"\n\t"
	"	.byte	-50"					"\n\t"
	"	.byte	-42"					"\n\t"
	"	.byte	-34"					"\n\t"
	"	.byte	-26"					"\n\t"
	"	.byte	-17"					"\n\t"
	"	.byte	-9"						"\n\t"
	"	.byte	-1"						"\n\t"

	"	.type	Four2Eight, @object"	"\n\t"
	"	.size	Four2Eight, 16"			"\n\t"
	"Four2Eight:"						"\n\t"
	"	.byte	0"						"\n\t"
	"	.byte	17"						"\n\t"
	"	.byte	34"						"\n\t"
	"	.byte	51"						"\n\t"
	"	.byte	68"						"\n\t"
	"	.byte	85"						"\n\t"
	"	.byte	102"					"\n\t"
	"	.byte	119"					"\n\t"
	"	.byte	-120"					"\n\t"
	"	.byte	-103"					"\n\t"
	"	.byte	-86"					"\n\t"
	"	.byte	-69"					"\n\t"
	"	.byte	-52"					"\n\t"
	"	.byte	-35"					"\n\t"
	"	.byte	-18"					"\n\t"
	"	.byte	-1"						"\n\t"

	"	.type	Three2Four, @object"	"\n\t"
	"	.size	Three2Four, 8"			"\n\t"
	"Three2Four:"						"\n\t"
	"	.byte	0"						"\n\t"
	"	.byte	2"						"\n\t"
	"	.byte	4"						"\n\t"
	"	.byte	6"						"\n\t"
	"	.byte	9"						"\n\t"
	"	.byte	11"						"\n\t"
	"	.byte	13"						"\n\t"
	"	.byte	15"						"\n\t"

	"	.type	Three2Eight, @object"	"\n\t"
	"	.size	Three2Eight, 8"			"\n\t"
	"Three2Eight:"						"\n\t"
	"	.byte	0"						"\n\t"
	"	.byte	36"						"\n\t"
	"	.byte	73"						"\n\t"
	"	.byte	109"					"\n\t"
	"	.byte	-110"					"\n\t"
	"	.byte	-74"					"\n\t"
	"	.byte	-37"					"\n\t"
	"	.byte	-1"						"\n\t"

	"	.type	Two2Eight, @object"		"\n\t"
	"	.size	Two2Eight, 4"			"\n\t"
	"Two2Eight:"						"\n\t"
	"	.byte	0"						"\n\t"
	"	.byte	85"						"\n\t"
	"	.byte	-86"					"\n\t"
	"	.byte	-1"						"\n\t"

	"	.type	One2Four, @object"		"\n\t"
	"	.size	One2Four, 2"			"\n\t"
	"One2Four:"							"\n\t"
	"	.byte	0"						"\n\t"
	"	.byte	15"						"\n\t"

	"	.type	One2Eight, @object"		"\n\t"
	"	.size	One2Eight, 2"			"\n\t"
	"One2Eight:"						"\n\t"
	"	.byte	0"						"\n\t"
	"	.byte	-1"						"\n\t"
);
#endif

// Un-swaps on the dword, works with non-dword aligned addresses
/*inline void UnswapCopy( void *src, void *dest, u32 numBytes )
{
	__asm
	{
		mov		ecx, 0
		mov		esi, dword ptr [src]
		mov		edi, dword ptr [dest]

		mov		ebx, esi
		and		ebx, 3			// ebx = number of leading bytes

		cmp		ebx, 0
 		jz		StartDWordLoop

		neg		ebx
		add		ebx, 4
		cmp		ebx, [numBytes]
		jle		NotGreater
		mov		ebx, [numBytes]
NotGreater:
		mov		ecx, ebx

		xor		esi, 3

LeadingLoop:				// Copies leading bytes, in reverse order (un-swaps)
		mov		al, byte ptr [esi]
		mov		byte ptr [edi], al
		sub		esi, 1
		add		edi, 1
		loop	LeadingLoop
		add		esi, 5

StartDWordLoop:
		mov		ecx, dword ptr [numBytes]
		sub		ecx, ebx			// Don't copy what's already been copied

		mov		ebx, ecx
		and		ebx, 3			// ebx = number of trailing bytes

		shr		ecx, 2			// ecx = number of dwords

		cmp		ecx, 0			// If there's nothing to do, don't do it
		jz		StartTrailingLoop

		// Copies from source to destination, bswap-ing first
DWordLoop:
		mov		eax, dword ptr [esi]
		bswap	eax
		mov		dword ptr [edi], eax
		add		esi, 4
		add		edi, 4
		loop	DWordLoop

StartTrailingLoop:
		cmp		ebx, 0
		jz		Done
		mov		ecx, ebx
		add		esi, 3

TrailingLoop:
		mov		al, byte ptr [esi]
		mov		byte ptr [esi], al
		sub		esi, 1
		add		edi, 1
		loop	TrailingLoop
Done:
	}
}*/

static inline void UnswapCopy( void *src, void *dest, u32 numBytes )
{
#ifndef __LINUX__
	__asm
	{
		mov		ecx, 0
		mov		esi, dword ptr [src]
		mov		edi, dword ptr [dest]

		mov		ebx, esi
		and		ebx, 3			// ebx = number of leading bytes

		cmp		ebx, 0
 		jz		StartDWordLoop
		neg		ebx
		add		ebx, 4

		cmp		ebx, [numBytes]
		jle		NotGreater
		mov		ebx, [numBytes]
NotGreater:
		mov		ecx, ebx
		xor		esi, 3
LeadingLoop:				// Copies leading bytes, in reverse order (un-swaps)
		mov		al, byte ptr [esi]
		mov		byte ptr [edi], al
		sub		esi, 1
		add		edi, 1
		loop	LeadingLoop
		add		esi, 5

StartDWordLoop:
		mov		ecx, dword ptr [numBytes]
		sub		ecx, ebx		// Don't copy what's already been copied

		mov		ebx, ecx
		and		ebx, 3
//		add		ecx, 3			// Round up to nearest dword
		shr		ecx, 2

		cmp		ecx, 0			// If there's nothing to do, don't do it
		jle		StartTrailingLoop

		// Copies from source to destination, bswap-ing first
DWordLoop:
		mov		eax, dword ptr [esi]
		bswap	eax
		mov		dword ptr [edi], eax
		add		esi, 4
		add		edi, 4
		loop	DWordLoop
StartTrailingLoop:
		cmp		ebx, 0
		jz		Done
		mov		ecx, ebx
		xor		esi, 3

TrailingLoop:
		mov		al, byte ptr [esi]
		mov		byte ptr [edi], al
		sub		esi, 1
		add		edi, 1
		loop	TrailingLoop
Done:
	}
#else // !__LINUX__
# if 0//def X86_ASM
	// ok, breaks with optimization enabled
	__asm__ __volatile__(
	".intel_syntax noprefix"			"\n\t"
	"	int		3"						"\n\t"
	"	push	ebx"					"\n\t"
	"	mov		ebx, esi"				"\n\t"
	"	and		ebx, 3"					"\n\t"			// ebx = number of leading bytes

	"	cmp		ebx, 0"					"\n\t"
 	"	jz		2f"						"\n\t"
	"	neg		ebx"					"\n\t"
	"	add		ebx, 4"					"\n\t"

	"	cmp		ebx, edx"				"\n\t"
	"	jle		0f"						"\n\t"
	"	mov		ebx, edx"				"\n\t"
	"0:"								"\n\t"
	"	mov		ecx, ebx"				"\n\t"
	"	xor		esi, 3"					"\n\t"
	"1:"								"\n\t"				// Copies leading bytes, in reverse order (un-swaps)
	"	mov		al, byte ptr [esi]"		"\n\t"
	"	mov		byte ptr [edi], al"		"\n\t"
	"	sub		esi, 1"					"\n\t"
	"	add		edi, 1"					"\n\t"
	"	loop	1b"						"\n\t"
	"	add		esi, 5"					"\n\t"

	"2:"								"\n\t"
	"	mov		ecx, edx"				"\n\t"
	"	sub		ecx, ebx"				"\n\t"		// Don't copy what's already been copied

	"	mov		ebx, ecx"				"\n\t"
	"	and		ebx, 3"					"\n\t"
//	"	add		ecx, 3"					"\n\t"			// Round up to nearest dword
	"	shr		ecx, 2"					"\n\t"

	"	cmp		ecx, 0"					"\n\t"			// If there's nothing to do, don't do it
	"	jle		4f"						"\n\t"

		// Copies from source to destination, bswap-ing first
	"3:"								"\n\t"
	"	mov		eax, dword ptr [esi]"	"\n\t"
	"	bswap	eax"					"\n\t"
	"	mov		dword ptr [edi], eax"	"\n\t"
	"	add		esi, 4"					"\n\t"
	"	add		edi, 4"					"\n\t"
	"	loop	3b"						"\n\t"
	"4:"								"\n\t"
	"	cmp		ebx, 0"					"\n\t"
	"	jz		6f"						"\n\t"
	"	mov		ecx, ebx"				"\n\t"
	"	xor		esi, 3"					"\n\t"

	"5:"								"\n\t"
	"	mov		al, byte ptr [esi]"		"\n\t"
	"	mov		byte ptr [edi], al"		"\n\t"
	"	sub		esi, 1"					"\n\t"
	"	add		edi, 1"					"\n\t"
	"	loop	5b"						"\n\t"
	"6:"								"\n\t"
	"	pop		ebx"					"\n\t"
	".att_syntax prefix"				"\n\t"
	: /* no output */
	: "S"(src), "D"(dest), "d"(numBytes), "c"(0)
	: "memory", "%eax" );
# else // X86_ASM
	// ok
	// copy leading bytes
	int leadingBytes = ((int)src) & 3;
	if (leadingBytes != 0)
	{
		leadingBytes = 4-leadingBytes;
		if (leadingBytes > numBytes)
			leadingBytes = numBytes;
		numBytes -= leadingBytes;

		src = (void *)((int)src ^ 3);
		for (int i = 0; i < leadingBytes; i++)
		{
			*(u8 *)(dest) = *(u8 *)(src);
			dest = (void *)((int)dest+1);
			src  = (void *)((int)src -1);
		}
		src = (void *)((int)src+5);
	}

	// copy dwords
	int numDWords = numBytes >> 2;
	while (numDWords--)
	{
		u32 dword = *(u32 *)src;
		__asm__ volatile( "bswapl %0\n\t" : "=q"(dword) : "0"(dword) );
		*(u32 *)dest = dword;
		dest = (void *)((int)dest+4);
		src  = (void *)((int)src +4);
	}

	// copy trailing bytes
	int trailingBytes = numBytes & 3;
	if (trailingBytes)
	{
		src = (void *)((int)src ^ 3);
		for (int i = 0; i < trailingBytes; i++)
		{
			*(u8 *)(dest) = *(u8 *)(src);
			dest = (void *)((int)dest+1);
			src  = (void *)((int)src -1);
		}
	}
# endif // !X86_ASM
#endif // __LINUX__
}

static inline void DWordInterleave( void *mem, u32 numDWords )
{
#ifndef __LINUX__
	__asm {
		mov		esi, dword ptr [mem]
		mov		edi, dword ptr [mem]
		add		edi, 4
		mov		ecx, dword ptr [numDWords]
DWordInterleaveLoop:
		mov		eax, dword ptr [esi]
		mov		ebx, dword ptr [edi]
		mov		dword ptr [esi], ebx
		mov		dword ptr [edi], eax
		add		esi, 8
		add		edi, 8
		loop	DWordInterleaveLoop
	}
#else // !__LINUX__
# ifdef X86_ASM
	// ok
	__asm__ volatile(
	".intel_syntax noprefix"			"\n\t"
	"	add		edi, 4"					"\n\t"
	"0:"								"\n\t"
	"	mov		eax, dword ptr [esi]"	"\n\t"
	"	mov		edx, dword ptr [edi]"	"\n\t"
	"	mov		dword ptr [esi], edx"	"\n\t"
	"	mov		dword ptr [edi], eax"	"\n\t"
	"	add		esi, 8"					"\n\t"
	"	add		edi, 8"					"\n\t"
	"	loop	0b"						"\n\t"
	".att_syntax prefix"				"\n\t"
	: /* no output */
	: "S"(mem), "D"(mem), "c"(numDWords)
	: "memory", "%eax", "%edx" );
# else
	// ok
	int tmp;
	while( numDWords-- )
	{
		tmp = *(int *)((int)mem + 0);
		*(int *)((int)mem + 0) = *(int *)((int)mem + 4);
		*(int *)((int)mem + 4) = tmp;

		mem = (void *)((int)mem + 8);
	}
# endif
#endif // __LINUX__
}

inline void QWordInterleave( void *mem, u32 numDWords )
{
#ifndef __LINUX__
	__asm
	{
	// Interleave the line on the qword
		mov		esi, dword ptr [mem]
		mov		edi, dword ptr [mem]
		add		edi, 8
		mov		ecx, dword ptr [numDWords]
		shr		ecx, 1
QWordInterleaveLoop:
		mov		eax, dword ptr [esi]
		mov		ebx, dword ptr [edi]
		mov		dword ptr [esi], ebx
		mov		dword ptr [edi], eax
		add		esi, 4
		add		edi, 4
		mov		eax, dword ptr [esi]
		mov		ebx, dword ptr [edi]
		mov		dword ptr [esi], ebx
		mov		dword ptr [edi], eax
		add		esi, 12
		add		edi, 12
		loop	QWordInterleaveLoop
	}
#else // !__LINUX__
# ifdef X86_ASM
	// ok
	__asm__ volatile(
	".intel_syntax noprefix"			"\n\t"
	"	add		edi, 8"					"\n\t"
	"	shr		ecx, 1"					"\n\t"
	"0:"								"\n\t"
	"	mov		eax, dword ptr [esi]"	"\n\t"
	"	mov		edx, dword ptr [edi]"	"\n\t"
	"	mov		dword ptr [esi], edx"	"\n\t"
	"	mov		dword ptr [edi], eax"	"\n\t"
	"	add		esi, 4"					"\n\t"
	"	add		edi, 4"					"\n\t"
	"	mov		eax, dword ptr [esi]"	"\n\t"
	"	mov		edx, dword ptr [edi]"	"\n\t"
	"	mov		dword ptr [esi], edx"	"\n\t"
	"	mov		dword ptr [edi], eax"	"\n\t"
	"	add		esi, 12"				"\n\t"
	"	add		edi, 12"				"\n\t"
	"	loop	0b"						"\n\t"
	".att_syntax prefix"				"\n\t"
	: /* no output */
	: "S"(mem), "D"(mem), "c"(numDWords)
	: "memory", "%eax", "%edx" );
# else
	// ok
	int tmp;
	numDWords >>= 1; // qwords
	while( numDWords-- )
	{
		tmp = *(int *)((int)mem + 0);
		*(int *)((int)mem + 0) = *(int *)((int)mem + 8);
		*(int *)((int)mem + 8) = tmp;

		tmp = *(int *)((int)mem + 4);
		*(int *)((int)mem + 4) = *(int *)((int)mem + 12);
		*(int *)((int)mem + 12) = tmp;

		mem = (void *)((int)mem + 16);
	}
# endif
#endif // __LINUX__
}


inline u32 swapdword( u32 value )
{
#ifndef __LINUX__
	__asm
	{
		mov		eax, dword ptr [value]
		bswap	eax
	}
#else // !__LINUX__
# ifdef X86_ASM
	__asm__ volatile( "bswapl %0" : "=r"(value) : "0"(value) );
	return value;
# else // X86_ASM
	return ((value & 0xff000000) >> 24) |
	       ((value & 0x00ff0000) >>  8) |
	       ((value & 0x0000ff00) <<  8) |
		   ((value & 0x000000ff) << 24);
# endif // !X86_ASM
#endif // __LINUX__
}

inline u16 swapword( u16 value )
{
#ifndef __LINUX__
	__asm
	{
		mov		ax, word ptr [value]
		xchg	ah, al
	}
#else // !__LINUX__
# ifdef X86_ASM
	__asm__ volatile(
		"xchgb	%%al, %%ah"				"\n\t"
	: "=a"(value)
	: "a"(value) );
	return value;
# else // X86_ASM
	return (value << 8) | (value >> 8);
# endif // X86_ASM
#endif // __LINUX__
}

inline u16 RGBA8888_RGBA4444( u32 color )
{
#ifndef __LINUX__
	__asm
	{
		mov		ebx, dword ptr [color]
		// R
		and		bl, 0F0h
		mov		ah, bl

		// G
		shr		bh, 4
		or		ah, bh

		bswap	ebx

		// B
		and		bh, 0F0h
		mov		al, bh

		// A
		shr		bl, 4
		or		al, bl
	}
#else // !__LINUX__
# ifdef X86_ASM
	__asm__ volatile(
	".intel_syntax noprefix"	"\n\t"
	"	and		cl, 0xF0"		"\n\t"
	"	mov		ah, cl"			"\n\t"

	"	shr		ch, 4"			"\n\t"
	"	or		ah, bh"			"\n\t"

	"	bswap	ecx"			"\n\t"

	"	and		ch, 0xF0"		"\n\t"
	"	mov		al, ch"			"\n\t"

	"	shr		cl, 4"			"\n\t"
	"	or		al, cl"			"\n\t"
	".att_syntax prefix"		"\n\t"
	: "=a"(color)
	: "c"(color) );
	return (u16)color;
# else // X86_ASM
/*	return ((color & 0xf0000000) >> 16) |
	       ((color & 0x00f00000) >> 12) |
	       ((color & 0x0000f000) >>  8) |
	       ((color & 0x000000f0) >>  4);*/
	return ((color & 0x000000f0) <<  8) |	// r
	       ((color & 0x0000f000) >>  4) |	// g
	       ((color & 0x00f00000) >> 16) |	// b
	       ((color & 0xf0000000) >> 28);	// a
# endif // !X86_ASM
#endif // __LINUX__
}

inline u32 RGBA5551_RGBA8888( u16 color )
{
#ifndef __LINUX__
	__asm
	{
		mov		ebx, 00000000h
		mov		cx, word ptr [color]
		xchg	cl, ch

		mov		bx, cx
		and		bx, 01h
        mov		al, byte ptr [One2Eight+ebx]

		mov		bx, cx
		shr		bx, 01h
		and		bx, 1Fh
		mov		ah, byte ptr [Five2Eight+ebx]

		bswap	eax

		mov		bx, cx
		shr		bx, 06h
		and		bx, 1Fh
		mov		ah, byte ptr [Five2Eight+ebx]

		mov		bx, cx
		shr		bx, 0Bh
		and		bx, 1Fh
		mov		al, byte ptr [Five2Eight+ebx]
	}
#else // !__LINUX__
# ifdef X86_ASM
	// ok
	u32 ret;
	__asm__ volatile(
	".intel_syntax noprefix"					"\n\t"
	"	push	ebx"							"\n\t"
	"	mov		ebx, 0"							"\n\t"
	"	xchg	cl, ch"							"\n\t"

	"	mov		bx, cx"							"\n\t"
	"	and		bx, 0x01"						"\n\t"
	"	mov		al, byte ptr [One2Eight+ebx]"	"\n\t"

	"	mov		bx, cx"							"\n\t"
	"	shr		bx, 0x01"						"\n\t"
	"	and		bx, 0x1F"						"\n\t"
	"	mov		ah, byte ptr [Five2Eight+ebx]"	"\n\t"

	"	bswap	eax"							"\n\t"

	"	mov		bx, cx"							"\n\t"
	"	shr		bx, 0x06"						"\n\t"
	"	and		bx, 0x1F"						"\n\t"
	"	mov		ah, byte ptr [Five2Eight+ebx]"	"\n\t"

	"	mov		bx, cx"							"\n\t"
	"	shr		bx, 0x0B"						"\n\t"
	"	and		bx, 0x1F"						"\n\t"
	"	mov		al, byte ptr [Five2Eight+ebx]"	"\n\t"
	"	pop		ebx"							"\n\t"
	".att_syntax prefix"						"\n\t"
	: "=a"(ret)
	: "c"(color) );
	return ret;
# else // X86_ASM
	// ok
	color = swapword( color );
	u8 r, g, b, a;
	r = Five2Eight[color >> 11];
	g = Five2Eight[(color >> 6) & 0x001f];
	b = Five2Eight[(color >> 1) & 0x001f];
	a = One2Eight [(color     ) & 0x0001];
	return (a << 24) | (b << 16) | (g << 8) | r;
# endif // !X86_ASM
#endif // __LINUX__
}

// Just swaps the word
inline u16 RGBA5551_RGBA5551( u16 color )
{
#ifndef __LINUX__
	__asm
	{
		mov		ax, word ptr [color]
		xchg	ah, al
	}
#else // !__LINUX__
# ifdef X86ASM
	__asm__ volatile( "xchgb	%%ah, %%al\n\t" : "=a"(color) : "a"(color) );
	return color;
# else
	return swapword( color );
# endif
#endif // __LINUX__
}

inline u32 IA88_RGBA8888( u16 color )
{
#ifndef __LINUX__
	__asm
	{
		mov		cx, word ptr [color]

        mov		al, ch
		mov		ah, cl

		bswap	eax

		mov		ah, cl
		mov		al, cl
	}
#else // !__LINUX__
# ifdef X86_ASM
	// ok
	u32 ret;
	__asm__ volatile(
	".intel_syntax noprefix"	"\n\t"
	"	mov		al, ch"			"\n\t"
	"	mov		ah, cl"			"\n\t"

	"	bswap	eax"			"\n\t"

	"	mov		ah, cl"			"\n\t"
	"	mov		al, cl"			"\n\t"
	".att_syntax prefix"		"\n\t"
	: "=a"(ret)
	: "c"(color) );
	return ret;
# else
	// ok
	u8 a = color >> 8;
	u8 i = color & 0x00FF;
	return (a << 24) | (i << 16) | (i << 8) | i;
# endif
#endif // __LINUX__
}

inline u16 IA88_RGBA4444( u16 color )
{
#ifndef __LINUX__
	__asm
	{
		mov		cx, word ptr [color]

		shr		cl, 4
		mov		ah, cl
		shl		cl, 4
		or		ah, cl
		mov		al, cl

		shr		ch, 4
		or		al, ch
	}
#else // !__LINUX__
# ifdef X86_ASM
	__asm__ volatile(
	".intel_syntax noprefix"	"\n\t"
	"	shr		cl, 4"			"\n\t"
	"	mov		ah, cl"			"\n\t"
	"	shl		cl, 4"			"\n\t"
	"	or		ah, cl"			"\n\t"
	"	mov		al, cl"			"\n\t"

	"	shr		ch, 4"			"\n\t"
	"	or		al, ch"			"\n\t"
	".att_syntax prefix"		"\n\t"
	: "=a"(color)
	: "c"(color) );
	return color;
# else // X86_ASM
	u8 i = color >> 12;
	u8 a = (color >> 4) & 0x000F;
	return (i << 12) | (i << 8) | (i << 4) | a;
# endif // !X86_ASM
#endif // __LINUX__
}

inline u16 IA44_RGBA4444( u8 color )
{
#ifndef __LINUX__
	__asm
	{
		mov		cl, byte ptr [color]
		mov		al, cl

		shr		cl, 4
		mov		ah, cl
		shl		cl, 4
		or		ah, cl
	}
#else // !__LINUX__
# ifdef X86_ASM
	u16 ret;
	__asm__ volatile(
	".intel_syntax noprefix"	"\n\t"
	"	mov		al, cl"			"\n\t"

	"	shr		cl, 4"			"\n\t"
	"	mov		ah, cl"			"\n\t"
	"	shl		cl, 4"			"\n\t"
	"	or		ah, cl"			"\n\t"
	".att_syntax prefix"		"\n\t"
	: "=a"(ret)
	: "c"(color) );
	return ret;
# else // X86_ASM
	return ((color & 0xf0) << 8) | ((color & 0xf0) << 4) | (color);
# endif // !X86_ASM
#endif // __LINUX__
}

inline u32 IA44_RGBA8888( u8 color )
{
#ifndef __LINUX__
	__asm
	{
		mov		ebx, 00000000h
		mov		cl, byte ptr [color]

		mov		bl, cl
		shr		bl, 04h
		mov		ch, byte ptr [Four2Eight+ebx]

		mov		bl, cl
		and		bl, 0Fh
		mov		cl, byte ptr [Four2Eight+ebx]

        mov		al, cl
		mov		ah, ch

		bswap	eax

		mov		ah, ch
		mov		al, ch
	}
#else // !__LINUX__
# ifdef X86_ASM
	// ok
	u32 ret;
	__asm__ volatile(
	".intel_syntax noprefix"					"\n\t"
	"	push	ebx"							"\n\t"
	"	mov		ebx, 0"							"\n\t"
	"	mov		bl, cl"							"\n\t"
	"	shr		bl, 0x04"						"\n\t"
	"	mov		ch, byte ptr [Four2Eight+ebx]"	"\n\t"

	"	mov		bl, cl"							"\n\t"
	"	and		bl, 0x0F"						"\n\t"
	"	mov		cl, byte ptr [Four2Eight+ebx]"	"\n\t"

	"	mov		al, cl"							"\n\t"
	"	mov		ah, ch"							"\n\t"

	"	bswap	eax"							"\n\t"

	"	mov		ah, ch"							"\n\t"
	"	mov		al, ch"							"\n\t"
	"	pop		ebx"							"\n\t"
	".att_syntax prefix"						"\n\t"
	: "=a"(ret)
	: "c"(color) );
	return ret;
# else // X86_ASM
	// ok
	u8 i = Four2Eight[color >> 4];
	u8 a = Four2Eight[color & 0x0F];
	return (a << 24) | (i << 16) | (i << 8) | i;
# endif // !X86_ASM
#endif // __LINUX__
}

inline u16 IA31_RGBA4444( u8 color )
{
#ifndef __LINUX__
	__asm
	{
		mov		ebx, 00000000h
		mov		cl, byte ptr [color]

		mov		bl, cl
		shr		bl, 01h
		mov		ch, byte ptr [Three2Four+ebx]
		mov		ah, ch
		shl		ch, 4
		or		ah, ch
		mov		al, ch

		mov		bl, cl
		and		bl, 01h
		mov		ch, byte ptr [One2Four+ebx]
		or		al, ch
	}
#else // !__LINUX__
# ifdef X86_ASM
	u16 ret;
	__asm__ volatile(
	".intel_syntax noprefix"					"\n\t"
	"	push	ebx"							"\n\t"
	"	mov		ebx, 0"							"\n\t"
	"	mov		bl, cl"							"\n\t"
	"	shr		bl, 0x01"						"\n\t"
	"	mov		ch, byte ptr [Three2Four+ebx]"	"\n\t"
	"	mov		ah, ch"							"\n\t"
	"	shl		ch, 4"							"\n\t"
	"	or		ah, ch"							"\n\t"
	"	mov		al, ch"							"\n\t"

	"	mov		bl, cl"							"\n\t"
	"	and		bl, 0x01"						"\n\t"
	"	mov		ch, byte ptr [One2Four+ebx]"	"\n\t"
	"	or		al, ch"							"\n\t"
	"	pop		ebx"							"\n\t"
	".att_syntax prefix"						"\n\t"
	: "=a"(ret)
	: "c"(color) );
	return ret;
# else // X86_ASM
	u8 i = Three2Four[color >> 1];
	u8 a = One2Four[color & 0x01];
	return (i << 12) | (i << 8) | (i << 4) | a;
# endif // !X86_ASM
#endif // __LINUX__
}

inline u32 IA31_RGBA8888( u8 color )
{
#ifndef __LINUX__
	__asm
	{
		mov		ebx, 00000000h
		mov		cl, byte ptr [color]

		mov		bl, cl
		shr		bl, 01h
		mov		ch, byte ptr [Three2Eight+ebx]

		mov		bl, cl
		and		bl, 01h
		mov		cl, byte ptr [One2Eight+ebx]

        mov		al, cl
		mov		ah, ch

		bswap	eax

		mov		ah, ch
		mov		al, ch
	}
#else // !__LINUX__
# ifdef X86_ASM
	u32 ret;
	__asm__ volatile(
	".intel_syntax noprefix"					"\n\t"
	"	push	ebx"							"\n\t"
	"	mov		ebx, 0"							"\n\t"
	"	mov		bl, cl"							"\n\t"
	"	shr		bl, 0x01"						"\n\t"
	"	mov		ch, byte ptr [Three2Eight+ebx]"	"\n\t"

	"	mov		bl, cl"							"\n\t"
	"	and		bl, 0x01"						"\n\t"
	"	mov		cl, byte ptr [One2Eight+ebx]"	"\n\t"

	"	mov		al, cl"							"\n\t"
	"	mov		ah, ch"							"\n\t"

	"	bswap	eax"							"\n\t"

	"	mov		ah, ch"							"\n\t"
	"	mov		al, ch"							"\n\t"
	"	pop		ebx"							"\n\t"
	".att_syntax prefix"						"\n\t"
	: "=a"(ret)
	: "c"(color) );
	return ret;
# else // X86_ASM
	u8 i = Three2Eight[color >> 1];
	u8 a = One2Eight[color & 0x01];
	return (i << 24) | (i << 16) | (i << 8) | a;
# endif // !X86_ASM
#endif // __LINUX__
}

inline u16 I8_RGBA4444( u8 color )
{
#ifndef __LINUX__
	__asm
	{
		mov		cl, byte ptr [color]

		shr		cl, 4
		mov		al, cl
		shl		cl, 4
		or		al, cl
		mov		ah, al
	}
#else // !__LINUX__
# ifdef X86_ASM
	u16 ret;
	__asm__ volatile(
	".intel_syntax noprefix"	"\n\t"
	"	shr		cl, 4"			"\n\t"
	"	mov		al, cl"			"\n\t"
	"	shl		cl, 4"			"\n\t"
	"	or		al, cl"			"\n\t"
	"	mov		ah, al"			"\n\t"
	".att_syntax prefix"		"\n\t"
	: "=a"(ret)
	: "c"(color) );
# else // X86_ASM
	u8 c = color >> 4;
	return (c << 12) | (c << 8) | (c << 4) | c;
# endif // !X86_ASM
#endif // __LINUX__
}

inline u32 I8_RGBA8888( u8 color )
{
#ifndef __LINUX__
	__asm
	{
		mov		cl, byte ptr [color]

		mov		al, cl
		mov		ah, cl
		bswap	eax
		mov		ah, cl
		mov		al, cl
	}
#else // !__LINUX__
# ifdef X86_ASM
	u32 ret;
	__asm__ volatile(
	".intel_syntax noprefix"	"\n\t"
	"	mov		al, cl"			"\n\t"
	"	mov		ah, cl"			"\n\t"
	"	bswap	eax"			"\n\t"
	"	mov		ah, cl"			"\n\t"
	"	mov		al, cl"			"\n\t"
	".att_syntax prefix"		"\n\t"
	: "=a"(ret)
	: "c"(color) );
	return ret;
# else // X86_ASM
	return (color << 24) | (color << 16) | (color << 8) | color;
# endif // !X86_ASM
#endif // __LINUX__
}

inline u16 I4_RGBA4444( u8 color )
{
#ifndef __LINUX__
	__asm
	{
		mov		cl, byte ptr [color]
		mov		al, cl
		shl		cl, 4
		or		al, cl
		mov		ah, al
	}
#else // !__LINUX__
# ifdef X86_ASM
	u16 ret;
	__asm__ volatile(
	".intel_syntax noprefix"	"\n\t"
	"	mov		al, cl"			"\n\t"
	"	shl		cl, 4"			"\n\t"
	"	or		al, cl"			"\n\t"
	"	mov		ah, al"			"\n\t"
	".att_syntax prefix"		"\n\t"
	: "=a"(ret)
	: "c"(color) );
	return ret;
# else // X86_ASM
	u16 ret = color & 0x0f;
	ret |= ret << 4;
	ret |= ret << 8;
	return ret;
# endif // !X86_ASM
#endif // __LINUX__
}

inline u32 I4_RGBA8888( u8 color )
{
#ifndef __LINUX__
	__asm
	{
		mov		ebx, 00000000h

		mov		bl, byte ptr [color]
		mov		cl, byte ptr [Four2Eight+ebx]

        mov		al, cl
		mov		ah, cl
		bswap	eax
		mov		ah, cl
		mov		al, cl
	}
#else // !__LINUX__
# ifdef X86_ASM
	// ok
	u32 ret;
	__asm__ volatile(
	".intel_syntax noprefix"					"\n\t"
	"	push	ebx"							"\n\t"

	"	and		ecx, 0x0f"						"\n\t"
	"	mov		ebx, ecx"						"\n\t"
	"	shl		ebx, 4"							"\n\t"
	"	or		ecx, ebx"						"\n\t"

//	"	mov		cl, byte ptr [Four2Eight+ebx]"	"\n\t" // this doesn't work for some reason

	"	mov		al, cl"							"\n\t"
	"	mov		ah, cl"							"\n\t"
	"	bswap	eax"							"\n\t"
	"	mov		ah, cl"							"\n\t"
	"	mov		al, cl"							"\n\t"
	"	pop		ebx"							"\n\t"
	".att_syntax prefix"						"\n\t"
	: "=a"(ret)
	: "c"(color) );
	return ret;
# else // X86_ASM
	// ok
	u8 c = Four2Eight[color];
	c |= c << 4;
	return (c << 24) | (c << 16) | (c << 8) | c;
# endif // !X86_ASM
#endif // __LINUX__
}

#ifdef X86_ASM_DEFINED
# define X86_ASM
# undef X86_ASM_DEFINED
#endif

#endif // CONVERT_H
