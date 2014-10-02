/*
 * QEMU model of Xsilon Hanadu 802.15.4 transceiver.
 *
 * Copyright (c) 2014 Martin Townsend <martin.townsend@xsilon.com>
 *
 */
#include "hw/sysbus.h"
#include "qemu/log.h"

#define TYPE_HANADU_TRX "xlnx,transceiver-1.00.a"
#define TYPE_HANADU_AFE "xlnx,afe-controller-2.00.a"
#define TYPE_HANADU_PUP "xlnx,powerup-controller-1.00.a"
#define TYPE_HANADU_MAC "xlnx,traffic-monitor-1.00.a"
#define TYPE_HANADU_TXB "xlnx,tx-buffer-3.00.a"
#define TYPE_HANADU_RXB "xlnx,rx-buffer-3.00.a"
#define TYPE_HANADU_HWV "xlnx,hardware-version-control-2.00.a"

#define HANADU_TRX_DEV(obj) \
     OBJECT_CHECK(struct han_trx_dev, (obj), TYPE_HANADU_TRX)
#define HANADU_AFE_DEV(obj) \
     OBJECT_CHECK(struct han_afe_dev, (obj), TYPE_HANADU_AFE)
#define HANADU_PUP_DEV(obj) \
     OBJECT_CHECK(struct han_pup_dev, (obj), TYPE_HANADU_PUP)
#define HANADU_MAC_DEV(obj) \
     OBJECT_CHECK(struct han_mac_dev, (obj), TYPE_HANADU_MAC)
#define HANADU_TXB_DEV(obj) \
     OBJECT_CHECK(struct han_txb_dev, (obj), TYPE_HANADU_TXB)
#define HANADU_RXB_DEV(obj) \
     OBJECT_CHECK(struct han_rxb_dev, (obj), TYPE_HANADU_RXB)
#define HANADU_HWV_DEV(obj) \
     OBJECT_CHECK(struct han_hwv_dev, (obj), TYPE_HANADU_HWV)


struct han_trx_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
    qemu_irq irq;

    uint32_t c_rxmem;
    uint32_t c_txmem;
};

struct han_afe_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
};

struct han_pup_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
};

struct han_mac_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
};

struct han_txb_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
};

struct han_rxb_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
};

struct han_hwv_dev {
    SysBusDevice busdev;
    MemoryRegion iomem;
};

/* __________________________________________________________ Hanadu Transceiver
 */

static Property han_trx_properties[] = {
    DEFINE_PROP_UINT32("rxmem", struct han_trx_dev, c_rxmem, 0x10000),
    DEFINE_PROP_UINT32("txmem", struct han_trx_dev, c_txmem, 0x10000),
//    DEFINE_NIC_PROPERTIES(XilinxAXIEnet, conf),
    DEFINE_PROP_END_OF_LIST(),
};

static uint64_t
han_trx_mem_region_read(void *opaque, hwaddr addr, unsigned size)
{
    return 0;
}

static void
han_trx_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
}

static const MemoryRegionOps han_trx_mem_region_ops = {
    .read = han_trx_mem_region_read,
    .write = han_trx_mem_region_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};


static void han_trx_realize(DeviceState *dev, Error **errp)
{
//    struct han_trx_dev *s = HANADU_TRX_DEV(dev);
//    Error *local_err = NULL;

    return;

//realize_fail:
//    if (!*errp) {
//        *errp = local_err;
//    }
}

static void han_trx_reset(DeviceState *dev)
{
//    struct han_trx_dev *s = HANADU_TRX_DEV(dev);
}


static void han_trx_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = han_trx_realize;
    dc->props = han_trx_properties;
    dc->reset = han_trx_reset;
}

static void han_trx_instance_init(Object *obj)
{
    struct han_trx_dev *s = HANADU_TRX_DEV(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    sysbus_init_irq(sbd, &s->irq);

    memory_region_init_io(&s->iomem, OBJECT(s), &han_trx_mem_region_ops, s,
                          "hantrx", 0x10000);
    sysbus_init_mmio(sbd, &s->iomem);
}

/* __________________________________________________________________ Hanadu AFE
 */

static uint64_t
han_afe_mem_region_read(void *opaque, hwaddr addr, unsigned size)
{
    return 0;
}

static void
han_afe_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
}

