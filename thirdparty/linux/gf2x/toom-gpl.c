/* This file is part of the gf2x library.

   Copyright 2007, 2008, 2009, 2010, 2013, 2015
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

/* General Toom_Cook multiplication, calls KarMul, Toom3Mul, Toom3WMul
   or Toom4Mul depending on which is expected to be the fastest. */

#include <limits.h>
#include <string.h>

/* from https://www.gnu.org/software/autoconf/manual/autoconf-2.60/html_node/Particular-Functions.html */
#if HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
# define alloca __builtin_alloca
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
void *alloca (size_t);
#endif

#include "gf2x.h"
#include "gf2x/gf2x-impl.h"

/* We need gf2x_addmul_1_n */
#include "gf2x/gf2x-small.h"

const char * gf2x_toom_gpl_status="GPL-licensed GF2X";

/* the following routines come from the irred-ntl package from Paul Zimmermann,
   (http://webloria.loria.fr/~zimmerma/irred/), who contributes them under
   LGPL for gf2x */

/* c <- a + b */
static
void Add (unsigned long *c, const unsigned long *a, const unsigned long *b,
          long n)
{
    long i;
    for (i = 0; i < n; i++)
	c[i] = a[i] ^ b[i];
}

/* {d,n} <- {a,n} + {b,n} + {c,k} with k <= n */
static void
Add2 (unsigned long *d, const unsigned long *a, const unsigned long *b,
      long n, const unsigned long *c, long k)
{
  long i;
  for (i = 0; i < k; i++)
    d[i] = a[i] ^ b[i] ^ c[i];
  for (; i < n; i++)
    d[i] = a[i] ^ b[i];
}

/* c <- c + a + b */
static
void Add3(unsigned long *c, const unsigned long *a, const unsigned long *b,
	  long n)
{
    long i;
    for (i = 0; i < n; i++)
	c[i] ^= a[i] ^ b[i];
}

/* c <- a + x * b, return carry out.
   Warning: c might overlap with b. */
static
unsigned long AddLsh1(unsigned long *c, const unsigned long *a,
		      const unsigned long *b, long n)
{
    unsigned long cy = 0UL, t;
    long i;
    for (i = 0; i < n; i++) {
	t = a[i] ^ ((b[i] << 1) | cy);
	cy = b[i] >> (GF2X_WORDSIZE - 1);
	c[i] = t;
    }
    return cy;
}

/* c <- x * {a, n} + x^2 * {b, r} with r <= n, return carry out.
   c should not overlap with a nor b. */
static
unsigned long AddLsh12a (unsigned long *c, const unsigned long *a, long n,
                         const unsigned long *b, long r)
{
    unsigned long cy = 0UL;
    long i;
    for (i = 0; i < r; i++) {
      c[i] = (a[i] << 1) ^ (b[i] << 2) ^ cy;
      cy = a[i] >> (GF2X_WORDSIZE - 1) ^ b[i] >> (GF2X_WORDSIZE - 2);
    }
    for (; i < n; i++)
      {
        c[i] = (a[i] << 1) ^ cy;
        cy = a[i] >> (GF2X_WORDSIZE - 1);
      }
    return cy;
}

/* c <- a + cy, return carry out (0 for n > 0, cy for n=0) */
static
unsigned long Add1(unsigned long *c, const unsigned long *a, long n,
		   unsigned long cy)
{
    if (n) {
	long i;
	c[0] = a[0] ^ cy;
	for (i = 1; i < n; i++)
	    c[i] = a[i];
	return 0;
    } else
	return cy;
}

/* let c = q*(1+x^2) + X^n*r with X = x^GF2X_WORDSIZE and deg(r) < 2
   then c <- q, returns r.
   (Algorithm from Michel Quercia.)
*/
static unsigned long DivOnePlusX2(unsigned long * c, long n)
{
    unsigned long t = 0;
    long i;
#if (GF2X_WORDSIZE == 64)
    /* mask[t] = t * (1 + x^2 + x^4 + ... + x^(GF2X_WORDSIZE-2)) */
    unsigned long mask[4] = {0UL, 0x5555555555555555UL,
                             0xAAAAAAAAAAAAAAAAUL, 0xFFFFFFFFFFFFFFFFUL};
#elif (GF2X_WORDSIZE == 32)
    unsigned long mask[4] = {0UL, 0x55555555UL, 0xAAAAAAAAUL, 0xFFFFFFFFUL};
#else
#error "GF2X_WORDSIZE should be 32 or 64"
#endif

    for (i = 0; i < n; i++) {
        /* invariant: t < 4 */
#ifndef GF2X_HAVE_PCLMUL_SUPPORT
        unsigned long u;
        /* u <- c[i] * (1 + x^2 + x^4 + ... + x^(GF2X_WORDSIZE-2)) */
        u = c[i] ^ (c[i] << 2);
	u ^= u << 4;
	u ^= u << 8;
	u ^= u << 16;
#if (GF2X_WORDSIZE == 64)
	u ^= u << 32;
#endif
	c[i] = u ^ mask[t];
#else /* use pclmul */
        unsigned long cc[2];
        gf2x_mul1 (cc, c[i], 0x5555555555555555UL);
        c[i] = cc[0] ^ mask[t];
#endif
	t = c[i] >> (GF2X_WORDSIZE - 2);
    }
    return t;
}



/********************************************************************
 * Below this line, experimental code
 * (C) 2007 Marco Bodrato <optimaltoom@bodrato.it>
 * This code is released under the GPL 2.0 license, or any later version.
 * Modified by Paul Zimmermann, April 2007.
 *
 * Reference: http://bodrato.it/papers/#WAIFI2007
 *
 * "Towards Optimal Toom-Cook Multiplication for Univariate and
 * Multivariate Polynomials in Characteristic 2 and 0." by Marco
 * BODRATO; in C.Carlet and B.Sunar, editors, "WAIFI'07 proceedings",
 * LNCS 4547, pp. 119-136. Springer, Madrid, Spain, June 21-22, 2007.
 */

#if (GF2X_MUL_TOOM_THRESHOLD < 17)
#error "GF2X_MUL_TOOM_THRESHOLD should be at least 17"
#endif

/* c <- ( c + b )/x, return carry */
static
unsigned long Rsh1Add(unsigned long *c, const unsigned long *b, long n)
{
    unsigned long cy = 0, t;

    long i;

    for (i = n - 1; i >= 0; i--) {
	t = c[i] ^ b[i];
	cy <<= GF2X_WORDSIZE - 1;
	c[i] = (t >> 1) | cy;
	cy = t;
    }
    return cy;
}

/* c <- ( c + b )/x + a, return carry */
static unsigned long
Rsh1Adda (unsigned long *c, const unsigned long *b, const unsigned long *a,
          long n)
{
    unsigned long cy = 0, t;

    long i;

    for (i = n - 1; i >= 0; i--) {
        t = c[i] ^ b[i];
	cy <<= GF2X_WORDSIZE - 1;
	c[i] = (t >> 1) ^ cy ^ a[i];
	cy = t;
    }
    return cy;
}

/* c <- c + (1+x^3) * b, return carry out */
static
unsigned long AddLsh13(unsigned long *c, const unsigned long *b, long n)
{
    unsigned long cy = 0UL, t;

    long i;

    for (i = 0; i < n; i++) {
	t = b[i];
	c[i] ^= t ^ (t << 3) ^ cy;
	cy = t >> (GF2X_WORDSIZE - 3);
    }
    return cy;
}

/* c <- c + a + b + d */
static
void Add4(unsigned long *c, const unsigned long *a, const unsigned long *b,
	  const unsigned long *d, long n)
{
    long i;
    for (i = 0; i < n; i++)
	c[i] ^= a[i] ^ b[i] ^ d[i];
}

/* let c = q*(1+x) + X^n*r with X = x^GF2X_WORDSIZE and deg(r) < 1
   then c <- q, returns r.
   (Algorithm from Michel Quercia.)
*/
static
unsigned long DivOnePlusX(unsigned long *c, long n)
{
    unsigned long t = 0;
    long i;
#if (GF2X_WORDSIZE == 64)
    /* mask[t] = t * (1 + x + x^2 + ... + x^(GF2X_WORDSIZE-1)) */
    unsigned long mask[2] = {0UL, 0xFFFFFFFFFFFFFFFFUL};
#elif (GF2X_WORDSIZE == 32)
    unsigned long mask[2] = {0UL, 0xFFFFFFFFUL};
#else
#error "GF2X_WORDSIZE should be 32 or 64"
#endif

    for (i = 0; i < n; i++) {
        /* invariant: t < 2 */
#ifndef GF2X_HAVE_PCLMUL_SUPPORT
        unsigned long u;
        /* u <- c[i] * (1 + x + x^2 + ... + x^(GF2X_WORDSIZE-1)) */
        u = c[i] ^ (c[i] << 1);
	u ^= u << 2;
	u ^= u << 4;
	u ^= u << 8;
	u ^= u << 16;
#if (GF2X_WORDSIZE == 64)
	u ^= u << 32;
#endif
        c[i] = u ^ mask[t];
#else /* use pclmul */
        unsigned long cc[2];
        gf2x_mul1 (cc, c[i], 0xFFFFFFFFFFFFFFFFUL);
        c[i] = cc[0] ^ mask[t];
#endif
	t = c[i] >> (GF2X_WORDSIZE - 1);
    }
    return t;
}

#if (defined(DEBUG))
static void dump(const unsigned long *a, long n)
{
    long i;
    for (i = 0; i < n; i++) {
	printf("+%lu*X^%lu", a[i], i);
	if ((i + 1) % 3 == 0)
	    printf("\n");
    }
    printf(":\n");
}
#endif

/*
\\ gp-pari check code.
default(echo, 1);

A = (a2*x^2 + a1*x + a0)*Mod(1,2)
B = (b2*x^2 + b1*x + b0)*Mod(1,2)
C = A * B
c0 = polcoeff(C, 0)
c1 = polcoeff(C, 1)
c2 = polcoeff(C, 2)
c3 = polcoeff(C, 3)
c4 = polcoeff(C, 4)

\\ --- Evaluation phase. 10 add, 4 shift, 5 mul.

W0 = (a2*y^2+a1*y)*Mod(1,2)
W4 = (b2*y^2+b1*y)*Mod(1,2)
W3 = (a2+a1+a0)   *Mod(1,2)
W2 = (b2+b1+b0)   *Mod(1,2)
W1 = W2 * W3                \\ C(1)
W3 = W3 + W0
W2 = W2 + W4
W0 = W0+a0        *Mod(1,2)
W4 = W4+b0        *Mod(1,2)
W3 = W2 * W3                \\ C(y+1)
W2 = W0 * W4                \\ C(y)
W4 = a2 * b2      *Mod(1,2) \\ C(\infty)
W0 = a0 * b0      *Mod(1,2) \\ C(0)

\\ ------ Interpolation phase. 10 add, 2 shift, 2 div.
W3 = W3 + W2
W3 == ( c4 + (y^2+y+1)*c3 + c2 + c1 )  \\ check

W2 = ( ( W2 + W0 )/y  + W3 + W4*(y^3+1) ) / (y+1)
\\W2 = ( W2 + W0 + W3*y + W4*(y^4+y) ) / (y^2+y)
W2 == ( c2 + c3 )  \\ check

W1 = W1 + W0
W1 == ( c4 + c3 + c2 + c1 )  \\ check

W3 = ( W3 + W1 ) / (y*(y+1))
W3 == ( c3 )  \\ check

W1 = W1 + W4 + W2
W1 == ( c1 )  \\ check

W2 = W2 + W3
W2 == ( c2 )  \\ check

C == W4*x^4+ W3*x^3+ W2*x^2+ W1*x + W0 \\ check

quit;

 */

