#ifndef SYSINFO_H
#define SYSINFO_H

typedef struct _SYSTEM_INFO {
    unsigned long vram;
    int w;
    int h;
    unsigned long end_free_maddr;
} SYSTEM_INFO;

extern SYSTEM_INFO *g_sys_info;

#endif
