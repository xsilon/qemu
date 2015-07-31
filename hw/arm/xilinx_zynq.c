/*
 * Xilinx Zynq Baseboard System emulation.
 *
 * Copyright (c) 2010 Xilinx.
 * Copyright (c) 2012 Peter A.G. Crosthwaite (peter.croshtwaite@petalogix.com)
 * Copyright (c) 2012 Petalogix Pty Ltd.
 * Written by Haibing Ma
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "hw/sysbus.h"
#include "hw/arm/arm.h"
#include "net/net.h"
#include "exec/address-spaces.h"
#include "sysemu/sysemu.h"
#include "hw/boards.h"
#include "hw/block/flash.h"
#include "sysemu/blockdev.h"
#include "hw/loader.h"
#include "hw/ssi.h"
#include "hw/i2c/i2c.h"
#include "qapi/qmp/qerror.h"

#define MAX_CPUS 2

#define NUM_SPI_FLASHES 4
#define NUM_QSPI_FLASHES 1
#define NUM_QSPI_BUSSES 2

#define FLASH_SIZE (64 * 1024 * 1024)
#define FLASH_SECTOR_SIZE (128 * 1024)

#define NUM_I2C_EEPROMS 2

#define IRQ_OFFSET 32 /* pic interrupts start from index 32 */

#define SMP_BOOT_ADDR 0x0fff0000
#define SMP_BOOTREG_ADDR 0xfffffff0

static const int dma_irqs[8] = {
    46, 47, 48, 49, 72, 73, 74, 75
};

/* Entry point for secondary CPU */
static uint32_t zynq_smpboot[] = {
    0xe3e0000f, /* ldr r0, =0xfffffff0 (mvn r0, #15) */
    0xe320f002, /* wfe */
    0xe5901000, /* ldr     r1, [r0] */
    0xe1110001, /* tst     r1, r1 */
    0x0afffffb, /* beq     <wfe> */
    0xe12fff11, /* bx      r1 */
    0,
};

static void zynq_write_secondary_boot(ARMCPU *cpu,
                                      const struct arm_boot_info *info)
{
    int n;

    for (n = 0; n < ARRAY_SIZE(zynq_smpboot); n++) {
        zynq_smpboot[n] = tswap32(zynq_smpboot[n]);
    }
    rom_add_blob_fixed("smpboot", zynq_smpboot, sizeof(zynq_smpboot),
                       SMP_BOOT_ADDR);
}

static struct arm_boot_info zynq_binfo = {};

static void gem_init(NICInfo *nd, uint32_t base, qemu_irq irq)
{
    DeviceState *dev;
    SysBusDevice *s;

    qemu_check_nic_model(nd, "cadence_gem");
    dev = qdev_create(NULL, "cadence_gem");
    qdev_set_nic_properties(dev, nd);
    qdev_init_nofail(dev);
    s = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(s, 0, base);
    sysbus_connect_irq(s, 0, irq);
}

static inline void zynq_init_spi_flashes(uint32_t base_addr, qemu_irq irq,
                                         bool is_qspi)
{
    DeviceState *dev;
    SysBusDevice *busdev;
    SSIBus *spi;
    DeviceState *flash_dev;
    int i, j;
    int num_busses =  is_qspi ? NUM_QSPI_BUSSES : 1;
    int num_ss = is_qspi ? NUM_QSPI_FLASHES : NUM_SPI_FLASHES;

    dev = qdev_create(NULL, is_qspi ? "xlnx.ps7-qspi" : "xlnx.ps7-spi");
    qdev_prop_set_uint8(dev, "num-txrx-bytes", is_qspi ? 4 : 1);
    qdev_prop_set_uint8(dev, "num-ss-bits", num_ss);
    qdev_prop_set_uint8(dev, "num-busses", num_busses);
    qdev_init_nofail(dev);
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, base_addr);
    if (is_qspi) {
        sysbus_mmio_map(busdev, 1, 0xFC000000);
    }
    sysbus_connect_irq(busdev, 0, irq);

    for (i = 0; i < num_busses; ++i) {
        char bus_name[16];
        qemu_irq cs_line;

        snprintf(bus_name, 16, "spi%d", i);
        spi = (SSIBus *)qdev_get_child_bus(dev, bus_name);

        for (j = 0; j < num_ss; ++j) {
            flash_dev = ssi_create_slave(spi, "n25q128");

            cs_line = qdev_get_gpio_in(flash_dev, 0);
            sysbus_connect_irq(busdev, i * num_ss + j + 1, cs_line);
        }
    }

}