/*
  c must have space for 2n words.
  stk must have space for max( KarMem(n), 5k+2 + ToomMem(k+1))
  where k = ceil(n/3).
 */

void gf2x_mul_tc3(unsigned long *c, const unsigned long *a,
	     const unsigned long *b, long n, unsigned long *stk)
{
    long k = (n + 2) / 3;	/* ceil(n/3) */
    long r = n - 2 * k;
    unsigned long cy;
    unsigned long *W0 = c;
    unsigned long *W1 = stk;
    unsigned long *W2 = c + 2 * k;
    unsigned long *W3 = stk + 2 * k;
    unsigned long *W4 = c + 4 * k;

    assert(c != a);
    assert(c != b);

/* \\ --- Evaluation phase. 10 add, 4 shift, 5 mul. */

/* W0 = (a2*y^2+a1*y) */
/* W4 = (b2*y^2+b1*y) */
/*    W0 = (a2*y+a1)*y */
/*    W4 = (b2*y+b1)*y */
    W0[k] = AddLsh12a (W0, a + k, k, a + 2 * k, r); /* x * a1 + x^2 * a2 */
    W4[2 + k] = AddLsh12a (W4 + 2, b + k, k, b + 2 * k, r); /* x * b1 + x^2 b2 */

    /* using W4[2+k] requires that k+3 words are available at W4=c+4k.
       Since c contains 2n=4k+2r words, then W4 contains 2r words, thus
       we need k+3 <= 2r. This is true for n >= 17.
       Also true for n = 9, 12, 14, 15 but timing tests show that
       this is not the fastest routine for such small n. */

    ASSERT(k + 3 <= 2 * r);

    /* {c, k+1}: x*a1+x^2*a2, {c+4k, k+1}: x*b1+x^2*b2 */

/* W3 = ((a2+a1)+a0)    */
/* W2 = ((b2+b1)+b0)    */
    Add2 (c + k + 1, a, a + k, k, a + 2 * k, r); /* a0 + a1 + a2 */
    Add2 (W2 + 2, b, b + k, k, b + 2 * k, r);    /* b0 + b1 + b2 */
/* W1 = W2 * W3                \\ C(1) */

    /* {c, k+1}: x*a1+x^2*a2, {c+k+1, k}: a0+a1+a2, {c+2k+2,k}: b0+b1+b2,
       {c+4k, k+1}: x*b1+x^2*b2 */

    gf2x_mul_toom(W1, c + k + 1, W2 + 2, k, W3);	/* W1 : 2*k */

    /* {c, k+1}: x*a1+x^2*a2, {c+k+1, k}: a0+a1+a2, {c+2k+2,k}: b0+b1+b2,
       {c+4k, k+1}: x*b1+x^2*b2, {stk, 2k}: C(1) */

/* W3 = W3 + W0 */
/* W2 = W2 + W4 */
    Add(c + k + 1, c + k + 1, W0, k);
    c[2 * k + 1] = W0[k];	/* a0 + (x+1)a1 + (x^2+1)a2 */
    Add(W2 + 2, W2 + 2, W4 + 2, k);
    W2[k + 2] = W4[k + 2];	/* b0 + (x+1)b1 + (x^2+1)b2 */
    /* since we use W2[k+2], we need k+3 words in W2, i.e., 3 <= k */
    //  ASSERT (k >= 3);

    /* {c, k+1}: x*a1+x^2*a2, {c+k+1, k+1}: a0+(1+x)*a1+(1+x^2)*a2,
       {c+2k+2,k+1}: b0+(1+x)*b1+(1+x^2)*b2,
       {c+4k, k+1}: x*b1+x^2*b2, {stk, 2k}: C(1) */

/* W0 = W0+a0    */
/* W4 = W4+b0    */
    Add(W0, W0, a, k);		/* a0 + (x)a1 + (x^2)a2 */
    Add(W4 + 2, W4 + 2, b, k);	/* b0 + (x)b1 + (x^2)b2 */

    /* {c, k+1}: a0+x*a1+x^2*a2, {c+k+1, k+1}: a0+(1+x)*a1+(1+x^2)*a2,
       {c+2k+2,k+1}: b0+(1+x)*b1+(1+x^2)*b2,
       {c+4k, k+1}: b0+x*b1+x^2*b2, {stk, 2k}: C(1) */

/* W3 = W2 * W3                \\ C(y+1) */
/* W2 = W0 * W4                \\ C(y) */
    gf2x_mul_toom(W3, W2 + 2, c + k + 1, k + 1, stk + 5 * k + 2);	/* W3 : 2*k+1 */

    /* {c, k+1}: a0+x*a1+x^2*a2, {c+k+1, k+1}: a0+(1+x)*a1+(1+x^2)*a2,
       {c+2k+2,k+1}: b0+(1+x)*b1+(1+x^2)*b2,
       {c+4k, k+1}: b0+x*b1+x^2*b2, {stk, 2k}: C(1), {stk+2k,2k+2}: C(1+x) */

    gf2x_mul_toom(W2, W0, W4 + 2, k + 1, stk + 4 * k + 3);
    cy = W4[0];			/* contains at most 3 bits */
    ASSERT(cy <= 7);

    /* {c, k+1}: a0+x*a1+x^2*a2, {c+2k, 2k+2}: C(x),
       {c+4k, k+1}: b0+x*b1+x^2*b2, {stk, 2k}: C(1), {stk+2k,2k+2}: C(1+x) */

/* W4 = a2 * b2                \\ C(\infty) */
/* W0 = a0 * b0                \\ C(0) */
    gf2x_mul_toom(W0, a, b, k, stk + 4 * k + 3);	/* W0 : 2*k */

    /* {c, 2k}: C(0), {c+2k, 2k+2}: C(x),
       {c+4k, k+1}: b0+x*b1+x^2*b2, {stk, 2k}: C(1), {stk+2k,2k+2}: C(1+x) */

    gf2x_mul_toom(W4, a + 2 * k, b + 2 * k, r, stk + 4 * k + 3);	/* W4 : 2*r */

    /* {c, 2k}: C(0), {c+2k, 2k}+cy: C(x),
       {c+4k, 2r}: C(Inf), {stk, 2k}: C(1), {stk+2k,2k+2}: C(1+x) */

/* \\ ------ Interpolation phase. 10 add, 2 shift, 2 div.  */

/* W3 = W3 + W2: W3 has at most 2k words + 3 bits, W2 has 2k words +
   at most 3 bits (stored in cy). */
    Add(W3, W3, W2, 2 * k);
    W3[2 * k] ^= cy;
    /* now W3 has at most 2k words + 1 bit, which can be non-zero only
       if r = k one most significant bit from a2 and b2 is set. */
    ASSERT(W3[2 * k] <= 1);

    /* {c, 2k}: C(0), {c+2k, 2k}+cy: C(x),
       {c+4k, 2r}: C(Inf), {stk, 2k}: C(1), {stk+2k,2k+2}: C(1+x)+C(x) */

/* W2 = ( ( W2 + W0 )/y  + W3 + W4*(y^3+1) ) / (y+1) */
/* \\W2 = ( W2 + W0 + W3*y + W4*(y^4+y) ) / (y^2+y) */
    /* W2 has 2k words + at most 3 bits (stored in cy), W0 has 2k words */
    Rsh1Adda (W2, W0, W3, 2 * k);
    W2[2 * k - 1] ^= cy << (GF2X_WORDSIZE - 1);
    /* now W2 has at most 2k words + 3 bits (cy >> 1), but since the final
       results will have 2k words only, we can ignore cy. */
    cy = AddLsh13(W2, W4, 2 * r);
    if (r != k)
	W2[2 * r] ^= cy;
    /* else ignore the carry, since W2 should have 2k words, taking into
       account the above ignored cy >> 1. */
    DivOnePlusX(W2, 2 * k);

/* W1 = W1 + W0 */
/* W1 == ( c4 + c3 + c2 + c1 )  \\ check */
    Add(W1, W1, W0, 2 * k);

/* W3 = ( W3 + W1 ) / (y*(y+1)) */
/* W3 == ( c3 )  \\ check */
    Rsh1Add(W3, W1, 2 * k);
    W3[2 * k - 1] |= W3[2 * k] << (GF2X_WORDSIZE - 1);
    DivOnePlusX(W3, 2 * k);

/* W1 = W1 + W4 */
    Add(W1, W1, W4, 2 * r);

    /* perform simultaneously W1 <- W1 + W2, W2 <- W2 + W3,
       and {c + k, 4k} <- {c + k, 4k} + {W1, 4k} */
    Add3(c + k, W1, W2, k);
    Add4(W2, W1 + k, W2 + k, W3, k);
    Add3(W2 + k, W3, W3 + k, k);
    Add(W4, W4, W3 + k, k);

/* C == W4*x^4+ W3*x^3+ W2*x^2+ W1*x + W0 \\ check */

    /* assume 5*k <= 2*n = 4*k + 2*r, i.e., k <= 2*r, which is true for n >= 8 */
    ASSERT(k <= 2 * r);
}

/*
 * Below this line, experimental code
 * (C) 2007 Richard Brent <trinom@rpbrent.com>
 * This code is released under the GPL 2.0 license, or any later version.
 *
 * Based on Sec. 5.2 of Marco Bodrato's paper (reference below)
 * but with full-word aligned operations to reduce overheads.
 *
 * Reference: http://bodrato.it/papers/#WAIFI2007
 *
 * "Towards Optimal Toom-Cook Multiplication for Univariate and
 * Multivariate Polynomials in Characteristic 2 and 0." by Marco
 * BODRATO; in C.Carlet and B.Sunar, editors, "WAIFI'07 proceedings",
 * LNCS 4547, pp. 119-136. Springer, Madrid, Spain, June 21-22, 2007.
 */

// Need GF2X_MUL_TOOMU_THRESHOLD >= 11 for internal reasons
// but calls to Toom should have size at least 8 so
// need GF2X_MUL_TOOMU_THRESHOLD >= 33.

#define MINI_GF2X_MUL_TOOMU_THRESHOLD 33

#if (GF2X_MUL_TOOMU_THRESHOLD < MINI_GF2X_MUL_TOOMU_THRESHOLD)
#error "GF2X_MUL_TOOMU_THRESHOLD should be at least 33"
#endif

