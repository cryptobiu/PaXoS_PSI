/* This file is part of the gf2x library.

   Copyright 2007, 2008, 2009, 2010, 2011, 2012, 2013
   Richard Brent, Pierrick Gaudry, Emmanuel Thome', Paul Zimmermann

   This program is free software; you can redistribute it and/or modify it
   under the terms of either:
    - If the archive contains a file named toom-gpl.c (not a trivial
    placeholder), the GNU General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.
    - If the archive contains a file named toom-gpl.c which is a trivial
    placeholder, the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the license text for more details.

   You should have received a copy of the GNU General Public License as
   well as the GNU Lesser General Public License along with this program;
   see the files COPYING and COPYING.LIB.  If not, write to the Free
   Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
   02110-1301, USA.
*/

#ifndef GF2X_MUL1_H_
#define GF2X_MUL1_H_

/* This file was generated automatically with
   ./gen_bb_mul_code 64 5. Don't edit it! */

#include "gf2x.h"
/* All gf2x source files for lowlevel functions must include gf2x-small.h
 * This is mandatory for the tuning mechanism. */
#include "gf2x/gf2x-small.h"

GF2X_STORAGE_CLASS_mul1 void
gf2x_mul1 (unsigned long *c, unsigned long a, unsigned long b)
{
   unsigned long hi, lo, tmp, A[32];

   A[0] = 0;		A[1] = a;
   A[2] = A[1] << 1;	A[3] = A[2] ^ a;
   A[4] = A[2] << 1;	A[5] = A[4] ^ a;
   A[6] = A[3] << 1;	A[7] = A[6] ^ a;
   A[8] = A[4] << 1;	A[9] = A[8] ^ a;
   A[10] = A[5] << 1;	A[11] = A[10] ^ a;
   A[12] = A[6] << 1;	A[13] = A[12] ^ a;
   A[14] = A[7] << 1;	A[15] = A[14] ^ a;
   A[16] = A[8] << 1;	A[17] = A[16] ^ a;
   A[18] = A[9] << 1;	A[19] = A[18] ^ a;
   A[20] = A[10] << 1;	A[21] = A[20] ^ a;
   A[22] = A[11] << 1;	A[23] = A[22] ^ a;
   A[24] = A[12] << 1;	A[25] = A[24] ^ a;
   A[26] = A[13] << 1;	A[27] = A[26] ^ a;
   A[28] = A[14] << 1;	A[29] = A[28] ^ a;
   A[30] = A[15] << 1;	A[31] = A[30] ^ a;

   lo = A[b >> 60];
   hi = lo >> 54;
   lo = (lo << 10) ^ (A[(b >> 55) & 31] << 5) ^ A[(b >> 50) & 31];
   hi = (hi << 10) | (lo >> 54);
   lo = (lo << 10) ^ (A[(b >> 45) & 31] << 5) ^ A[(b >> 40) & 31];
   hi = (hi << 10) | (lo >> 54);
   lo = (lo << 10) ^ (A[(b >> 35) & 31] << 5) ^ A[(b >> 30) & 31];
   hi = (hi << 10) | (lo >> 54);
   lo = (lo << 10) ^ (A[(b >> 25) & 31] << 5) ^ A[(b >> 20) & 31];
   hi = (hi << 10) | (lo >> 54);
   lo = (lo << 10) ^ (A[(b >> 15) & 31] << 5) ^ A[(b >> 10) & 31];
   hi = (hi << 10) | (lo >> 54);
   lo = (lo << 10) ^ (A[(b >> 5) & 31] << 5) ^ A[b & 31];

   tmp = -((a >> 63) & 1);
   tmp &= ((b & 0xeffbfeffbfeffbfe) >> 1);
   hi = hi ^ tmp;
   tmp = -((a >> 62) & 1);
   tmp &= ((b & 0xcff3fcff3fcff3fc) >> 2);
   hi = hi ^ tmp;
   tmp = -((a >> 61) & 1);
   tmp &= ((b & 0x8fe3f8fe3f8fe3f8) >> 3);
   hi = hi ^ tmp;
   tmp = -((a >> 60) & 1);
   tmp &= ((b & 0xfc3f0fc3f0fc3f0) >> 4);
   hi = hi ^ tmp;
   tmp = -((a >> 59) & 1);
   tmp &= ((b & 0xf83e0f83e0f83e0) >> 5);
   hi = hi ^ tmp;
   tmp = -((a >> 58) & 1);
   tmp &= ((b & 0xf03c0f03c0f03c0) >> 6);
   hi = hi ^ tmp;
   tmp = -((a >> 57) & 1);
   tmp &= ((b & 0xe0380e0380e0380) >> 7);
   hi = hi ^ tmp;
   tmp = -((a >> 56) & 1);
   tmp &= ((b & 0xc0300c0300c0300) >> 8);
   hi = hi ^ tmp;
   tmp = -((a >> 55) & 1);
   tmp &= ((b & 0x802008020080200) >> 9);
   hi = hi ^ tmp;

   c[0] = lo;
   c[1] = hi;
}

