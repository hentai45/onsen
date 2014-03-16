/**
 * ATA コマンド
 */

//=============================================================================
// 公開ヘッダ

#ifndef HEADER_ATA_CMD
#define HEADER_ATA_CMD

#include "ata/common.h"

int ata_cmd_identify_device(struct ATA_DEV *dev);


#endif


//=============================================================================
// 非公開ヘッダ

#include "asmfunc.h"
#include "ata/pio.h"
#include "debug.h"

#define ATA_CMD_IDENTIFY_DEVICE  0xEC
#define ATAPI_CMD_IDENTIFY_PACKET_DEVICE  0xA1


//=============================================================================
// 関数

int ata_cmd_identify_device(struct ATA_DEV *dev)
{
    if (ata_select_device(dev) < 0) {
        DBGF("ERROR");
        return -1;
    }

    int cmd;
    bool check_ready = false;

    if (dev->type == ATA_TYPE_PATA) {
        cmd = ATA_CMD_IDENTIFY_DEVICE;

        if (dev->type != ATA_TYPE_UNKNOWN)
            check_ready = true;
    } else if (dev->type == ATA_TYPE_PATAPI) {
        cmd = ATAPI_CMD_IDENTIFY_PACKET_DEVICE;
    }

    return ata_pio_datain_cmd(cmd, dev, dev->identity, 1, check_ready);
}
