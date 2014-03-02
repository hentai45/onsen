/**
 * Color
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
#define RGB32_TO_8(rgb32)  (conv_rgb32_to_8(rgb32))
#define RGB8_TO_32(rgb8)  (conv_rgb8_to_32(rgb8))

#define GET_RED32(rgb)   (((rgb) & 0xFF0000) >> 16)
#define GET_GREEN32(rgb) (((rgb) & 0x00FF00) >> 8)
#define GET_BLUE32(rgb)  ((rgb) & 0x0000FF)

#define GET_RED16(rgb)   (((rgb) & 0xF800) >> 8)
#define GET_GREEN16(rgb) (((rgb) & 0x07E0) >> 3)
#define GET_BLUE16(rgb)  (((rgb) & 0x001F) << 3)

#define COL_BLACK    (0x000000)
#define COL_RED      (0xFF0000)
#define COL_GREEN    (0x00FF00)
#define COL_BLUE     (0x0000FF)
#define COL_WHITE    (0xFFFFFF)

void color_init(void);
COLOR8 conv_rgb32_to_8(COLOR32 color);
COLOR32 conv_rgb8_to_32(unsigned char c);

#endif

#define COLOR_MAX  (232)

static COLOR32 l_color_tbl[COLOR_MAX] = {
    0x000000,
    0xFF0000,
    0x00FF00,
    0xFFFF00,
    0x0000FF,
    0xFF00FF,
    0x00FFFF,
    0xFFFFFF,
    0xC6C6C6,
    0x840000,
    0x008400,
    0x848400,
    0x000084,
    0x840084,
    0x008484,
    0x848484
};


void color_init(void)
{
    for (int b = 0; b < 6; b++) {
        for (int g = 0; g < 6; g++) {
            for (int r = 0; r < 6; r++) {
                int i = 16 + r + g * 6 + b * 36;
                COLOR32 color = RGB32(r * 51, g * 51, b * 51);

                l_color_tbl[i] = color;
            }
        }
    }
}


COLOR8 conv_rgb32_to_8(COLOR32 color)
{
    int r = GET_RED32(color)   / 51;
    int g = GET_GREEN32(color) / 51;
    int b = GET_BLUE32(color)  / 51;

    return 16 + r + g * 6 + b * 36;
}


COLOR32 conv_rgb8_to_32(unsigned char c)
{
    if (c < COLOR_MAX)
        return l_color_tbl[c];
    else
        return 0xFFFFFF;
}
