/**
 * IDENTITY DEVICE のデータを解析した情報
 */

//=============================================================================
// 公開ヘッダ

#ifndef HEADER_ATA_INFO
#define HEADER_ATA_INFO

#include <stdbool.h>
#include <stdint.h>
#include "ata/common.h"


#define SUPPORT_PIO_MODE3  1
#define SUPPORT_PIO_MODE4  2


struct ATA_INFO {
    uint16_t default_cylinders;  // 論理シリンダ数（デフォルト転送モード）
    uint16_t default_heads;      // 論理ヘッド数  （デフォルト転送モード）
    uint16_t default_sectors;    // 論理セクタ数  （デフォルト転送モード）
    uint16_t cur_cylinders;      // 論理シリンダ数（現在の転送モード）
    uint16_t cur_heads;          // 論理ヘッド数  （現在の転送モード）
    uint16_t cur_sectors;        // 論理セクタ数  （現在の転送モード）
    uint8_t support_pio_mode;
    bool support_lba;
    bool support_iordy;
    bool support_pow_mng;
};


void ata_anal_id(struct ATA_DEV *dev);


#endif


//=============================================================================
// 非公開ヘッダ

#include "debug.h"
#include "memory.h"


//=============================================================================
// 関数


void ata_anal_id(struct ATA_DEV *dev)
{
    uint16_t *id = dev->identity;
    struct ATA_INFO *info = (struct ATA_INFO *) mem_alloc(sizeof(struct ATA_INFO));
    dev->info = info;

    info->default_cylinders = id[1];
    info->default_heads     = id[3];
    info->default_sectors   = id[6];

    info->support_iordy = id[49] & 0x800;

    if (id[53] & 2) {  // ワード64-70が有効
        info->support_pio_mode = id[64] & 0x007F;
    } else {
        info->support_pio_mode = 0;
    }

    info->cur_cylinders = id[54];
    info->cur_heads     = id[55];
    info->cur_sectors   = id[56];

    // LBAモードでアドレス可能な全セクタ数が設定されているなら
    // LBA方式に対応していると判断する
    if ((id[60] | id[61]) != 0) {
        info->support_lba = true;
    } else {
        info->support_lba = false;
    }

    info->support_pow_mng = id[82] & 8;

    dbgf("default: c = %d, h = %d, s = %d\n",
            info->default_cylinders, info->default_heads, info->default_sectors);
    dbgf("cur:     c = %d, h = %d, s = %d\n",
            info->cur_cylinders, info->cur_heads, info->cur_sectors);
    dbgf("support pio mode = %X\n", info->support_pio_mode);
    dbgf("support iordy    = %d\n", info->support_iordy);
    dbgf("support pow mng  = %X\n", info->support_pow_mng);
}