static inline void zynq_init_zc70x_i2c(uint32_t base_addr, qemu_irq irq)
{
    DeviceState *dev = sysbus_create_simple("xlnx.ps7-i2c", base_addr, irq);
    i2c_bus *i2c = (i2c_bus *)qdev_get_child_bus(dev, "i2c");
    int i, bus;

    dev = i2c_create_slave(i2c, "pca9548", 0);
    for (bus = 2; bus <= 3; bus++) {
        char bus_name[16];

        snprintf(bus_name, sizeof(bus_name), "i2c@%d", bus);
        i2c = (i2c_bus *)qdev_get_child_bus(dev, bus_name);
        assert(i2c);

        assert(NUM_I2C_EEPROMS <= 2); /* not enough address space for anymore */
        for (i = 0; i < NUM_I2C_EEPROMS; ++i) {
            DeviceState *eeprom_dev = i2c_create_slave_no_init(i2c, "at.24c08",
                                                               0x50 + 0x4 * i);
            qdev_prop_set_uint16(eeprom_dev, "size", 1024); /* M24C08 */
            qdev_init_nofail(eeprom_dev);
        }
    }
}

static void zynq_init(QEMUMachineInitArgs *args)
{
    ram_addr_t ram_size = args->ram_size;
    const char *cpu_model = args->cpu_model;
    const char *kernel_filename = args->kernel_filename;
    const char *kernel_cmdline = args->kernel_cmdline;
    const char *initrd_filename = args->initrd_filename;
    ARMCPU *cpus[MAX_CPUS];
    MemoryRegion *address_space_mem = get_system_memory();
    MemoryRegion *ext_ram = g_new(MemoryRegion, 1);
    MemoryRegion *ocm_ram = g_new(MemoryRegion, 1);
    DeviceState *dev;
    SysBusDevice *busdev;
    qemu_irq *irqp;
    qemu_irq pic[64];
    NICInfo *nd;
    int n;
    qemu_irq cpu_irq[MAX_CPUS];

    if (!cpu_model) {
        cpu_model = "cortex-a9";
    }

    for (n = 0; n < smp_cpus; n++) {
        cpus[n] = cpu_arm_init(cpu_model);
        if (!cpus[n]) {
            fprintf(stderr, "Unable to find CPU definition\n");
            exit(1);
        }
        irqp = arm_pic_init_cpu(cpus[n]);
        cpu_irq[n] = irqp[ARM_PIC_CPU_IRQ];
    }

    /* max 2GB ram */
    if (ram_size > 0x80000000) {
        ram_size = 0x80000000;
    }

    /* DDR remapped to address zero.  */
    memory_region_init_ram(ext_ram, "zynq.ext_ram", ram_size);
    vmstate_register_ram_global(ext_ram);
    memory_region_add_subregion(address_space_mem, 0, ext_ram);

    /* 256K of on-chip memory */
    memory_region_init_ram(ocm_ram, "zynq.ocm_ram", 256 << 10);
    vmstate_register_ram_global(ocm_ram);
    memory_region_add_subregion(address_space_mem, 0xFFFC0000, ocm_ram);

    /* pl353 */
    dev = qdev_create(NULL, "arm.pl35x");
    /* FIXME: handle this somewhere central */
    object_property_add_child(container_get(qdev_get_machine(), "/unattached"),
                              "pl353", OBJECT(dev), NULL);
    qdev_prop_set_uint8(dev, "x", 3);
    {
        DriveInfo *dinfo = drive_get_next(IF_PFLASH);
        BlockDriverState *bs =  dinfo ? dinfo->bdrv : NULL;
        DeviceState *att_dev = qdev_create(NULL, "cfi.pflash02");
        Error *errp = NULL;

        if (bs && qdev_prop_set_drive(att_dev, "drive", bs)) {
            abort();
        }
        qdev_prop_set_uint32(att_dev, "num-blocks",
                             FLASH_SIZE/FLASH_SECTOR_SIZE);
        qdev_prop_set_uint32(att_dev, "sector-length", FLASH_SECTOR_SIZE);
        qdev_prop_set_uint8(att_dev, "width", 1);
        qdev_prop_set_uint8(att_dev, "mappings", 1);
        qdev_prop_set_uint8(att_dev, "big-endian", 0);
        qdev_prop_set_uint16(att_dev, "id0", 0x0066);
        qdev_prop_set_uint16(att_dev, "id1", 0x0022);
        qdev_prop_set_uint16(att_dev, "id2", 0x0000);
        qdev_prop_set_uint16(att_dev, "id3", 0x0000);
        qdev_prop_set_uint16(att_dev, "unlock-addr0", 0x0aaa);
        qdev_prop_set_uint16(att_dev, "unlock-addr1", 0x0555);
        qdev_prop_set_string(att_dev, "name", "pl353.pflash");
        qdev_init_nofail(att_dev);
        object_property_set_link(OBJECT(dev), OBJECT(att_dev), "dev0", &errp);
        assert_no_error(errp);

        dinfo = drive_get_next(IF_PFLASH);
        att_dev = nand_init(dinfo ? dinfo->bdrv : NULL, NAND_MFR_STMICRO, 0xaa);
        object_property_set_link(OBJECT(dev), OBJECT(att_dev), "dev1", &errp);
        assert_no_error(errp);
    }
    qdev_init_nofail(dev);
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, 0xe000e000);
    sysbus_mmio_map(busdev, 1, 0xe2000000);
    sysbus_mmio_map(busdev, 2, 0xe1000000);

    dev = qdev_create(NULL, "xilinx,zynq_slcr");
    qdev_init_nofail(dev);
    Error *errp = NULL;
    object_property_set_link(OBJECT(dev), OBJECT(cpus[0]), "cpu0", &errp);
    assert_no_error(errp);
    if (smp_cpus > 1) {
        object_property_set_link(OBJECT(dev), OBJECT(cpus[1]), "cpu1", NULL);
        assert_no_error(errp);
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, 0xF8000000);

    dev = qdev_create(NULL, "a9mpcore_priv");
    qdev_prop_set_uint32(dev, "num-cpu", smp_cpus);
    qdev_init_nofail(dev);
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, 0xF8F00000);
    for (n = 0; n < smp_cpus; n++) {
        sysbus_connect_irq(busdev, n, cpu_irq[n]);
    }

    for (n = 0; n < 64; n++) {
        pic[n] = qdev_get_gpio_in(dev, n);
    }

