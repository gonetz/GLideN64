#include "Types.h"
#include "GBI.h"

static inline s16 GetResult1( u32 A, u32 B, u32 C, u32 D, u32 E )
{
	s16 x = 0;
	s16 y = 0;
	s16 r = 0;

	if (A == C) x += 1; else if (B == C) y += 1;
	if (A == D) x += 1; else if (B == D) y += 1;
	if (x <= 1) r += 1; 
	if (y <= 1) r -= 1;

	return r;
}

static inline s16 GetResult2( u32 A, u32 B, u32 C, u32 D, u32 E) 
{
	s16 x = 0; 
	s16 y = 0;
	s16 r = 0;

	if (A == C) x += 1; else if (B == C) y += 1;
	if (A == D) x += 1; else if (B == D) y += 1;
	if (x <= 1) r -= 1; 
	if (y <= 1) r += 1;

	return r;
}


static inline s16 GetResult( u32 A, u32 B, u32 C, u32 D )
{
	s16 x = 0; 
	s16 y = 0;
	s16 r = 0;

	if (A == C) x += 1; else if (B == C) y += 1;
	if (A == D) x += 1; else if (B == D) y += 1;
	if (x <= 1) r += 1; 
	if (y <= 1) r -= 1;

	return r;
}

static inline u16 INTERPOLATE4444( u16 A, u16 B)
{
	if (A != B)
		return	((A & 0xEEEE) >> 1) + 
				((B & 0xEEEE) >> 1) |
				(A & B & 0x1111);
	else
		return A;
}

static inline u16 INTERPOLATE5551( u16 A, u16 B)
{
	if (A != B)
		return	((A & 0xF7BC) >> 1) + 
				((B & 0xF7BC) >> 1) |
				(A & B & 0x0843);
	else
		return A;
}

static inline u32 INTERPOLATE8888( u32 A, u32 B)
{
	if (A != B)
		return	((A & 0xFEFEFEFE) >> 1) + 
				((B & 0xFEFEFEFE) >> 1) |
				(A & B & 0x01010101);
	else
		return A;
}

static inline u16 Q_INTERPOLATE4444( u16 A, u16 B, u16 C, u16 D)
{
	u16 x =	((A & 0xCCCC) >> 2) +
				((B & 0xCCCC) >> 2) +
				((C & 0xCCCC) >> 2) +
				((D & 0xCCCC) >> 2);
	u16 y =	(((A & 0x3333) +
				(B & 0x3333) +
				(C & 0x3333) +
				(D & 0x3333)) >> 2) & 0x3333;
	return x | y;
}

static inline u16 Q_INTERPOLATE5551( u16 A, u16 B, u16 C, u16 D)
{
	u16 x =	((A & 0xE738) >> 2) +
				((B & 0xE738) >> 2) +
				((C & 0xE738) >> 2) +
				((D & 0xE738) >> 2);
	u16 y =	(((A & 0x18C6) +
				(B & 0x18C6) +
				(C & 0x18C6) +
				(D & 0x18C6)) >> 2) & 0x18C6;
	u16 z =	((A & 0x0001) +
		        (B & 0x0001) +
				(C & 0x0001) +
				(D & 0x0001)) > 2 ? 1 : 0;
	return x | y | z;
}

static inline u32 Q_INTERPOLATE8888( u32 A, u32 B, u32 C, u32 D)
{
	u32 x =	((A & 0xFCFCFCFC) >> 2) +
				((B & 0xFCFCFCFC) >> 2) +
				((C & 0xFCFCFCFC) >> 2) +
				((D & 0xFCFCFCFC) >> 2);
	u32 y =	(((A & 0x03030303) +
				(B & 0x03030303) +
				(C & 0x03030303) +
				(D & 0x03030303)) >> 2) & 0x03030303;
	return x | y;
}

