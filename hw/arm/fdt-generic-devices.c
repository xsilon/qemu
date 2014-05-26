#include "qemu-common.h"

static const TypeInfo fdt_qom_aliases [] = {
    {   .name = "xlnx.ps7-ethernet",        .parent = "cadence_gem"         },
    {   .name = "xlnx.ps7-ttc",             .parent = "cadence_ttc"         },
    {   .name = "cdns.ttc",                 .parent = "cadence_ttc"         },
    {   .name = "xlnx.ps7-uart",            .parent = "cadence_uart"        },
    {   .name = "xlnx.xuartps",             .parent = "cadence_uart"        },
    {   .name = "arm.cortex-a9-twd-timer",  .parent = "arm_mptimer"         },
    {   .name = "xlnx.ps7-slcr",            .parent = "xilinx,zynq_slcr"    },
    {   .name = "xlnx.zynq-slcr",           .parent = "xilinx,zynq_slcr"    },
    {   .name = "arm.cortex-a9-gic",        .parent = "arm_gic"             },
    {   .name = "arm.gic",                  .parent = "arm_gic"             },
    {   .name = "arm.cortex-a9-scu",        .parent = "a9-scu"              },
    {   .name = "xlnx.ps7-usb",             .parent = "xlnx,ps7-usb"        },
    {   .name = "xlnx.zynq-usb",            .parent = "xlnx,ps7-usb"        },
    {   .name = "xlnx.zynq-qspi",           .parent = "xlnx.ps7-qspi"       },
};

static void fdt_generic_register_types(void)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(fdt_qom_aliases); ++i) {
        type_register_static(&fdt_qom_aliases[i]);
    }
}

type_init(fdt_generic_register_types)


