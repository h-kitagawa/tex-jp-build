/* This is dvipdfmx, an eXtended version of dvipdfm by Mark A. Wicks.

    Copyright (C) 2002-2020 by Jin-Hwan Cho and Shunsaku Hirata,
    the dvipdfmx project team.

    Copyright (C) 1998, 1999 by Mark A. Wicks <mwicks@kettering.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/

/*
 * Unicode related:
 *  Conversion between UTF-* and UCS-*.
 *  ToUnicode CMap
 *
 * Normalization?
 *
 * I made some unused functions here for completeness and FUTURE USE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "system.h"
#include "mem.h"
#include "error.h"

#include "unicode.h"

#define UC_DEBUG     3
#define UC_DEBUG_STR "UC"

#define UC_REPLACEMENT_CHAR 0x0000FFFD

#define UC_SUR_SHIFT      10
#define UC_SUR_MASK       0x03FFU
#define UC_SUR_LOW_START  0xDC00U
#define UC_SUR_HIGH_START 0xD800U
#define UC_SUR_END        0xE000U

int
UC_is_valid (int32_t ucv)
{
  if ( ucv < 0 || ucv > 0x10FFFFL ||
      (ucv >= 0x0000D800L && ucv <= 0x0000DFFFL))
    return 0;
  return 1;
}

int
UC_UTF16BE_is_valid_string (const unsigned char *p, const unsigned char *endptr)
{
  if (p + 1 >= endptr)
   return 0;
  while (p < endptr) {
    int32_t ucv = UC_UTF16BE_decode_char(&p, endptr);
    if (!UC_is_valid(ucv))
      return 0;
  }
  return 1;
}

int
UC_UTF8_is_valid_string (const unsigned char *p, const unsigned char *endptr)
{
  if (p >= endptr)
   return 0;
  while (p < endptr) {
    int32_t ucv = UC_UTF8_decode_char(&p, endptr);
    if (!UC_is_valid(ucv))
      return 0;
  }
  return 1;
}

int32_t
UC_UTF16BE_decode_char (const unsigned char **pp, const unsigned char *endptr)
{
  const unsigned char *p = *pp;
  int32_t  ucv = -1;
  uint16_t first, second;

  if (p + 1 >= endptr)
    return -1;

  first = ((p[0]) << 8|p[1]); p += 2;
  if (first >= UC_SUR_HIGH_START && first < UC_SUR_LOW_START) {
    if (p + 1 >= endptr)
      return -1;
    second = (p[0] << 8|p[1]); p += 2;
    ucv    =  second & UC_SUR_MASK;
    ucv   |= (first  & UC_SUR_MASK) << UC_SUR_SHIFT;
    ucv   += 0x00010000;
  } else if (first >= UC_SUR_LOW_START && first < UC_SUR_END) {
    return -1;
  } else {
    ucv = first;
  }

  *pp = p;
  return ucv;
}

size_t
UC_UTF16BE_encode_char (int32_t ucv, unsigned char **pp, unsigned char *endptr)
{
  int count = 0;
  unsigned char *p = *pp;

  if (ucv >= 0 && ucv <= 0xFFFF) {
    if (p + 2 > endptr)
      return 0;
    p[0] = (ucv >> 8) & 0xff;
    p[1] = ucv & 0xff;
    count = 2;
  } else if (ucv >= 0x010000 && ucv <= 0x10FFFF) {
    unsigned short high, low;

    if (p + 4 > endptr)
      return 0;
    ucv  -= 0x00010000;
    high = (ucv >> UC_SUR_SHIFT) + UC_SUR_HIGH_START;
    low  = (ucv &  UC_SUR_MASK)  + UC_SUR_LOW_START;
    p[0] = (high >> 8) & 0xff;
    p[1] = (high & 0xff);
    p[2] = (low >> 8) & 0xff;
    p[3] = (low & 0xff);
    count = 4;
  } else {
    if (p + 2 > endptr)
      return 0;
    p[0] = (UC_REPLACEMENT_CHAR >> 8) & 0xff;
    p[1] = (UC_REPLACEMENT_CHAR & 0xff);
    count = 2;
  }

  *pp += count;
  return count;
}

int32_t
UC_UTF8_decode_char (const unsigned char **pp, const unsigned char *endptr)
{
  const unsigned char *p = *pp;
  int32_t  ucv;
  unsigned char c = *p++;
  int      nbytes;

  if (c <= 0x7f) {
    ucv    = c;
    nbytes = 0;
  } else if ((c & 0xe0) == 0xc0) { /* 110x xxxx */
    ucv    = c & 31;
    nbytes = 1;
  } else if ((c & 0xf0) == 0xe0) { /* 1110 xxxx */
    ucv    = c & 0x0f;
    nbytes = 2;
  } else if ((c & 0xf8) == 0xf0) { /* 1111 0xxx */
    ucv    = c & 0x07;
    nbytes = 3;
  } else if ((c & 0xfc) == 0xf8) { /* 1111 10xx */
    ucv    = c & 0x03;
    nbytes = 4;
  } else if ((c & 0xfe) == 0xfc) { /* 1111 110x */
    ucv    = c & 0x01;
    nbytes = 5;
  } else {
    return -1;
  }
  if (p + nbytes > endptr)
    return -1;
  while (nbytes-- > 0) {
    c = *p++;
    if ((c & 0xc0) != 0x80)
      return -1;
    ucv = (ucv << 6) | (c & 0x3f);
  }

  *pp = p;
  return ucv;
}

size_t
UC_UTF8_encode_char (int32_t ucv, unsigned char **pp, unsigned char *endptr)
{
  int  count = 0;
  unsigned char *p = *pp;

  ASSERT( pp && *pp && endptr );

  if (!UC_is_valid(ucv))
    return 0;

  if (ucv < 0x7f) {
    if (p + 1 > endptr)
      return 0;
    p[0]  = (unsigned char) ucv;
    count = 1;
  } else if (ucv <= 0x7ff) {
    if (p + 2 > endptr)
      return 0;
    p[0] = (unsigned char) (0xc0 | (ucv >> 6));
    p[1] = (unsigned char) (0x80 | (ucv & 0x3f));
    count = 2;
  } else if (ucv <= 0xffff) {
    if (p + 3 > endptr)
      return 0;
    p[0] = (unsigned char) (0xe0 | (ucv >> 12));
    p[1] = (unsigned char) (0x80 | ((ucv >> 6) & 0x3f));
    p[2] = (unsigned char) (0x80 | (ucv & 0x3f));
    count = 3;
  } else if (ucv <= 0x1fffff) {
    if (p + 4 > endptr)
      return 0;
    p[0] = (unsigned char) (0xf0 | (ucv >> 18));
    p[1] = (unsigned char) (0x80 | ((ucv >> 12) & 0x3f));
    p[2] = (unsigned char) (0x80 | ((ucv >>  6) & 0x3f));
    p[3] = (unsigned char) (0x80 | (ucv & 0x3f));
    count = 4;
  } else if (ucv <= 0x3ffffff) {
    if (p + 5 > endptr)
      return 0;
    p[0] = (unsigned char) (0xf8 | (ucv >> 24));
    p[1] = (unsigned char) (0x80 | ((ucv >> 18) & 0x3f));
    p[2] = (unsigned char) (0x80 | ((ucv >> 12) & 0x3f));
    p[3] = (unsigned char) (0x80 | ((ucv >>  6) & 0x3f));
    p[4] = (unsigned char) (0x80 | (ucv & 0x3f));
    count = 5;
  } else if (ucv <= 0x7fffffff) {
     if (p + 6 > endptr)
      return 0;
    p[0] = (unsigned char) (0xfc | (ucv >> 30));
    p[1] = (unsigned char) (0x80 | ((ucv >> 24) & 0x3f));
    p[2] = (unsigned char) (0x80 | ((ucv >> 18) & 0x3f));
    p[3] = (unsigned char) (0x80 | ((ucv >> 12) & 0x3f));
    p[4] = (unsigned char) (0x80 | ((ucv >>  6) & 0x3f));
    p[5] = (unsigned char) (0x80 | (ucv & 0x3f));
    count = 6;
  }

  *pp += count;
  return count;
}