/*
  Unbalanced Toom-Cook multiplication, assumes a takes sa words,
  b takes n = sb = (sa+1)/2 words,
  returns product c of sa+sb words using five multiplications of
  size (n/2 + O(1)) by (n/2 + O(1)).  See Bodrato, pg. 125, top right.

  c should not overlap the inputs.

  stk must have space for sp(sa) = gf2x_toomuspace(sa) words, where

    sp(sa) = 2*sa + 32 + gf2x_toomspace(sa/4 + 4)
          >= 4*(2*ceil(n/2) + 3) + gf2x_toomspace(floor(n/2) + 3)

  and gf2x_toomspace(n) is the maximum space needed for the Toom-Cook routines
  KarMul, Toom3Mul, Toom3wMul, Toom4Mul.

  It is assumed that sa >= 33 so n >= 17.
*/


void gf2x_mul_tc3u(unsigned long * c, const unsigned long * a, long sa,
	      const unsigned long * b, unsigned long * stk)
{
    ASSERT(sa >= MINI_GF2X_MUL_TOOMU_THRESHOLD);
    // n should be at least 6 for internal
    // reasons and 17 so calls to Toom
    // have size at least 9, so need
    // sa >= 33.

    long n = (sa + 1) / 2;	// Assume sb == n == ceil(sa/2)
    long k = (n + 1) / 2;	// ceil(n/2)
    long d = n & 1;		// d = odd(n) = 2k - n, n = 2k - d
    long rb = n - k;		// Size(b1) = k - d
    long ra = sa - 3 * k;	// Size(a3) = ra = k - 2d - odd(sa)
    long sc = sa + n;		// Size(c) = 6k - 3d - odd(sa)

    const unsigned long *a0 = a;	// Aliases for four parts of a
    const unsigned long *a1 = a0 + k;
    const unsigned long *a2 = a1 + k;
    const unsigned long *a3 = a2 + k;
    const unsigned long *b0 = b;	// Aliases for two parts of b
    const unsigned long *b1 = b0 + k;

    long k2 = 2 * (k + 3);	// Size of temporary arrays

    unsigned long *W0 = c;		// Overlap W0 (size 2*k) with c
    unsigned long *W1 = stk;
    unsigned long *W2 = c + 2 * k;	// Overlap W2 with c + 2*k ...
    unsigned long *W3 = W1 + k2;
    unsigned long *W4 = W3 + k2;	// But not W4 as W2 too large
    unsigned long *W5 = W4 + k2;	// W5 is synonymous with W3 in
    // Bodrato's paper
    stk += 4 * k2;		// 4 temporaries of size k2

    unsigned long s, t;
    long j;

// In the comments y = x**w where w = wordlength = NTL_BITS_PER_LONG
// y can be thought of as a w-bit shift operator.
// Bodrato's code corresponds to w = 1, which minimises the size of
// the arguments in the Toom calls, but requires a lot of fiddly
// bit-operations.  By choosing w = 32 or 64 we simplify the coding
// and obtain opportunities for loop optimisation. Both methods have
// the same asymptotic complexity.
//
// If the equal-size multiplication is O(n^alpha) then we expect Toom3uMul
// to be worthwhile when alpha > lg(5/2) = 1.3219...
// TC2 has alpha = lg(3)       = 1.58...,
// TC3 has alpha = lg(5)/lg(3) = 1.46...,
// TC4 has alpha = lg(7)/2     = 1.40...,
// thus in all these cases Toom3uMul should be worthwhile on average
// (saving about 5.5% for the case of TC4, and more for other cases).
// However, this analysis does not take O(n) overheads into account
// so it is inaccurate for small n.
//
// In the comments " + " means addition in GF(2) and " ^ " means
// exponentiation.

// Evaluation phase                                Size is (max) size in words

// W5 = a3 + a2 + a1 + a0 == A(1)               // Size(W5) := k
// W2 = b1 + b0 == B(1)                         // Size(W2) := k

    for (j = 0; j < ra; j++) {
	W5[j] = a3[j] ^ a2[j] ^ a1[j] ^ a0[j];
	W2[j] = b1[j] ^ b0[j];
    }

    for (; j < rb; j++) {
	W5[j] = a2[j] ^ a1[j] ^ a0[j];	// No a3[j] here
	W2[j] = b1[j] ^ b0[j];
    }

    for (; j < k; j++) {
	W5[j] = a2[j] ^ a1[j] ^ a0[j];
	W2[j] = b0[j];		// No b1[j] here
    }

// Calls to Toom mixed with further evaluation. There are 5 calls
// to Toom with sizes at most k+3-d = n/2 + 3 = (sa+1)/4 + 3.

// W1 = W2 * W5 == C(1)

    gf2x_mul_toom(W1, W2, W5, k, stk);	// Size(W1) := 2*k

// W0 = a3*y^3 + a2*y^2 + a1*y == A(y) - a0     // Size(W0) := k+3-d (at most)

    W0[0] = 0;
    W0[1] = a1[0];
    W0[2] = a2[0] ^ a1[1];

    for (j = 0; j < k - 3; j++)	// Assumes k > 2, i.e. n > 4
	W0[j + 3] = a3[j] ^ a2[j + 1] ^ a1[j + 2];

    W0[j + 3] = a2[j + 1] ^ a1[j + 2];	// Fix up a3 contribution later
    j++;
    W0[j + 3] = a2[j + 1];	// No a1[j+2] here, a3 later
    for (j++; j < rb; j++)	// Size(W0) := k+3-d (at most)
	W0[j + 3] = 0;		// Need k+3-d <= 2*k
    // which is true if n > 4.
    for (j = k - 3; j < ra; j++)
	W0[j + 3] ^= a3[j];	// Fix up a3 contribution

// W5 += W0 + a3*(y^2 + y)                      // Size(W5) := k+3-d
// W0 += a0 == A(y)                             // Size(W0)  = k+3-d > k

    for (j = k; j < rb + 3; j++)	// rb == k-d so rb+3 == k+3-d
	W5[j] = 0;

    W5[0] ^= W0[0];
    W0[0] ^= a0[0];

    t = 0;
    for (j = 1; j < ra; j++)	// Usual case, ra-1 iterations
    {
	unsigned long u;
	u = W0[j];
	W0[j] = u ^ a0[j];
	s = a3[j - 1];
	W5[j] ^= s ^ (t ^ u);
	t = s;
    }

    s = a3[j - 1];
    W5[j] ^= W0[j] ^ s ^ t;
    j++;
    W5[j] ^= s;

    for (; j < rb + 3; j++)
	W5[j] ^= W0[j];

    for (j = ra; j < k; j++)
	W0[j] ^= a0[j];

// Pad W2 to size k+3-d for future Toom call (which requires equal-sized
// inputs).  This is (asymptotically) more efficient that calling AddMul1.

    for (j = k; j < rb + 3; j++)
	W2[j] = 0;		// Size(W2) := k+3-d

// W2 += b1*y
// W4  = W2 + b1 == B(y)                        // Size(W4) := k+3-d
    // but 2 high words zero

    W4[0] = W2[0] ^ b1[0];
    t = b1[0];
    for (j = 1; j < rb; j++) {
	unsigned long s;
	s = W2[j] ^ t;
	W2[j] = s;
	t = b1[j];
	W4[j] = s ^ t;
    }

    W2[j] ^= t;

    for (; j < rb + 3; j++)
	W4[j] = W2[j];

// W3 = W5 * W2 == C(1+y)                       // Size(W3) := 2*(k+3-d)
    // but high 2 words zero
    gf2x_mul_toom(W3, W5, W2, rb + 3, stk);

// W2 = W0 * W4 == C(y)

    gf2x_mul_toom(W2, W0, W4, rb + 3, stk);	// Size(W2) := 2*(k+3-d)
    // but 2 high words zero

// W4 = a3 * b1 == C(infinity)

    if (ra < rb) {
	for (j = 0; j < ra; j++)	// W5 := a3 padded to
	    W5[j] = a3[j];	// size rb > ra

	for (; j < rb; j++)
	    W5[j] = 0;		// Size(W5) := rb

	gf2x_mul_toom(W4, W5, b1, rb, stk);	// Size(W4) := 2*rb
    }

    else

	gf2x_mul_toom(W4, a3, b1, rb, stk);	// Avoid copy if ra == rb

// W0 = a0 * b0 == C(0)

    gf2x_mul_toom(W0, a0, b0, k, stk);	// Size(W0) := 2*k;

// Interpolation phase

// W3 += W2 == c1 + c2 + c3*(1 + y + y^2) + c4
// W2 += W0 == C(y) + C(0)

    for (j = 0; j < 2 * k; j++) {	// First 2*k iterations
	unsigned long s;
	s = W2[j];
	W3[j] ^= s;		// Size(W0) = 2*k
	W2[j] = s ^ W0[j];	// other sizes 2*rb + 4
    }				// ignoring known zeros

    for (; j < 2 * rb + 4; j++)
	W3[j] ^= W2[j];		// Last 4 - 2*d iterations

    ASSERT(W2[0] == 0);		// Division should be exact

// W2 = W2/y + W3

    for (j = 0; j < 2 * rb + 3; j++)
	W2[j] = W2[j + 1] ^ W3[j];
    W2[j] = W3[j];		// Size(W2) := 2*rb + 4

// W2 = (W2 + W4*(1+y^3))/(1+y) == c2 + c3

    for (j = 0, s = 0; j < 3; j++) {
	s ^= W2[j] ^ W4[j];
	W2[j] = s;		// first 3 iterations special
    }
    for (; j < 2 * rb; j++) {
	s ^= W2[j] ^ W4[j] ^ W4[j - 3];	// next 2*rb-3 are usual case
	W2[j] = s;
    }

    for (; j < 2 * rb + 3; j++) {
	s ^= W2[j] ^ W4[j - 3];	// next 3 are special
	W2[j] = s;
    }

//  W2[j] = 0;                                  // Size(W2) = 2*rb + 4
    // but last word zero
    // so Size(W2) := 2*rb + 3
// W1 += W0 == c1 + c2 + c3 + c4
// W3 += W1 == c3*y*(1+y)

    for (j = 0; j < 2 * k; j++) {
	unsigned long s;
	s = W0[j] ^ W1[j];
	W1[j] = s;		// Size(W0) = Size(W1) = 2*k
	W3[j] ^= s;		// Size(W3) = 2*rb + 4 > 2*k
    }

    ASSERT(W3[0] == 0);		// Next division exact

// W3 = W3/(y + y^2) == c3

    for (j = 0, s = 0; j < 2 * rb + 3; j++) {
	s ^= W3[j + 1];
	W3[j] = s;
    }
// W3[j] = 0;
    ASSERT(s == 0);		// Division exact
    // Size(W3) := 2*rb + 2

// W1 += W2 + W4 == c1                          // Size(W4) == 2*rb
// W2 += W3      == c2                          //  <= Size(W1) == 2*k
    //  <= Size(W3) == 2*rb + 2
    //  <  Size(W2) == 2*rb + 3
    for (j = 0; j < 2 * rb; j++) {	// Usual case
	unsigned long s;
	s = W2[j];
	W1[j] ^= s ^ W4[j];
	W2[j] = s ^ W3[j];
    }

    for (; j < 2 * k; j++) {	// Next 2*d iterations
	unsigned long s;
	s = W2[j];
	W1[j] ^= s;		// No W4[j] here
	W2[j] = s ^ W3[j];
    }

    for (; j < 2 * rb + 2; j++) {	// Next 2*(1-d) iterations
	unsigned long s;
	s = W2[j];
	W1[j] = s;		// Extending size of W1
	W2[j] = s ^ W3[j];
    }
    W1[j] = W2[j];		// Size(W1) := 2*rb + 3
    // Size(W2)  = 2*rb + 3

// c = W0 + W1*y + W2*y^2 + W3*y^3 + W4*y^4
// We already have
// W0[j] == c[j] for j = 0 .. 2*k-1 because W0 = c, and
// W2[j] == c[j] for j = 2*k .. 2*k+2*rb+2 because W2 = c + 2*k

    ASSERT(3 - 2 * d + 4 * k <= sc);

    for (j = 0; j < 3 - 2 * d; j++)	// 3 - 2*d words of W2
	c[j + 4 * k] ^= W4[j];	// overlap the W4 region

    for (; j < sc - 4 * k; j++)	// Copy rest of W4
	c[j + 4 * k] = W4[j];	// Here c was undefined

    ASSERT(2 * rb + 3 + k <= sc);

    for (j = 0; j < 2 * rb + 3; j++)
	c[j + k] ^= W1[j];

    ASSERT(2 * rb + 2 + 3 * k <= sc);	// True if n >= 6 so need
    // GF2X_MUL_TOOMU_THRESHOLD >= 6
    for (j = 0; j < 2 * rb + 2; j++)
	c[j + 3 * k] ^= W3[j];

}

