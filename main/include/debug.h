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

// pushal でスタックに格納されるレジスタ
typedef struct _REGISTERS {
    int edi, esi, ebp, ebx, edx, ecx, eax;
} REGISTERS;

#define DBGF(fmt, ...)  dbgf("%s %d %s : " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)


void temp_dbgf(const char *fmt, ...);
void dbgf(const char *fmt, ...);
void dbg_clear(void);
void dbg_reg(const REGISTERS *r);
void dbg_seg(void);

void dbg_fault(const char *msg, int *esp);

extern FILE_T *f_debug;
extern FILE_T *f_dbg_temp;

extern int g_dbg_temp_flg;

#endif