int32_t
UC_Combine_CJK_compatibility_ideograph (int32_t ucv, int32_t uvs)
{
  /* https://www.unicode.org/Public/UCD/latest/ucd/StandardizedVariants.txt */
  static const int32_t CJK_compatibility_ideographs[][3] = {
    {0x349E, 0xFE00, 0x2F80C},
    {0x34B9, 0xFE00, 0x2F813},
    {0x34BB, 0xFE00, 0x2F9CA},
    {0x34DF, 0xFE00, 0x2F81F},
    {0x3515, 0xFE00, 0x2F824},
    {0x36EE, 0xFE00, 0x2F867},
    {0x36FC, 0xFE00, 0x2F868},
    {0x3781, 0xFE00, 0x2F876},
    {0x382F, 0xFE00, 0x2F883},
    {0x3862, 0xFE00, 0x2F888},
    {0x387C, 0xFE00, 0x2F88A},
    {0x38C7, 0xFE00, 0x2F896},
    {0x38E3, 0xFE00, 0x2F89B},
    {0x391C, 0xFE00, 0x2F8A2},
    {0x393A, 0xFE00, 0x2F8A1},
    {0x3A2E, 0xFE00, 0x2F8C2},
    {0x3A6C, 0xFE00, 0x2F8C7},
    {0x3AE4, 0xFE00, 0x2F8D1},
    {0x3B08, 0xFE00, 0x2F8D0},
    {0x3B19, 0xFE00, 0x2F8CE},
    {0x3B49, 0xFE00, 0x2F8DE},
    {0x3B9D, 0xFE00, 0xFAD2},
    {0x3B9D, 0xFE01, 0x2F8E7},
    {0x3C18, 0xFE00, 0x2F8EE},
    {0x3C4E, 0xFE00, 0x2F8F2},
    {0x3D33, 0xFE00, 0x2F90A},
    {0x3D96, 0xFE00, 0x2F916},
    {0x3EAC, 0xFE00, 0x2F92A},
    {0x3EB8, 0xFE00, 0x2F92C},
    {0x3EB8, 0xFE01, 0x2F92D},
    {0x3F1B, 0xFE00, 0x2F933},
    {0x3FFC, 0xFE00, 0x2F93E},
    {0x4008, 0xFE00, 0x2F93F},
    {0x4018, 0xFE00, 0xFAD3},
    {0x4039, 0xFE00, 0xFAD4},
    {0x4039, 0xFE01, 0x2F949},
    {0x4046, 0xFE00, 0x2F94B},
    {0x4096, 0xFE00, 0x2F94C},
    {0x40E3, 0xFE00, 0x2F951},
    {0x412F, 0xFE00, 0x2F958},
    {0x4202, 0xFE00, 0x2F960},
    {0x4227, 0xFE00, 0x2F964},
    {0x42A0, 0xFE00, 0x2F967},
    {0x4301, 0xFE00, 0x2F96D},
    {0x4334, 0xFE00, 0x2F971},
    {0x4359, 0xFE00, 0x2F974},
    {0x43D5, 0xFE00, 0x2F981},
    {0x43D9, 0xFE00, 0x2F8D7},
    {0x440B, 0xFE00, 0x2F984},
    {0x446B, 0xFE00, 0x2F98E},
    {0x452B, 0xFE00, 0x2F9A7},
    {0x455D, 0xFE00, 0x2F9AE},
    {0x4561, 0xFE00, 0x2F9AF},
    {0x456B, 0xFE00, 0x2F9B2},
    {0x45D7, 0xFE00, 0x2F9BF},
    {0x45F9, 0xFE00, 0x2F9C2},
    {0x4635, 0xFE00, 0x2F9C8},
    {0x46BE, 0xFE00, 0x2F9CD},
    {0x46C7, 0xFE00, 0x2F9CE},
    {0x4995, 0xFE00, 0x2F9EF},
    {0x49E6, 0xFE00, 0x2F9F2},
    {0x4A6E, 0xFE00, 0x2F9F8},
    {0x4A76, 0xFE00, 0x2F9F9},
    {0x4AB2, 0xFE00, 0x2F9FC},
    {0x4B33, 0xFE00, 0x2FA03},
    {0x4BCE, 0xFE00, 0x2FA08},
    {0x4CCE, 0xFE00, 0x2FA0D},
    {0x4CED, 0xFE00, 0x2FA0E},
    {0x4CF8, 0xFE00, 0x2FA11},
    {0x4D56, 0xFE00, 0x2FA16},
    {0x4E0D, 0xFE00, 0xF967},
    {0x4E26, 0xFE00, 0xFA70},
    {0x4E32, 0xFE00, 0xF905},
    {0x4E38, 0xFE00, 0x2F801},
    {0x4E39, 0xFE00, 0xF95E},
    {0x4E3D, 0xFE00, 0x2F800},
    {0x4E41, 0xFE00, 0x2F802},
    {0x4E82, 0xFE00, 0xF91B},
    {0x4E86, 0xFE00, 0xF9BA},
    {0x4EAE, 0xFE00, 0xF977},
    {0x4EC0, 0xFE00, 0xF9FD},
    {0x4ECC, 0xFE00, 0x2F819},
    {0x4EE4, 0xFE00, 0xF9A8},
    {0x4F60, 0xFE00, 0x2F804},
    {0x4F80, 0xFE00, 0xFA73},
    {0x4F86, 0xFE00, 0xF92D},
    {0x4F8B, 0xFE00, 0xF9B5},
    {0x4FAE, 0xFE00, 0xFA30},
    {0x4FAE, 0xFE01, 0x2F805},
    {0x4FBB, 0xFE00, 0x2F806},
    {0x4FBF, 0xFE00, 0xF965},
    {0x5002, 0xFE00, 0x2F807},
    {0x502B, 0xFE00, 0xF9D4},
    {0x507A, 0xFE00, 0x2F808},
    {0x5099, 0xFE00, 0x2F809},
    {0x50CF, 0xFE00, 0x2F80B},
    {0x50DA, 0xFE00, 0xF9BB},
    {0x50E7, 0xFE00, 0xFA31},
    {0x50E7, 0xFE01, 0x2F80A},
    {0x5140, 0xFE00, 0xFA0C},
    {0x5145, 0xFE00, 0xFA74},
    {0x514D, 0xFE00, 0xFA32},
    {0x514D, 0xFE01, 0x2F80E},
    {0x5154, 0xFE00, 0x2F80F},
    {0x5164, 0xFE00, 0x2F810},
    {0x5167, 0xFE00, 0x2F814},
    {0x5168, 0xFE00, 0xFA72},
    {0x5169, 0xFE00, 0xF978},
    {0x516D, 0xFE00, 0xF9D1},
    {0x5177, 0xFE00, 0x2F811},
    {0x5180, 0xFE00, 0xFA75},
    {0x518D, 0xFE00, 0x2F815},
    {0x5192, 0xFE00, 0x2F8D2},
    {0x5195, 0xFE00, 0x2F8D3},
    {0x5197, 0xFE00, 0x2F817},
    {0x51A4, 0xFE00, 0x2F818},
    {0x51AC, 0xFE00, 0x2F81A},
    {0x51B5, 0xFE00, 0xFA71},
    {0x51B5, 0xFE01, 0x2F81B},
    {0x51B7, 0xFE00, 0xF92E},
    {0x51C9, 0xFE00, 0xF979},
    {0x51CC, 0xFE00, 0xF955},
    {0x51DC, 0xFE00, 0xF954},
    {0x51DE, 0xFE00, 0xFA15},
    {0x51F5, 0xFE00, 0x2F81D},
    {0x5203, 0xFE00, 0x2F81E},
    {0x5207, 0xFE00, 0xFA00},
    {0x5207, 0xFE01, 0x2F850},
    {0x5217, 0xFE00, 0xF99C},
    {0x5229, 0xFE00, 0xF9DD},
    {0x523A, 0xFE00, 0xF9FF},
    {0x523B, 0xFE00, 0x2F820},
    {0x5246, 0xFE00, 0x2F821},
    {0x5272, 0xFE00, 0x2F822},
    {0x5277, 0xFE00, 0x2F823},
    {0x5289, 0xFE00, 0xF9C7},
    {0x529B, 0xFE00, 0xF98A},
    {0x52A3, 0xFE00, 0xF99D},
    {0x52B3, 0xFE00, 0x2F992},
    {0x52C7, 0xFE00, 0xFA76},
    {0x52C7, 0xFE01, 0x2F825},
    {0x52C9, 0xFE00, 0xFA33},
    {0x52C9, 0xFE01, 0x2F826},
    {0x52D2, 0xFE00, 0xF952},
    {0x52DE, 0xFE00, 0xF92F},
    {0x52E4, 0xFE00, 0xFA34},
    {0x52E4, 0xFE01, 0x2F827},
    {0x52F5, 0xFE00, 0xF97F},
    {0x52FA, 0xFE00, 0xFA77},
    {0x52FA, 0xFE01, 0x2F828},
    {0x5305, 0xFE00, 0x2F829},
    {0x5306, 0xFE00, 0x2F82A},
    {0x5317, 0xFE00, 0xF963},
    {0x5317, 0xFE01, 0x2F82B},
    {0x533F, 0xFE00, 0xF9EB},
    {0x5349, 0xFE00, 0x2F82C},
    {0x5351, 0xFE00, 0xFA35},
    {0x5351, 0xFE01, 0x2F82D},
    {0x535A, 0xFE00, 0x2F82E},
    {0x5373, 0xFE00, 0x2F82F},
    {0x5375, 0xFE00, 0xF91C},
    {0x537D, 0xFE00, 0x2F830},
    {0x537F, 0xFE00, 0x2F831},
    {0x537F, 0xFE01, 0x2F832},
    {0x537F, 0xFE02, 0x2F833},
    {0x53C3, 0xFE00, 0xF96B},
    {0x53CA, 0xFE00, 0x2F836},
    {0x53DF, 0xFE00, 0x2F837},
    {0x53E5, 0xFE00, 0xF906},
    {0x53EB, 0xFE00, 0x2F839},
    {0x53F1, 0xFE00, 0x2F83A},
    {0x5406, 0xFE00, 0x2F83B},
    {0x540F, 0xFE00, 0xF9DE},
    {0x541D, 0xFE00, 0xF9ED},
    {0x5438, 0xFE00, 0x2F83D},
    {0x5442, 0xFE00, 0xF980},
    {0x5448, 0xFE00, 0x2F83E},
    {0x5468, 0xFE00, 0x2F83F},
    {0x549E, 0xFE00, 0x2F83C},
    {0x54A2, 0xFE00, 0x2F840},
    {0x54BD, 0xFE00, 0xF99E},
    {0x54F6, 0xFE00, 0x2F841},
    {0x5510, 0xFE00, 0x2F842},
    {0x5553, 0xFE00, 0x2F843},
    {0x5555, 0xFE00, 0xFA79},
    {0x5563, 0xFE00, 0x2F844},
    {0x5584, 0xFE00, 0x2F845},
    {0x5584, 0xFE01, 0x2F846},
    {0x5587, 0xFE00, 0xF90B},
    {0x5599, 0xFE00, 0xFA7A},
    {0x5599, 0xFE01, 0x2F847},
    {0x559D, 0xFE00, 0xFA36},
    {0x559D, 0xFE01, 0xFA78},
    {0x55AB, 0xFE00, 0x2F848},
    {0x55B3, 0xFE00, 0x2F849},
    {0x55C0, 0xFE00, 0xFA0D},
    {0x55C2, 0xFE00, 0x2F84A},
    {0x55E2, 0xFE00, 0xFA7B},
    {0x5606, 0xFE00, 0xFA37},
    {0x5606, 0xFE01, 0x2F84C},
    {0x5651, 0xFE00, 0x2F84E},
    {0x5668, 0xFE00, 0xFA38},
    {0x5674, 0xFE00, 0x2F84F},
    {0x56F9, 0xFE00, 0xF9A9},
    {0x5716, 0xFE00, 0x2F84B},
    {0x5717, 0xFE00, 0x2F84D},
    {0x578B, 0xFE00, 0x2F855},
    {0x57CE, 0xFE00, 0x2F852},
    {0x57F4, 0xFE00, 0x2F853},
    {0x580D, 0xFE00, 0x2F854},
    {0x5831, 0xFE00, 0x2F857},
    {0x5832, 0xFE00, 0x2F856},
    {0x5840, 0xFE00, 0xFA39},
    {0x585A, 0xFE00, 0xFA10},
    {0x585A, 0xFE01, 0xFA7C},
    {0x585E, 0xFE00, 0xF96C},
    {0x58A8, 0xFE00, 0xFA3A},
    {0x58AC, 0xFE00, 0x2F858},
    {0x58B3, 0xFE00, 0xFA7D},
    {0x58D8, 0xFE00, 0xF94A},
    {0x58DF, 0xFE00, 0xF942},
    {0x58EE, 0xFE00, 0x2F851},
    {0x58F2, 0xFE00, 0x2F85A},
    {0x58F7, 0xFE00, 0x2F85B},
    {0x5906, 0xFE00, 0x2F85C},
    {0x591A, 0xFE00, 0x2F85D},
    {0x5922, 0xFE00, 0x2F85E},
    {0x5944, 0xFE00, 0xFA7E},
    {0x5948, 0xFE00, 0xF90C},
    {0x5951, 0xFE00, 0xF909},
    {0x5954, 0xFE00, 0xFA7F},
    {0x5962, 0xFE00, 0x2F85F},
    {0x5973, 0xFE00, 0xF981},
    {0x59D8, 0xFE00, 0x2F865},
    {0x59EC, 0xFE00, 0x2F862},
    {0x5A1B, 0xFE00, 0x2F863},
    {0x5A27, 0xFE00, 0x2F864},
    {0x5A62, 0xFE00, 0xFA80},
    {0x5A66, 0xFE00, 0x2F866},
    {0x5AB5, 0xFE00, 0x2F986},
    {0x5B08, 0xFE00, 0x2F869},
    {0x5B28, 0xFE00, 0xFA81},
    {0x5B3E, 0xFE00, 0x2F86A},
    {0x5B3E, 0xFE01, 0x2F86B},
    {0x5B85, 0xFE00, 0xFA04},
    {0x5BC3, 0xFE00, 0x2F86D},
    {0x5BD8, 0xFE00, 0x2F86E},
    {0x5BE7, 0xFE00, 0xF95F},
    {0x5BE7, 0xFE01, 0xF9AA},
    {0x5BE7, 0xFE02, 0x2F86F},
    {0x5BEE, 0xFE00, 0xF9BC},
    {0x5BF3, 0xFE00, 0x2F870},
    {0x5BFF, 0xFE00, 0x2F872},
    {0x5C06, 0xFE00, 0x2F873},
    {0x5C22, 0xFE00, 0x2F875},
    {0x5C3F, 0xFE00, 0xF9BD},
    {0x5C60, 0xFE00, 0x2F877},
    {0x5C62, 0xFE00, 0xF94B},
    {0x5C64, 0xFE00, 0xFA3B},
    {0x5C65, 0xFE00, 0xF9DF},
    {0x5C6E, 0xFE00, 0xFA3C},
    {0x5C6E, 0xFE01, 0x2F878},
    {0x5C8D, 0xFE00, 0x2F87A},
    {0x5CC0, 0xFE00, 0x2F879},
    {0x5D19, 0xFE00, 0xF9D5},
    {0x5D43, 0xFE00, 0x2F87C},
    {0x5D50, 0xFE00, 0xF921},
    {0x5D6B, 0xFE00, 0x2F87F},
    {0x5D6E, 0xFE00, 0x2F87E},
    {0x5D7C, 0xFE00, 0x2F880},
    {0x5DB2, 0xFE00, 0x2F9F4},
    {0x5DBA, 0xFE00, 0xF9AB},
    {0x5DE1, 0xFE00, 0x2F881},
    {0x5DE2, 0xFE00, 0x2F882},
    {0x5DFD, 0xFE00, 0x2F884},
    {0x5E28, 0xFE00, 0x2F885},
    {0x5E3D, 0xFE00, 0x2F886},
    {0x5E69, 0xFE00, 0x2F887},
    {0x5E74, 0xFE00, 0xF98E},
    {0x5EA6, 0xFE00, 0xFA01},
    {0x5EB0, 0xFE00, 0x2F88B},
    {0x5EB3, 0xFE00, 0x2F88C},
    {0x5EB6, 0xFE00, 0x2F88D},
    {0x5EC9, 0xFE00, 0xF9A2},
    {0x5ECA, 0xFE00, 0xF928},
    {0x5ECA, 0xFE01, 0x2F88E},
    {0x5ED2, 0xFE00, 0xFA82},
    {0x5ED3, 0xFE00, 0xFA0B},
    {0x5ED9, 0xFE00, 0xFA83},
    {0x5EEC, 0xFE00, 0xF982},
    {0x5EFE, 0xFE00, 0x2F890},
    {0x5F04, 0xFE00, 0xF943},
    {0x5F22, 0xFE00, 0x2F894},
    {0x5F22, 0xFE01, 0x2F895},
    {0x5F53, 0xFE00, 0x2F874},
    {0x5F62, 0xFE00, 0x2F899},
    {0x5F69, 0xFE00, 0xFA84},
    {0x5F6B, 0xFE00, 0x2F89A},
    {0x5F8B, 0xFE00, 0xF9D8},
    {0x5F9A, 0xFE00, 0x2F89C},
    {0x5FA9, 0xFE00, 0xF966},
    {0x5FAD, 0xFE00, 0xFA85},
    {0x5FCD, 0xFE00, 0x2F89D},
    {0x5FD7, 0xFE00, 0x2F89E},
    {0x5FF5, 0xFE00, 0xF9A3},
    {0x5FF9, 0xFE00, 0x2F89F},
    {0x6012, 0xFE00, 0xF960},
    {0x601C, 0xFE00, 0xF9AC},
    {0x6075, 0xFE00, 0xFA6B},
    {0x6081, 0xFE00, 0x2F8A0},
    {0x6094, 0xFE00, 0xFA3D},
    {0x6094, 0xFE01, 0x2F8A3},
    {0x60C7, 0xFE00, 0x2F8A5},
    {0x60D8, 0xFE00, 0xFA86},
    {0x60E1, 0xFE00, 0xF9B9},
    {0x6108, 0xFE00, 0xFA88},
    {0x6144, 0xFE00, 0xF9D9},
    {0x6148, 0xFE00, 0x2F8A6},
    {0x614C, 0xFE00, 0x2F8A7},
    {0x614C, 0xFE01, 0x2F8A9},
    {0x614E, 0xFE00, 0xFA87},
    {0x614E, 0xFE01, 0x2F8A8},
    {0x6160, 0xFE00, 0xFA8A},
    {0x6168, 0xFE00, 0xFA3E},
    {0x617A, 0xFE00, 0x2F8AA},
    {0x618E, 0xFE00, 0xFA3F},
    {0x618E, 0xFE01, 0xFA89},
    {0x618E, 0xFE02, 0x2F8AB},
    {0x6190, 0xFE00, 0xF98F},
    {0x61A4, 0xFE00, 0x2F8AD},
    {0x61AF, 0xFE00, 0x2F8AE},
    {0x61B2, 0xFE00, 0x2F8AC},
    {0x61DE, 0xFE00, 0x2F8AF},
    {0x61F2, 0xFE00, 0xFA40},
    {0x61F2, 0xFE01, 0xFA8B},
    {0x61F2, 0xFE02, 0x2F8B0},
    {0x61F6, 0xFE00, 0xF90D},
    {0x61F6, 0xFE01, 0x2F8B1},
    {0x6200, 0xFE00, 0xF990},
    {0x6210, 0xFE00, 0x2F8B2},
    {0x621B, 0xFE00, 0x2F8B3},
    {0x622E, 0xFE00, 0xF9D2},
    {0x6234, 0xFE00, 0xFA8C},
    {0x625D, 0xFE00, 0x2F8B4},
    {0x62B1, 0xFE00, 0x2F8B5},
    {0x62C9, 0xFE00, 0xF925},
    {0x62CF, 0xFE00, 0xF95B},
    {0x62D3, 0xFE00, 0xFA02},
    {0x62D4, 0xFE00, 0x2F8B6},
    {0x62FC, 0xFE00, 0x2F8BA},
    {0x62FE, 0xFE00, 0xF973},
    {0x633D, 0xFE00, 0x2F8B9},
    {0x6350, 0xFE00, 0x2F8B7},
    {0x6368, 0xFE00, 0x2F8BB},
    {0x637B, 0xFE00, 0xF9A4},
    {0x6383, 0xFE00, 0x2F8BC},
    {0x63A0, 0xFE00, 0xF975},
    {0x63A9, 0xFE00, 0x2F8C1},
    {0x63C4, 0xFE00, 0xFA8D},
    {0x63C5, 0xFE00, 0x2F8C0},
    {0x63E4, 0xFE00, 0x2F8BD},
    {0x641C, 0xFE00, 0xFA8E},
    {0x6422, 0xFE00, 0x2F8BF},
    {0x6452, 0xFE00, 0xFA8F},
    {0x6469, 0xFE00, 0x2F8C3},
    {0x6477, 0xFE00, 0x2F8C6},
    {0x647E, 0xFE00, 0x2F8C4},
    {0x649A, 0xFE00, 0xF991},
    {0x649D, 0xFE00, 0x2F8C5},
    {0x64C4, 0xFE00, 0xF930},
    {0x654F, 0xFE00, 0xFA41},
    {0x654F, 0xFE01, 0x2F8C8},
    {0x6556, 0xFE00, 0xFA90},
    {0x656C, 0xFE00, 0x2F8C9},
    {0x6578, 0xFE00, 0xF969},
    {0x6599, 0xFE00, 0xF9BE},
    {0x65C5, 0xFE00, 0xF983},
    {0x65E2, 0xFE00, 0xFA42},
    {0x65E3, 0xFE00, 0x2F8CB},
    {0x6613, 0xFE00, 0xF9E0},
    {0x6649, 0xFE00, 0x2F8CD},
    {0x6674, 0xFE00, 0xFA12},
    {0x6674, 0xFE01, 0xFA91},
    {0x6688, 0xFE00, 0xF9C5},
    {0x6691, 0xFE00, 0xFA43},
    {0x6691, 0xFE01, 0x2F8CF},
    {0x669C, 0xFE00, 0x2F8D5},
    {0x66B4, 0xFE00, 0xFA06},
    {0x66C6, 0xFE00, 0xF98B},
    {0x66F4, 0xFE00, 0xF901},
    {0x66F8, 0xFE00, 0x2F8CC},
    {0x6700, 0xFE00, 0x2F8D4},
    {0x6717, 0xFE00, 0xF929},
    {0x6717, 0xFE01, 0xFA92},
    {0x6717, 0xFE02, 0x2F8D8},
    {0x671B, 0xFE00, 0xFA93},
    {0x671B, 0xFE01, 0x2F8D9},
    {0x6721, 0xFE00, 0x2F8DA},
    {0x674E, 0xFE00, 0xF9E1},
    {0x6753, 0xFE00, 0x2F8DC},
    {0x6756, 0xFE00, 0xFA94},
    {0x675E, 0xFE00, 0x2F8DB},
    {0x677B, 0xFE00, 0xF9C8},
    {0x6785, 0xFE00, 0x2F8E0},
    {0x6797, 0xFE00, 0xF9F4},
    {0x67F3, 0xFE00, 0xF9C9},
    {0x67FA, 0xFE00, 0x2F8DF},
    {0x6817, 0xFE00, 0xF9DA},
    {0x681F, 0xFE00, 0x2F8E5},
    {0x6852, 0xFE00, 0x2F8E1},
    {0x6881, 0xFE00, 0xF97A},
    {0x6885, 0xFE00, 0xFA44},
    {0x6885, 0xFE01, 0x2F8E2},
    {0x688E, 0xFE00, 0x2F8E4},
    {0x68A8, 0xFE00, 0xF9E2},
    {0x6914, 0xFE00, 0x2F8E6},
    {0x6942, 0xFE00, 0x2F8E8},
    {0x69A3, 0xFE00, 0x2F8E9},
    {0x69EA, 0xFE00, 0x2F8EA},
    {0x6A02, 0xFE00, 0xF914},
    {0x6A02, 0xFE01, 0xF95C},
    {0x6A02, 0xFE02, 0xF9BF},
    {0x6A13, 0xFE00, 0xF94C},
    {0x6AA8, 0xFE00, 0x2F8EB},
    {0x6AD3, 0xFE00, 0xF931},
    {0x6ADB, 0xFE00, 0x2F8ED},
    {0x6B04, 0xFE00, 0xF91D},
    {0x6B21, 0xFE00, 0x2F8EF},
    {0x6B54, 0xFE00, 0x2F8F1},
    {0x6B72, 0xFE00, 0x2F8F3},
    {0x6B77, 0xFE00, 0xF98C},
    {0x6B79, 0xFE00, 0xFA95},
    {0x6B9F, 0xFE00, 0x2F8F4},
    {0x6BAE, 0xFE00, 0xF9A5},
    {0x6BBA, 0xFE00, 0xF970},
    {0x6BBA, 0xFE01, 0xFA96},
    {0x6BBA, 0xFE02, 0x2F8F5},
    {0x6BBB, 0xFE00, 0x2F8F6},
    {0x6C4E, 0xFE00, 0x2F8FA},
    {0x6C67, 0xFE00, 0x2F8FE},
    {0x6C88, 0xFE00, 0xF972},
    {0x6CBF, 0xFE00, 0x2F8FC},
    {0x6CCC, 0xFE00, 0xF968},
    {0x6CCD, 0xFE00, 0x2F8FD},
    {0x6CE5, 0xFE00, 0xF9E3},
    {0x6D16, 0xFE00, 0x2F8FF},
    {0x6D1B, 0xFE00, 0xF915},
    {0x6D1E, 0xFE00, 0xFA05},
    {0x6D34, 0xFE00, 0x2F907},
    {0x6D3E, 0xFE00, 0x2F900},
    {0x6D41, 0xFE00, 0xF9CA},
    {0x6D41, 0xFE01, 0xFA97},
    {0x6D41, 0xFE02, 0x2F902},
    {0x6D69, 0xFE00, 0x2F903},
    {0x6D6A, 0xFE00, 0xF92A},
    {0x6D77, 0xFE00, 0xFA45},
    {0x6D77, 0xFE01, 0x2F901},
    {0x6D78, 0xFE00, 0x2F904},
    {0x6D85, 0xFE00, 0x2F905},
    {0x6DCB, 0xFE00, 0xF9F5},
    {0x6DDA, 0xFE00, 0xF94D},
    {0x6DEA, 0xFE00, 0xF9D6},
    {0x6DF9, 0xFE00, 0x2F90E},
    {0x6E1A, 0xFE00, 0xFA46},
    {0x6E2F, 0xFE00, 0x2F908},
    {0x6E6E, 0xFE00, 0x2F909},
    {0x6E9C, 0xFE00, 0xF9CB},
    {0x6EBA, 0xFE00, 0xF9EC},
    {0x6EC7, 0xFE00, 0x2F90C},
    {0x6ECB, 0xFE00, 0xFA99},
    {0x6ECB, 0xFE01, 0x2F90B},
    {0x6ED1, 0xFE00, 0xF904},
    {0x6EDB, 0xFE00, 0xFA98},
    {0x6F0F, 0xFE00, 0xF94E},
    {0x6F22, 0xFE00, 0xFA47},
    {0x6F22, 0xFE01, 0xFA9A},
    {0x6F23, 0xFE00, 0xF992},
    {0x6F6E, 0xFE00, 0x2F90F},
    {0x6FC6, 0xFE00, 0x2F912},
    {0x6FEB, 0xFE00, 0xF922},
    {0x6FFE, 0xFE00, 0xF984},
    {0x701B, 0xFE00, 0x2F915},
    {0x701E, 0xFE00, 0xFA9B},
    {0x701E, 0xFE01, 0x2F914},
    {0x7039, 0xFE00, 0x2F913},
    {0x704A, 0xFE00, 0x2F917},
    {0x7070, 0xFE00, 0x2F835},
    {0x7077, 0xFE00, 0x2F919},
    {0x707D, 0xFE00, 0x2F918},
    {0x7099, 0xFE00, 0xF9FB},
    {0x70AD, 0xFE00, 0x2F91A},
    {0x70C8, 0xFE00, 0xF99F},
    {0x70D9, 0xFE00, 0xF916},
    {0x7145, 0xFE00, 0x2F91C},
    {0x7149, 0xFE00, 0xF993},
    {0x716E, 0xFE00, 0xFA48},
    {0x716E, 0xFE01, 0xFA9C},
    {0x719C, 0xFE00, 0x2F91E},
    {0x71CE, 0xFE00, 0xF9C0},
    {0x71D0, 0xFE00, 0xF9EE},
    {0x7210, 0xFE00, 0xF932},
    {0x721B, 0xFE00, 0xF91E},
    {0x7228, 0xFE00, 0x2F920},
    {0x722B, 0xFE00, 0xFA49},
    {0x7235, 0xFE00, 0xFA9E},
    {0x7235, 0xFE01, 0x2F921},
    {0x7250, 0xFE00, 0x2F922},
    {0x7262, 0xFE00, 0xF946},
    {0x7280, 0xFE00, 0x2F924},
    {0x7295, 0xFE00, 0x2F925},
    {0x72AF, 0xFE00, 0xFA9F},
    {0x72C0, 0xFE00, 0xF9FA},
    {0x72FC, 0xFE00, 0xF92B},
    {0x732A, 0xFE00, 0xFA16},
    {0x732A, 0xFE01, 0xFAA0},
    {0x7375, 0xFE00, 0xF9A7},
    {0x737A, 0xFE00, 0x2F928},
    {0x7387, 0xFE00, 0xF961},
    {0x7387, 0xFE01, 0xF9DB},
    {0x738B, 0xFE00, 0x2F929},
    {0x73A5, 0xFE00, 0x2F92B},
    {0x73B2, 0xFE00, 0xF9AD},
    {0x73DE, 0xFE00, 0xF917},
    {0x7406, 0xFE00, 0xF9E4},
    {0x7409, 0xFE00, 0xF9CC},
    {0x7422, 0xFE00, 0xFA4A},
    {0x7447, 0xFE00, 0x2F92E},
    {0x745C, 0xFE00, 0x2F92F},
    {0x7469, 0xFE00, 0xF9AE},
    {0x7471, 0xFE00, 0xFAA1},
    {0x7471, 0xFE01, 0x2F930},
    {0x7485, 0xFE00, 0x2F931},
    {0x7489, 0xFE00, 0xF994},
    {0x7498, 0xFE00, 0xF9EF},
    {0x74CA, 0xFE00, 0x2F932},
    {0x7506, 0xFE00, 0xFAA2},
    {0x7524, 0xFE00, 0x2F934},
    {0x753B, 0xFE00, 0xFAA3},
    {0x753E, 0xFE00, 0x2F936},
    {0x7559, 0xFE00, 0xF9CD},
    {0x7565, 0xFE00, 0xF976},
    {0x7570, 0xFE00, 0xF962},
    {0x7570, 0xFE01, 0x2F938},
    {0x75E2, 0xFE00, 0xF9E5},
    {0x7610, 0xFE00, 0x2F93A},
    {0x761D, 0xFE00, 0xFAA4},
    {0x761F, 0xFE00, 0xFAA5},
    {0x7642, 0xFE00, 0xF9C1},
    {0x7669, 0xFE00, 0xF90E},
    {0x76CA, 0xFE00, 0xFA17},
    {0x76CA, 0xFE01, 0xFAA6},
    {0x76DB, 0xFE00, 0xFAA7},
    {0x76E7, 0xFE00, 0xF933},
    {0x76F4, 0xFE00, 0xFAA8},
    {0x76F4, 0xFE01, 0x2F940},
    {0x7701, 0xFE00, 0xF96D},
    {0x771E, 0xFE00, 0x2F945},
    {0x771F, 0xFE00, 0x2F946},
    {0x771F, 0xFE01, 0x2F947},
    {0x7740, 0xFE00, 0xFAAA},
    {0x774A, 0xFE00, 0xFAA9},
    {0x774A, 0xFE01, 0x2F948},
    {0x778B, 0xFE00, 0x2F94A},
    {0x77A7, 0xFE00, 0xFA9D},
    {0x784E, 0xFE00, 0x2F94E},
    {0x786B, 0xFE00, 0xF9CE},
    {0x788C, 0xFE00, 0xF93B},
    {0x788C, 0xFE01, 0x2F94F},
    {0x7891, 0xFE00, 0xFA4B},
    {0x78CA, 0xFE00, 0xF947},
    {0x78CC, 0xFE00, 0xFAAB},
    {0x78CC, 0xFE01, 0x2F950},
    {0x78FB, 0xFE00, 0xF964},
    {0x792A, 0xFE00, 0xF985},
    {0x793C, 0xFE00, 0xFA18},
    {0x793E, 0xFE00, 0xFA4C},
    {0x7948, 0xFE00, 0xFA4E},
    {0x7949, 0xFE00, 0xFA4D},
    {0x7950, 0xFE00, 0xFA4F},
    {0x7956, 0xFE00, 0xFA50},
    {0x7956, 0xFE01, 0x2F953},
    {0x795D, 0xFE00, 0xFA51},
    {0x795E, 0xFE00, 0xFA19},
    {0x7965, 0xFE00, 0xFA1A},
    {0x797F, 0xFE00, 0xF93C},
    {0x798D, 0xFE00, 0xFA52},
    {0x798E, 0xFE00, 0xFA53},
    {0x798F, 0xFE00, 0xFA1B},
    {0x798F, 0xFE01, 0x2F956},
    {0x79AE, 0xFE00, 0xF9B6},
    {0x79CA, 0xFE00, 0xF995},
    {0x79EB, 0xFE00, 0x2F957},
    {0x7A1C, 0xFE00, 0xF956},
    {0x7A40, 0xFE00, 0xFA54},
    {0x7A40, 0xFE01, 0x2F959},
    {0x7A4A, 0xFE00, 0x2F95A},
    {0x7A4F, 0xFE00, 0x2F95B},
    {0x7A81, 0xFE00, 0xFA55},
    {0x7AB1, 0xFE00, 0xFAAC},
    {0x7ACB, 0xFE00, 0xF9F7},
    {0x7AEE, 0xFE00, 0x2F95F},
    {0x7B20, 0xFE00, 0xF9F8},
    {0x7BC0, 0xFE00, 0xFA56},
    {0x7BC0, 0xFE01, 0xFAAD},
    {0x7BC6, 0xFE00, 0x2F962},
    {0x7BC9, 0xFE00, 0x2F963},
    {0x7C3E, 0xFE00, 0xF9A6},
    {0x7C60, 0xFE00, 0xF944},
    {0x7C7B, 0xFE00, 0xFAAE},
    {0x7C92, 0xFE00, 0xF9F9},
    {0x7CBE, 0xFE00, 0xFA1D},
    {0x7CD2, 0xFE00, 0x2F966},
    {0x7CD6, 0xFE00, 0xFA03},
    {0x7CE3, 0xFE00, 0x2F969},
    {0x7CE7, 0xFE00, 0xF97B},
    {0x7CE8, 0xFE00, 0x2F968},
    {0x7D00, 0xFE00, 0x2F96A},
    {0x7D10, 0xFE00, 0xF9CF},
    {0x7D22, 0xFE00, 0xF96A},
    {0x7D2F, 0xFE00, 0xF94F},
    {0x7D5B, 0xFE00, 0xFAAF},
    {0x7D63, 0xFE00, 0x2F96C},
    {0x7DA0, 0xFE00, 0xF93D},
    {0x7DBE, 0xFE00, 0xF957},
    {0x7DC7, 0xFE00, 0x2F96E},
    {0x7DF4, 0xFE00, 0xF996},
    {0x7DF4, 0xFE01, 0xFA57},
    {0x7DF4, 0xFE02, 0xFAB0},
    {0x7E02, 0xFE00, 0x2F96F},
    {0x7E09, 0xFE00, 0xFA58},
    {0x7E37, 0xFE00, 0xF950},
    {0x7E41, 0xFE00, 0xFA59},
    {0x7E45, 0xFE00, 0x2F970},
    {0x7F3E, 0xFE00, 0xFAB1},
    {0x7F72, 0xFE00, 0xFA5A},
    {0x7F79, 0xFE00, 0xF9E6},
    {0x7F7A, 0xFE00, 0x2F976},
    {0x7F85, 0xFE00, 0xF90F},
    {0x7F95, 0xFE00, 0x2F978},
    {0x7F9A, 0xFE00, 0xF9AF},
    {0x7FBD, 0xFE00, 0xFA1E},
    {0x7FFA, 0xFE00, 0x2F979},
    {0x8001, 0xFE00, 0xF934},
    {0x8005, 0xFE00, 0xFA5B},
    {0x8005, 0xFE01, 0xFAB2},
    {0x8005, 0xFE02, 0x2F97A},
    {0x8046, 0xFE00, 0xF9B0},
    {0x8060, 0xFE00, 0x2F97D},
    {0x806F, 0xFE00, 0xF997},
    {0x8070, 0xFE00, 0x2F97F},
    {0x807E, 0xFE00, 0xF945},
    {0x808B, 0xFE00, 0xF953},
    {0x80AD, 0xFE00, 0x2F8D6},
    {0x80B2, 0xFE00, 0x2F982},
    {0x8103, 0xFE00, 0x2F983},
    {0x813E, 0xFE00, 0x2F985},
    {0x81D8, 0xFE00, 0xF926},
    {0x81E8, 0xFE00, 0xF9F6},
    {0x81ED, 0xFE00, 0xFA5C},
    {0x8201, 0xFE00, 0x2F893},
    {0x8201, 0xFE01, 0x2F98B},
    {0x8204, 0xFE00, 0x2F98C},
    {0x8218, 0xFE00, 0xFA6D},
    {0x826F, 0xFE00, 0xF97C},
    {0x8279, 0xFE00, 0xFA5D},
    {0x8279, 0xFE01, 0xFA5E},
    {0x828B, 0xFE00, 0x2F990},
    {0x8291, 0xFE00, 0x2F98F},
    {0x829D, 0xFE00, 0x2F991},
    {0x82B1, 0xFE00, 0x2F993},
    {0x82B3, 0xFE00, 0x2F994},
    {0x82BD, 0xFE00, 0x2F995},
    {0x82E5, 0xFE00, 0xF974},
    {0x82E5, 0xFE01, 0x2F998},
    {0x82E6, 0xFE00, 0x2F996},
    {0x831D, 0xFE00, 0x2F999},
    {0x8323, 0xFE00, 0x2F99C},
    {0x8336, 0xFE00, 0xF9FE},
    {0x8352, 0xFE00, 0xFAB3},
    {0x8353, 0xFE00, 0x2F9A0},
    {0x8363, 0xFE00, 0x2F99A},
    {0x83AD, 0xFE00, 0x2F99B},
    {0x83BD, 0xFE00, 0x2F99D},
    {0x83C9, 0xFE00, 0xF93E},
    {0x83CA, 0xFE00, 0x2F9A1},
    {0x83CC, 0xFE00, 0x2F9A2},
    {0x83DC, 0xFE00, 0x2F9A3},
    {0x83E7, 0xFE00, 0x2F99E},
    {0x83EF, 0xFE00, 0xFAB4},
    {0x83F1, 0xFE00, 0xF958},
    {0x843D, 0xFE00, 0xF918},
    {0x8449, 0xFE00, 0xF96E},
    {0x8457, 0xFE00, 0xFA5F},
    {0x8457, 0xFE01, 0x2F99F},
    {0x84EE, 0xFE00, 0xF999},
    {0x84F1, 0xFE00, 0x2F9A8},
    {0x84F3, 0xFE00, 0x2F9A9},
    {0x84FC, 0xFE00, 0xF9C2},
    {0x8516, 0xFE00, 0x2F9AA},
    {0x8564, 0xFE00, 0x2F9AC},
    {0x85CD, 0xFE00, 0xF923},
    {0x85FA, 0xFE00, 0xF9F0},
    {0x8606, 0xFE00, 0xF935},
    {0x8612, 0xFE00, 0xFA20},
    {0x862D, 0xFE00, 0xF91F},
    {0x863F, 0xFE00, 0xF910},
    {0x8650, 0xFE00, 0x2F9B3},
    {0x865C, 0xFE00, 0xF936},
    {0x865C, 0xFE01, 0x2F9B4},
    {0x8667, 0xFE00, 0x2F9B5},
    {0x8669, 0xFE00, 0x2F9B6},
    {0x8688, 0xFE00, 0x2F9B8},
    {0x86A9, 0xFE00, 0x2F9B7},
    {0x86E2, 0xFE00, 0x2F9BA},
    {0x870E, 0xFE00, 0x2F9B9},
    {0x8728, 0xFE00, 0x2F9BC},
    {0x876B, 0xFE00, 0x2F9BD},
    {0x8779, 0xFE00, 0xFAB5},
    {0x8779, 0xFE01, 0x2F9BB},
    {0x8786, 0xFE00, 0x2F9BE},
    {0x87BA, 0xFE00, 0xF911},
    {0x87E1, 0xFE00, 0x2F9C0},
    {0x8801, 0xFE00, 0x2F9C1},
    {0x881F, 0xFE00, 0xF927},
    {0x884C, 0xFE00, 0xFA08},
    {0x8860, 0xFE00, 0x2F9C3},
    {0x8863, 0xFE00, 0x2F9C4},
    {0x88C2, 0xFE00, 0xF9A0},
    {0x88CF, 0xFE00, 0xF9E7},
    {0x88D7, 0xFE00, 0x2F9C6},
    {0x88DE, 0xFE00, 0x2F9C7},
    {0x88E1, 0xFE00, 0xF9E8},
    {0x88F8, 0xFE00, 0xF912},
    {0x88FA, 0xFE00, 0x2F9C9},
    {0x8910, 0xFE00, 0xFA60},
    {0x8941, 0xFE00, 0xFAB6},
    {0x8964, 0xFE00, 0xF924},
    {0x8986, 0xFE00, 0xFAB7},
    {0x898B, 0xFE00, 0xFA0A},
    {0x8996, 0xFE00, 0xFA61},
    {0x8996, 0xFE01, 0xFAB8},
    {0x8AA0, 0xFE00, 0x2F9CF},
    {0x8AAA, 0xFE00, 0xF96F},
    {0x8AAA, 0xFE01, 0xF9A1},
    {0x8ABF, 0xFE00, 0xFAB9},
    {0x8ACB, 0xFE00, 0xFABB},
    {0x8AD2, 0xFE00, 0xF97D},
    {0x8AD6, 0xFE00, 0xF941},
    {0x8AED, 0xFE00, 0xFABE},
    {0x8AED, 0xFE01, 0x2F9D0},
    {0x8AF8, 0xFE00, 0xFA22},
    {0x8AF8, 0xFE01, 0xFABA},
    {0x8AFE, 0xFE00, 0xF95D},
    {0x8AFE, 0xFE01, 0xFABD},
    {0x8B01, 0xFE00, 0xFA62},
    {0x8B01, 0xFE01, 0xFABC},
    {0x8B39, 0xFE00, 0xFA63},
    {0x8B39, 0xFE01, 0xFABF},
    {0x8B58, 0xFE00, 0xF9FC},
    {0x8B80, 0xFE00, 0xF95A},
    {0x8B8A, 0xFE00, 0xFAC0},
    {0x8B8A, 0xFE01, 0x2F9D1},
    {0x8C48, 0xFE00, 0xF900},
    {0x8C55, 0xFE00, 0x2F9D2},
    {0x8CAB, 0xFE00, 0x2F9D4},
    {0x8CC1, 0xFE00, 0x2F9D5},
    {0x8CC2, 0xFE00, 0xF948},
    {0x8CC8, 0xFE00, 0xF903},
    {0x8CD3, 0xFE00, 0xFA64},
    {0x8D08, 0xFE00, 0xFA65},
    {0x8D08, 0xFE01, 0xFAC1},
    {0x8D1B, 0xFE00, 0x2F9D6},
    {0x8D77, 0xFE00, 0x2F9D7},
    {0x8DBC, 0xFE00, 0x2F9DB},
    {0x8DCB, 0xFE00, 0x2F9DA},
    {0x8DEF, 0xFE00, 0xF937},
    {0x8DF0, 0xFE00, 0x2F9DC},
    {0x8ECA, 0xFE00, 0xF902},
    {0x8ED4, 0xFE00, 0x2F9DE},
    {0x8F26, 0xFE00, 0xF998},
    {0x8F2A, 0xFE00, 0xF9D7},
    {0x8F38, 0xFE00, 0xFAC2},
    {0x8F38, 0xFE01, 0x2F9DF},
    {0x8F3B, 0xFE00, 0xFA07},
    {0x8F62, 0xFE00, 0xF98D},
    {0x8F9E, 0xFE00, 0x2F98D},
    {0x8FB0, 0xFE00, 0xF971},
    {0x8FB6, 0xFE00, 0xFA66},
    {0x9023, 0xFE00, 0xF99A},
    {0x9038, 0xFE00, 0xFA25},
    {0x9038, 0xFE01, 0xFA67},
    {0x9072, 0xFE00, 0xFAC3},
    {0x907C, 0xFE00, 0xF9C3},
    {0x908F, 0xFE00, 0xF913},
    {0x9094, 0xFE00, 0x2F9E2},
    {0x90CE, 0xFE00, 0xF92C},
    {0x90DE, 0xFE00, 0xFA2E},
    {0x90F1, 0xFE00, 0x2F9E3},
    {0x90FD, 0xFE00, 0xFA26},
    {0x9111, 0xFE00, 0x2F9E4},
    {0x911B, 0xFE00, 0x2F9E6},
    {0x916A, 0xFE00, 0xF919},
    {0x9199, 0xFE00, 0xFAC4},
    {0x91B4, 0xFE00, 0xF9B7},
    {0x91CC, 0xFE00, 0xF9E9},
    {0x91CF, 0xFE00, 0xF97E},
    {0x91D1, 0xFE00, 0xF90A},
    {0x9234, 0xFE00, 0xF9B1},
    {0x9238, 0xFE00, 0x2F9E7},
    {0x9276, 0xFE00, 0xFAC5},
    {0x927C, 0xFE00, 0x2F9EA},
    {0x92D7, 0xFE00, 0x2F9E8},
    {0x92D8, 0xFE00, 0x2F9E9},
    {0x9304, 0xFE00, 0xF93F},
    {0x934A, 0xFE00, 0xF99B},
    {0x93F9, 0xFE00, 0x2F9EB},
    {0x9415, 0xFE00, 0x2F9EC},
    {0x958B, 0xFE00, 0x2F9EE},
    {0x95AD, 0xFE00, 0xF986},
    {0x95B7, 0xFE00, 0x2F9F0},
    {0x962E, 0xFE00, 0xF9C6},
    {0x964B, 0xFE00, 0xF951},
    {0x964D, 0xFE00, 0xFA09},
    {0x9675, 0xFE00, 0xF959},
    {0x9678, 0xFE00, 0xF9D3},
    {0x967C, 0xFE00, 0xFAC6},
    {0x9686, 0xFE00, 0xF9DC},
    {0x96A3, 0xFE00, 0xF9F1},
    {0x96B7, 0xFE00, 0xFA2F},
    {0x96B8, 0xFE00, 0xF9B8},
    {0x96C3, 0xFE00, 0x2F9F3},
    {0x96E2, 0xFE00, 0xF9EA},
    {0x96E3, 0xFE00, 0xFA68},
    {0x96E3, 0xFE01, 0xFAC7},
    {0x96F6, 0xFE00, 0xF9B2},
    {0x96F7, 0xFE00, 0xF949},
    {0x9723, 0xFE00, 0x2F9F5},
    {0x9732, 0xFE00, 0xF938},
    {0x9748, 0xFE00, 0xF9B3},
    {0x9756, 0xFE00, 0xFA1C},
    {0x9756, 0xFE01, 0xFAC8},
    {0x97DB, 0xFE00, 0xFAC9},
    {0x97E0, 0xFE00, 0x2F9FA},
    {0x97FF, 0xFE00, 0xFA69},
    {0x97FF, 0xFE01, 0xFACA},
    {0x980B, 0xFE00, 0xFACB},
    {0x980B, 0xFE01, 0x2F9FE},
    {0x980B, 0xFE02, 0x2F9FF},
    {0x9818, 0xFE00, 0xF9B4},
    {0x9829, 0xFE00, 0x2FA00},
    {0x983B, 0xFE00, 0xFA6A},
    {0x983B, 0xFE01, 0xFACC},
    {0x985E, 0xFE00, 0xF9D0},
    {0x98E2, 0xFE00, 0x2FA02},
    {0x98EF, 0xFE00, 0xFA2A},
    {0x98FC, 0xFE00, 0xFA2B},
    {0x9928, 0xFE00, 0xFA2C},
    {0x9929, 0xFE00, 0x2FA04},
    {0x99A7, 0xFE00, 0x2FA05},
    {0x99C2, 0xFE00, 0x2FA06},
    {0x99F1, 0xFE00, 0xF91A},
    {0x99FE, 0xFE00, 0x2FA07},
    {0x9A6A, 0xFE00, 0xF987},
    {0x9B12, 0xFE00, 0xFACD},
    {0x9B12, 0xFE01, 0x2FA0A},
    {0x9B6F, 0xFE00, 0xF939},
    {0x9C40, 0xFE00, 0x2FA0B},
    {0x9C57, 0xFE00, 0xF9F2},
    {0x9CFD, 0xFE00, 0x2FA0C},
    {0x9D67, 0xFE00, 0x2FA0F},
    {0x9DB4, 0xFE00, 0xFA2D},
    {0x9DFA, 0xFE00, 0xF93A},
    {0x9E1E, 0xFE00, 0xF920},
    {0x9E7F, 0xFE00, 0xF940},
    {0x9E97, 0xFE00, 0xF988},
    {0x9E9F, 0xFE00, 0xF9F3},
    {0x9EBB, 0xFE00, 0x2FA15},
    {0x9ECE, 0xFE00, 0xF989},
    {0x9EF9, 0xFE00, 0x2FA17},
    {0x9EFE, 0xFE00, 0x2FA18},
    {0x9F05, 0xFE00, 0x2FA19},
    {0x9F0F, 0xFE00, 0x2FA1A},
    {0x9F16, 0xFE00, 0x2FA1B},
    {0x9F3B, 0xFE00, 0x2FA1C},
    {0x9F43, 0xFE00, 0xFAD8},
    {0x9F8D, 0xFE00, 0xF9C4},
    {0x9F8E, 0xFE00, 0xFAD9},
    {0x9F9C, 0xFE00, 0xF907},
    {0x9F9C, 0xFE01, 0xF908},
    {0x9F9C, 0xFE02, 0xFACE},
    {0x20122, 0xFE00, 0x2F803},
    {0x2051C, 0xFE00, 0x2F812},
    {0x20525, 0xFE00, 0x2F91B},
    {0x2054B, 0xFE00, 0x2F816},
    {0x2063A, 0xFE00, 0x2F80D},
    {0x20804, 0xFE00, 0x2F9D9},
    {0x208DE, 0xFE00, 0x2F9DD},
    {0x20A2C, 0xFE00, 0x2F834},
    {0x20B63, 0xFE00, 0x2F838},
    {0x214E4, 0xFE00, 0x2F859},
    {0x216A8, 0xFE00, 0x2F860},
    {0x216EA, 0xFE00, 0x2F861},
    {0x219C8, 0xFE00, 0x2F86C},
    {0x21B18, 0xFE00, 0x2F871},
    {0x21D0B, 0xFE00, 0x2F8F8},
    {0x21DE4, 0xFE00, 0x2F87B},
    {0x21DE6, 0xFE00, 0x2F87D},
    {0x22183, 0xFE00, 0x2F889},
    {0x2219F, 0xFE00, 0x2F939},
    {0x22331, 0xFE00, 0x2F891},
    {0x22331, 0xFE01, 0x2F892},
    {0x226D4, 0xFE00, 0x2F8A4},
    {0x22844, 0xFE00, 0xFAD0},
    {0x2284A, 0xFE00, 0xFACF},
    {0x22B0C, 0xFE00, 0x2F8B8},
    {0x22BF1, 0xFE00, 0x2F8BE},
    {0x2300A, 0xFE00, 0x2F8CA},
    {0x232B8, 0xFE00, 0x2F897},
    {0x2335F, 0xFE00, 0x2F980},
    {0x23393, 0xFE00, 0x2F989},
    {0x2339C, 0xFE00, 0x2F98A},
    {0x233C3, 0xFE00, 0x2F8DD},
    {0x233D5, 0xFE00, 0xFAD1},
    {0x2346D, 0xFE00, 0x2F8E3},
    {0x236A3, 0xFE00, 0x2F8EC},
    {0x238A7, 0xFE00, 0x2F8F0},
    {0x23A8D, 0xFE00, 0x2F8F7},
    {0x23AFA, 0xFE00, 0x2F8F9},
    {0x23CBC, 0xFE00, 0x2F8FB},
    {0x23D1E, 0xFE00, 0x2F906},
    {0x23ED1, 0xFE00, 0x2F90D},
    {0x23F5E, 0xFE00, 0x2F910},
    {0x23F8E, 0xFE00, 0x2F911},
    {0x24263, 0xFE00, 0x2F91D},
    {0x242EE, 0xFE00, 0xFA6C},
    {0x243AB, 0xFE00, 0x2F91F},
    {0x24608, 0xFE00, 0x2F923},
    {0x24735, 0xFE00, 0x2F926},
    {0x24814, 0xFE00, 0x2F927},
    {0x24C36, 0xFE00, 0x2F935},
    {0x24C92, 0xFE00, 0x2F937},
    {0x24FA1, 0xFE00, 0x2F93B},
    {0x24FB8, 0xFE00, 0x2F93C},
    {0x25044, 0xFE00, 0x2F93D},
    {0x250F2, 0xFE00, 0x2F942},
    {0x250F3, 0xFE00, 0x2F941},
    {0x25119, 0xFE00, 0x2F943},
    {0x25133, 0xFE00, 0x2F944},
    {0x25249, 0xFE00, 0xFAD5},
    {0x2541D, 0xFE00, 0x2F94D},
    {0x25626, 0xFE00, 0x2F952},
    {0x2569A, 0xFE00, 0x2F954},
    {0x256C5, 0xFE00, 0x2F955},
    {0x2597C, 0xFE00, 0x2F95C},
    {0x25AA7, 0xFE00, 0x2F95D},
    {0x25AA7, 0xFE01, 0x2F95E},
    {0x25BAB, 0xFE00, 0x2F961},
    {0x25C80, 0xFE00, 0x2F965},
    {0x25CD0, 0xFE00, 0xFAD6},
    {0x25F86, 0xFE00, 0x2F96B},
    {0x261DA, 0xFE00, 0x2F898},
    {0x26228, 0xFE00, 0x2F972},
    {0x26247, 0xFE00, 0x2F973},
    {0x262D9, 0xFE00, 0x2F975},
    {0x2633E, 0xFE00, 0x2F977},
    {0x264DA, 0xFE00, 0x2F97B},
    {0x26523, 0xFE00, 0x2F97C},
    {0x265A8, 0xFE00, 0x2F97E},
    {0x267A7, 0xFE00, 0x2F987},
    {0x267B5, 0xFE00, 0x2F988},
    {0x26B3C, 0xFE00, 0x2F997},
    {0x26C36, 0xFE00, 0x2F9A4},
    {0x26CD5, 0xFE00, 0x2F9A6},
    {0x26D6B, 0xFE00, 0x2F9A5},
    {0x26F2C, 0xFE00, 0x2F9AD},
    {0x26FB1, 0xFE00, 0x2F9B0},
    {0x270D2, 0xFE00, 0x2F9B1},
    {0x273CA, 0xFE00, 0x2F9AB},
    {0x27667, 0xFE00, 0x2F9C5},
    {0x278AE, 0xFE00, 0x2F9CB},
    {0x27966, 0xFE00, 0x2F9CC},
    {0x27CA8, 0xFE00, 0x2F9D3},
    {0x27ED3, 0xFE00, 0xFAD7},
    {0x27F2F, 0xFE00, 0x2F9D8},
    {0x285D2, 0xFE00, 0x2F9E0},
    {0x285ED, 0xFE00, 0x2F9E1},
    {0x2872E, 0xFE00, 0x2F9E5},
    {0x28BFA, 0xFE00, 0x2F9ED},
    {0x28D77, 0xFE00, 0x2F9F1},
    {0x29145, 0xFE00, 0x2F9F6},
    {0x291DF, 0xFE00, 0x2F81C},
    {0x2921A, 0xFE00, 0x2F9F7},
    {0x2940A, 0xFE00, 0x2F9FB},
    {0x29496, 0xFE00, 0x2F9FD},
    {0x295B6, 0xFE00, 0x2FA01},
    {0x29B30, 0xFE00, 0x2FA09},
    {0x2A0CE, 0xFE00, 0x2FA10},
    {0x2A105, 0xFE00, 0x2FA12},
    {0x2A20E, 0xFE00, 0x2FA13},
    {0x2A291, 0xFE00, 0x2FA14},
    {0x2A392, 0xFE00, 0x2F88F},
    {0x2A600, 0xFE00, 0x2FA1D},
    /* Must be sorted. */
  };
  static const size_t CJK_compatibility_ideographs_count
      = sizeof(CJK_compatibility_ideographs)
          / sizeof(CJK_compatibility_ideographs[0]);

  size_t lower = 0;
  size_t upper = CJK_compatibility_ideographs_count;

  while (lower < upper) {
    size_t middle = (lower + upper) / 2;
    int32_t u = CJK_compatibility_ideographs[middle][0];
    int32_t v = CJK_compatibility_ideographs[middle][1];
    if (ucv == u && uvs == v) {
      return CJK_compatibility_ideographs[middle][2];
    } else if (ucv < u || (ucv == u && uvs < v)) {
      upper = middle;
    } else if (ucv > u || (ucv == u && uvs > v)) {
      lower = middle + 1;
    }
  }
  return -1;
}