/*
 * Below this line, experimental code
 * (C) 2007 Richard Brent <trinom@rpbrent.com>
 * This code is released under the GPL 2.0 license, or any later version.
 *
 * Based on Marco Bodrato's mul-tc3.c but with full-word aligned
 * operations to reduce overheads.
 *
 * Reference: http://bodrato.it/papers/#WAIFI2007
 *
 * "Towards Optimal Toom-Cook Multiplication for Univariate and
 * Multivariate Polynomials in Characteristic 2 and 0." by Marco
 * BODRATO; in C.Carlet and B.Sunar, editors, "WAIFI'07 proceedings",
 * LNCS 4547, pp. 119-136. Springer, Madrid, Spain, June 21-22, 2007.
 */

#if (GF2X_MUL_TOOMW_THRESHOLD < 8)
#error "GF2X_MUL_TOOMW_THRESHOLD should be at least 8"
#endif

/*
  c must have space for 2n words and should not overlap the inputs.
  stk must have space for sp(n) = gf2x_toomspace(n) words

    sp(n) = (n lt 8) ? KarMem(7) : 8*(n/3 + 3) + sp(n/3 + 2)

  and  KarMem(7) = 19 is the space required by KarMul.

  A simpler bound on the memory required is 5*n + 17 (equality at n = 19).
*/


#if 0
void gf2x_mul_tc3w(unsigned long * c, const unsigned long * a, const unsigned long * b,
	      long n, unsigned long * stk)
{
    long k = (n + 2) / 3;	// size of a0, a1, b0, b1
    long r = n - 2 * k;		// size of a2, b2
    long d = (r < k) ? 1 : 0;	// 1 if r < k, 0 otherwise
    long kd = k - d;

    const unsigned long *a0 = a;	// Aliases for three parts of a
    const unsigned long *a1 = a + k;
    const unsigned long *a2 = a + 2 * k;
    const unsigned long *b0 = b;	// Ditto for b
    const unsigned long *b1 = b + k;
    const unsigned long *b2 = b + 2 * k;

    long k2 = 2 * (k + 2);	// Size of temporary arrays

    unsigned long *W0 = c;		// Overlap W0 (size 2*k) with c
    unsigned long *W1 = stk;
    unsigned long *W2 = c + 2 * k;	// Overlap W2 with c + 2*k ...
    unsigned long *W3 = W1 + k2;
    unsigned long *W4 = W3 + k2;	// But not W4 as W2 too large
    unsigned long *W5 = W4 + k2;	// W5 is synonymous with W3 in
    // Bodrato's mul-tc3.c
    stk += 4 * k2;		// 4 temporaries of size k2

    long j;
    unsigned long s, u2, v2;

// In the comments y = x**w where w = wordlength = NTL_BITS_PER_LONG
// y can be thought of as a w-bit shift operator.
// Bodrato's code corresponds to w = 1, which minimises the size of
// the arguments in the recursive calls, but requires a lot of fiddly
// bit-operations.  By choosing w = 32 or 64 we simplify the coding
// and obtain opportunities for loop optimisation. Both methods have
// the same asymptotic complexity O(n**(ln(5)/ln(3))) = O(n**1.464).

// We try to combine loops as far as possible to reduce overheads and memory
// references. This often means splitting a loop into the "usual" case and
// "special" cases at the start or end, due to different size arrays etc.

// In the comments " + " means addition in GF(2) and " ^ " means
// exponentiation.

// Evaluation phase                                Size is (max) size in words

// W0 = a1*y + a2*y^2 == A(y) - a0 == A(1+y) - A(1)
// W4 = b1*y + b2*y^2 == B(y) - b0 == B(1+y) - B(1)
// W5 = a0 + a1 + a2 == A(1)
// W2 = b0 + b1 + b2 == B(1)

    W0[0] = W4[0] = 0;
    W0[1] = a1[0];
    W4[1] = b1[0];		// No a2, b2 here
    W5[0] = a0[0] ^ a1[0] ^ (u2 = a2[0]);
    W2[0] = b0[0] ^ b1[0] ^ (v2 = b2[0]);
    for (j = 1; j < r; j++)	// Next r-1 iterations
    {				// This is the usual case
	unsigned long u1, v1;
	W0[j + 1] = (u1 = a1[j]) ^ u2;	// Size(a1) = Size(b1) = k
	W4[j + 1] = (v1 = b1[j]) ^ v2;
	W5[j] = a0[j] ^ u1 ^ (u2 = a2[j]);	// Size(a2) = Size(b2) = r
	W2[j] = b0[j] ^ v1 ^ (v2 = b2[j]);
    }
    for (; j < k; j++)		// Last iterations for W5, W2
    {
	W0[j + 1] = a1[j];	// Omit a2, b2 here
	W4[j + 1] = b1[j];
	W5[j] = a0[j] ^ a1[j];	// Size(W5) := k
	W2[j] = b0[j] ^ b1[j];	// Size(W2) := k;
    }
    W0[k + 1] = W4[k + 1] = 0;	// In case r == k
    W0[r + 1] ^= a2[r - 1];	// Size(W0) := kd+2
    W4[r + 1] ^= b2[r - 1];	// Size(W4) := kd+2

// Recursive calls mixed with further evaluation
// There are 5 recursive calls with sizes at most k+2.
// Thus it is necessary that n > 4 (but we assume that
// Karatsuba's method or some other method will be used
// for very small n, say n < GF2X_MUL_TOOMW_THRESHOLD).

// W1 = W2 * W5 == C(1)

    gf2x_mul_toom(W1, W2, W5, k, stk);	// Size(W1) := 2*k

// W5 += W0 == A(1+y)                           // Size(W5) < Size(W0)
// W2 += W4 == B(1+y)                           // Size(W2) < Size(W4)
// W0 += a0 == A(y)                             // Size(W0) > Size(a0)
// W4 += b0 == B(y)                             // Size(W4) > Size(b0)

    for (j = 0; j < k; j++)	// First k iterations
    {
	unsigned long u, v;
	W5[j] ^= (u = W0[j]);
	W2[j] ^= (v = W4[j]);
	W0[j] = u ^ a0[j];
	W4[j] = v ^ b0[j];
    }

    for (; j < kd + 2; j++)	// Last 2-d iterations
    {
	W5[j] = W0[j];		// Size(W5) := kd+2
	W2[j] = W4[j];		// Size(W2) := kd+2
    }

// W3 = W2 * W5 == C(1+y)

// Output argument in recursive call must differ from inputs.
// That is why we need both W3 and W5.

// ASSERT ((kd+2) <= (n/3 + 2));                // Explaining the space bound

    gf2x_mul_toom(W3, W2, W5, kd + 2, stk);	// Size(W3) := 2*kd + 4

// W2 = W0 * W4 == C(y)

    gf2x_mul_toom(W2, W0, W4, kd + 2, stk);	// Size(W2) := 2*kd + 4

// W0 = a0 * b0 == c0 == C(0/1) == C(0)

    gf2x_mul_toom(W0, a0, b0, k, stk);	// Size(W0) := 2*k
    // so c[0..(2k-1)] defined
// W4 = a2 * b2 == c4 == C(1/0) == C(infinity)

    gf2x_mul_toom(W4, a2, b2, r, stk);	// Size(W4) := 2*r

// Interpolation phase

// W3 += W2 == c1 + c2 + c3*(1 + y + y^2) + c4
// W2 += W0 == C(y) + C(0)

    for (j = 0; j < 2 * k; j++) {	// First 2*k iterations
	s = W2[j];
	W3[j] ^= s;		// Size(W0) = 2*k
	W2[j] = s ^ W0[j];	// other sizes 2*kd + 4
    }

    for (; j < 2 * kd + 4; j++)
	W3[j] ^= W2[j];		// Last 4 - 2*d iterations

// ASSERT (W2[0] == 0);                         // Division should be exact

// W2 = W2/y + W3

    for (j = 0; j < 2 * kd + 3; j++)
	W2[j] = W2[j + 1] ^ W3[j];
    W2[j] = W3[j];		// Size(W2) := 2*kd + 4

// W2 = (W2 + W4*(1+y^3))/(1+y) == c2 + c3

    for (j = 0, s = 0; j < 3; j++) {
	s ^= W2[j] ^ W4[j];
	W2[j] = s;		// first 3 iterations special
    }
    for (; j < 2 * r; j++) {
	s ^= W2[j] ^ W4[j] ^ W4[j - 3];	// next 2r-3 are usual case
	W2[j] = s;
    }

    for (; j < 2 * r + 3; j++) {
	s ^= W2[j] ^ W4[j - 3];	// next 3 are special
	W2[j] = s;
    }

    for (; j < 2 * kd + 4; j++) {
	s ^= W2[j];		// last (k-r-d) == 0 or 1
	W2[j] = s;		// Size(W2) = 2*kd + 3
    }

// ASSERT (s == 0);                             // Division should be exact

// W1 += W0 == c1 + c2 + c3 + c4
// W3 += W1 == c3*y*(1+y)

    for (j = 0; j < 2 * k; j++) {
	s = W0[j] ^ W1[j];
	W1[j] = s;		// Size(W0) = Size(W1) = 2*k
	W3[j] ^= s;		// Size(W3) = 2*kd + 4 > 2*k
    }

// ASSERT (W3[0] == 0);                         // Next division exact

// W3 = W3/(y + y^2) == c3

    for (j = 0, s = 0; j < 2 * kd + 3; j++) {
	s ^= W3[j + 1];
	W3[j] = s;
    }
    W3[j] = 0;
// ASSERT (s == 0);                             // Division exact
    // Size(W3) := 2*kd + 2

// W1 += W2 + W4 == c1                          // Size(W4) == 2*r
// W2 += W3      == c2                          //  <= Size(W1) == 2*k
    //  <= Size(W3) == 2*kd + 2
    //  <  Size(W2) == 2*kd + 4
    for (j = 0; j < 2 * r; j++) {	// Usual case
	s = W2[j];
	W1[j] ^= s ^ W4[j];
	W2[j] = s ^ W3[j];
    }
    for (; j < 2 * k; j++) {	// Next 2*(k-r) iterations
	s = W2[j];
	W1[j] ^= s;		// No W4[j] here
	W2[j] = s ^ W3[j];
    }
    for (; j < 2 * kd + 2; j++) {	// Next 2*(1-d) iterations
	s = W2[j];
	W1[j] = s;		// Extending size of W1
	W2[j] = s ^ W3[j];
    }
    for (; j < 2 * kd + 4; j++)	// Last 2 iterations
	W1[j] = W2[j];		// Size(W1) := 2*kd + 4
    // Size(W2)  = 2*kd + 4

// c = W0 + W1*y + W2*y^2 + W3*y^3 + W4*y^4
// We already have
// W0[j] == c[j] for j = 0 .. 2*k-1 because W0 = c, and
// W2[j] == c[j] for j = 2*k .. 2*k+2*kd+3 because W2 = c + 2*k

    for (j = 0; j < 4 - 2 * d; j++)	// 4 - 2*d words of W2
	c[j + 4 * k] ^= W4[j];	// overlap the W4 region

    for (; j < 2 * r; j++)	// Copy rest of W4
	c[j + 4 * k] = W4[j];	// Here c was undefined

    for (j = 0; j < 2 * kd + 4; j++)
	c[j + k] ^= W1[j];

// ASSERT (2*kd + 2 + 3*k <= 2*n);              // True if n >= 8 so need
    // GF2X_MUL_TOOMW_THRESHOLD >= 8
    for (j = 0; j < 2 * kd + 2; j++)
	c[j + 3 * k] ^= W3[j];

}
#else
/*
 * Below this line, experimental code
 * (C) 2007 Richard Brent <trinom@rpbrent.com>
 * This code is released under the GPL 2.0 license, or any later version.
 *
 * Based on Marco Bodrato's mul-tc3.c but with full-word aligned
 * operations to reduce overheads.
 *
 * Reference: http://bodrato.it/papers/#WAIFI2007
 *
 * "Towards Optimal Toom-Cook Multiplication for Univariate and
 * Multivariate Polynomials in Characteristic 2 and 0." by Marco
 * BODRATO; in C.Carlet and B.Sunar, editors, "WAIFI'07 proceedings",
 * LNCS 4547, pp. 119-136. Springer, Madrid, Spain, June 21-22, 2007.
 */

