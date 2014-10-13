/*
 * QEMU model of Xsilon Hanadu 802.15.4 transceiver.
 *
 * Copyright (c) 2014 Martin Townsend <martin.townsend@xsilon.com>
 *
 */
#include "hw/sysbus.h"
#include "qemu/log.h"
#include "hanadu.h"
#include "hanadu-inl.h"
#include "xsilon.h"

#include <assert.h>

enum han_state {
	HAN_STATE_IDLE = 0,
	HAN_STATE_RX,
	HAN_STATE_TX_CSMA,
	HAN_STATE_TX_BACKOFF,
	HAN_STATE_TX,
};

struct txbuf {
	uint8_t data[128];
};
struct rxbuf {
	uint8_t data[128];
};

static struct hanadu {
	uint8_t dip_board;
	uint8_t dip_afe;

	enum han_state state;

	struct han_rx {
		bool enabled;
		struct rxbuf buf[4];
	} rx;
	struct han_tx {
		bool enabled;
		uint8_t bufsel;
		struct txbuf buf[2];
	} tx;
} han;


void
hanadu_set_default_dip_board(uint8_t dip)
{
	han.dip_board = dip;
}

void
hanadu_set_default_dip_afe(uint8_t dip)
{
	han.dip_afe = dip;
}

static void
hanadu_tx_buffer_to_netsim(struct han_trxm_dev *s, uint8_t * data, uint16_t len, uint8_t repcode)
{

	/* If buffer successfully sent then generate interrupt */
	qemu_set_irq(s->tx_irq, 1);
	s->tx_irq_state = 1;

	/* loop it back to receiver buffer 0 */
#if 1
	memcpy(han.rx.buf[0].data, data, len);

	/* Need write accessor functions until then just write to regs */
	han_trxm_rxm_psdulen0_set(s, len);
	han_trxm_rxm_repcode0_set(s, repcode);
	han_trxm_rxm_memory_bank_next_to_process_set(s, 0);


#endif
}

/* __________________________________________________________ Hanadu Transceiver
 */

static void
han_trxm_txm_enable_changed(uint32_t value, void *hw_block)
{
	if(value) {
		assert(han.tx.enabled == false);
		han.tx.enabled = true;
	} else {
		assert(han.tx.enabled == true);
		han.tx.enabled = false;
	}
}

static void
han_trxm_rxm_enable_changed(uint32_t value, void *hw_block)
{
	if(value) {
		assert(han.rx.enabled == false);
		han.rx.enabled = true;
	} else {
		assert(han.rx.enabled == true);
		han.rx.enabled = false;
	}
}

static void
han_trxm_txm_mem_bank_select_changed(uint32_t value, void *hw_block)
{
	han.tx.bufsel = value;
}

static int
_han_trxm_mem_region_read(void *opaque, hwaddr addr, unsigned size, uint64_t *value)
{
	struct han_trxm_dev *s = HANADU_TRXM_DEV(opaque);

	if(s->tx_irq_state) {
		qemu_set_irq(s->tx_irq, 0);
		s->tx_irq_state = 0;
		qemu_set_irq(s->rx_irq, 1);
		s->rx_irq_state = 1;
	} else if(s->rx_irq_state) {
		qemu_set_irq(s->rx_irq, 0);
		s->rx_irq_state = 0;
	}
	return 0;
}



static void
han_trxm_txm_start_changed(uint32_t value, void *hw_block)
{
	struct han_trxm_dev *s = HANADU_TRXM_DEV(hw_block);
	uint16_t psdu_len;
	uint8_t rep_code;
	uint8_t * buf;

	if(value) {
		/* Start transmission */
		if(han.tx.bufsel == 0) {
			psdu_len = han_trxm_txm_psdu_len0_get(hw_block);
			rep_code = han_trxm_txm_rep_code0_get(hw_block);
			buf = han.tx.buf[0].data;
		} else {
			psdu_len = han_trxm_txm_psdu_len1_get(hw_block);
			rep_code = han_trxm_txm_rep_code1_get(hw_block);
			buf = han.tx.buf[1].data;
		}
		/* Send buffer to powerline network simulator */
		hanadu_tx_buffer_to_netsim(s, buf, psdu_len, rep_code);
	} else {
		/* Tx finished, just do a sanity check */
	}
}

static Property han_trxm_properties[] = {
    DEFINE_PROP_UINT32("rxmem", struct han_trxm_dev, c_rxmem, 0x10000),
    DEFINE_PROP_UINT32("txmem", struct han_trxm_dev, c_txmem, 0x10000),
//    DEFINE_NIC_PROPERTIES(XilinxAXIEnet, conf),
    DEFINE_PROP_END_OF_LIST(),
};

static const MemoryRegionOps han_trxm_mem_region_ops = {
    .read = han_trxm_mem_region_read,
    .write = han_trxm_mem_region_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};


static void han_trxm_realize(DeviceState *dev, Error **errp)
{
//    struct han_trxm_dev *s = HANADU_TRXM_DEV(dev);
//    Error *local_err = NULL;

    return;

//realize_fail:
//    if (!*errp) {
//        *errp = local_err;
//    }
}

static void han_trxm_reset(DeviceState *dev)
{
//    struct han_trxm_dev *s = HANADU_TRXM_DEV(dev);
}


static void han_trxm_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = han_trxm_realize;
    dc->props = han_trxm_properties;
    dc->reset = han_trxm_reset;
}

