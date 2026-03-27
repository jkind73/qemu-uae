/*
 * PowerPC CPU library code for use with UAE
 * Copyright 2014 Frode Solheim <frode@fs-uae.net>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include <glib.h>
#include "target/ppc/cpu-models.h"
#include "target/ppc/helper_regs.h"
#include "system/system.h"
#include "system/cpus.h"
#include "system/memory.h"
#include "system/address-spaces.h"
#include "system/runstate.h"
#include "system/cpu-timers.h"
#include "qemu/main-loop.h"
#include "exec/tb-flush.h"
#include "hw/core/cpu.h"
#include "target/ppc/cpu-qom.h"
#include "target/ppc/cpu.h"
#include "hw/ppc/ppc.h"
#include "qemu-uae.h"
#include "uae/qemu-uae.h"
#include "uae/qemu.h"
#include "uae/ppc.h"
#include "uae/log.h"
#include "uae/qemu-uae.h"

#define BUSFREQ 66000000UL
#define TBFREQ 16600000UL
#define MAX_MEMORY_REGIONS 128

static struct {
    CPUPPCState *env;
    PowerPCCPU *cpu;
    int cpu_state;
    QemuThread pause_thread;
    uint32_t hid1;
} state;

static void qemu_uae_log_cpu_state(void);

/* Safe logger wrapper to handle early startup before WinUAE patches pointers */
static void G_GNUC_PRINTF(1, 2) uae_log_safe(const char *format, ...);
#define uae_log(...) uae_log_safe(__VA_ARGS__)

static uint64_t indirect_read(void *opaque, hwaddr addr, unsigned size)
{
    addr += (uintptr_t) opaque;
    uint64_t retval = 0;
    bool locked = bql_locked();

    if (locked) bql_unlock();
    if (size == 8) {
        uae_ppc_io_mem_read64(addr, &retval);
    } else {
        uint32_t v32 = 0;
        uae_ppc_io_mem_read(addr, &v32, size);
        retval = v32;
    }
    if (locked) bql_lock();
    return retval;
}

static void indirect_write(void *opaque, hwaddr addr, uint64_t data,
                           unsigned size)
{
    addr += (uintptr_t) opaque;
    bool locked = bql_locked();

    if (locked) bql_unlock();
    if (size == 8) {
        uae_ppc_io_mem_write64(addr, data);
    } else {
        uae_ppc_io_mem_write(addr, (uint32_t)data, size);
    }
    if (locked) bql_lock();
}

static const MemoryRegionOps indirect_ops = {
    .read = indirect_read,
    .write = indirect_write,
    .endianness = DEVICE_BIG_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 8,
        //.unaligned = true,
    },
    .impl = {
        .min_access_size = 1,
        .max_access_size = 8,
        //.unaligned = true,
    },
};

void PPCCALL qemu_uae_ppc_version(int *major, int *minor, int *revision)
{
    if (major) *major = 3;
    if (minor) *minor = 8;
    if (revision) *revision = 0;
}