#if (GF2X_MUL_TOOMW_THRESHOLD < 8)
#error "GF2X_MUL_TOOMW_THRESHOLD should be at least 8"
#endif

/*
  c must have space for 2n words and should not overlap the inputs.
  stk must have space for sp(n) = gf2x_toomspace(n) words

    sp(n) = (n lt 8) ? KarMem(n) : 8*(ceil(n/3) + 2) + sp(floor(n/3) + 2)

  and  KarMem(7) = 21 is the space required by KarMul.

  A simpler bound on the memory required is 5*n + 29 (equality at n = 25):

  * this is true for n < 8 since KarMulMem(n) = 3*ceil(n/2) +
    KarMulMem(ceil(n/2)), which gives KarMulMem(2)=3, KarMulMem(3)=9,
    KarMulMem(4)=9, KarMulMem(5)=18, KarMulMem(6)=18, KarMulMem(7)=21.
    This is always <= 5*n+29.
    * for n >= 8:
      sp(n) <= 8*(ceil(n/3) + 2) + sp(floor(n/3) + 2)
            <= 8*(ceil(n/3) + 2) + 5*(floor(n/3) + 2) + 29 [induction]
            <= 8 * (n/3 + 2/3 + 2) + 5 * (n/3 + 2) + 29
            <= 13/3 * n + 94/3 + 29
            = 5 * n + 29 - (2*n-94)/3

      We first check by exhaustive search that for 8 <= n < 47 we have
      sp(n) <= 5*n+29, and it follows by induction for n >= 47.
*/


void gf2x_mul_tc3w (unsigned long *c, const unsigned long *a, const unsigned long *b,
              long n, unsigned long *stk)

{
  long k = (n + 2) / 3; 		// size of a0, a1, b0, b1
  long r = n - 2*k;			// size of a2, b2
  long d = (r < k) ? 1 : 0;		// 1 if r < k, 0 otherwise
  long kd = k - d;
  long k2 = 2*(k+2);			// Size of 4 temporary arrays

  /* k = ceil(n/3), kd = floor(n/3) */

  const unsigned long *a0 = a;		// Aliases for three parts of a
  const unsigned long *a1 = a + k;
  const unsigned long *a2 = a + 2*k;
  const unsigned long *b0 = b;		// Ditto for b
  const unsigned long *b1 = b + k;
  const unsigned long *b2 = b + 2*k;

  unsigned long *W0 = c;			// Overlap W0 (size 2*k) with c
  unsigned long *W1 = stk;
  unsigned long *W2 = c + 2*k;		// Overlap W2 with c + 2*k ...
  unsigned long *W3 = W1 + k2;
  unsigned long *W4 = W3 + k2;		// But not W4 as W2 too large
  unsigned long *W5 = W4 + k2;		// W5 is synonymous with W3 in
                                        // Bodrato's mul-tc3.c
  stk += 4*k2;	  			// 4 temporaries of size k2

  long j;
  unsigned long s;

// In the comments y = x**w where w = wordlength = NTL_BITS_PER_LONG
// y can be thought of as a w-bit shift operator.
// Bodrato's code corresponds to w = 1, which minimises the size of
// the arguments in the recursive calls, but requires a lot of fiddly
// bit-operations.  By choosing w = 32 or 64 we simplify the coding
// and obtain opportunities for loop optimisation. Both methods have
// the same asymptotic complexity O(n**(ln(5)/ln(3))) = O(n**1.464).

// We try to combine loops as far as possible to reduce overheads and memory
// references. This often means splitting a loop into the "usual" case and
// "special" cases at the start or end, due to different size arrays etc.

// In the comments " + " means addition in GF(2) and " ^ " means
// exponentiation.

// Evaluation phase				   Size is (max) size in words

// W0 = a1*y + a2*y^2 == A(y) - a0 == A(1+y) - A(1)
// W4 = b1*y + b2*y^2 == B(y) - b0 == B(1+y) - B(1)
// W5 = a0 + a1 + a2 == A(1)
// W2 = b0 + b1 + b2 == B(1)

  W0[0] = W4[0] = 0;
  W0[1] = a1[0];
  W4[1] = b1[0];				// No a2, b2 here
  W5[0] = a0[0] ^ a1[0] ^ a2[0];
  W2[0] = b0[0] ^ b1[0] ^ b2[0];
  W5[1] = a0[1] ^ a1[1] ^ a2[1];
  W2[1] = b0[1] ^ b1[1] ^ b2[1];
  for (j = 2; j < r; j++)			// Next r-1 iterations
    {						// This is the usual case
    W0[j] = a1[j-1] ^ a2[j-2];		// Size(a1) = Size(b1) = k
    W4[j] = b1[j-1] ^ b2[j-2];
    W5[j] = a0[j] ^ a1[j] ^ a2[j];		// Size(a2) = Size(b2) = r
    W2[j] = b0[j] ^ b1[j] ^ b2[j];
    }
  /* the following loop runs at most twice, since k <= r + 2 */
  for (; j < k; j++)
    {
    W0[j] = a1[j-1] ^ a2[j-2];
    W4[j] = b1[j-1] ^ b2[j-2];
    W5[j] = a0[j] ^ a1[j];
    W2[j] = b0[j] ^ b1[j];
    }
  W0[k] = a1[k-1] ^ ((k-2 < r) ? a2[k-2] : 0);
  W4[k] = b1[k-1] ^ ((k-2 < r) ? b2[k-2] : 0);
  W0[k+1] = (k-1 < r) ? a2[k-1] : 0;
  W4[k+1] = (k-1 < r) ? b2[k-1] : 0;

// Recursive calls mixed with further evaluation
// There are 5 recursive calls with sizes at most k+2.
// Thus it is necessary that n > 4 (but we assume that
// Karatsuba's method or some other method will be used
// for very small n, say n < GF2X_MUL_TOOMW_THRESHOLD).

// W1 = W2 * W5	== C(1)

  gf2x_mul_toom (W1, W2, W5, k, stk);			// Size(W1) := 2*k

// W5 += W0 == A(1+y)				// Size(W5) < Size(W0)
// W2 += W4 == B(1+y)				// Size(W2) < Size(W4)
// W0 += a0 == A(y)				// Size(W0) > Size(a0)
// W4 += b0 == B(y)				// Size(W4) > Size(b0)

  for (j = 0; j < k; j++)			// First k iterations
    {
    unsigned long u, v;
    W5[j] ^= (u = W0[j]);
    W2[j] ^= (v = W4[j]);
    W0[j]  = u ^ a0[j];
    W4[j]  = v ^ b0[j];
    }

  for (; j < kd+2; j++)				// Last 2-d iterations
    {
    W5[j] = W0[j];				// Size(W5) := kd+2
    W2[j] = W4[j];				// Size(W2) := kd+2
    }

// W3 = W2 * W5 == C(1+y)

// Output argument in recursive call must differ from inputs.
// That is why we need both W3 and W5.

// ASSERT ((kd+2) <= (n/3 + 2));		// Explaining the space bound

  gf2x_mul_toom (W3, W2, W5, kd+2, stk);			// Size(W3) := 2*kd + 4

// W2 = W0 * W4 == C(y)

  gf2x_mul_toom (W2, W0, W4, kd+2, stk);			// Size(W2) := 2*kd + 4

// W0 = a0 * b0 == c0 == C(0/1) == C(0)

  gf2x_mul_toom (W0, a0, b0, k, stk);			// Size(W0) := 2*k
						// so c[0..(2k-1)] defined
// W4 = a2 * b2 == c4 == C(1/0) == C(infinity)

  gf2x_mul_toom (W4, a2, b2, r, stk);			// Size(W4) := 2*r

// Interpolation phase

// W3 += W2 == c1 + c2 + c3*(1 + y + y^2) + c4
// W2 += W0 == C(y) + C(0)

  for (j = 0; j < 2*k; j++)
    {						// First 2*k iterations
    s = W2[j];
    W3[j] ^= s;					// Size(W0) = 2*k
    W2[j]  = s ^ W0[j];				// other sizes 2*kd + 4
    }

  for (; j < 2*kd+4; j++)
    W3[j] ^= W2[j];				// Last 4 - 2*d iterations

// ASSERT (W2[0] == 0);				// Division should be exact

// W2 = W2/y + W3

  for (j = 0; j < 2*kd + 3; j++)
    W2[j] = W2[j+1] ^ W3[j];
  W2[j] = W3[j];				// Size(W2) := 2*kd + 4

// W2 = (W2 + W4*(1+y^3))/(1+y) == c2 + c3

  for (j = 0, s = 0; j < 3; j++)
    {
    s ^= W2[j] ^ W4[j];
    W2[j] = s;					// first 3 iterations special
    }
  for (; j < 2*r; j++)
    {
    s ^= W2[j] ^ W4[j] ^ W4[j-3];		// next 2r-3 are usual case
    W2[j] = s;
    }

  for (; j < 2*r+3; j++)
    {
    s ^= W2[j] ^ W4[j-3];			// next 3 are special
    W2[j] = s;
    }

  for (; j < 2*kd+4; j++)
    {
    s ^= W2[j]; 				// last (k-r-d) == 0 or 1
    W2[j] = s;					// Size(W2) = 2*kd + 3
    }

// ASSERT (s == 0);				// Division should be exact

// W1 += W0 == c1 + c2 + c3 + c4
// W3 += W1 == c3*y*(1+y)

  for (long j = 0; j < 2*k; j++)
    {
    s = W0[j] ^ W1[j];
    W1[j] = s;					// Size(W0) = Size(W1) = 2*k
    W3[j] ^= s;					// Size(W3) = 2*kd + 4 > 2*k
    }

// ASSERT (W3[0] == 0);				// Next division exact

// W3 = W3/(y + y^2) == c3

  for (j = 0, s = 0; j < 2*kd + 3; j++)
    {
    s ^= W3[j+1];
    W3[j] = s;
    }
  W3[j] = 0;
// ASSERT (s == 0);				// Division exact
			  			// Size(W3) := 2*kd + 2

// W1 += W2 + W4 == c1				// Size(W4) == 2*r
// W2 += W3      == c2				//  <= Size(W1) == 2*k
						//  <= Size(W3) == 2*kd + 2
						//  <  Size(W2) == 2*kd + 4
  for (j = 0; j < 2*r; j++)
    {						// Usual case
    s = W2[j];
    W1[j] ^= s ^ W4[j];
    W2[j]  = s ^ W3[j];
    }
  for (; j < 2*k; j++)
    {						// Next 2*(k-r) iterations
    s = W2[j];
    W1[j] ^= s;					// No W4[j] here
    W2[j]  = s ^ W3[j];
    }
  for (; j < 2*kd + 2; j++)
    {						// Next 2*(1-d) iterations
    s = W2[j];
    W1[j] = s;					// Extending size of W1
    W2[j] = s ^ W3[j];
    }
  for (; j < 2*kd + 4; j++)			// Last 2 iterations
    W1[j] = W2[j];				// Size(W1) := 2*kd + 4
    						// Size(W2)  = 2*kd + 4

// c = W0 + W1*y + W2*y^2 + W3*y^3 + W4*y^4
// We already have
// W0[j] == c[j] for j = 0 .. 2*k-1 because W0 = c, and
// W2[j] == c[j] for j = 2*k .. 2*k+2*kd+3 because W2 = c + 2*k

  for (j = 0; j < 4 - 2*d; j++)			// 4 - 2*d words of W2
    c[j+4*k] ^= W4[j];    			// overlap the W4 region

  for (; j < 2*r; j++)				// Copy rest of W4
    c[j+4*k] = W4[j];    			// Here c was undefined

  for (long j = 0; j < 2*kd + 4; j++)
    c[j+k]   ^= W1[j];

// ASSERT (2*kd + 2 + 3*k <= 2*n);		// True if n >= 8 so need
						// GF2X_MUL_TOOMW_THRESHOLD >= 8
  for (long j = 0; j < 2*kd + 2; j++)
    c[j+3*k] ^= W3[j];

}
#endif