//    zynq_init_zc70x_i2c(0xE0004000, pic[57-IRQ_OFFSET]);
//    zynq_init_zc70x_i2c(0xE0005000, pic[80-IRQ_OFFSET]);
    dev = qdev_create(NULL, "xlnx,ps7-usb");
    dev->id = "zynq-usb-0";
    qdev_init_nofail(dev);
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, 0xE0002000);
    sysbus_connect_irq(busdev, 0, pic[53-IRQ_OFFSET]);

    dev = qdev_create(NULL, "xlnx,ps7-usb");
    dev->id = "zynq-usb-1";
    busdev = SYS_BUS_DEVICE(dev);
    qdev_init_nofail(dev);
    sysbus_mmio_map(busdev, 0, 0xE0003000);
    sysbus_connect_irq(busdev, 0, pic[76-IRQ_OFFSET]);

    zynq_init_spi_flashes(0xE0006000, pic[58-IRQ_OFFSET], false);
    zynq_init_spi_flashes(0xE0007000, pic[81-IRQ_OFFSET], false);
    zynq_init_spi_flashes(0xE000D000, pic[51-IRQ_OFFSET], true);

#if 0
    sysbus_create_simple("xlnx,ps7-usb", 0xE0002000, pic[53-IRQ_OFFSET]);
    sysbus_create_simple("xlnx,ps7-usb", 0xE0003000, pic[76-IRQ_OFFSET]);
