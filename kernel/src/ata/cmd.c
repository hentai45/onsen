/**
 * ATA コマンド
 */

//=============================================================================
// 公開ヘッダ

#ifndef HEADER_ATA_CMD
#define HEADER_ATA_CMD

// SET FEATURES の Subcommand code
#define SET_TRANSFER  0x03

#include "ata/common.h"

int ata_cmd_read_sectors(struct ATA_DEV *dev, uint32_t lba, void *buf, int cnt);
int ata_cmd_identify_device(struct ATA_DEV *dev);
int ata_cmd_set_features(struct ATA_DEV *dev, uint8_t sub_cmd, uint8_t mode);
int ata_cmd_idle(struct ATA_DEV *dev);
int ata_cmd_init_dev_params(struct ATA_DEV *dev);


#endif


//=============================================================================
// 非公開ヘッダ

#include "asmfunc.h"
#include "ata/info.h"
#include "debug.h"


#define ATA_CMD_READ_SECTORS              0x20
#define ATA_CMD_IDLE                      0xE3
#define ATA_CMD_IDENTIFY_DEVICE           0xEC
#define ATA_CMD_SET_FEATURES              0xEF
#define ATA_CMD_INITIALIZE_DEVICE_PARAMETERS  0x91

#define ATAPI_CMD_IDENTIFY_PACKET_DEVICE  0xA1
#define ATAPI_CMD_IDLE                    0xE1

#define SELECT_DEV(dev)  do {          \
    if (ata_select_device(dev) < 0) {  \
        DBGF("device select error");   \
        return -1;                     \
    }                                  \
} while (0)


static int non_data_cmd(uint8_t cmd, struct ATA_DEV *dev, bool check_ready);
static int pio_data_in_cmd(uint8_t cmd, struct ATA_DEV *dev,
        void *buf, int cnt, bool check_ready);


//=============================================================================
// 公開関数


int ata_cmd_read_sectors(struct ATA_DEV *dev, uint32_t lba, void *buf, int cnt)
{
    uint8_t sec_no, cyl_lo, cyl_hi, dev_head;

    if (dev->info->support_lba) {
        // TODO: check valid

        sec_no   = lba & 0xFF;
        cyl_lo   = (lba >> 8) & 0xFF;
        cyl_hi   = (lba >> 16) & 0xFF;
        dev_head = ((lba >> 24) & 0xF) | ATA_DH_LBA;

        uint16_t h = dev->info->cur_heads;
        uint16_t s = dev->info->cur_sectors;

        uint16_t cc = lba / (s * h);
        uint16_t hh = (lba / s) % h;
        uint16_t ss = (lba % s) + 1;

        dbgf("read: lba = %d, c = %d, h = %d, s = %d\n", lba, cc, hh, ss);
    } else {
        // TODO: check valid

        // LBA => CHS

        uint16_t h = dev->info->cur_heads;
        uint16_t s = dev->info->cur_sectors;

        sec_no   = (lba % s) + 1;
        uint16_t cyl = lba / (s * h);
        cyl_lo   = cyl & 0xFF;
        cyl_hi   = (cyl >> 8) & 0xFF;
        dev_head = (lba / s) % h;

        dbgf("read: lba = %d, c = %d, h = %d, s = %d\n", lba, cyl, dev_head, sec_no);
    }

    if (ata_select_device_ext(dev, dev_head) < 0) {
        DBGF("ERROR");
        return -1;
    }

    outb(ATA_PORT_SECTOR_CNT(dev), cnt);
    outb(ATA_PORT_SECTOR_NO(dev),  sec_no);
    outb(ATA_PORT_CYL_LO(dev),     cyl_lo);
    outb(ATA_PORT_CYL_HI(dev),     cyl_hi);

    return pio_data_in_cmd(ATA_CMD_READ_SECTORS, dev, buf, cnt, true);
}


int ata_cmd_identify_device(struct ATA_DEV *dev)
{
    SELECT_DEV(dev);

    int cmd;
    bool check_ready = false;

    if (dev->type == ATA_TYPE_PATA) {
        cmd = ATA_CMD_IDENTIFY_DEVICE;

        if (dev->type != ATA_TYPE_UNKNOWN)
            check_ready = true;
    } else if (dev->type == ATA_TYPE_PATAPI) {
        cmd = ATAPI_CMD_IDENTIFY_PACKET_DEVICE;
    }

    return pio_data_in_cmd(cmd, dev, dev->identity, 1, check_ready);
}