void _2xSaI4444( u16 *srcPtr, u16 *destPtr, u16 width, u16 height, s32 clampS, s32 clampT )
{
	u16 destWidth = width << 1;
	u16 destHeight = height << 1;

    u32 colorA, colorB, colorC, colorD,
          colorE, colorF, colorG, colorH,
          colorI, colorJ, colorK, colorL,
          colorM, colorN, colorO, colorP;
	u32 product, product1, product2;

	s16 row0, row1, row2, row3;
	s16 col0, col1, col2, col3;

	for (u16 y = 0; y < height; y++)
	{
		if (y > 0)
			row0 = -width;
		else
			row0 = clampT ? 0 : (height - 1) * width;

		row1 = 0;

		if (y < height - 1)
		{
			row2 = width;

			if (y < height - 2) 
				row3 = width << 1;
			else
				row3 = clampT ? width : -y * width;
		}
		else
		{
			row2 = clampT ? 0 : -y * width;
			row3 = clampT ? 0 : (1 - y) * width;
		}

        for (u16 x = 0; x < width; x++)
        {
			if (x > 0)
				col0 = -1;
			else
				col0 = clampS ? 0 : width - 1;

			col1 = 0;

			if (x < width - 1)
			{
				col2 = 1;

				if (x < width - 2) 
					col3 = 2;
				else
					col3 = clampS ? 1 : -x;
			}
			else
			{
				col2 = clampS ? 0 : -x;
				col3 = clampS ? 0 : 1 - x;
			}

//---------------------------------------
// Map of the pixels:                    I|E F|J
//                                       G|A B|K
//                                       H|C D|L
//                                       M|N O|P
            colorI = *(srcPtr + col0 + row0);
            colorE = *(srcPtr + col1 + row0);
            colorF = *(srcPtr + col2 + row0);
            colorJ = *(srcPtr + col3 + row0);

            colorG = *(srcPtr + col0 + row1);
            colorA = *(srcPtr + col1 + row1);
            colorB = *(srcPtr + col2 + row1);
            colorK = *(srcPtr + col3 + row1);

            colorH = *(srcPtr + col0 + row2);
            colorC = *(srcPtr + col1 + row2);
            colorD = *(srcPtr + col2 + row2);
            colorL = *(srcPtr + col3 + row2);

            colorM = *(srcPtr + col0 + row3);
            colorN = *(srcPtr + col1 + row3);
            colorO = *(srcPtr + col2 + row3);
            colorP = *(srcPtr + col3 + row3);

            if ((colorA == colorD) && (colorB != colorC))
            {
                if ( ((colorA == colorE) && (colorB == colorL)) ||
                    ((colorA == colorC) && (colorA == colorF) && (colorB != colorE) && (colorB == colorJ)) )
                    product = colorA;
                else
                    product = INTERPOLATE4444(colorA, colorB);

                if (((colorA == colorG) && (colorC == colorO)) ||
                    ((colorA == colorB) && (colorA == colorH) && (colorG != colorC) && (colorC == colorM)) )
                    product1 = colorA;
                else
                    product1 = INTERPOLATE4444(colorA, colorC);

                product2 = colorA;
            }
            else if ((colorB == colorC) && (colorA != colorD))
            {
                if (((colorB == colorF) && (colorA == colorH)) ||
                    ((colorB == colorE) && (colorB == colorD) && (colorA != colorF) && (colorA == colorI)) )
                    product = colorB;
                else
                    product = INTERPOLATE4444(colorA, colorB);
 
                if (((colorC == colorH) && (colorA == colorF)) ||
                    ((colorC == colorG) && (colorC == colorD) && (colorA != colorH) && (colorA == colorI)) )
                    product1 = colorC;
                else
                    product1 = INTERPOLATE4444(colorA, colorC);
                product2 = colorB;
            }
            else if ((colorA == colorD) && (colorB == colorC))
            {
                if (colorA == colorB)
                {
                    product = colorA;
                    product1 = colorA;
                    product2 = colorA;
                }
                else
                {
                    s16 r = 0;
                    product1 = INTERPOLATE4444(colorA, colorC);
                    product = INTERPOLATE4444(colorA, colorB);

                    r += GetResult1 (colorA, colorB, colorG, colorE, colorI);
                    r += GetResult2 (colorB, colorA, colorK, colorF, colorJ);
                    r += GetResult2 (colorB, colorA, colorH, colorN, colorM);
                    r += GetResult1 (colorA, colorB, colorL, colorO, colorP);

                    if (r > 0)
                        product2 = colorA;
                    else if (r < 0)
                        product2 = colorB;
                    else
                        product2 = Q_INTERPOLATE4444(colorA, colorB, colorC, colorD);
                }
            }
            else
            {
                product2 = Q_INTERPOLATE4444(colorA, colorB, colorC, colorD);

                if ((colorA == colorC) && (colorA == colorF) && (colorB != colorE) && (colorB == colorJ))
                    product = colorA;
                else if ((colorB == colorE) && (colorB == colorD) && (colorA != colorF) && (colorA == colorI))
                    product = colorB;
                else
                    product = INTERPOLATE4444(colorA, colorB);

                if ((colorA == colorB) && (colorA == colorH) && (colorG != colorC) && (colorC == colorM))
                    product1 = colorA;
                else if ((colorC == colorG) && (colorC == colorD) && (colorA != colorH) && (colorA == colorI))
                    product1 = colorC;
                else
                    product1 = INTERPOLATE4444(colorA, colorC);
            }

			destPtr[0] = colorA;
			destPtr[1] = product;
			destPtr[destWidth] = product1;
			destPtr[destWidth + 1] = product2;

			srcPtr++;
			destPtr += 2;
        }
		destPtr += destWidth;
	}
}

