/**
 * ATA
 *
 * ## 参考
 * http://wiki.osdev.org/ATA_PIO_Mode
 * 『ATA(IDE)/ATAPIの徹底研究』
 */

//=============================================================================
// 公開ヘッダ

#ifndef HEADER_ATA_COMMON
#define HEADER_ATA_COMMON

#include <stdbool.h>
#include <stdint.h>

// ---- ATA シグネチャ

#define ATA_SIG_PATA    0x0000
#define ATA_SIG_PATAPI  0xEB14
//#define ATA_SIG_SATA    0xC33C
//#define ATA_SIG_SATAPI  0x9669


// ---- ATA デバイスタイプ

#define ATA_TYPE_NON     (1 << 0)  // 未接続
#define ATA_TYPE_UNKNOWN (1 << 1)  // 不明デバイス
#define ATA_TYPE_PATA    (1 << 2)
#define ATA_TYPE_PATAPI  (1 << 3)
//#define ATA_TYPE_SATA    (1 << 4)
//#define ATA_TYPE_SATAPI  (1 << 5)


// ---- ATA モード

#define PIO_MODE0     0
#define PIO_MODE3     3
#define PIO_MODE4     4
#define PIO_MODE_NON  0xFF


// ---- I/O ポート

#define ATA_PORT_DATA(dev)          ((dev)->base_port + 0)
#define ATA_PORT_FEATURES(dev)      ((dev)->base_port + 1)
#define ATA_PORT_ERR(dev)           ((dev)->base_port + 1)
#define ATA_PORT_SECTOR_CNT(dev)    ((dev)->base_port + 2)
#define ATA_PORT_SECTOR_NO(dev)     ((dev)->base_port + 3)
#define ATA_PORT_LBA_LOW(dev)       ((dev)->base_port + 3)
#define ATA_PORT_CYL_LO(dev)        ((dev)->base_port + 4)
#define ATA_PORT_LBA_MID(dev)       ((dev)->base_port + 4)
#define ATA_PORT_CYL_HI(dev)        ((dev)->base_port + 5)
#define ATA_PORT_LBA_HI(dev)        ((dev)->base_port + 5)
#define ATA_PORT_DRIVE_HEAD(dev)    ((dev)->base_port + 6)
#define ATA_PORT_CMD(dev)           ((dev)->base_port + 7)
#define ATA_PORT_STATUS(dev)        ((dev)->base_port + 7)
#define ATA_PORT_DEV_CTRL(dev)      ((dev)->dev_ctrl_port)
#define ATA_PORT_ALT_STATUS(dev)    ((dev)->dev_ctrl_port)


// ---- ATA レジスタ

// Status レジスタ
#define ATA_ST_ERR   (1 << 0)  // エラー
#define ATA_ST_DRQ   (1 << 3)  // データ・リクエスト。1 = データ転送要求しているとき
#define ATA_ST_DRDY  (1 << 6)  // デバイス・レディ。1 = デバイスがレディのとき
#define ATA_ST_BSY   (1 << 7)  // ビジー


// Device/Head レジスタ
#define ATA_DH_SEL_DEV0  0         // マスタ
#define ATA_DH_SEL_DEV1  (1 << 4)  // スレーブ
#define ATA_DH_LBA       (1 << 6)
#define ATA_DH_OBS       0xA0      // obsolete

// Device Control レジスタ
#define ATA_DC_nIEN  (1 << 1)  // INTRQ信号の有効／無効
#define ATA_DC_SRST  (1 << 2)  // ソフトウェア・リセット


struct ATA_INFO;

struct ATA_DEV {
    int base_port;
    int irq;
    int dev_ctrl_port;  // Device Control Registers/Alternate Status ports
    int type;  // デバイスタイプ
    int sel_dev;  // マスタならATA_DH_SEL_DEV0, スレーブならATA_DH_SEL_DEV1
    int mode;
    uint16_t identity[256];  // IDENTIFY DEVICE コマンドを実行したした結果wwwwww
    struct ATA_INFO *info;
};


extern struct ATA_DEV *g_ata0;
extern struct ATA_DEV *g_ata1;


void ata_init(void);

int ata_select_device(struct ATA_DEV *dev);
int ata_select_device_ext(struct ATA_DEV *dev, uint8_t ext_flags);
void ata_wait_5ms(struct ATA_DEV *dev);
void ata_wait_400ns(struct ATA_DEV *dev);
int ata_wait_busy_clear(struct ATA_DEV *dev);
int ata_wait_ready_set(struct ATA_DEV *dev);
int ata_wait_busy_and_request_clear(struct ATA_DEV *dev);

#endif


//=============================================================================
// 非公開ヘッダ

#include "asmfunc.h"
#include "ata/cmd.h"
#include "ata/info.h"
#include "debug.h"


#define ATA_RETRY_MAX  8
#define ATA_TIMEOUT  10000000L