#ifdef HAVE_KARAX
#include <emmintrin.h>
/* 128-bit variant, copied from gf2x_mul_tc3w, see the comments
   in gf2x_mul_tc3w.
   Assume all parameters are 128-bit aligned.
 */
static void
gf2x_mul_tc3x_internal (__m128i *c, const __m128i *a, const __m128i *b,
                        long n, __m128i *stk)

{
  assert ((((uintptr_t) c) % 16) == 0);
  assert ((((uintptr_t) a) % 16) == 0);
  assert ((((uintptr_t) b) % 16) == 0);
  assert ((((uintptr_t) stk) % 16) == 0);

  long k = (n + 2) / 3; 		// size of a0, a1, b0, b1
  long r = n - 2*k;			// size of a2, b2
  long d = (r < k) ? 1 : 0;		// 1 if r < k, 0 otherwise
  long kd = k - d;
  long k2 = 2*(k+2);			// Size of 4 temporary arrays

  const __m128i *a0 = a;		// Aliases for three parts of a
  const __m128i *a1 = a + k;
  const __m128i *a2 = a + 2*k;
  const __m128i *b0 = b;		// Ditto for b
  const __m128i *b1 = b + k;
  const __m128i *b2 = b + 2*k;

  __m128i *W0 = c;			// Overlap W0 (size 2*k) with c
  __m128i *W1 = stk;
  __m128i *W2 = c + 2*k;		// Overlap W2 with c + 2*k ...
  __m128i *W3 = W1 + k2;
  __m128i *W4 = W3 + k2;		// But not W4 as W2 too large
  __m128i *W5 = W4 + k2;		// W5 is synonymous with W3 in
  					// Bodrato's mul-tc3.c
  stk += 4*k2;	  			// 4 temporaries of size k2

  long j;
  __m128i s;

  // W0[0] = W4[0] = 0;
  __m128i zero = _mm_set_epi32 (0, 0, 0, 0);
  W0[0] = W4[0] = zero;
  W0[1] = a1[0];
  W4[1] = b1[0];				// No a2, b2 here
  W5[0] = a0[0] ^ a1[0] ^ a2[0];
  W2[0] = b0[0] ^ b1[0] ^ b2[0];
  W5[1] = a0[1] ^ a1[1] ^ a2[1];
  W2[1] = b0[1] ^ b1[1] ^ b2[1];
  for (j = 2; j < r; j++)			// Next r-1 iterations
    {						// This is the usual case
    W0[j] = a1[j-1] ^ a2[j-2];		// Size(a1) = Size(b1) = k
    W4[j] = b1[j-1] ^ b2[j-2];
    W5[j] = a0[j] ^ a1[j] ^ a2[j];		// Size(a2) = Size(b2) = r
    W2[j] = b0[j] ^ b1[j] ^ b2[j];
    }
  /* the following loop runs at most twice, since k <= r + 2 */
  for (; j < k; j++)
    {
    W0[j] = a1[j-1] ^ a2[j-2];
    W4[j] = b1[j-1] ^ b2[j-2];
    W5[j] = a0[j] ^ a1[j];
    W2[j] = b0[j] ^ b1[j];
    }
  W0[k] = a1[k-1] ^ ((k-2 < r) ? a2[k-2] : zero);
  W4[k] = b1[k-1] ^ ((k-2 < r) ? b2[k-2] : zero);
  W0[k+1] = (k-1 < r) ? a2[k-1] : zero;
  W4[k+1] = (k-1 < r) ? b2[k-1] : zero;

  gf2x_mul_toom ((unsigned long*) W1, (unsigned long*) W2,
                 (unsigned long*) W5, 2 * k, (unsigned long*) stk);

  for (j = 0; j < k; j++)			// First k iterations
    {
      W5[j] ^= W0[j];
      W2[j] ^= W4[j];
      W0[j] ^= a0[j];
      W4[j] ^= b0[j];
    }

  for (; j < kd+2; j++)				// Last 2-d iterations
    {
      W5[j] = W0[j];				// Size(W5) := kd+2
      W2[j] = W4[j];				// Size(W2) := kd+2
    }

  gf2x_mul_toom ((unsigned long*) W3, (unsigned long*) W2,
                 (unsigned long*) W5, 2 * (kd + 2), (unsigned long*) stk);

  gf2x_mul_toom ((unsigned long*) W2, (unsigned long*) W0,
                 (unsigned long*) W4, 2 * (kd + 2), (unsigned long*) stk);

  gf2x_mul_toom ((unsigned long*) W0, (unsigned long*) a0,
                 (unsigned long*) b0, 2 * k, (unsigned long*) stk);

  gf2x_mul_toom ((unsigned long*) W4, (unsigned long*) a2,
                 (unsigned long*) b2, 2 * r, (unsigned long*) stk);

  for (j = 0; j < 2*k; j++)
    {						// First 2*k iterations
      W3[j] ^= W2[j];					// Size(W0) = 2*k
      W2[j] ^= W0[j];				// other sizes 2*kd + 4
    }

  for (; j < 2*kd+4; j++)
    W3[j] ^= W2[j];				// Last 4 - 2*d iterations

  for (j = 0; j < 2*kd + 3; j++)
    W2[j] = W2[j+1] ^ W3[j];
  W2[j] = W3[j];				// Size(W2) := 2*kd + 4

  for (j = 0, s = zero; j < 3; j++)
    {
    s ^= W2[j] ^ W4[j];
    W2[j] = s;					// first 3 iterations special
    }
  for (; j < 2*r; j++)
    {
    s ^= W2[j] ^ W4[j] ^ W4[j-3];		// next 2r-3 are usual case
    W2[j] = s;
    }

  for (; j < 2*r+3; j++)
    {
    s ^= W2[j] ^ W4[j-3];			// next 3 are special
    W2[j] = s;
    }

  for (; j < 2*kd+4; j++)
    {
    s ^= W2[j]; 				// last (k-r-d) == 0 or 1
    W2[j] = s;					// Size(W2) = 2*kd + 3
    }

  for (long j = 0; j < 2*k; j++)
    {
    s = W0[j] ^ W1[j];
    W1[j] = s;					// Size(W0) = Size(W1) = 2*k
    W3[j] ^= s;					// Size(W3) = 2*kd + 4 > 2*k
    }

  for (j = 0, s = zero; j < 2*kd + 3; j++)
    {
    s ^= W3[j+1];
    W3[j] = s;
    }
  W3[j] = zero;
						//  <  Size(W2) == 2*kd + 4
  for (j = 0; j < 2*r; j++)
    {						// Usual case
    s = W2[j];
    W1[j] ^= s ^ W4[j];
    W2[j]  = s ^ W3[j];
    }
  for (; j < 2*k; j++)
    {						// Next 2*(k-r) iterations
    s = W2[j];
    W1[j] ^= s;					// No W4[j] here
    W2[j]  = s ^ W3[j];
    }
  for (; j < 2*kd + 2; j++)
    {						// Next 2*(1-d) iterations
    s = W2[j];
    W1[j] = s;					// Extending size of W1
    W2[j] = s ^ W3[j];
    }
  for (; j < 2*kd + 4; j++)			// Last 2 iterations
    W1[j] = W2[j];				// Size(W1) := 2*kd + 4
                                                // Size(W2)  = 2*kd + 4
  for (j = 0; j < 4 - 2*d; j++)			// 4 - 2*d words of W2
    c[j+4*k] ^= W4[j];    			// overlap the W4 region

  for (; j < 2*r; j++)				// Copy rest of W4
    c[j+4*k] = W4[j];    			// Here c was undefined

  for (long j = 0; j < 2*kd + 4; j++)
    c[j+k] ^= W1[j];

  for (long j = 0; j < 2*kd + 2; j++)
    c[j+3*k] ^= W3[j];
}