static bool initialize(const char *model)
{
    static bool initialized = false;
    if (initialized) {
        return false;
    }
    initialized = true;

    bql_lock_uae();

    /* Grab the first CPU created by qemu_init() during mac99 boot */
    if (first_cpu == NULL) {
        fprintf(stderr, "[QEMU-UAE] FATAL - No first_cpu found after boot!\n");
        uae_log("QEMU: FATAL - No first_cpu found after boot!\n");
        return false;
    }

    state.cpu = POWERPC_CPU(first_cpu);
    state.env = &state.cpu->env;
    
    {
        FILE *f = fopen("e:\\git\\qemu-uae\\winuae\\qemu-uae-debug.log", "a");
        if (f) {
            fprintf(f, "[QEMU-UAE] initialize() called. CPU=%p\n", state.cpu);
            fclose(f);
        }
    }
    OutputDebugStringA("[QEMU-UAE] state.cpu initialized\n");
    uae_log("QEMU: state.cpu initialized: %p\n", state.cpu);

    cpu_synchronize_all_post_init();
    OutputDebugStringA("[QEMU-UAE] Post-init synchronized\n");
    uae_log("QEMU: Post-init synchronized.\n");

    /* Force High Exception Prefix (MSR[IP]/MSR[EP]) so exceptions and reset 
     * vectors map to 0xFFF00xxx (where Kickstart ROM is mapped). */
    state.env->msr |= (1ULL << MSR_EP) | (1ULL << MSR_IP);
    state.env->excp_prefix = 0xFFF00000;
    state.env->hreset_vector = 0xFFF00100; /* Set permanent hardware reset vector */
    hreg_compute_hflags(state.env);
    state.env->nip = 0xFFF00100; /* Force initial instruction pointer to High ROM */
    OutputDebugStringA("[QEMU-UAE] PPC High Prefix set, NIP=0xfff00100\n");
    uae_log("QEMU: PPC Exception Prefix set to High (0xFFF00000), NIP 0xFFF00100\n");

    /* Log CPU model identifier */
    uae_log("QEMU: PPC CPU linked successfully. CPU PVR 0x%08x\n", state.env->spr[SPR_PVR]);
    fflush(stderr);
    bql_unlock_uae();
    return true;
}

/* Safe logger wrapper implementation */
static void G_GNUC_PRINTF(1, 2) uae_log_safe(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    if (!uae_log) {
        /* Fallback to OutputDebugString if WinUAE hasn't patched the log callback yet */
        char buf[512];
        vsnprintf(buf, sizeof(buf), format, args);
        OutputDebugStringA(buf);
    } else {
        uae_log(format, args); /* Note: uae_log is uae_log_function (va_list version) */
    }
    va_end(args);
}

bool PPCCALL qemu_uae_ppc_init(const char* model, uint32_t hid1)
{
    /* In case qemu_uae_init hasn't been called by the user yet */
    qemu_uae_init_internal();

    /* Wait for QEMU background thread to finish qemu_init() */
    qemu_uae_wait_until_started();

    const char *qemu_model = model;
    if (g_ascii_strcasecmp(model, "603ev") == 0) {
        qemu_model = "603e7v1";
    } else if (g_ascii_strcasecmp(model, "604e") == 0) {
        qemu_model = "604e_v2.4";
    }
    char buf[256];
    sprintf(buf, "[QEMU-UAE] qemu_uae_ppc_init with model %s => %s\n", model, qemu_model);
    OutputDebugStringA(buf);
    state.hid1 = hid1;
    bool res = initialize(qemu_model);
    return res;
}



PPCAPI bool qemu_uae_ppc_in_cpu_thread_impl(void)
{
    return false;
}

static void qemu_uae_lock_if_needed(void)
{
    if (qemu_uae_ppc_in_cpu_thread() == false) {
        bql_lock_uae();
    }
}

static void qemu_uae_unlock_if_needed(void)
{
    if (qemu_uae_ppc_in_cpu_thread() == false) {
        bql_unlock_uae();
    }
}

void PPCCALL qemu_uae_ppc_set_pc(int cpu, uint32_t value)
{
    uae_log("QEMU: set_pc called with 0x%08x\n", value);
    qemu_uae_lock_if_needed();
    if (state.env) {
        state.env->nip = value;
    }
    qemu_uae_unlock_if_needed();
}

void PPCCALL qemu_uae_ppc_atomic_raise_ext_exception(void)
{
    qemu_uae_ppc_external_interrupt_impl(1);
}

void PPCCALL qemu_uae_ppc_atomic_cancel_ext_exception(void)
{
    qemu_uae_ppc_external_interrupt_impl(0);
}

struct UAEregion
{
    struct MemoryRegion *region;
    hwaddr addr;
    unsigned int size;
};

static struct UAEregion added_regions[MAX_MEMORY_REGIONS + 1];