static const MemoryRegionOps han_afe_mem_region_ops = {
    .read = han_afe_mem_region_read,
    .write = han_afe_mem_region_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void han_afe_realize(DeviceState *dev, Error **errp)
{
//    struct han_afe_dev *s = HANADU_AFE_DEV(dev);
//    Error *local_err = NULL;

    return;

//realize_fail:
//    if (!*errp) {
//        *errp = local_err;
//    }
}

static void han_afe_reset(DeviceState *dev)
{
//    struct han_afe_dev *s = HANADU_AFE_DEV(dev);
}


static void han_afe_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = han_afe_realize;
    dc->props = NULL;
    dc->reset = han_afe_reset;
}

static void han_afe_instance_init(Object *obj)
{
    struct han_afe_dev *s = HANADU_AFE_DEV(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, OBJECT(s), &han_afe_mem_region_ops, s,
                          "hanafe", 0x10000);
    sysbus_init_mmio(sbd, &s->iomem);
}

/* __________________________________________________________________ Hanadu PUP
 */

static uint64_t
han_pup_mem_region_read(void *opaque, hwaddr addr, unsigned size)
{
    return 0;
}

static void
han_pup_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
}

static const MemoryRegionOps han_pup_mem_region_ops = {
    .read = han_pup_mem_region_read,
    .write = han_pup_mem_region_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void han_pup_realize(DeviceState *dev, Error **errp)
{
//    struct han_pup_dev *s = HANADU_PUP_DEV(dev);
//    Error *local_err = NULL;

    return;

//realize_fail:
//    if (!*errp) {
//        *errp = local_err;
//    }
}

static void han_pup_reset(DeviceState *dev)
{
//    struct han_pup_dev *s = HANADU_PUP_DEV(dev);
}


static void han_pup_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = han_pup_realize;
    dc->props = NULL;
    dc->reset = han_pup_reset;
}

static void han_pup_instance_init(Object *obj)
{
    struct han_pup_dev *s = HANADU_PUP_DEV(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, OBJECT(s), &han_pup_mem_region_ops, s,
                          "hanpup", 0x10000);
    sysbus_init_mmio(sbd, &s->iomem);
}

/* __________________________________________________________________ Hanadu MAC
 */

static uint64_t
han_mac_mem_region_read(void *opaque, hwaddr addr, unsigned size)
{
    return 0;
}

static void
han_mac_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
}

static const MemoryRegionOps han_mac_mem_region_ops = {
    .read = han_mac_mem_region_read,
    .write = han_mac_mem_region_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void han_mac_realize(DeviceState *dev, Error **errp)
{
//    struct han_mac_dev *s = HANADU_MAC_DEV(dev);
//    Error *local_err = NULL;

    return;

//realize_fail:
//    if (!*errp) {
//        *errp = local_err;
//    }
}

static void han_mac_reset(DeviceState *dev)
{
//    struct han_mac_dev *s = HANADU_MAC_DEV(dev);
}


static void han_mac_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = han_mac_realize;
    dc->props = NULL;
    dc->reset = han_mac_reset;
}

static void han_mac_instance_init(Object *obj)
{
    struct han_mac_dev *s = HANADU_MAC_DEV(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, OBJECT(s), &han_mac_mem_region_ops, s,
                          "hanmac", 0x10000);
    sysbus_init_mmio(sbd, &s->iomem);
}

/* __________________________________________________________________ Hanadu TXB
 */

static uint64_t
han_txb_mem_region_read(void *opaque, hwaddr addr, unsigned size)
{
    return 0;
}

static void
han_txb_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
}

static const MemoryRegionOps han_txb_mem_region_ops = {
    .read = han_txb_mem_region_read,
    .write = han_txb_mem_region_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void han_txb_realize(DeviceState *dev, Error **errp)
{
//    struct han_txb_dev *s = HANADU_TXB_DEV(dev);
//    Error *local_err = NULL;

    return;

//realize_fail:
//    if (!*errp) {
//        *errp = local_err;
//    }
}

static void han_txb_reset(DeviceState *dev)
{
//    struct han_txb_dev *s = HANADU_TXB_DEV(dev);
}


static void han_txb_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = han_txb_realize;
    dc->props = NULL;
    dc->reset = han_txb_reset;
}

static void han_txb_instance_init(Object *obj)
{
    struct han_txb_dev *s = HANADU_TXB_DEV(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, OBJECT(s), &han_txb_mem_region_ops, s,
                          "hantxb", 0x10000);
    sysbus_init_mmio(sbd, &s->iomem);
}

/* __________________________________________________________________ Hanadu RXB
 */

