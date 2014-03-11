#ifndef SYSINFO_H
#define SYSINFO_H

struct SYSTEM_INFO {
    unsigned long vram;
    int w;
    int h;
    int color_width;
    int mmap_entries;
    unsigned long end_free_maddr;
};

extern struct SYSTEM_INFO *g_sys_info;

#endif