static void ppc_cpu_map_add(PPCMemoryRegion *r)
{
    int i;
    for (i = 0; i < MAX_MEMORY_REGIONS; i++) {
        MemoryRegion *mem;
        struct UAEregion *mem_reg = &added_regions[i];
        if (mem_reg->region != NULL)
            continue;

        if (r->type == PPC_MEM_RAM && r->memory) {
            uae_log("QEMU: %02d %08x [+%8x]  =>  %p  \"%s\" (Direct RAM)\n",
                    i, r->start, r->size, r->memory, r->name);
            mem = g_malloc0(sizeof(MemoryRegion));
            memory_region_init_ram_ptr(mem, NULL, r->name, r->size, r->memory);
        } else {
            uae_log("QEMU: %02d %08x [+%8x]  =>  %p  \"%s\" (Indirect IO)\n",
                    i, r->start, r->size, r->memory, r->name);
            mem = g_malloc0(sizeof(MemoryRegion));
            memory_region_init_io(mem, NULL, &indirect_ops,
                                  (void *) (uintptr_t) r->start, r->name,
                                  r->size);
        }
        memory_region_add_subregion(get_system_memory(), r->start, mem);
        mem_reg->region = mem;
        mem_reg->addr = r->start;
        mem_reg->size = r->size;
        return;
    }
}

static void ppc_cpu_map_clear(void)
{
    int i;
    for (i = 0; i < MAX_MEMORY_REGIONS; i++) {
        struct UAEregion *mem = &added_regions[i];
        if (mem->region == NULL)
            continue;
        memory_region_del_subregion(get_system_memory(), mem->region);
        object_unparent(OBJECT(mem->region));
        mem->region = NULL;
    }
}

void PPCCALL qemu_uae_ppc_map_memory(PPCMemoryRegion *regions, int count)
{
    int i;
    /* Wait for QEMU initialization if it's still running in the other thread */
    qemu_uae_wait_until_started();
    
    qemu_uae_lock_if_needed();
    uae_log("QEMU: Map memory regions (count=%d):\n", count);
    ppc_cpu_map_clear();
    for (i = 0; i < count; i++) {
        ppc_cpu_map_add(&regions[i]);
    }
    qemu_uae_unlock_if_needed();
}

PPCAPI void qemu_uae_ppc_external_interrupt_impl(bool enable)
{
    if (state.cpu) {
        /* uae_log is now a safe macro */
        uae_log("QEMU: Setting EXT IRQ to %d\n", enable);
        ppc_set_irq(state.cpu, PPC_INTERRUPT_EXT, enable ? 1 : 0);
    }
}

PPCAPI int qemu_uae_lock_impl(int type)
{
    int result = 0;
    if (type == QEMU_UAE_LOCK_TRYLOCK) {
        result = bql_trylock_uae();
    } else if (type == QEMU_UAE_LOCK_TRYLOCK_CANCEL) {
        bql_trylock_cancel_uae();
    } else if (type == QEMU_UAE_LOCK_ACQUIRE) {
        bql_lock_uae();
    } else if (type == QEMU_UAE_LOCK_RELEASE) {
        bql_unlock_uae();
    }
    return result;
}

#include "system/hw_accel.h"

void PPCCALL qemu_uae_ppc_run_continuous(void)
{
    char buf[256];
    sprintf(buf, "[QEMU-UAE] qemu_uae_ppc_run_continuous (cpu=%p)\n", state.cpu);
    OutputDebugStringA(buf);

    qemu_uae_wait_until_started();

    bql_lock_uae();
    cpu_enable_ticks();
    runstate_set(RUN_STATE_RUNNING);
    vm_state_notify(1, RUN_STATE_RUNNING);
    resume_all_vcpus();
    qemu_uae_set_started();

    if (state.cpu) {
        cpu_synchronize_state(CPU(state.cpu));
        uae_log("QEMU: Resumed vCPUs. NIP: %016llx, MSR: %016llx, Prefix: %016llx\n", 
                (unsigned long long)state.env->nip, (unsigned long long)state.env->msr, (unsigned long long)state.env->excp_prefix);
        sprintf(buf, "QEMU: Resumed vCPUs. NIP: %016" PRIx64 "\n", (uint64_t)state.env->nip);
        OutputDebugStringA(buf);
    }
    bql_unlock_uae();
}

