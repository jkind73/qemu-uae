#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "uae/qemu.h"
#include "uae/ppc.h"

extern void qemu_uae_wait_until_started(void);
extern bool qemu_uae_main_loop_should_exit(void);
int uae_test_harness_main(int argc, char **argv);

int uae_test_harness_main(int argc, char **argv) {
    printf("===========================================\n");
    printf("--- UAE Test Harness Booting QEMU-UAE ---\n");
    printf("===========================================\n");

    printf("[Harness] Invoking qemu_uae_init()...\n");
    qemu_uae_init();
    
    printf("[Harness] Invoking qemu_uae_start()...\n");
    qemu_uae_start();
    
    printf("[Harness] Waiting for QEMU thread to report started...\n");
    qemu_uae_wait_until_started();
    
    printf("[Harness] Initializing PPC CPU context...\n");
    ppc_cpu_init("750", 0);
    
    printf("[Harness] QEMU started. Mapping test memory...\n");
    
    void *test_ram = calloc(1, 1024 * 1024);
    if (!test_ram) {
        printf("[Harness] ERROR: Failed to allocate test RAM\n");
        return 1;
    }
    
    /* Write an infinite loop opcode at the start (Branch to self: 0x48000000 in Big Endian) */
    /* QEMU expects host endianness in some places, but PowerPC executes Big Endian instructions. 
       Usually memory arrays mapped natively are read exactly as they are in BE. */
    uint32_t *instructions = (uint32_t *)test_ram;
    /* Assuming compiling on Little Endian MSYS2 Windows */
    instructions[0] = 0x00000048; /* Hex swapped for little endian host so memory is native 0x48000000 */
    
    PPCMemoryRegion region = {
        .start = 0x0F000000,
        .size = 1024 * 1024,
        .memory = test_ram,
        .name = (char *)"test_ram",
        .alias = 0,
        .type = PPC_MEM_RAM
    };
    
    ppc_cpu_map_memory(&region, 1);
    
    printf("[Harness] Memory mapped at 0x0F000000.\n");
    
    printf("[Harness] Setting PC to 0x0F000000 and initiating RUN state...\n");
    ppc_cpu_set_pc(0, 0x0F000000);
    ppc_cpu_set_state(PPC_CPU_STATE_RUNNING);
    ppc_cpu_run_continuous();
    
    printf("[Harness] CPU is now executing! Monitoring emulator thread...\n");
    
    bool irq_state = false;
    while (!qemu_uae_main_loop_should_exit()) {
        irq_state = !irq_state;
        printf("[Harness] Toggling IRQ to %s...\n", irq_state ? "ON" : "OFF");
        qemu_uae_ppc_external_interrupt(irq_state);

#ifdef _WIN32
        Sleep(5000);
#else
        sleep(5);
#endif
    }
    
    printf("[Harness] QEMU exit signal caught. Terminating harness.\n");
    return 0;
}