static struct ATA_DEV l_ata0 = {
    .base_port = 0x01F0,
    .irq = 14,
    .dev_ctrl_port = 0x03F6,
    .type = ATA_TYPE_UNKNOWN,
    .sel_dev = ATA_DH_SEL_DEV0
};

static struct ATA_DEV l_ata1 = {
    .base_port = 0x01F0,
    .irq = 14,
    .dev_ctrl_port = 0x03F6,
    .type = ATA_TYPE_UNKNOWN,
    .sel_dev = ATA_DH_SEL_DEV1
};

struct ATA_DEV *g_ata0 = &l_ata0;
struct ATA_DEV *g_ata1 = &l_ata1;


static int init_ata_dev(struct ATA_DEV *dev);
static void get_signature(struct ATA_DEV *dev);
static int get_identity(struct ATA_DEV *dev);
static void check_device_mode(struct ATA_DEV *dev);
static int init_device_mode(struct ATA_DEV *dev);

static void select_device(struct ATA_DEV *dev);
static void soft_reset(struct ATA_DEV *dev, bool enable_intr);

//=============================================================================
// 関数

void ata_init(void)
{
    soft_reset(g_ata0, true);

    init_ata_dev(g_ata0);
    init_ata_dev(g_ata1);

    struct ATA_DEV *default_dev = g_ata0;

    if (g_ata0->type == ATA_TYPE_PATA || g_ata0->type == ATA_TYPE_PATAPI) {
        // nop
    } else if (g_ata1->type == ATA_TYPE_PATA || g_ata1->type == ATA_TYPE_PATAPI) {
        default_dev = g_ata1;
    }

    select_device(default_dev);
}


static int init_ata_dev(struct ATA_DEV *dev)
{
    dbgf("---------- ata device %d ----------\n", (dev->sel_dev) ? 1 : 0);

    get_signature(dev);
    get_identity(dev);

    if (dev->type != ATA_TYPE_PATA && dev->type != ATA_TYPE_PATAPI) {
        return -1;
    }

    check_device_mode(dev);
    init_device_mode(dev);

    dbgf("\n");

    return 0;
}


static void get_signature(struct ATA_DEV *dev)
{
    dev->type = ATA_TYPE_UNKNOWN;

    for (int i = 0; i < ATA_RETRY_MAX; i++) {
        select_device(dev);

        if (inb(ATA_PORT_STATUS(dev)) == 0xFF) {
            dbgf("[ata] no connection (status == 0xFF)\n");
            break;  // 未接続
        }

        if (ata_wait_busy_clear(dev) < 0) {
            dbgf("[ata] no connection (busy)\n");
            break;  // 未接続
        }

        if ((inb(ATA_PORT_ERR(dev)) & 0x7F) != 1) {
            dbgf("[ata] err\n");
            break;  // デバイス不良
        }

        int sel_dev = inb(ATA_PORT_DRIVE_HEAD(dev)) & 0x10;

        if (sel_dev == dev->sel_dev) {
            int cyl_lo = inb(ATA_PORT_CYL_LO(dev));
            int cyl_hi = inb(ATA_PORT_CYL_HI(dev));
            int sig = (cyl_hi << 8) | cyl_lo;


            switch (sig) {
            case ATA_SIG_PATA:
                dbgf("[ata] type is ATA\n");
                dev->type = ATA_TYPE_PATA;
                break;

            case ATA_SIG_PATAPI:
                dbgf("[ata] type is ATAPI\n");
                dev->type = ATA_TYPE_PATAPI;
                break;

            default:
                dbgf("[ata] sig = %X\n", sig);
                dev->type = ATA_TYPE_UNKNOWN;
                break;
            }
 
            break;
        }
    }
}


// ATA デバイス接続確認
static int get_identity(struct ATA_DEV *dev)
{
    outb(ATA_PORT_DRIVE_HEAD(dev), dev->sel_dev);

    if (dev->type == ATA_TYPE_UNKNOWN) {
        dev->type = ATA_TYPE_NON;
        return -1;
    }

    if (ata_wait_busy_clear(dev) < 0) {
        DBGF("ERROR");
        dev->type = ATA_TYPE_UNKNOWN;
        return -1;
    }

    for (int i = 0; i < ATA_RETRY_MAX; i++) {
        int ret1 = ata_cmd_identify_device(dev);
        ata_wait_5ms(dev);
        int ret2 = ata_cmd_identify_device(dev);

        if (ret1 != ret2) {
            ata_wait_5ms(dev);
            continue;
        }

        if (ret1 < 0) {
            DBGF("ERROR");
            dev->type = ATA_TYPE_NON;
            return -1;
        }

        ata_anal_id(dev);
        return 0;
    }

    DBGF("ERROR");
    dev->type = ATA_TYPE_UNKNOWN;
    return -1;
}


