/**
 * color
 */

#ifndef HEADER_COLOR
#define HEADER_COLOR

typedef unsigned char  COLOR8;
typedef unsigned short COLOR16;
typedef unsigned long  COLOR32;

#define RGB32(r, g, b) ((COLOR32) (((r) & 0xFF) << 16) | (((g) & 0xFF) << 8) | ((b) & 0xFF))
#define RGB16(r, g, b) ((COLOR16) (((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3))
#define RGB32_TO_16(rgb32)  ((COLOR16) (((rgb32) & 0xF80000) >> 8) | (((rgb32) & 0xFC00) >> 5) | (((rgb32) & 0xF8) >> 3))
#define RGB16_TO_32(rgb16)  ((COLOR32) (((rgb16) & 0xF800) << 8) | (((rgb16) & 0x07E0) << 5) | (((rgb16) & 0x001F) << 3))

#define GET_RED16(rgb)   (((rgb) & 0xF800) >> 8)
#define GET_GREEN16(rgb) (((rgb) & 0x07E0) >> 3)
#define GET_BLUE16(rgb)  (((rgb) & 0x001F) << 3)

#define COL_BLACK    (0x000000)
#define COL_RED      (0xFF0000)
#define COL_GREEN    (0x00FF00)
#define COL_BLUE     (0x0000FF)
#define COL_WHITE    (0xFFFFFF)


#endif