/* wrapper for gf2x_mul_tc3x_internal */
void
gf2x_mul_tc3x (unsigned long *c, const unsigned long *a,
               const unsigned long *b, long n, unsigned long *stk)
{
  unsigned long *cc, *aa, *bb;

  /* ensure stk is 128-bit aligned */
  if ((uintptr_t) stk % 16)
    stk ++;

  if (n & 1) /* n is odd */
    {
      aa = alloca ((4 * n + 5) * sizeof (unsigned long));
      if (((uintptr_t) aa) % 16)
        aa ++;
      /* now aa is 128-bit aligned */
      memcpy (aa, a, n * sizeof(unsigned long));
      aa[n] = 0;
      bb = aa + n + 1;
      memcpy (bb, b, n * sizeof(unsigned long));
      bb[n] = 0;
      cc = bb + n + 1;
      gf2x_mul_tc3x_internal ((__m128i*) cc, (__m128i*) aa,
                              (__m128i*) bb, (n + 1) >> 1, (__m128i*) stk);
      memcpy (c, cc, 2 * n * sizeof(unsigned long));
    }
  else /* n is even */
    {
      if (((uintptr_t) a % 16) == 0)
        aa = (unsigned long*) a;
      else
        {
          aa = alloca ((n + 1) * sizeof (unsigned long));
          if (((uintptr_t) aa) % 16)
            aa ++;
          memcpy (aa, a, n * sizeof(unsigned long));
        }
      if (((uintptr_t) b % 16) == 0)
        bb = (unsigned long*) b;
      else
        {
          bb = alloca ((n + 1) * sizeof (unsigned long));
          if ((uintptr_t) bb % 16)
            bb ++;
          memcpy (bb, b, n * sizeof(unsigned long));
        }
      if (((uintptr_t) c % 16) == 0)
        cc = c;
      else
        {
          cc = alloca ((2 * n + 1) * sizeof (unsigned long));
          if ((uintptr_t) cc % 16)
            cc ++;
        }
      gf2x_mul_tc3x_internal ((__m128i*) cc, (__m128i*) aa,
                              (__m128i*) bb, n >> 1, (__m128i*) stk);
      if (cc != c)
        memcpy (c, cc, 2 * n * sizeof(unsigned long));
    }
}
#endif /* HAVE_KARAX */

/********************************************************************
 * Below this line, experimental code
 * (C) 2007 Marco Bodrato <optimaltoom@bodrato.it>
 * Modified by Paul Zimmermann, April 2007.
 * This code is released under the GPL 2.0 license, or any later version.
 *
 * Reference: http://bodrato.it/papers/#WAIFI2007
 *
 * "Towards Optimal Toom-Cook Multiplication for Univariate and
 * Multivariate Polynomials in Characteristic 2 and 0." by Marco
 * BODRATO; in C.Carlet and B.Sunar, editors, "WAIFI'07 proceedings",
 * LNCS 4547, pp. 119-136. Springer, Madrid, Spain, June 21-22, 2007.
 */

#if (GF2X_MUL_TOOM4_THRESHOLD < 30)
#error "GF2X_MUL_TOOM4_THRESHOLD should be at least 30"
#endif

/* c <- x * a + x^2 * b, return carry out */
static
unsigned long AddLsh1Lsh2(unsigned long * c, const unsigned long * a, const unsigned long * b, long n)
{
    unsigned long cy = 0UL, t;
    long i;
    for (i = 0; i < n; i++) {
	t = (a[i] << 1) ^ ((b[i] << 2) | cy);
	cy = (a[i] >> (GF2X_WORDSIZE - 1)) ^ (b[i] >> (GF2X_WORDSIZE - 2));
	c[i] = t;
    }
    return cy;
}

/* c <- a + x^2 * b, return carry out */
static
unsigned long AddLsh2(unsigned long * c, const unsigned long * a, const unsigned long * b, long n)
{
    unsigned long cy = 0UL, t;
    long i;
    for (i = 0; i < n; i++) {
	t = a[i] ^ ((b[i] << 2) | cy);
	cy = b[i] >> (GF2X_WORDSIZE - 2);
	c[i] = t;
    }
    return cy;
}

/* c <- a + x^6 * b, return carry out */
static
unsigned long AddLsh6(unsigned long * c, const unsigned long * a, const unsigned long * b, long n)
{
    unsigned long cy = 0UL, t;

    long i;

    for (i = 0; i < n; i++) {
	t = a[i] ^ ((b[i] << 6) | cy);
	cy = b[i] >> (GF2X_WORDSIZE - 6);
	c[i] = t;
    }
    return cy;
}

/* let c = q*(x+x^4) + X^n*r with X = x^GF2X_WORDSIZE and deg(r) < 1
   then c <- q, returns r.
*/
static unsigned long DivExactD1(unsigned long * c, long n)
{
    unsigned long t = 0;
    long i;

    for (i = 0; i < n; i++) {
	t ^= (c[i] >> 1) | ((i + 1 < n)
			    ? (c[i + 1] << (GF2X_WORDSIZE - 1)) : 0);
	t ^= t << 3 ^ t << 6;
	t ^= t << 9 ^ t << 18;
	t ^= t << 27
#if (GF2X_WORDSIZE == 64)
	    ^ t << 54
#elif (GF2X_WORDSIZE != 32)
#error "GF2X_WORDSIZE should be 32 or 64"
#endif
	    ;
	c[i] = t;
	t >>= (GF2X_WORDSIZE - 3);
    }
    return t;
}

/* let c = q*(x^2+x^4) + X^n*r with X = x^GF2X_WORDSIZE and deg(r) < 1
   then c <- q, returns r.
*/
static unsigned long DivExactD2(unsigned long * c, long n)
{
    /* c <- c/x^2 */
    unsigned long cy = 0, t;

    long i;

    for (i = n - 1; i >= 0; i--) {
	t = (c[i] >> 2) | (cy << (GF2X_WORDSIZE - 2));
	cy = c[i];		/* no need to mask the low 2 bits, since they will disappear
				   with the next cy << (GF2X_WORDSIZE - 2) */
	c[i] = t;
    }
    return DivOnePlusX2(c, n);
}

#if 0
/* Same as DivExactD2, but with one pass only. However, does not seem to give
   a significant speedup, thus disabled for now. */
static unsigned long DivExactD2a(unsigned long * c, long n)
{
    unsigned long t, ci;

    t = c[0];
    t ^= t << 2;
    t ^= t << 4;
    t ^= t << 8;
    t ^= t << 16;
#if (GF2X_WORDSIZE == 64)
    t ^= t << 32;
#elif (GF2X_WORDSIZE != 32)
#error "GF2X_WORDSIZE should be 32 or 64"
#endif
    ci = t;
    t >>= (GF2X_WORDSIZE - 2);
    for (long i = 1; i < n; i++) {
	t ^= c[i];
	t ^= t << 2;
	t ^= t << 4;
	t ^= t << 8;
	t ^= t << 16;
#if (GF2X_WORDSIZE == 64)
	t ^= t << 32;
#endif
	/* now t is the result of the division of c[i] by (1+x^2), and
	   t >> (GF2X_WORDSIZE - 2) the corresponding carry */
	c[i - 1] = (ci >> 2) | (t << (GF2X_WORDSIZE - 2));
	ci = t;
	t >>= (GF2X_WORDSIZE - 2);
    }
    c[n - 1] = ci >> 2;
    return t;
}
#endif

/*
\\ gp-pari check code.
\\ (C) 2007 Marco Bodrato <optimaltoom@bodrato.it>
\\ This code is released under the GPL 2.0 license, or any later version.

U0=u0*Mod(1,2);U1=u1*Mod(1,2);U2=u2*Mod(1,2);U3=u3*Mod(1,2);
V0=v0*Mod(1,2);V1=v1*Mod(1,2);V2=v2*Mod(1,2);V3=v3*Mod(1,2);

U = U3*Y^3 + U2*Y^2*X + U1*Y*X^2 + U0*X^3
V = V3*Y^3 + V2*Y^2*X + V1*Y*X^2 + V0*X^3

\\ P(X,Y): P0=(1,0); P1=(x+1,1); P2=(x,1); P3=(1,1); P4=(1,x); P5=(1,x+1); P6=(0,1)
\\Evaluation phase: 13*2 add, 7*2 shift, 2Smul; 7 mul (n)

W1 = U0 + U1 + U2 + U3    ; W2 = V0 + V1 + V2 + V3
W0 = U1 +(U2 + U3*x)*x    ; W6 = V1 +(V2 + V3*x)*x
W4 = W1 +(W0 + U3*(x+1))*x; W3 = W2 +(W6 + V3*(x+1))*x
W0 = W0*x + U0            ; W6 = W6*x + V0

W5 = W4 * W3              ; W4 = W0 * W6
W3 = W1 * W2

W0 =(U2 +(U1 + U0*x)*x)*x ; W6 =(V2 +(V1 + V0*x)*x)*x
W1 = W1 + W0 + U0*(x^2+x) ; W2 = W2 + W6 + V0*(x^2+x)
W0 = W0 + U3              ; W6 = W6 + V3

W1 = W1 * W2              ; W2 = W0 * W6
W6 = U3 * V3              ; W0 = U0 * V0

\\Interpolation: 22 add, 4 shift, 5 Smul, 4 div (2n)
d1=(x^4+x)*Mod(1,2)	; d1== (x)^1*(x+1)^1*(x^2+x+1)^1 *Mod(1,2)
d2=(x^4+x^2)*Mod(1,2)	; d2== (x)^2*(x+1)^2*(x^2+x+1)^0 *Mod(1,2)

W1 = W1 + W2 + W0*(x^4+x^2+1)
W5 =(W5 + W4 + W1 + W6*(x^4+x^2+1))/d1
W2 = W2 + W6 + W0*(x^6)
W4 = W4 + W2 + W0 + W6*(x^6)
W4 =(W4 + W5*(x^5+x))/d2
W3 = W3 + W0 + W6
W1 = W1 + W3
W2 = W2 +(W1 + W3*x)*x
W3 = W3 + W4 + W5
W1 =(W1 + W3*(x^2+x))/d1
W5 = W5 + W1
W2 =(W2 + W5*(x^2+x))/d2
W4 = W4 + W2

\\Recomposition
W  = W6*Y^6 + W5*Y^5*X + W4*Y^4*X^2+ W3*Y^3*X^3+ W2*Y^2*X^4+ W1*Y*X^5 + W0*X^6
W == U*V

Memory Usage: stk must have space for sp(n), where
sp(n) = 6k+2 + sp(k+1) with k = ceil(n/4).
*/