static uint64_t
han_rxb_mem_region_read(void *opaque, hwaddr addr, unsigned size)
{
    return 0;
}

static void
han_rxb_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
}

static const MemoryRegionOps han_rxb_mem_region_ops = {
    .read = han_rxb_mem_region_read,
    .write = han_rxb_mem_region_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void han_rxb_realize(DeviceState *dev, Error **errp)
{
//    struct han_rxb_dev *s = HANADU_RXB_DEV(dev);
//    Error *local_err = NULL;

    return;

//realize_fail:
//    if (!*errp) {
//        *errp = local_err;
//    }
}

static void han_rxb_reset(DeviceState *dev)
{
//    struct han_rxb_dev *s = HANADU_RXB_DEV(dev);
}


static void han_rxb_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = han_rxb_realize;
    dc->props = NULL;
    dc->reset = han_rxb_reset;
}

static void han_rxb_instance_init(Object *obj)
{
    struct han_rxb_dev *s = HANADU_RXB_DEV(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, OBJECT(s), &han_rxb_mem_region_ops, s,
                          "hanrxb", 0x10000);
    sysbus_init_mmio(sbd, &s->iomem);
}

/* __________________________________________________________________ Hanadu HWV
 */

static uint64_t
han_hwv_mem_region_read(void *opaque, hwaddr addr, unsigned size)
{
    addr >>= 2;

    switch (addr) {
    case 0:
    case 1:
    case 2:
       return 9999;
    case 3:
        return 1;
    }

    return 0;
}

static void
han_hwv_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
    /* This is a read only register block */
}

static const MemoryRegionOps han_hwv_mem_region_ops = {
    .read = han_hwv_mem_region_read,
    .write = han_hwv_mem_region_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void han_hwv_realize(DeviceState *dev, Error **errp)
{
//    struct han_hwv_dev *s = HANADU_HWV_DEV(dev);
//    Error *local_err = NULL;

    return;

//realize_fail:
//    if (!*errp) {
//        *errp = local_err;
//    }
}

static void han_hwv_reset(DeviceState *dev)
{
//    struct han_hwv_dev *s = HANADU_HWV_DEV(dev);
}


static void han_hwv_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = han_hwv_realize;
    dc->props = NULL;
    dc->reset = han_hwv_reset;
}

static void han_hwv_instance_init(Object *obj)
{
    struct han_hwv_dev *s = HANADU_HWV_DEV(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, OBJECT(s), &han_hwv_mem_region_ops, s,
                          "hanhwv", 0x10000);
    sysbus_init_mmio(sbd, &s->iomem);
}

/* _________________________________________________________ Hanadu Registration
 */

static const TypeInfo han_trx_info = {
    .name  = TYPE_HANADU_TRX,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(struct han_trx_dev),
    .class_init = han_trx_class_init,
    .instance_init = han_trx_instance_init,
};

static const TypeInfo han_afe_info = {
    .name  = TYPE_HANADU_AFE,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(struct han_afe_dev),
    .class_init = han_afe_class_init,
    .instance_init = han_afe_instance_init,
};

static const TypeInfo han_pup_info = {
    .name  = TYPE_HANADU_PUP,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(struct han_pup_dev),
    .class_init = han_pup_class_init,
    .instance_init = han_pup_instance_init,
};

static const TypeInfo han_mac_info = {
    .name  = TYPE_HANADU_MAC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(struct han_mac_dev),
    .class_init = han_mac_class_init,
    .instance_init = han_mac_instance_init,
};

static const TypeInfo han_txb_info = {
    .name  = TYPE_HANADU_TXB,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(struct han_txb_dev),
    .class_init = han_txb_class_init,
    .instance_init = han_txb_instance_init,
};

static const TypeInfo han_rxb_info = {
    .name  = TYPE_HANADU_RXB,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(struct han_rxb_dev),
    .class_init = han_rxb_class_init,
    .instance_init = han_rxb_instance_init,
};

static const TypeInfo han_hwv_info = {
    .name  = TYPE_HANADU_HWV,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(struct han_hwv_dev),
    .class_init = han_hwv_class_init,
    .instance_init = han_hwv_instance_init,
};

static void han_register_types(void)
{
    type_register_static(&han_trx_info);
    type_register_static(&han_afe_info);
    type_register_static(&han_pup_info);
    type_register_static(&han_mac_info);
    type_register_static(&han_txb_info);
    type_register_static(&han_rxb_info);
    type_register_static(&han_hwv_info);
}

type_init(han_register_types)