void _2xSaI5551( u16 *srcPtr, u16 *destPtr, u16 width, u16 height, s32 clampS, s32 clampT )
{
	u16 destWidth = width << 1;
	u16 destHeight = height << 1;

    u32 colorA, colorB, colorC, colorD,
          colorE, colorF, colorG, colorH,
          colorI, colorJ, colorK, colorL,
          colorM, colorN, colorO, colorP;
	u32 product, product1, product2;

	s16 row0, row1, row2, row3;
	s16 col0, col1, col2, col3;

	for (u16 y = 0; y < height; y++)
	{
		if (y > 0)
			row0 = -width;
		else
			row0 = clampT ? 0 : (height - 1) * width;

		row1 = 0;

		if (y < height - 1)
		{
			row2 = width;

			if (y < height - 2) 
				row3 = width << 1;
			else
				row3 = clampT ? width : -y * width;
		}
		else
		{
			row2 = clampT ? 0 : -y * width;
			row3 = clampT ? 0 : (1 - y) * width;
		}

        for (u16 x = 0; x < width; x++)
        {
			if (x > 0)
				col0 = -1;
			else
				col0 = clampS ? 0 : width - 1;

			col1 = 0;

			if (x < width - 1)
			{
				col2 = 1;

				if (x < width - 2) 
					col3 = 2;
				else
					col3 = clampS ? 1 : -x;
			}
			else
			{
				col2 = clampS ? 0 : -x;
				col3 = clampS ? 0 : 1 - x;
			}

//---------------------------------------
// Map of the pixels:                    I|E F|J
//                                       G|A B|K
//                                       H|C D|L
//                                       M|N O|P
            colorI = *(srcPtr + col0 + row0);
            colorE = *(srcPtr + col1 + row0);
            colorF = *(srcPtr + col2 + row0);
            colorJ = *(srcPtr + col3 + row0);

            colorG = *(srcPtr + col0 + row1);
            colorA = *(srcPtr + col1 + row1);
            colorB = *(srcPtr + col2 + row1);
            colorK = *(srcPtr + col3 + row1);

            colorH = *(srcPtr + col0 + row2);
            colorC = *(srcPtr + col1 + row2);
            colorD = *(srcPtr + col2 + row2);
            colorL = *(srcPtr + col3 + row2);

            colorM = *(srcPtr + col0 + row3);
            colorN = *(srcPtr + col1 + row3);
            colorO = *(srcPtr + col2 + row3);
            colorP = *(srcPtr + col3 + row3);

            if ((colorA == colorD) && (colorB != colorC))
            {
                if ( ((colorA == colorE) && (colorB == colorL)) ||
                    ((colorA == colorC) && (colorA == colorF) && (colorB != colorE) && (colorB == colorJ)) )
                    product = colorA;
                else
                    product = INTERPOLATE5551(colorA, colorB);

                if (((colorA == colorG) && (colorC == colorO)) ||
                    ((colorA == colorB) && (colorA == colorH) && (colorG != colorC) && (colorC == colorM)) )
                    product1 = colorA;
                else
                    product1 = INTERPOLATE5551(colorA, colorC);

                product2 = colorA;
            }
            else if ((colorB == colorC) && (colorA != colorD))
            {
                if (((colorB == colorF) && (colorA == colorH)) ||
                    ((colorB == colorE) && (colorB == colorD) && (colorA != colorF) && (colorA == colorI)) )
                    product = colorB;
                else
                    product = INTERPOLATE5551(colorA, colorB);
 
                if (((colorC == colorH) && (colorA == colorF)) ||
                    ((colorC == colorG) && (colorC == colorD) && (colorA != colorH) && (colorA == colorI)) )
                    product1 = colorC;
                else
                    product1 = INTERPOLATE5551(colorA, colorC);
                product2 = colorB;
            }
            else if ((colorA == colorD) && (colorB == colorC))
            {
                if (colorA == colorB)
                {
                    product = colorA;
                    product1 = colorA;
                    product2 = colorA;
                }
                else
                {
                    s16 r = 0;
                    product1 = INTERPOLATE5551(colorA, colorC);
                    product = INTERPOLATE5551(colorA, colorB);

                    r += GetResult1 (colorA, colorB, colorG, colorE, colorI);
                    r += GetResult2 (colorB, colorA, colorK, colorF, colorJ);
                    r += GetResult2 (colorB, colorA, colorH, colorN, colorM);
                    r += GetResult1 (colorA, colorB, colorL, colorO, colorP);

                    if (r > 0)
                        product2 = colorA;
                    else if (r < 0)
                        product2 = colorB;
                    else
                        product2 = Q_INTERPOLATE5551(colorA, colorB, colorC, colorD);
                }
            }
            else
            {
                product2 = Q_INTERPOLATE5551(colorA, colorB, colorC, colorD);

                if ((colorA == colorC) && (colorA == colorF) && (colorB != colorE) && (colorB == colorJ))
                    product = colorA;
                else if ((colorB == colorE) && (colorB == colorD) && (colorA != colorF) && (colorA == colorI))
                    product = colorB;
                else
                    product = INTERPOLATE5551(colorA, colorB);

                if ((colorA == colorB) && (colorA == colorH) && (colorG != colorC) && (colorC == colorM))
                    product1 = colorA;
                else if ((colorC == colorG) && (colorC == colorD) && (colorA != colorH) && (colorA == colorI))
                    product1 = colorC;
                else
                    product1 = INTERPOLATE5551(colorA, colorC);
            }

			destPtr[0] = colorA;
			destPtr[1] = product;
			destPtr[destWidth] = product1;
			destPtr[destWidth + 1] = product2;

			srcPtr++;
			destPtr += 2;
        }
		destPtr += destWidth;
	}
}