static void check_device_mode(struct ATA_DEV *dev)
{
    if (dev->type != ATA_TYPE_PATA && dev->type != ATA_TYPE_PATAPI) {
        dev->mode = PIO_MODE_NON;
    }

    if (dev->info->support_pio_mode & SUPPORT_PIO_MODE4) {
        dev->mode = PIO_MODE4;
    } else if (dev->info->support_pio_mode & SUPPORT_PIO_MODE3) {
        dev->mode = PIO_MODE3;
    } else {
        dev->mode = PIO_MODE0;
    }
}


static int init_device_mode(struct ATA_DEV *dev)
{
    if (dev->type != ATA_TYPE_PATA && dev->type != ATA_TYPE_PATAPI) {
        return -1;
    }

    if (ata_select_device(dev) < 0) {
        DBGF("ERROR");
        return -1;
    }

    if (dev->type == ATA_TYPE_PATA) {
        if (ata_cmd_init_dev_params(dev) < 0) {
            DBGF("ERROR");
            return -1;
        }

        if (ata_cmd_idle(dev) < 0) {
            DBGF("ERROR");
            return -1;
        }
    } else if (dev->type == ATA_TYPE_PATAPI) {
        if (dev->info->support_pow_mng) {
            if (ata_cmd_idle(dev) < 0) {
                DBGF("ERROR");
                return -1;
            }
        }
    } else {
        ERROR("");
    }

    if (dev->info->support_iordy) {
        if (ata_cmd_set_features(dev, SET_TRANSFER, 0x08 | (dev->mode & 7)) < 0) {
            ata_cmd_set_features(dev, SET_TRANSFER, 0);
            dev->mode = 0;
        } else {
            dbgf("successful set features\n");
        }
    } else {
        ata_cmd_set_features(dev, SET_TRANSFER, 0);
        dev->mode = 0;
    }

    return 0;
}


static void select_device(struct ATA_DEV *dev)
{
    outb(ATA_PORT_DRIVE_HEAD(dev), dev->sel_dev | ATA_DH_OBS);
    ata_wait_400ns(dev);
}


// デバイス セレクション プロトコル
int ata_select_device(struct ATA_DEV *dev)
{
    ata_wait_busy_and_request_clear(dev);

    outb(ATA_PORT_DRIVE_HEAD(dev), dev->sel_dev | ATA_DH_OBS);
    ata_wait_400ns(dev);

    return ata_wait_busy_and_request_clear(dev);
}


int ata_select_device_ext(struct ATA_DEV *dev, uint8_t ext_flags)
{
    ata_wait_busy_and_request_clear(dev);

    outb(ATA_PORT_DRIVE_HEAD(dev), dev->sel_dev | ATA_DH_OBS | ext_flags);
    ata_wait_400ns(dev);

    return ata_wait_busy_and_request_clear(dev);
}


// ソフトウェア・リセット直後はデバイス0が選択されている
static void soft_reset(struct ATA_DEV *dev, bool enable_intr)
{
    uint8_t ex_flg = (enable_intr) ? 0 : ATA_DC_nIEN;

    outb(ATA_PORT_DEV_CTRL(dev), ATA_DC_SRST | ex_flg); // ソフトウェア・リセット
    ata_wait_5ms(dev);
    outb(ATA_PORT_DEV_CTRL(dev), ex_flg);  // ソフトウェア・リセット解除
    ata_wait_5ms(dev);
}


void ata_wait_5ms(struct ATA_DEV *dev)
{
    for (int i = 0; i < 50; i++) {
        inb(ATA_PORT_DEV_CTRL(dev));  // これが約100nsと仮定する
    }
}


void ata_wait_400ns(struct ATA_DEV *dev)
{
    inb(ATA_PORT_DEV_CTRL(dev));
    inb(ATA_PORT_DEV_CTRL(dev));
    inb(ATA_PORT_DEV_CTRL(dev));
    inb(ATA_PORT_DEV_CTRL(dev));
}


// BUSYビットがクリアされるまで待つ
int ata_wait_busy_clear(struct ATA_DEV *dev)
{
    for (int i = 0; i < ATA_TIMEOUT; i++) {
        if ((inb(ATA_PORT_ALT_STATUS(dev)) & ATA_ST_BSY) == 0)
            return 0;
    }

    DBGF("time out");
    return -1;
}


// DRDYビットがセットされるまで待つ
int ata_wait_ready_set(struct ATA_DEV *dev)
{
    for (int i = 0; i < ATA_TIMEOUT; i++) {
        if (inb(ATA_PORT_ALT_STATUS(dev)) & ATA_ST_DRDY)
            return 0;
    }

    DBGF("time out");
    return -1;
}


// BUSYビットとDRQビットがクリアされるまで待つ
int ata_wait_busy_and_request_clear(struct ATA_DEV *dev)
{
    for (int i = 0; i < ATA_TIMEOUT; i++) {
        int status = inb(ATA_PORT_ALT_STATUS(dev));
        if ((status & (ATA_ST_BSY | ATA_ST_DRQ)) == 0)
            return 0;
    }

    DBGF("time out");
    return -1;
}
