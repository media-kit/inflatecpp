/*
  By Mark Adler - https://github.com/madler/zlib/blob/master/adler32.c

  Copyright (C) 1995-2017 Jean-loup Gailly and Mark Adler
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.
  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:
  1. The origin of this software must not be misrepresented; you must not
         claim that you wrote the original software. If you use this software
         in a product, an acknowledgment in the product documentation would be
         appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
         misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
  Jean-loup Gailly        Mark Adler
  jloup@gzip.org          madler@alumni.caltech.edu
  The data format used by the zlib library is described by RFCs (Request for
  Comments) 1950 to 1952 in the files http://tools.ietf.org/html/rfc1950
  (zlib format), rfc1951 (deflate format) and rfc1952 (gzip format).
 */

#ifndef _ADLER_32_H
#define _ADLER_32_H

#include <cstdio>
#include <iostream>

#define BASE 65521U
#define NMAX 5552

#define DO1(buf, i)    \
  {                    \
    adler += (buf)[i]; \
    sum2 += adler;     \
  }
#define DO2(buf, i) \
  DO1(buf, i);      \
  DO1(buf, i + 1);
#define DO4(buf, i) \
  DO2(buf, i);      \
  DO2(buf, i + 2);
#define DO8(buf, i) \
  DO4(buf, i);      \
  DO4(buf, i + 4);
#define DO16(buf) \
  DO8(buf, 0);    \
  DO8(buf, 8);
#define MOD(a) a %= BASE
#define MOD28(a) a %= BASE
#define MOD63(a) a %= BASE

unsigned int adler32_z(unsigned int adler, const unsigned char* buf,
                       unsigned int len) {
  unsigned long sum2;
  unsigned n;

  /* split Adler-32 into component sums */
  sum2 = (adler >> 16) & 0xffff;
  adler &= 0xffff;

  /* in case user likes doing a byte at a time, keep it fast */
  if (len == 1) {
    adler += buf[0];
    if (adler >= BASE) adler -= BASE;
    sum2 += adler;
    if (sum2 >= BASE) sum2 -= BASE;
    return adler | (sum2 << 16);
  }

  /* initial Adler-32 value (deferred check for len == 1 speed) */
  if (buf == NULL) return 1L;

  /* in case short lengths are provided, keep it somewhat fast */
  if (len < 16) {
    while (len--) {
      adler += *buf++;
      sum2 += adler;
    }
    if (adler >= BASE) adler -= BASE;
    MOD28(sum2); /* only added so many BASE's */
    return adler | (sum2 << 16);
  }

  /* do length NMAX blocks -- requires just one modulo operation */
  while (len >= NMAX) {
    len -= NMAX;
    n = NMAX / 16; /* NMAX is divisible by 16 */
    do {
      DO16(buf); /* 16 sums unrolled */
      buf += 16;
    } while (--n);
    MOD(adler);
    MOD(sum2);
  }

  /* do remaining bytes (less than NMAX, still just one modulo) */
  if (len) { /* avoid modulos if none remaining */
    while (len >= 16) {
      len -= 16;
      DO16(buf);
      buf += 16;
    }
    while (len--) {
      adler += *buf++;
      sum2 += adler;
    }
    MOD(adler);
    MOD(sum2);
  }

  /* return recombined sums */
  return adler | (sum2 << 16);
}

#endif /* !_ADLER_32_H */