void _2xSaI8888( u32 *srcPtr, u32 *destPtr, u16 width, u16 height, s32 clampS, s32 clampT )
{
	u16 destWidth = width << 1;
	u16 destHeight = height << 1;

    u32 colorA, colorB, colorC, colorD,
          colorE, colorF, colorG, colorH,
          colorI, colorJ, colorK, colorL,
          colorM, colorN, colorO, colorP;
	u32 product, product1, product2;

	s16 row0, row1, row2, row3;
	s16 col0, col1, col2, col3;

	for (u16 y = 0; y < height; y++)
	{
		if (y > 0)
			row0 = -width;
		else
			row0 = clampT ? 0 : (height - 1) * width;

		row1 = 0;

		if (y < height - 1)
		{
			row2 = width;

			if (y < height - 2) 
				row3 = width << 1;
			else
				row3 = clampT ? width : -y * width;
		}
		else
		{
			row2 = clampT ? 0 : -y * width;
			row3 = clampT ? 0 : (1 - y) * width;
		}

        for (u16 x = 0; x < width; x++)
        {
			if (x > 0)
				col0 = -1;
			else
				col0 = clampS ? 0 : width - 1;

			col1 = 0;

			if (x < width - 1)
			{
				col2 = 1;

				if (x < width - 2) 
					col3 = 2;
				else
					col3 = clampS ? 1 : -x;
			}
			else
			{
				col2 = clampS ? 0 : -x;
				col3 = clampS ? 0 : 1 - x;
			}

//---------------------------------------
// Map of the pixels:                    I|E F|J
//                                       G|A B|K
//                                       H|C D|L
//                                       M|N O|P
            colorI = *(srcPtr + col0 + row0);
            colorE = *(srcPtr + col1 + row0);
            colorF = *(srcPtr + col2 + row0);
            colorJ = *(srcPtr + col3 + row0);

            colorG = *(srcPtr + col0 + row1);
            colorA = *(srcPtr + col1 + row1);
            colorB = *(srcPtr + col2 + row1);
            colorK = *(srcPtr + col3 + row1);

            colorH = *(srcPtr + col0 + row2);
            colorC = *(srcPtr + col1 + row2);
            colorD = *(srcPtr + col2 + row2);
            colorL = *(srcPtr + col3 + row2);

            colorM = *(srcPtr + col0 + row3);
            colorN = *(srcPtr + col1 + row3);
            colorO = *(srcPtr + col2 + row3);
            colorP = *(srcPtr + col3 + row3);

            if ((colorA == colorD) && (colorB != colorC))
            {
                if ( ((colorA == colorE) && (colorB == colorL)) ||
                    ((colorA == colorC) && (colorA == colorF) && (colorB != colorE) && (colorB == colorJ)) )
                    product = colorA;
                else
                    product = INTERPOLATE8888(colorA, colorB);

                if (((colorA == colorG) && (colorC == colorO)) ||
                    ((colorA == colorB) && (colorA == colorH) && (colorG != colorC) && (colorC == colorM)) )
                    product1 = colorA;
                else
                    product1 = INTERPOLATE8888(colorA, colorC);

                product2 = colorA;
            }
            else if ((colorB == colorC) && (colorA != colorD))
            {
                if (((colorB == colorF) && (colorA == colorH)) ||
                    ((colorB == colorE) && (colorB == colorD) && (colorA != colorF) && (colorA == colorI)) )
                    product = colorB;
                else
                    product = INTERPOLATE8888(colorA, colorB);
 
                if (((colorC == colorH) && (colorA == colorF)) ||
                    ((colorC == colorG) && (colorC == colorD) && (colorA != colorH) && (colorA == colorI)) )
                    product1 = colorC;
                else
                    product1 = INTERPOLATE8888(colorA, colorC);
                product2 = colorB;
            }
            else if ((colorA == colorD) && (colorB == colorC))
            {
                if (colorA == colorB)
                {
                    product = colorA;
                    product1 = colorA;
                    product2 = colorA;
                }
                else
                {
                    s16 r = 0;
                    product1 = INTERPOLATE8888(colorA, colorC);
                    product = INTERPOLATE8888(colorA, colorB);

                    r += GetResult1 (colorA, colorB, colorG, colorE, colorI);
                    r += GetResult2 (colorB, colorA, colorK, colorF, colorJ);
                    r += GetResult2 (colorB, colorA, colorH, colorN, colorM);
                    r += GetResult1 (colorA, colorB, colorL, colorO, colorP);

                    if (r > 0)
                        product2 = colorA;
                    else if (r < 0)
                        product2 = colorB;
                    else
                        product2 = Q_INTERPOLATE8888(colorA, colorB, colorC, colorD);
                }
            }
            else
            {
                product2 = Q_INTERPOLATE8888(colorA, colorB, colorC, colorD);

                if ((colorA == colorC) && (colorA == colorF) && (colorB != colorE) && (colorB == colorJ))
                    product = colorA;
                else if ((colorB == colorE) && (colorB == colorD) && (colorA != colorF) && (colorA == colorI))
                    product = colorB;
                else
                    product = INTERPOLATE8888(colorA, colorB);

                if ((colorA == colorB) && (colorA == colorH) && (colorG != colorC) && (colorC == colorM))
                    product1 = colorA;
                else if ((colorC == colorG) && (colorC == colorD) && (colorA != colorH) && (colorA == colorI))
                    product1 = colorC;
                else
                    product1 = INTERPOLATE8888(colorA, colorC);
            }

			destPtr[0] = colorA;
			destPtr[1] = product;
			destPtr[destWidth] = product1;
			destPtr[destWidth + 1] = product2;

			srcPtr++;
			destPtr += 2;
        }
		destPtr += destWidth;
	}
}

