#ifdef __cplusplus
extern "C" {
#endif

#ifndef TEX8BENC_H
#define TEX8BENC_H

/* UCS -> T1 mapping table */
static unsigned short UCStoT1enc[] = {
    /* from,  to */
    0xA1,    0xBD,
    0xA3,    0xBB,
    0xA7,    0x9F,
    0xA8,    0x04,
    0xAB,    0x13,
    0xAF,    0x09,
    0xB4,    0x01,
    0xB8,    0x0B,
    0xBB,    0x14,
    0xBF,    0xBE,
    0xC0,    0xC0,
    0xC1,    0xC1,
    0xC2,    0xC2,
    0xC3,    0xC3,
    0xC4,    0xC4,
    0xC5,    0xC5,
    0xC6,    0xC6,
    0xC7,    0xC7,
    0xC8,    0xC8,
    0xC9,    0xC9,
    0xCA,    0xCA,
    0xCB,    0xCB,
    0xCC,    0xCC,
    0xCD,    0xCD,
    0xCE,    0xCE,
    0xCF,    0xCF,
    0xD0,    0xD0,
    0xD1,    0xD1,
    0xD2,    0xD2,
    0xD3,    0xD3,
    0xD4,    0xD4,
    0xD5,    0xD5,
    0xD6,    0xD6,
    0xD8,    0xD8,
    0xD9,    0xD9,
    0xDA,    0xDA,
    0xDB,    0xDB,
    0xDC,    0xDC,
    0xDD,    0xDD,
    0xDE,    0xDE,
    0xDF,    0xFF,
    0xE0,    0xE0,
    0xE1,    0xE1,
    0xE2,    0xE2,
    0xE3,    0xE3,
    0xE4,    0xE4,
    0xE5,    0xE5,
    0xE6,    0xE6,
    0xE7,    0xE7,
    0xE8,    0xE8,
    0xE9,    0xE9,
    0xEA,    0xEA,
    0xEB,    0xEB,
    0xEC,    0xEC,
    0xED,    0xED,
    0xEE,    0xEE,
    0xEF,    0xEF,
    0xF0,    0xF0,
    0xF1,    0xF1,
    0xF2,    0xF2,
    0xF3,    0xF3,
    0xF4,    0xF4,
    0xF5,    0xF5,
    0xF6,    0xF6,
    0xF8,    0xF8,
    0xF9,    0xF9,
    0xFA,    0xFA,
    0xFB,    0xFB,
    0xFC,    0xFC,
    0xFD,    0xFD,
    0xFE,    0xFE,
    0xFF,    0xB8,
   0x102,    0x80,
   0x103,    0xA0,
   0x104,    0x81,
   0x105,    0xA1,
   0x106,    0x82,
   0x107,    0xA2,
   0x10C,    0x83,
   0x10D,    0xA3,
   0x10E,    0x84,
   0x10F,    0xA4,
   0x110,    0xD0,
   0x111,    0x9E,
   0x118,    0x86,
   0x119,    0xA6,
   0x11A,    0x85,
   0x11B,    0xA5,
   0x11E,    0x87,
   0x11F,    0xA7,
   0x130,    0x9D,
   0x131,    0x19,
   0x132,    0x9C,
   0x133,    0xBC,
   0x139,    0x88,
   0x13A,    0xA8,
   0x13D,    0x89,
   0x13E,    0xA9,
   0x141,    0x8A,
   0x142,    0xAA,
   0x143,    0x8B,
   0x144,    0xAB,
   0x147,    0x8C,
   0x148,    0xAC,
   0x14A,    0x8D,
   0x14B,    0xAD,
   0x150,    0x8E,
   0x151,    0xAE,
   0x152,    0xD7,
   0x153,    0xF7,
   0x154,    0x8F,
   0x155,    0xAF,
   0x158,    0x90,
   0x159,    0xB0,
   0x15A,    0x91,
   0x15B,    0xB1,
   0x15E,    0x93,
   0x15F,    0xB3,
   0x160,    0x92,
   0x161,    0xB2,
   0x162,    0xB5,
   0x163,    0xB5,
   0x164,    0x94,
   0x165,    0xB4,
   0x16E,    0x97,
   0x16F,    0xB7,
   0x170,    0x96,
   0x171,    0xB6,
   0x178,    0x98,
   0x179,    0x99,
   0x17A,    0xB9,
   0x17B,    0x9B,
   0x17C,    0xBB,
   0x17D,    0x9A,
   0x17E,    0xBA,
   0x237,    0x1A,
   0x2C6,    0x02,
   0x2C7,    0x07,
   0x2D8,    0x08,
   0x2D9,    0x0A,
   0x2DA,    0x06,
   0x2DB,    0x0C,
   0x2DC,    0x03,
   0x2DD,    0x05,
  0x1E9E,    0xDF,
  0x2010,    0x7F,
  0x2013,    0x15,
  0x2014,    0x16,
  0x2018,    0x60,
  0x2019,    0x27,
  0x201A,    0x0D,
  0x201C,    0x10,
  0x201D,    0x11,
  0x201E,    0x12,
  0x2039,    0x0E,
  0x203A,    0x0F,
  0x2423,    0x20,
};

/* UCS -> TS1 mapping table */
static unsigned short UCStoTS1enc[] = {
    /* from,  to */
    0xA2,    0xA2,
    0xA4,    0xA4,
    0xA5,    0xA5,
    0xA6,    0xA6,
    0xA7,    0xA7,
    0xA8,    0xA8,
    0xA9,    0xA9,
    0xAA,    0xAA,
    0xAC,    0xAC,
    0xAE,    0xAE,
    0xAF,    0xAF,
    0xB0,    0xB0,
    0xB1,    0xB1,
    0xB2,    0xB2,
    0xB3,    0xB3,
    0xB4,    0xB4,
    0xB5,    0xB5,
    0xB6,    0xB6,
    0xB7,    0xB7,
    0xB9,    0xB9,
    0xBA,    0xBA,
    0xBC,    0xBC,
    0xBD,    0xBD,
    0xBE,    0xBE,
    0xD7,    0xD6,
    0xF7,    0xF6,
   0x17F,    0x49,
   0x192,    0x8C,
   0xE3F,    0x9A,
  0x2030,    0x87,
  0x2031,    0x98,
  0x203D,    0x94,
  0x2044,    0x2F,
  0x20A1,    0x8D,
  0x20A4,    0x92,
  0x20A6,    0x8F,
  0x20A9,    0x8E,
  0x20AB,    0x96,
  0x20AC,    0xBF,
  0x20B1,    0x91,
  0x2103,    0x89,
  0x2116,    0x9B,
  0x2117,    0xAD,
  0x211E,    0x93,
  0x2120,    0x9F,
  0x2122,    0x97,
  0x2126,    0x57,
  0x2127,    0x4D,
  0x212E,    0x9E,
  0x2190,    0x18,
  0x2191,    0x5E,
  0x2192,    0x19,
  0x2193,    0x5F,
  0x2212,    0x3D,
  0x221A,    0xBB,
  0x2329,    0x3C,
  0x232A,    0x3E,
  0x2422,    0x20,
  0x25E6,    0x9E,
  0x25EF,    0x4F,
  0x266A,    0x6E,
  0x26AD,    0x6D,
  0x26AE,    0x63,
  0x27E6,    0x5B,
  0x27E7,    0x5D,
  0x2E17,    0x2D,
  0x2E18,    0x95,
};

#endif /* TEX8BENC_H */

#ifdef __cplusplus
}
#endif
