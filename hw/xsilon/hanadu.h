
#ifndef _HANADU_QDEV_INC
#define _HANADU_QDEV_INC

#include "hw/sysbus.h"
#include "qom/object.h"
#include "hanadu-defs-qdev.h"

#define TYPE_HANADU_TRXM "xlnx,transceiver-1.00.a"
#define TYPE_HANADU_AFE "xlnx,afe-controller-2.00.a"
#define TYPE_HANADU_PWR "xlnx,powerup-controller-1.00.a"
#define TYPE_HANADU_MAC "xlnx,traffic-monitor-1.00.a"
#define TYPE_HANADU_TXB "xlnx,tx-buffer-3.00.a"
#define TYPE_HANADU_RXB "xlnx,rx-buffer-3.00.a"
#define TYPE_HANADU_HWVERS "xlnx,hardware-version-control-2.00.a"


#define HANADU_TRXM_DEV(obj) \
     OBJECT_CHECK(struct han_trxm_dev, (obj), TYPE_HANADU_TRXM)
#define HANADU_AFE_DEV(obj) \
     OBJECT_CHECK(struct han_afe_dev, (obj), TYPE_HANADU_AFE)
#define HANADU_PWR_DEV(obj) \
     OBJECT_CHECK(struct han_pwr_dev, (obj), TYPE_HANADU_PWR)
#define HANADU_MAC_DEV(obj) \
     OBJECT_CHECK(struct han_mac_dev, (obj), TYPE_HANADU_MAC)
#define HANADU_TXB_DEV(obj) \
     OBJECT_CHECK(struct han_txb_dev, (obj), TYPE_HANADU_TXB)
#define HANADU_RXB_DEV(obj) \
     OBJECT_CHECK(struct han_rxb_dev, (obj), TYPE_HANADU_RXB)
#define HANADU_HWVERS_DEV(obj) \
     OBJECT_CHECK(struct han_hwvers_dev, (obj), TYPE_HANADU_HWVERS)

typedef int (*mem_region_write_fnp)(void *opaque, hwaddr addr, uint64_t value, unsigned size);
typedef int (*mem_region_read_fnp)(void *opaque, hwaddr addr, unsigned size, uint64_t *value);

struct han_trxm_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
    mem_region_write_fnp mem_region_write;
    mem_region_read_fnp mem_region_read;
    qemu_irq rx_irq;
    qemu_irq rx_fail_irq;
    qemu_irq tx_irq;
    bool rx_irq_state;
    bool tx_irq_state;

    struct han_regmap_trxm regs;

    uint32_t c_rxmem;
    uint32_t c_txmem;
};

struct han_afe_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
    mem_region_write_fnp mem_region_write;
    mem_region_read_fnp mem_region_read;

    struct han_regmap_afe regs;
};

struct han_pwr_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
    mem_region_write_fnp mem_region_write;
    mem_region_read_fnp mem_region_read;

    struct han_regmap_pwr regs;
};

struct han_mac_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
    mem_region_write_fnp mem_region_write;
    mem_region_read_fnp mem_region_read;

    struct han_regmap_mac regs;
};

struct han_txb_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
};

struct han_rxb_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
};

struct han_hwvers_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
    mem_region_write_fnp mem_region_write;
    mem_region_read_fnp mem_region_read;

    struct han_regmap_hwvers regs;
};



#endif