GF2X_STORAGE_CLASS_mul_1_n unsigned long
gf2x_mul_1_n (unsigned long *cp, const unsigned long *bp, long sb, unsigned long a)
{
   long i;
   unsigned long carry = 0, b;
   unsigned long hi, lo, tmp, A[32];

   A[0] = 0;		A[1] = a;
   A[2] = A[1] << 1;	A[3] = A[2] ^ a;
   A[4] = A[2] << 1;	A[5] = A[4] ^ a;
   A[6] = A[3] << 1;	A[7] = A[6] ^ a;
   A[8] = A[4] << 1;	A[9] = A[8] ^ a;
   A[10] = A[5] << 1;	A[11] = A[10] ^ a;
   A[12] = A[6] << 1;	A[13] = A[12] ^ a;
   A[14] = A[7] << 1;	A[15] = A[14] ^ a;
   A[16] = A[8] << 1;	A[17] = A[16] ^ a;
   A[18] = A[9] << 1;	A[19] = A[18] ^ a;
   A[20] = A[10] << 1;	A[21] = A[20] ^ a;
   A[22] = A[11] << 1;	A[23] = A[22] ^ a;
   A[24] = A[12] << 1;	A[25] = A[24] ^ a;
   A[26] = A[13] << 1;	A[27] = A[26] ^ a;
   A[28] = A[14] << 1;	A[29] = A[28] ^ a;
   A[30] = A[15] << 1;	A[31] = A[30] ^ a;

   for (i = 0; i < sb; i++)
     {
       b = bp[i];
   lo = A[b >> 60];
   hi = lo >> 54;
   lo = (lo << 10) ^ (A[(b >> 55) & 31] << 5) ^ A[(b >> 50) & 31];
   hi = (hi << 10) | (lo >> 54);
   lo = (lo << 10) ^ (A[(b >> 45) & 31] << 5) ^ A[(b >> 40) & 31];
   hi = (hi << 10) | (lo >> 54);
   lo = (lo << 10) ^ (A[(b >> 35) & 31] << 5) ^ A[(b >> 30) & 31];
   hi = (hi << 10) | (lo >> 54);
   lo = (lo << 10) ^ (A[(b >> 25) & 31] << 5) ^ A[(b >> 20) & 31];
   hi = (hi << 10) | (lo >> 54);
   lo = (lo << 10) ^ (A[(b >> 15) & 31] << 5) ^ A[(b >> 10) & 31];
   hi = (hi << 10) | (lo >> 54);
   lo = (lo << 10) ^ (A[(b >> 5) & 31] << 5) ^ A[b & 31];

   tmp = -((a >> 63) & 1);
   tmp &= ((b & 0xeffbfeffbfeffbfe) >> 1);
   hi = hi ^ tmp;
   tmp = -((a >> 62) & 1);
   tmp &= ((b & 0xcff3fcff3fcff3fc) >> 2);
   hi = hi ^ tmp;
   tmp = -((a >> 61) & 1);
   tmp &= ((b & 0x8fe3f8fe3f8fe3f8) >> 3);
   hi = hi ^ tmp;
   tmp = -((a >> 60) & 1);
   tmp &= ((b & 0xfc3f0fc3f0fc3f0) >> 4);
   hi = hi ^ tmp;
   tmp = -((a >> 59) & 1);
   tmp &= ((b & 0xf83e0f83e0f83e0) >> 5);
   hi = hi ^ tmp;
   tmp = -((a >> 58) & 1);
   tmp &= ((b & 0xf03c0f03c0f03c0) >> 6);
   hi = hi ^ tmp;
   tmp = -((a >> 57) & 1);
   tmp &= ((b & 0xe0380e0380e0380) >> 7);
   hi = hi ^ tmp;
   tmp = -((a >> 56) & 1);
   tmp &= ((b & 0xc0300c0300c0300) >> 8);
   hi = hi ^ tmp;
   tmp = -((a >> 55) & 1);
   tmp &= ((b & 0x802008020080200) >> 9);
   hi = hi ^ tmp;

   cp[i] = carry ^ lo;
   carry = hi;
    }
   return carry;
}