void gf2x_mul_tc4(unsigned long * c, const unsigned long * a, const unsigned long * b,
	      long n, unsigned long * stk)
{
    long k = (n + 3) / 4;	/* ceil(n/4) */
    long r = n - 3 * k;
    unsigned long cy, cy1, cy2, cy3, cy4;
    unsigned long *W0 = c;
    unsigned long *W1 = stk;
    unsigned long *W2 = c + 2 * k;
    unsigned long *W3 = stk + 2 * k;
    unsigned long *W4 = c + 4 * k;
    unsigned long *W5 = stk + 4 * k;
    unsigned long *W6 = c + 6 * k;
    unsigned long *newstk = stk + 6 * k + 2;

/* \\Evaluation phase: 13*2 add, 7*2 shift, 2Smul; 7 mul (n) */

/* W1 = U0 + U1 + U2 + U3    ; W2 = V0 + V1 + V2 + V3 */
    Add(W1, a, a + 3 * k, r);
    Add1(W1 + r, a + r, k - r, 0);
    Add3(W1, a + k, a + 2 * k, k);	/* U0 + U1 + U2 + U3 */
    Add(W2 + 2, b, b + 3 * k, r);
    Add1(W2 + r + 2, b + r, k - r, 0);
    Add3(W2 + 2, b + k, b + 2 * k, k);	/* V0 + V1 + V2 + V3 */
/*   Add (W1,     a, a + k,     k); */
/*   Add (W1,    W1, a + 2 * k, k); */
/*   Add (W1,    W1, a + 3 * k, r); /\* U0 + U1 + U2 + U3 *\/ */
/*   Add (W2+2,   b, b + k,     k); */
/*   Add (W2+2,W2+2, b + 2 * k, k); */
/*   Add (W2+2,W2+2, b + 3 * k, r); /\* V0 + V1 + V2 + V3 *\/ */

/* W0 = U1 +(U2 + U3*x)*x    ; W6 = V1 +(V2 + V3*x)*x */
    cy = AddLsh1(W0, a + 2 * k, a + 3 * k, r);	/* U2 + x U3 */
    cy = Add1(W0 + r, a + 2 * k + r, k - r, cy);
    W0[k] = (cy << 1) ^ AddLsh1(W0, a + k, W0, k);	/* U1+x U2 + x^2 U3 */
    cy = AddLsh1(W6 + 2, b + 2 * k, b + 3 * k, r);	/* V2 + x V3 */
    cy = Add1(W6 + 2 + r, b + 2 * k + r, k - r, cy);
    W6[k + 2] = (cy << 1) ^ AddLsh1(W6 + 2, b + k, W6 + 2, k);	/* V1+x V2 + x^2 V3 */
    /* since we use W6[k+2], and we have space for 2r words in W6, we need
       k+3 <= 2*r, which requires n>=30. */
/* W4 = W1 +(W0 + U3*(x+1))*x; W3 = W2 +(W6 + V3*(x+1))*x */
    cy = AddLsh1(W4 + 2, W0, a + 3 * k, r);	/* W0 + x U3 */
    cy = Add1(W4 + 2 + r, W0 + r, k + 1 - r, cy);	/* cy == 0 */
    ASSERT(cy == 0);
    Add(W4 + 2, W4 + 2, a + 3 * k, r);	/* W0 + x U3 + U3 */
    W4[k + 2] = (W4[k + 2] << 1) ^ AddLsh1(W4 + 2, W1, W4 + 2, k);	/* W1+x(W0 +(x+1) U3) */
    cy = AddLsh1(W3 + 2, W6 + 2, b + 3 * k, r);	/* W6 + x V3 */
    cy = Add1(W3 + 2 + r, W6 + 2 + r, k + 1 - r, cy);	/* cy == 0 */
    Add(W3 + 2, W3 + 2, b + 3 * k, r);	/* W6 + x V3 + V3 */
    W3[k + 2] = (W3[k + 2] << 1) ^ AddLsh1(W3 + 2, W2 + 2, W3 + 2, k);	/* W2+x(W6 + (x+1) V3) */
/* W0 = W0*x + U0            ; W6 = W6*x + V0 */
    W0[k] = (W0[k] << 1) ^ AddLsh1(W0, a, W0, k);	/* U0+x W0 */
    W6[k + 2] = (W6[k + 2] << 1) ^ AddLsh1(W6 + 2, b, W6 + 2, k);	/* V0+x W6 */
/* W5 = W4 * W3              ; W4 = W0 * W6 */
    gf2x_mul_toom(W5, W4 + 2, W3 + 2, k + 1, newstk);	/* W5 : 2*k+1 */

    gf2x_mul_toom(W4, W0, W6 + 2, k + 1, newstk);	/* W4 : 2*k+1 */
    cy4 = W6[0];		/* Take care of overlapping byte. */
/* W3 = W1 * W2 */
    gf2x_mul_toom(W3, W1, W2 + 2, k, newstk);	/* W3 : 2*k */

/* W0 =(U2 +(U1 + U0*x)*x)*x ; W6 =(V2 +(V1 + V0*x)*x)*x */
    cy = AddLsh1(W0, a + 1 * k, a + 0 * k, k);	/* U1 + x U0 */
    W0[k] = (cy << 2) ^ AddLsh1Lsh2(W0, a + 2 * k, W0, k);	/* U2+x U1 + x^2 U0 */
    cy = AddLsh1(W6 + 2, b + 1 * k, b + 0 * k, k);	/* V1 + x V0 */
    W6[k + 2] = (cy << 2) ^ AddLsh1Lsh2(W6 + 2, b + 2 * k, W6 + 2, k);	/* V2+x V1 + x^2 V0 */
/* W1 = W1 + W0 + U0*(x^2+x) ; W2 = W2 + W6 + V0*(x^2+x) */
    W1[k] = gf2x_addmul_1_n(W1, W1, a, k, 4 + 2);
    Add(W0 + k + 1, W1, W0, k + 1);
    W2[k + 2] = gf2x_addmul_1_n(W2 + 2, W2 + 2, b, k, 4 + 2);
    Add(W2 + 2, W2 + 2, W6 + 2, k + 1);
/* W0 = W0 + U3              ; W6 = W6 + V3 */
    Add(W0, W0, a + 3 * k, r);	/* + U3 */
    Add(W6 + 2, W6 + 2, b + 3 * k, r);	/* + V3 */
/* W1 = W1 * W2              ; W2 = W0 * W6 */
    cy = W3[0];
    cy2 = W3[1];		/* Take care of overlapping byte. */
    gf2x_mul_toom(W1, W0 + k + 1, W2 + 2, k + 1, newstk);	/* W1 : 2*k+1 */
    cy1 = W3[0];
    W3[0] = cy;
    W3[1] = cy2;
    cy = W4[0];
    cy2 = W4[1];		/* Take care of overlapping byte. */
    gf2x_mul_toom(W2, W0, W6 + 2, k + 1, newstk);	/* W2 : 2*k+1 */
    W4[1] = cy2;
    cy2 = W4[0];
    W4[0] = cy;
/* W6 = U3 * V3              ; W0 = U0 * V0 */
    gf2x_mul_toom(W0, a, b, k, newstk);	/* W0 : 2*k */
    gf2x_mul_toom(W6, a + 3 * k, b + 3 * k, r, newstk);	/* W6 : 2*r */
/* \\Interpolation: 22 add, 4 shift, 5 Smul, 4 div (2n) */
/* d1=(x^4+x)*Mod(1,2)	; d1== (x)^1*(x+1)^1*(x^2+x+1)^1 *Mod(1,2) */
/* d2=(x^4+x^2)*Mod(1,2)	; d2== (x)^2*(x+1)^2*(x^2+x+1)^0 *Mod(1,2) */

/* W1 = W1 + W2 + W0*(x^4+x^2+1) */

    Add(W1, W1, W2, 2 * k);
    cy1 ^= cy2 ^ gf2x_addmul_1_n(W1, W1, W0, 2 * k, 16 + 4 + 1);

/* W5 =(W5 + W4 + W1 + W6*(x^4+x^2+1))/d1 */
    Add3(W5, W4, W1, 2 * k);
    W5[2 * k] ^= cy1 ^ cy4;
    W5[2 * r] ^= gf2x_addmul_1_n(W5, W5, W6, 2 * r, 16 + 4 + 1);
    DivExactD1(W5, 2 * k + 1);

/* W2 = W2 + W6 + W0*(x^6) */
    Add(W2, W2, W6, 2 * r);
    cy2 ^= AddLsh6(W2, W2, W0, 2 * k);
/* W4 = W4 + W2 + W0 + W6*(x^6) */
    Add3(W4, W2, W0, 2 * k);
    cy3 = AddLsh6(W4, W4, W6, 2 * r);
    cy = W6[0];			/* save W6[0]=W4[2k]: we cannot do it before the AddLsh6 call
				   because W6 is used as input */
    W6[0] = cy4 ^ cy2;
    W4[2 * r] ^= cy3;		/* must come after W6[0] = cy4 in case r=k */
/* W4 =(W4 + W5*(x^5+x))/d2 */
    gf2x_addmul_1_n(W4, W4, W5, 2 * k + 1, 32 + 2);
    DivExactD2(W4, 2 * k + 1);
    W6[0] = cy;
/* W3 = W3 + W0 + W6 */
    Add3(W3, W0, W6, 2 * r);
    if (r != k)
	Add(W3 + 2 * r, W3 + 2 * r, W0 + 2 * r, 2 * (k - r));	/* warning: 2r instead of r */
/* W1 = W1 + W3 */
    Add(W1, W1, W3, 2 * k);
/* W2 = W2 +(W1 + W3*x)*x */
    cy2 ^=
	AddLsh1(W2, W2, W1, 2 * k) ^ AddLsh2(W2, W2, W3, 2 * k) ^ (cy1 << 1);
/* W3 = W3 + W4 + W5 */
    Add3(W3, W4, W5, 2 * k);
/* W1 =(W1 + W3*(x^2+x))/d1 */
    cy = W3[0];
    cy1 ^= gf2x_addmul_1_n(W1, W1, W3, 2 * k, 4 + 2);
    W3[0] = cy1;
    DivExactD1(W1, 2 * k + 1);
    W3[0] = cy;
/* W5 = W5 + W1 */
    Add(W5, W5, W1, 2 * k);
/* W2 =(W2 + W5*(x^2+x))/d2 */
    cy = W4[0];
    W4[0] = cy2 ^ gf2x_addmul_1_n(W2, W2, W5, 2 * k, 4 + 2);
    DivExactD2(W2, 2 * k + 1);
    W4[0] = cy;
/* W4 = W4 + W2 */
    Add(W4, W4, W2, 2 * k);

/* \\Recomposition */
/* W = W6*Y^6 + W5*Y^5 + W4*Y^4+ W3*Y^3+ W2*Y^2+ W1*Y + W0 */
    Add(c + k, c + k, W1, 6 * k);

}