int ata_cmd_set_features(struct ATA_DEV *dev, uint8_t sub_cmd, uint8_t mode)
{
    SELECT_DEV(dev);

    outb(ATA_PORT_FEATURES(dev), sub_cmd);
    outb(ATA_PORT_SECTOR_CNT(dev), mode);

    return non_data_cmd(ATA_CMD_SET_FEATURES, dev, true);
}


int ata_cmd_idle(struct ATA_DEV *dev)
{
    SELECT_DEV(dev);

    int cmd;

    if (dev->type == ATA_TYPE_PATA) {
        cmd = ATA_CMD_IDLE;
    } else if (dev->type == ATA_TYPE_PATAPI) {
        cmd = ATAPI_CMD_IDLE;
    } else {
        return -1;
    }

    return non_data_cmd(cmd, dev, true);
}


int ata_cmd_init_dev_params(struct ATA_DEV *dev)
{
    if (ata_select_device_ext(dev, dev->info->default_heads - 1) < 0) {
        DBGF("ERROR");
        return -1;
    }

    outb(ATA_PORT_SECTOR_CNT(dev), dev->info->default_sectors);

    return non_data_cmd(ATA_CMD_INITIALIZE_DEVICE_PARAMETERS, dev, false);
}

//=============================================================================
// 非公開関数


static int non_data_cmd(uint8_t cmd, struct ATA_DEV *dev, bool check_ready)
{
    if (check_ready)
        ata_wait_ready_set(dev);

    outb(ATA_PORT_CMD(dev), cmd);

    ata_wait_400ns(dev);

    if (ata_wait_busy_clear(dev) < 0) {
        DBGF("ERROR. cmd = %X", cmd);
        return -2;
    }

    inb(ATA_PORT_ALT_STATUS(dev));  // 空読み
    int status = inb(ATA_PORT_STATUS(dev));

    if (status & ATA_ST_ERR) {
        DBGF("ERROR. cmd = %X", cmd);
        //int err = inb(ATA_PORT_ERR(dev));
        return -1;
    }

    return 0;
}


static void read_data(struct ATA_DEV *dev, uint16_t *buf, int len);

static int pio_data_in_cmd(uint8_t cmd, struct ATA_DEV *dev,
        void *buf, int cnt, bool check_ready)
{
    if (check_ready)
        ata_wait_ready_set(dev);

    outb(ATA_PORT_CMD(dev), cmd);

    ata_wait_400ns(dev);

    inb(ATA_PORT_ALT_STATUS(dev));  // 空読み

    uint16_t *p = (uint16_t *) buf;

    for (int i = 0; i < cnt; i++) {
        if (ata_wait_busy_clear(dev) < 0) {
            DBGF("ERROR. cmd = %X", cmd);
            return -2;
        }

        int status = inb(ATA_PORT_ALT_STATUS(dev));

        if (status & ATA_ST_ERR) {
            DBGF("ERROR. cmd = %X", cmd);
            break;  // コマンド実行エラー
        }

        if ((status & ATA_ST_DRQ) == 0) {
            DBGF("ERROR. cmd = %X", cmd);
            return -3;  // なぜかデータが用意されていない
        }

        read_data(dev, p, 256);
        p += 256;
    }

    inb(ATA_PORT_ALT_STATUS(dev));  // 空読み
    int status = inb(ATA_PORT_STATUS(dev));

    if (status & ATA_ST_ERR) {
        DBGF("ERROR. cmd = %X", cmd);
        //int err = inb(ATA_PORT_ERR(dev));
        return -1;
    }

    return 0;
}


static void read_data(struct ATA_DEV *dev, uint16_t *buf, int len)
{
    if (buf) {
        for (int i = 0; i < len; i++) {
            *buf++ = inw(ATA_PORT_DATA(dev));
        }
    } else {
        // データの空読み
        for (int i = 0; i < len; i++) {
            inw(ATA_PORT_DATA(dev));
        }
    }
}