GF2X_STORAGE_CLASS_addmul_1_n unsigned long
gf2x_addmul_1_n (unsigned long *dp, const unsigned long *cp, const unsigned long* bp, long sb,
        unsigned long a)
{
   long i;
   unsigned long carry = 0, b;
   unsigned long hi, lo, tmp, A[32];

   A[0] = 0;		A[1] = a;
   A[2] = A[1] << 1;	A[3] = A[2] ^ a;
   A[4] = A[2] << 1;	A[5] = A[4] ^ a;
   A[6] = A[3] << 1;	A[7] = A[6] ^ a;
   A[8] = A[4] << 1;	A[9] = A[8] ^ a;
   A[10] = A[5] << 1;	A[11] = A[10] ^ a;
   A[12] = A[6] << 1;	A[13] = A[12] ^ a;
   A[14] = A[7] << 1;	A[15] = A[14] ^ a;
   A[16] = A[8] << 1;	A[17] = A[16] ^ a;
   A[18] = A[9] << 1;	A[19] = A[18] ^ a;
   A[20] = A[10] << 1;	A[21] = A[20] ^ a;
   A[22] = A[11] << 1;	A[23] = A[22] ^ a;
   A[24] = A[12] << 1;	A[25] = A[24] ^ a;
   A[26] = A[13] << 1;	A[27] = A[26] ^ a;
   A[28] = A[14] << 1;	A[29] = A[28] ^ a;
   A[30] = A[15] << 1;	A[31] = A[30] ^ a;

   for (i = 0; i < sb; i++)
     {
       b = bp[i];
   lo = A[b >> 60];
   hi = lo >> 54;
   lo = (lo << 10) ^ (A[(b >> 55) & 31] << 5) ^ A[(b >> 50) & 31];
   hi = (hi << 10) | (lo >> 54);
   lo = (lo << 10) ^ (A[(b >> 45) & 31] << 5) ^ A[(b >> 40) & 31];
   hi = (hi << 10) | (lo >> 54);
   lo = (lo << 10) ^ (A[(b >> 35) & 31] << 5) ^ A[(b >> 30) & 31];
   hi = (hi << 10) | (lo >> 54);
   lo = (lo << 10) ^ (A[(b >> 25) & 31] << 5) ^ A[(b >> 20) & 31];
   hi = (hi << 10) | (lo >> 54);
   lo = (lo << 10) ^ (A[(b >> 15) & 31] << 5) ^ A[(b >> 10) & 31];
   hi = (hi << 10) | (lo >> 54);
   lo = (lo << 10) ^ (A[(b >> 5) & 31] << 5) ^ A[b & 31];

   tmp = -((a >> 63) & 1);
   tmp &= ((b & 0xeffbfeffbfeffbfe) >> 1);
   hi = hi ^ tmp;
   tmp = -((a >> 62) & 1);
   tmp &= ((b & 0xcff3fcff3fcff3fc) >> 2);
   hi = hi ^ tmp;
   tmp = -((a >> 61) & 1);
   tmp &= ((b & 0x8fe3f8fe3f8fe3f8) >> 3);
   hi = hi ^ tmp;
   tmp = -((a >> 60) & 1);
   tmp &= ((b & 0xfc3f0fc3f0fc3f0) >> 4);
   hi = hi ^ tmp;
   tmp = -((a >> 59) & 1);
   tmp &= ((b & 0xf83e0f83e0f83e0) >> 5);
   hi = hi ^ tmp;
   tmp = -((a >> 58) & 1);
   tmp &= ((b & 0xf03c0f03c0f03c0) >> 6);
   hi = hi ^ tmp;
   tmp = -((a >> 57) & 1);
   tmp &= ((b & 0xe0380e0380e0380) >> 7);
   hi = hi ^ tmp;
   tmp = -((a >> 56) & 1);
   tmp &= ((b & 0xc0300c0300c0300) >> 8);
   hi = hi ^ tmp;
   tmp = -((a >> 55) & 1);
   tmp &= ((b & 0x802008020080200) >> 9);
   hi = hi ^ tmp;

   dp[i] = cp[i] ^ (carry ^ lo);
   carry = hi;
    }
   return carry;
}

#endif	/* GF2X_MUL1_H_ */