static void han_trxm_instance_init(Object *obj)
{
    struct han_trxm_dev *s = HANADU_TRXM_DEV(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, OBJECT(s), &han_trxm_mem_region_ops, s,
                          "hantrxm", 0x10000);
    sysbus_init_mmio(sbd, &s->iomem);

    sysbus_init_irq(sbd, &s->rx_irq);
    sysbus_init_irq(sbd, &s->rx_fail_irq);
    sysbus_init_irq(sbd, &s->tx_irq);

    s->mem_region_write = NULL;

    /* Override Register/Bitfield changed functions */
    s->mem_region_read = _han_trxm_mem_region_read;
    s->regs.field_changed.txm_enable_changed = han_trxm_txm_enable_changed;
    s->regs.field_changed.rxm_enable_changed = han_trxm_rxm_enable_changed;
    s->regs.field_changed.txm_start_changed = han_trxm_txm_start_changed;
    s->regs.field_changed.txm_mem_bank_select_changed = han_trxm_txm_mem_bank_select_changed;
}

/* __________________________________________________________________ Hanadu AFE
 */

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
    s->mem_region_write = NULL;
}

/* __________________________________________________________________ Hanadu PWR
 */

static const MemoryRegionOps han_pwr_mem_region_ops = {
    .read = han_pwr_mem_region_read,
    .write = han_pwr_mem_region_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void han_pwr_realize(DeviceState *dev, Error **errp)
{
//    struct han_pwr_dev *s = HANADU_PWR_DEV(dev);
//    Error *local_err = NULL;

    return;

//realize_fail:
//    if (!*errp) {
//        *errp = local_err;
//    }
}

static void han_pwr_reset(DeviceState *dev)
{
    struct han_pwr_dev *s = HANADU_PWR_DEV(dev);

    s->regs.pup_dips1 = han.dip_board;
    s->regs.pup_dips2 = han.dip_afe;
}


static void han_pwr_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = han_pwr_realize;
    dc->props = NULL;
    dc->reset = han_pwr_reset;
}

static void han_pwr_instance_init(Object *obj)
{
    struct han_pwr_dev *s = HANADU_PWR_DEV(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, OBJECT(s), &han_pwr_mem_region_ops, s,
                          "hanpwr", 0x10000);
    sysbus_init_mmio(sbd, &s->iomem);
    s->mem_region_write = NULL;
}

/* __________________________________________________________________ Hanadu MAC
 */

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
    s->mem_region_write = NULL;
}

/* __________________________________________________________________ Hanadu TXB
 */

static uint64_t
han_txb_mem_region_read(void *opaque, hwaddr addr, unsigned size)
{
	uint8_t * buf;

	assert(size == 1);
	if(addr < 0x1000) {
		addr >>= 2;
		buf = han.tx.buf[0].data + addr;
	} else {
		addr -= 0x1000;
		addr >>= 2;
		buf = han.tx.buf[1].data + addr;
	}
	assert(addr <= 128);

	return (uint64_t)*buf;
}

static void
han_txb_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
	uint8_t * buf;

	assert(size == 1);
	if(addr < 0x1000) {
		addr >>= 2;
		buf = han.tx.buf[0].data + addr;
	} else {
		addr -= 0x1000;
		addr >>= 2;
		buf = han.tx.buf[1].data + addr;
	}
	assert(addr <= 128);

	*buf = (uint8_t)value;
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

/* Override the auto generated functions */
uint64_t
han_hwvers_mem_region_read(void *opaque, hwaddr addr, unsigned size)
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

void
han_hwvers_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
    /* This is a read only register block */
}

static const MemoryRegionOps han_hwvers_mem_region_ops = {
    .read = han_hwvers_mem_region_read,
    .write = han_hwvers_mem_region_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void han_hwvers_realize(DeviceState *dev, Error **errp)
{
//    struct han_hwv_dev *s = HANADU_HWV_DEV(dev);
//    Error *local_err = NULL;

    return;

//realize_fail:
//    if (!*errp) {
//        *errp = local_err;
//    }
}

static void han_hwvers_reset(DeviceState *dev)
{
//    struct han_hwv_dev *s = HANADU_HWV_DEV(dev);
}


static void han_hwvers_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = han_hwvers_realize;
    dc->props = NULL;
    dc->reset = han_hwvers_reset;
}

static void han_hwvers_instance_init(Object *obj)
{
    struct han_hwvers_dev *s = HANADU_HWVERS_DEV(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, OBJECT(s), &han_hwvers_mem_region_ops, s,
                          "hanhwv", 0x10000);
    sysbus_init_mmio(sbd, &s->iomem);
    s->mem_region_write = NULL;
}

/* _________________________________________________________ Hanadu Registration
 */

static const TypeInfo han_trxm_info = {
    .name  = TYPE_HANADU_TRXM,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(struct han_trxm_dev),
    .class_init = han_trxm_class_init,
    .instance_init = han_trxm_instance_init,
};

static const TypeInfo han_afe_info = {
    .name  = TYPE_HANADU_AFE,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(struct han_afe_dev),
    .class_init = han_afe_class_init,
    .instance_init = han_afe_instance_init,
};

static const TypeInfo han_pwr_info = {
    .name  = TYPE_HANADU_PWR,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(struct han_pwr_dev),
    .class_init = han_pwr_class_init,
    .instance_init = han_pwr_instance_init,
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
    .name  = TYPE_HANADU_HWVERS,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(struct han_hwvers_dev),
    .class_init = han_hwvers_class_init,
    .instance_init = han_hwvers_instance_init,
};

static void han_register_types(void)
{
    type_register_static(&han_trxm_info);
    type_register_static(&han_afe_info);
    type_register_static(&han_pwr_info);
    type_register_static(&han_mac_info);
    type_register_static(&han_txb_info);
    type_register_static(&han_rxb_info);
    type_register_static(&han_hwv_info);
}

type_init(han_register_types)