void PPCCALL qemu_uae_ppc_run_single(int count)
{
    /* If WinUAE uses single-step, we just redirect it to continuous for now
     * as TCG doesn't easily support 'n' instructions from an external lock. */
    static bool warned = false;
    if (!warned) {
        OutputDebugStringA("QEMU: qemu_uae_ppc_run_single called Redirecting.\n");
        warned = true;
    }
    qemu_uae_ppc_run_continuous();
}

void PPCCALL qemu_uae_ppc_reset(void)
{
    OutputDebugStringA("[QEMU-UAE] qemu_uae_ppc_reset\n");
    qemu_uae_wait_until_started();
    
    qemu_uae_lock_if_needed();
    if (state.cpu) {
        cpu_reset(CPU(state.cpu));
        state.env->spr[SPR_HID1] = state.hid1;
        
        /* Force High Exception Prefix (MSR[IP]/MSR[EP]) so exceptions and reset 
         * vectors map to 0xFFF00xxx (where Kickstart ROM is mapped).
         * Use direct updates to avoid BQL deadlock in hreg_store_msr. */
        state.env->msr |= (1ULL << MSR_EP) | (1ULL << MSR_IP);
        state.env->excp_prefix = 0xFFF00000;
        state.env->hreset_vector = 0xFFF00100;
        hreg_compute_hflags(state.env);
        state.env->nip = 0xFFF00100;

        uae_log("QEMU: Resetted. NIP = 0x%08" PRIx64 ", MSR = 0x%08" PRIx64 "\n", 
                (uint64_t)state.env->nip, (uint64_t)state.env->msr);
        uae_log("QEMU: Flushing JIT TBs\n");
        queue_tb_flush(CPU(state.cpu));
        fflush(stderr);
    }
    qemu_uae_unlock_if_needed();
}

uint64_t PPCCALL qemu_uae_ppc_get_dec(void)
{
    return 0;
}

void PPCCALL qemu_uae_ppc_do_dec(int value)
{
}

void PPCCALL qemu_uae_ppc_pause(int pause)
{
    qemu_uae_ppc_set_state(pause ? PPC_CPU_STATE_PAUSED : PPC_CPU_STATE_RUNNING);
}

static void qemu_uae_log_cpu_state(void)
{
    int flags = 0;
    uae_log("QEMU: PPC CPU dump:\n");
    /* Passing stderr as the output destination */
    cpu_dump_state(CPU(state.cpu), stderr, flags);
}

bool PPCCALL qemu_uae_ppc_check_state(int check_state)
{
    return state.cpu_state == check_state;
}

void PPCCALL qemu_uae_ppc_set_state(int set_state)
{
    uae_log("QEMU: qemu_uae_ppc_set_state %d\n", set_state);
    qemu_uae_wait_until_started();
    
    qemu_uae_lock_if_needed();
    if (set_state == PPC_CPU_STATE_PAUSED) {
        pause_all_vcpus();
        state.cpu_state = PPC_CPU_STATE_PAUSED;
        uae_log("QEMU: Paused!\n");
        qemu_uae_log_cpu_state();
    } else if (set_state == PPC_CPU_STATE_RUNNING) {
        resume_all_vcpus();
        state.cpu_state = PPC_CPU_STATE_RUNNING;
        uae_log("QEMU: Resumed!\n");
    }
    qemu_uae_unlock_if_needed();
}

void PPCCALL qemu_uae_ppc_close(void)
{
    uae_log("QEMU: qemu_uae_ppc_close\n");
}

void PPCCALL qemu_uae_ppc_stop(void)
{
    uae_log("QEMU: qemu_uae_ppc_stop\n");
    qemu_uae_ppc_set_state(PPC_CPU_STATE_PAUSED);
}

/* Storage for callback functions set by WinUAE via uae_api.c exports */
/* (Declared extern in uae/ppc.h and uae/log.h) */
