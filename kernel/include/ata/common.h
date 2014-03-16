// Generated by mkhdr

#ifndef HEADER_ATA_COMMON
#define HEADER_ATA_COMMON

#include <stdbool.h>
#include <stdint.h>

// ATA シグネチャ
#define ATA_SIG_PATA    0x0000
#define ATA_SIG_PATAPI  0xEB14
//#define ATA_SIG_SATA    0xC33C
//#define ATA_SIG_SATAPI  0x9669

// ATA デバイスタイプ
#define ATA_TYPE_NON     (1 << 0)  // 未接続
#define ATA_TYPE_UNKNOWN (1 << 1)  // 不明デバイス
#define ATA_TYPE_PATA    (1 << 2)
#define ATA_TYPE_PATAPI  (1 << 3)
//#define ATA_TYPE_SATA    (1 << 4)
//#define ATA_TYPE_SATAPI  (1 << 5)


// ---- I/O ポート

#define ATA_PORT_DATA(dev)          ((dev)->base_port + 0)
#define ATA_PORT_FEATURES(dev)      ((dev)->base_port + 1)
#define ATA_PORT_ERR(dev)           ((dev)->base_port + 1)
#define ATA_PORT_SECTOR_CNT(dev)    ((dev)->base_port + 2)
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

// Device Control レジスタ
#define ATA_DC_nIEN  (1 << 1)  // INTRQ信号の有効／無効
#define ATA_DC_SRST  (1 << 2)  // ソフトウェア・リセット


struct ATA_DEV {
    int base_port;
    int irq;
    int dev_ctrl_port;  // Device Control Registers/Alternate Status ports
    int type;  // デバイスタイプ
    int sel_dev;  // マスタならATA_DH_SEL_DEV0, スレーブならATA_DH_SEL_DEV1
    uint16_t identity[256];  // IDENTIFY DEVICE コマンドを実行したした結果wwwwww
};


extern struct ATA_DEV *g_ata0;
extern struct ATA_DEV *g_ata1;


void ata_init(void);

int ata_select_device(struct ATA_DEV *dev);
void ata_wait_5ms(struct ATA_DEV *dev);
void ata_wait_400ns(struct ATA_DEV *dev);
int ata_wait_busy_clear(struct ATA_DEV *dev);
int ata_wait_ready_set(struct ATA_DEV *dev);
int ata_wait_busy_and_request_clear(struct ATA_DEV *dev);
void ata_read_data(struct ATA_DEV *dev, uint16_t *buf, int len);

#endif
