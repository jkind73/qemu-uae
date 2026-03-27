#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "hw/ppc/ppc.h"
#include "hw/core/boards.h"
#include "system/system.h"
#include "system/cpus.h"
#include "target/ppc/cpu-qom.h"
#include "target/ppc/cpu.h"
#include "hw/core/qdev-properties.h"
#include "system/address-spaces.h"
#include "uae/qemu-uae.h"

/*
 * Minimal PowerPC machine for WinUAE integration.
 * This machine is designed to be as passive as possible, allowing
 * WinUAE to control the CPU state and memory mapping.
 */

static void uae_machine_init(MachineState *machine)
{
    const char *cpu_model = machine->cpu_type ? machine->cpu_type : POWERPC_CPU_TYPE_NAME("604e");
    PowerPCCPU *cpu;

    /* Create the CPU */
    cpu = POWERPC_CPU(cpu_create(cpu_model));
    if (!cpu) {
        error_report("Unable to find PowerPC CPU definition");
        exit(1);
    }

    /* 
     * WinUAE expects the CPU to be in a specific state.
     * We initialize the interrupt controller (MPC) for the CPU.
     */
    ppc6xx_irq_init(cpu);

    /* 
     * In UAE mode, memory is mapped by WinUAE via our bridge.
     * We do not allocate any internal RAM here to avoid collisions.
     */
}

static void uae_machine_class_init(ObjectClass *oc, const void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);

    mc->desc = "UAE PowerPC Machine";
    mc->init = uae_machine_init;
    mc->default_cpu_type = POWERPC_CPU_TYPE_NAME("604e");
}

static const TypeInfo uae_machine_info = {
    .name          = MACHINE_TYPE_NAME("uae"),
    .parent        = TYPE_MACHINE,
    .class_init    = uae_machine_class_init,
};

static void uae_machine_register_types(void)
{
    type_register_static(&uae_machine_info);
}

type_init(uae_machine_register_types)
