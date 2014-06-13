/**
 * PCI
 */


//=============================================================================
// 公開ヘッダ

#ifndef HEADER_PCI
#define HEADER_PCI

#include <stdint.h>

// PCI
struct PCI_CONF {
    int bus;
    int dev;
    int func;

    uint32_t io_addr;
    uint32_t mm_addr;

    uint16_t dev_id;
    uint16_t vendor_id;
    uint16_t status;
    uint16_t cmd;
    uint8_t  cls;
    uint8_t  sub_cls;
    uint8_t  prog_if;
    uint8_t  rev;
    uint8_t  header_type;
    uint32_t base_addr[6];
    uint8_t  irq_pin;
    uint8_t  irq_line;
};


void pci_init(void);
void pci_enum(void);
struct PCI_CONF *pci_get_conf(uint16_t vendor_id, uint16_t dev_id);
void pci_write_cmd(struct PCI_CONF *conf);
void pci_read_cmd(struct PCI_CONF *conf);


#endif


//=============================================================================
// 非公開ヘッダ

#include "debug.h"

#define MAX_CONF 16

#define PCI_MAX_BUS  0xFF
#define PCI_MAX_DEV  0x20
#define PCI_MAX_FUNC 0x08

#define PCI_CONF_OFFSET_CMD  0x04


static struct PCI_CONF l_confs[MAX_CONF];


static uint16_t pci_get_vendor_id(uint8_t bus, uint8_t dev);
static void pci_get_config(struct PCI_CONF *conf, uint8_t bus, uint8_t dev, uint8_t func);
static void pci_print_conf(struct PCI_CONF *conf);

static uint32_t pci_read_conf_dword(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset);
static uint16_t pci_read_conf_word(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset);

static void pci_write_conf_word(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint16_t data);

static uint32_t get_pci_addr(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset);


//=============================================================================
// 公開関数


void pci_init(void)
{
    for (int i = 0; i < MAX_CONF; i++) {
        l_confs[i].vendor_id = 0xFFFF;
        l_confs[i].io_addr = 0;
        l_confs[i].mm_addr = 0;
    }

    int num_conf = 0;

    for (int bus = 0; bus < PCI_MAX_BUS; bus++) {
        for (int dev = 0; dev < PCI_MAX_DEV; dev++) {
            uint16_t vendor_id = pci_get_vendor_id(bus, dev);

            if (vendor_id != 0xFFFF) {
                pci_get_config(&l_confs[num_conf], bus, dev, 0);

                num_conf++;
                if (num_conf >= MAX_CONF)
                    return;
            }
        }
    }
}


void pci_enum(void)
{
    for (int i = 0; i < MAX_CONF; i++) {
        if (l_confs[i].vendor_id == 0xFFFF)
            break;

        pci_print_conf(&l_confs[i]);
    }
}


struct PCI_CONF *pci_get_conf(uint16_t vendor_id, uint16_t dev_id)
{
    for (int i = 0; i < MAX_CONF; i++) {
        struct PCI_CONF *conf = &l_confs[i];
        if (conf->vendor_id == 0xFFFF)
            break;

        if (conf->vendor_id == vendor_id && conf->dev_id == dev_id)
            return conf;
    }

    return 0;
}


void pci_write_cmd(struct PCI_CONF *conf)
{
    pci_write_conf_word(conf->bus, conf->dev, conf->func, PCI_CONF_OFFSET_CMD, conf->cmd);
}


void pci_read_cmd(struct PCI_CONF *conf)
{
    conf->cmd = pci_read_conf_word(conf->bus, conf->dev, conf->func, PCI_CONF_OFFSET_CMD);
}


//=============================================================================
// 非公開関数

static uint16_t pci_get_vendor_id(uint8_t bus, uint8_t dev)
{
    uint32_t v = pci_read_conf_dword(bus, dev, 0, 0);

    return v & 0xFFFF;
}


static void pci_get_config(struct PCI_CONF *conf, uint8_t bus, uint8_t dev, uint8_t func)
{
    conf->bus = bus;
    conf->dev = dev;
    conf->func = func;

    uint32_t v = pci_read_conf_dword(bus, dev, 0, 0x00);
    conf->vendor_id = v & 0xFFFF;
    conf->dev_id    = (v >> 16) & 0xFFFF;

    v = pci_read_conf_dword(bus, dev, 0, 0x04);
    conf->status  = (v >> 16) & 0xFFFF;
    conf->cmd     = v & 0xFFFF;

    v = pci_read_conf_dword(bus, dev, 0, 0x08);
    conf->cls     = (v >> 24) & 0xFF;
    conf->sub_cls = (v >> 16) & 0xFF;
    conf->prog_if = (v >>  8) & 0xFF;
    conf->rev     = v & 0xFF;

    v = pci_read_conf_dword(bus, dev, 0, 0x0C);
    conf->header_type = (v >> 16) & 0xFF;

    for (int i = 0; i < 6; i++) {
        v = pci_read_conf_dword(bus, dev, 0, 0x10 + (i * 4));
        conf->base_addr[i] = v;

        if (v & 1) {
            if (conf->io_addr == 0)
                conf->io_addr = v & 0xFFFFFFFC;
        } else {
            if (conf->mm_addr == 0)
                conf->mm_addr = v & 0xFFFFFFF0;
        }
    }

    v = pci_read_conf_dword(bus, dev, 0, 0x3C);
    conf->irq_pin  = (v >>  8) & 0xFF;
    conf->irq_line = v & 0xFF;
}


static void pci_print_conf(struct PCI_CONF *conf)
{
    dbgf("bus = %d, dev = %d, %X:%X, class = %X\n", conf->bus, conf->dev, conf->vendor_id, conf->dev_id, conf->cls);

    if (conf->io_addr)
        dbgf("io = %X\n", conf->io_addr);

    if (conf->mm_addr)
        dbgf("memory = %X\n", conf->mm_addr);

    dbgf("irq = %d\n\n", conf->irq_line);
}


static uint32_t pci_read_conf_dword(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{
    uint32_t addr = get_pci_addr(bus, dev, func, offset);

    outl(0x0CF8, addr);

    return inl(0x0CFC);
}


static uint16_t pci_read_conf_word(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{
    uint32_t addr = get_pci_addr(bus, dev, func, offset);

    outl(0x0CF8, addr);

    if ((offset & 0x02) == 0)
        return inw(0x0CFC);
    else
        return inw(0x0CFE);
}


static void pci_write_conf_word(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint16_t data)
{
    uint32_t addr = get_pci_addr(bus, dev, func, offset);

    outl(0x0CF8, addr);

    if ((offset & 0x02) == 0)
        outw(0x0CFC, data);
    else
        outw(0x0CFE, data);
}


static uint32_t get_pci_addr(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{
    ASSERT(dev < PCI_MAX_DEV, "dev = 0x%X", dev);
    ASSERT(func < PCI_MAX_FUNC, "func = 0x%X", func);
    ASSERT((offset & 0x03) == 0, "offset = 0x%X", offset);

    return 0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | offset;
}
