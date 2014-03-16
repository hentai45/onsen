/**
 * ATA PIOモード
 */

//=============================================================================
// 公開ヘッダ

#ifndef HEADER_ATA_PIO
#define HEADER_ATA_PIO

#include "ata/common.h"

int ata_pio_datain_cmd(uint8_t cmd, struct ATA_DEV *dev, void *buf, int cnt, bool check_ready);

#endif


//=============================================================================
// 非公開ヘッダ

#include "asmfunc.h"
#include "debug.h"



//=============================================================================
// 関数

int ata_pio_datain_cmd(uint8_t cmd, struct ATA_DEV *dev, void *buf, int cnt, bool check_ready)
{
    if (check_ready)
        ata_wait_ready_set(dev);

    outb(ATA_PORT_CMD(dev), cmd);

    ata_wait_400ns(dev);

    inb(ATA_PORT_ALT_STATUS(dev));  // 空読み

    uint16_t *p = (uint16_t *) buf;

    for (int i = 0; i < cnt; i++) {
        if (ata_wait_busy_clear(dev) < 0) {
            DBGF("ERROR");
            return -2;
        }

        int status = inb(ATA_PORT_ALT_STATUS(dev));

        if (status & ATA_ST_ERR) {
            DBGF("ERROR");
            break;  // コマンド実行エラー
        }

        if ((status & ATA_ST_DRQ) == 0) {
            DBGF("ERROR");
            return -3;  // なぜかデータが用意されていない
        }

        ata_read_data(dev, p, 256);
        p += 256;
    }

    inb(ATA_PORT_ALT_STATUS(dev));  // 空読み
    int status = inb(ATA_PORT_STATUS(dev));

    if (status & ATA_ST_ERR) {
        DBGF("ERROR");
        //int err = inb(ATA_PORT_ERR(dev));  // TODO
        return -1;
    }

    return 0;
}
