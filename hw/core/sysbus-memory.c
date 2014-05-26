/*
 * Sysbus Attached Memory.
 *
 * Copyright (c) 2014 Xilinx Inc.
 * Written by Peter Crosthwaite <peter.crosthwaite@xilinx.com>
 *
 * This code is licensed under the GPL.
 */

#include "hw/sysbus.h"

#define TYPE_SYS_BUS_MEMORY "sysbus-memory"

#define SYS_BUS_MEMORY(obj) \
         OBJECT_CHECK(SysBusMemory, (obj), TYPE_SYS_BUS_MEMORY)

typedef struct SysBusMemory {
    SysBusDevice parent_obj;

    uint64_t size;
    bool read_only;

    MemoryRegion mem;
} SysBusMemory;

static void sysbus_memory_realize(DeviceState *dev, Error **errp)
{
    SysBusMemory *s = SYS_BUS_MEMORY(dev);

    memory_region_init_ram(&s->mem, OBJECT(dev),
                           dev->id ? dev->id : "sysbus-memory", s->size);
    memory_region_set_readonly(&s->mem, s->read_only);
    vmstate_register_ram_global(&s->mem);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->mem);
}

static Property sysbus_memory_props[] = {
    DEFINE_PROP_UINT64("size", SysBusMemory, size, 0),
    DEFINE_PROP_BOOL("read-only", SysBusMemory, read_only, false),
    DEFINE_PROP_END_OF_LIST(),
};

static void sysbus_memory_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->props = sysbus_memory_props;
    dc->realize = sysbus_memory_realize;
}

static const TypeInfo sysbus_memory_info = {
    .name          = TYPE_SYS_BUS_MEMORY,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(SysBusMemory),
    .class_init    = sysbus_memory_class_init,
};

static void sysbus_memory_register_types(void)
{
    type_register_static(&sysbus_memory_info);
}

type_init(sysbus_memory_register_types)
