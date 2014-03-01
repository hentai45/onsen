/**
 * GDT
 */

//=============================================================================
// 公開ヘッダ

#ifndef HEADER_GDT
#define HEADER_GDT

#include <stdbool.h>


// ---- セグメントの属性を表す数値
#define SEG_TYPE_CODE       0x9A
#define SEG_TYPE_DATA       0x92
#define SEG_TYPE_STACK      0x96
#define SEG_TYPE_LDT        0x82
#define SEG_TYPE_TSS        0x89
// #define SEG_TYPE_TSS_BUSY   0x8B
#define SEG_TYPE_CALL_GATE  0x84
#define SEG_TYPE_INTR_GATE  0x8E
#define SEG_TYPE_TRAP_GATE  0x8F
#define SEG_TYPE_TASK_GATE  0x85


// ---- セグメントの32ビット属性を表す数値
// SEG32_BIG_SEGはset_gate_descで自動で設定する
#define SEG32_BIG_SEG       0x80 ///< リミットの数値をページ単位(4KB)ととらえる
// #define SEG32_SMALL_SEG     0x00

#define SEG32_CODE386       0x40
#define SEG32_DATA386       0x40
// #define SEG32_CODE286       0x00


#define KERNEL_DS  (1 << 3)  // カーネルのデータセグメントのセレクタ値
#define KERNEL_CS  (2 << 3)  // カーネルのコードセグメントのセレクタ値
#define USER_DS    (3 << 3)  // アプリケーションのデータセグメントのセレクタ値
#define USER_CS    (4 << 3)  // アプリケーションのコードセグメントのセレクタ値
#define SEG_TSS    5         // TSSをGDTの何番から割当てるのか


struct TSS;
void gdt_init(void);
void set_seg_desc(int no, unsigned long addr,
        unsigned long limit, int segtype, int seg32type, int dpl);
void set_code_seg(int no, unsigned long addr, unsigned long limit, int dpl);
void set_data_seg(int no, unsigned long addr, unsigned long limit, int dpl);

#endif


//=============================================================================
// 非公開ヘッダ

#include "asmfunc.h"
#include "memory.h"
#include "str.h"
#include "task.h"


typedef struct SEG_DESC {
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char  base_mid;
    unsigned char  type;
    unsigned char  limit_high;  /// @attention 上位４ビットはtypeのつづき
    unsigned char  base_high;
} __attribute__ ((__packed__)) SEG_DESC;


static SEG_DESC *gdt = (SEG_DESC *) VADDR_GDT;


typedef struct {
    unsigned short limit;
    unsigned long base;
} __attribute__ ((__packed__)) GDTR;


inline static void load_gdtr(GDTR *gdtr)
{
    __asm__ volatile("lgdt (%0)" : : "q" (gdtr));
}


//=============================================================================
// 公開関数

void gdt_init(void)
{
    // 初期化
    for (int i = 0; i <= LIMIT_GDT / 8; i++) {
        set_seg_desc(i, 0, 0, 0, 0, 0);
    }

    set_data_seg(KERNEL_DS >> 3, 0x00000000, 0xFFFFFFFF, /* dpl = */ 0);
    set_code_seg(KERNEL_CS >> 3, 0x00000000, 0xFFFFFFFF, /* dpl = */ 0);
    set_data_seg(USER_DS >> 3  , 0x00000000, 0xFFFFFFFF, /* dpl = */ 3);
    set_code_seg(USER_CS >> 3  , 0x00000000, 0xFFFFFFFF, /* dpl = */ 3);

    GDTR gdtr;
    gdtr.limit = LIMIT_GDT;
    gdtr.base = VADDR_GDT;
    load_gdtr(&gdtr);

    reload_segments(KERNEL_CS, KERNEL_DS);
}


void set_code_seg(int no, unsigned long addr, unsigned long limit, int dpl)
{
    set_seg_desc(no, addr, limit, SEG_TYPE_CODE, SEG32_CODE386, dpl);
}


void set_data_seg(int no, unsigned long addr, unsigned long limit, int dpl)
{
    set_seg_desc(no, addr, limit, SEG_TYPE_DATA, SEG32_DATA386, dpl);
}


void set_seg_desc(int no, unsigned long addr,
        unsigned long limit, int segtype, int seg32type, int dpl)
{
    SEG_DESC *desc = gdt + no;

    if (limit > 0xFFFFF) {
        seg32type |= SEG32_BIG_SEG;
        limit /= 0x1000;
    }

    // セグメントのベースアドレスを３つのフィールドに分割して設定
    desc->base_low  = (unsigned int) (addr & 0xFFFF);
    desc->base_mid  = (unsigned char) ((addr >> 16) & 0xFF);
    desc->base_high = (unsigned char) ((addr >> 24) & 0xFF);

    // セグメントのリミット値を２つのフィールドに分割して設定
    // 合わせて32ビット属性も設定
    desc->limit_low = (unsigned int) (limit & 0xFFFF);
    desc->limit_high = (unsigned char) ((limit >> 16) & 0x0F) + seg32type;

    // セグメントの属性と特権レベルを設定
    desc->type = segtype | (dpl << 5);
}


//=============================================================================
// 非公開関数


