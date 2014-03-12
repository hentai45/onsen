#ifndef SYSINFO_H
#define SYSINFO_H

#include <stdint.h>

struct SYSTEM_INFO {
    uint32_t vram;
    int32_t w;
    int32_t h;
    int32_t color_width;
    int32_t mmap_entries;
};

extern struct SYSTEM_INFO *g_sys_info;

#endif
