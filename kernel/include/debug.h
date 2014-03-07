// Generated by mkhdr

#ifndef HEADER_DEBUG
#define HEADER_DEBUG

#include <stdarg.h>
#include "file.h"

//-----------------------------------------------------------------------------
// メイン

void debug_main(void);


//-----------------------------------------------------------------------------
// 画面出力

typedef struct _INT_REGISTERS {
    // asm_inthander.S で積まれたスタックの内容
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;  // pushal
    unsigned int ds, es;

    // 以下は、例外発生時にCPUが自動でpushしたもの
    unsigned int err_code;
    unsigned int eip, cs, eflags, app_esp, app_ss;
} INT_REGISTERS;


#define DBGF(fmt, ...)  dbgf("%s %d %s : " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)


void temp_dbgf(const char *fmt, ...);
void dbgf(const char *fmt, ...);
void dbg_clear(void);
void dbg_seg(void);

void dbg_fault(const char *msg, int no, INT_REGISTERS *regs);

extern FILE_T *f_debug;
extern FILE_T *f_dbg_temp;

extern int g_dbg_temp_flg;

#endif