#endif

    sysbus_create_simple("cadence_uart", 0xE0000000, pic[59-IRQ_OFFSET]);
    sysbus_create_simple("cadence_uart", 0xE0001000, pic[82-IRQ_OFFSET]);

    sysbus_create_varargs("cadence_ttc", 0xF8001000,
            pic[42-IRQ_OFFSET], pic[43-IRQ_OFFSET], pic[44-IRQ_OFFSET], NULL);
    sysbus_create_varargs("cadence_ttc", 0xF8002000,
            pic[69-IRQ_OFFSET], pic[70-IRQ_OFFSET], pic[71-IRQ_OFFSET], NULL);

    for (n = 0; n < nb_nics; n++) {
        nd = &nd_table[n];
        if (n == 0) {
            gem_init(nd, 0xE000B000, pic[54-IRQ_OFFSET]);
        } else if (n == 1) {
            gem_init(nd, 0xE000C000, pic[77-IRQ_OFFSET]);
        }
    }

    dev = qdev_create(NULL, "generic-sdhci");
    qdev_init_nofail(dev);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, 0xE0100000);
    sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, pic[56-IRQ_OFFSET]);

    dev = qdev_create(NULL, "generic-sdhci");
    qdev_init_nofail(dev);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, 0xE0101000);
    sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, pic[79-IRQ_OFFSET]);

    dev = qdev_create(NULL, "pl330");
    qdev_prop_set_uint8(dev, "num_chnls",  8);
    qdev_prop_set_uint8(dev, "num_periph_req",  4);
    qdev_prop_set_uint8(dev, "num_events",  16);

    qdev_prop_set_uint8(dev, "data_width",  64);
    qdev_prop_set_uint8(dev, "wr_cap",  8);
    qdev_prop_set_uint8(dev, "wr_q_dep",  16);
    qdev_prop_set_uint8(dev, "rd_cap",  8);
    qdev_prop_set_uint8(dev, "rd_q_dep",  16);
    qdev_prop_set_uint16(dev, "data_buffer_dep",  256);

    qdev_init_nofail(dev);
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, 0xF8003000);
    sysbus_connect_irq(busdev, 0, pic[45-IRQ_OFFSET]); /* abort irq line */
    for (n = 0; n < 8; ++n) { /* event irqs */
        sysbus_connect_irq(busdev, n + 1, pic[dma_irqs[n] - IRQ_OFFSET]);
    }

    dev = qdev_create(NULL, "xlnx.ps7-dev-cfg");
    object_property_add_child(qdev_get_machine(), "xilinx-devcfg", OBJECT(dev),
                              NULL);
    qdev_init_nofail(dev);
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_connect_irq(busdev, 0, pic[40-IRQ_OFFSET]);
    sysbus_mmio_map(busdev, 0, 0xF8007000);

    dev = qdev_create(NULL, "xlnx,transceiver-1.00.a");
    dev->id = "zynq-hanadu-trx-0";
    qdev_init_nofail(dev);
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, 0x74c00000);
    sysbus_connect_irq(busdev, 0, pic[29]);
    sysbus_connect_irq(busdev, 1, pic[30]);
    sysbus_connect_irq(busdev, 2, pic[31]);

    dev = qdev_create(NULL, "xlnx,afe-controller-2.00.a");
    dev->id = "zynq-hanadu-afe-0";
    qdev_init_nofail(dev);
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, 0x79400000);

    dev = qdev_create(NULL, "xlnx,hardware-version-control-2.00.a");
    dev->id = "zynq-hanadu-hwvers-0";
    qdev_init_nofail(dev);
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, 0x77800000);

    dev = qdev_create(NULL, "xlnx,powerup-controller-1.00.a");
    dev->id = "zynq-hanadu-pup-0";
    qdev_init_nofail(dev);
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, 0x70800000);

    dev = qdev_create(NULL, "xlnx,rx-buffer-3.00.a");
    dev->id = "zynq-hanadu-rxbuf-0";
    qdev_init_nofail(dev);
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, 0x77c00000);

    dev = qdev_create(NULL, "xlnx,traffic-monitor-1.00.a");
    dev->id = "zynq-hanadu-trafmon-0";
    qdev_init_nofail(dev);
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, 0x71000000);

    dev = qdev_create(NULL, "xlnx,tx-buffer-3.00.a");
    dev->id = "zynq-hanadu-txbuf-0";
    qdev_init_nofail(dev);
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, 0x7ea00000);

    dev = qdev_create(NULL, "xlnx,system-info-1.00.a");
    dev->id = "zynq-hanadu-sysinfo-0";
    qdev_init_nofail(dev);
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, 0x7fff8000);

    dev = qdev_create(NULL, "xlnx,preamble-ram-1.00.a");
    dev->id = "zynq-hanadu-preambleram-0";
    qdev_init_nofail(dev);
    busdev = SYS_BUS_DEVICE(dev);
    sysbus_mmio_map(busdev, 0, 0x7ea02000);

    zynq_binfo.ram_size = ram_size;
    zynq_binfo.kernel_filename = kernel_filename;
    zynq_binfo.kernel_cmdline = kernel_cmdline;
    zynq_binfo.initrd_filename = initrd_filename;
    zynq_binfo.nb_cpus = smp_cpus;
    zynq_binfo.write_secondary_boot = zynq_write_secondary_boot;
    zynq_binfo.smp_loader_start = SMP_BOOT_ADDR;
    zynq_binfo.smp_bootreg_addr = SMP_BOOTREG_ADDR;
    zynq_binfo.board_id = 0xd32;
    zynq_binfo.loader_start = 0;
    arm_load_kernel(arm_env_get_cpu(first_cpu), &zynq_binfo);
}

static QEMUMachine zynq_machine = {
    .name = "xilinx-zynq-a9",
    .desc = "Xilinx Zynq Platform Baseboard for Cortex-A9",
    .init = zynq_init,
    .block_default_type = IF_SCSI,
    .max_cpus = MAX_CPUS,
    .no_sdcard = 1,
    DEFAULT_MACHINE_OPTIONS,
};

static void zynq_machine_init(void)
{
    qemu_register_machine(&zynq_machine);
}

machine_init(zynq_machine_init);
