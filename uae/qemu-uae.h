#ifndef QEMU_UAE_H
#define QEMU_UAE_H

void runstate_init(void);
void qemu_tcg_wait_io_event(void);
void qemu_wait_io_event_common(CPUState *cpu);

 /* cpus.c */

void qemu_uae_mutex_lock(void);
void qemu_uae_mutex_unlock(void);
int qemu_uae_mutex_trylock(void);
void qemu_uae_mutex_trylock_cancel(void);

/* system/runstate.c covered by system/system.h */

bool main_loop_should_exit(void);

#include "uae/ppc.h"

/* qemu-uae-cpu.c */

bool qemu_uae_main_loop_should_exit(void);
void qemu_uae_ppc_version(int *major, int *minor, int *revision);
/* qemu_uae_ppc_init is already in uae/qemu.h */
void qemu_uae_ppc_close(void);
void qemu_uae_ppc_stop(void);
void qemu_uae_ppc_atomic_raise_ext_exception(void);
void qemu_uae_ppc_atomic_cancel_ext_exception(void);
void qemu_uae_ppc_map_memory(PPCMemoryRegion *regions, int count);
void qemu_uae_ppc_set_pc(int cpu, uint32_t value);
void qemu_uae_ppc_run_continuous(void);
void qemu_uae_ppc_run_single(int count);
uint64_t qemu_uae_ppc_get_dec(void);
void qemu_uae_ppc_do_dec(int value);
void qemu_uae_ppc_pause(int pause);
void qemu_uae_ppc_reset(void);
void qemu_uae_ppc_set_state(int state);
bool qemu_uae_ppc_check_state(int state);

bool qemu_uae_ppc_in_cpu_thread_impl(void);
void qemu_uae_ppc_external_interrupt_impl(bool enable);

/* uaeppc_* modern aliases */
PPCAPI const char* PPCCALL uaeppc_get_version(void);
PPCAPI int PPCCALL uaeppc_init(void* config);
PPCAPI void PPCCALL uaeppc_reset(void);
PPCAPI void PPCCALL uaeppc_execute(void);
PPCAPI void PPCCALL uaeppc_shutdown(void);
PPCAPI uint64_t PPCCALL uaeppc_get_capabilities(void);

/* qemu-uae-main.c */

void qemu_uae_init_internal(void);
void qemu_uae_start_internal(void);
void qemu_uae_set_started(void);
void qemu_uae_wait_until_started(void);
/* qemu_uae_lock_impl is already in uae/qemu.h */

/* uae-stubs.c */
void uae_qemu_mutex_lock(void);
void uae_qemu_mutex_unlock(void);

#endif /* QEMU_UAE_H */
