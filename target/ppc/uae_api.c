#include "qemu/osdep.h"
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "uae/qemu.h"
#include "uae/ppc.h"

#include "uae/log.h"
#include "uae/qemu-uae.h"

#define UAE_API __declspec(dllexport)

/* 
 * Callback pointers patched by WinUAE (ppc.cpp: uae_patch_library_ppc)
 * These MUST be exported as global variables.
 */
UAE_API uae_ppc_io_mem_read_function uae_ppc_io_mem_read = NULL;
UAE_API uae_ppc_io_mem_write_function uae_ppc_io_mem_write = NULL;
UAE_API uae_ppc_io_mem_read64_function uae_ppc_io_mem_read64 = NULL;
UAE_API uae_ppc_io_mem_write64_function uae_ppc_io_mem_write64 = NULL;
UAE_API uae_log_function uae_log = NULL;


/* 
 * 1. Global Handshake (qemuvga/qemu.cpp)
 */
UAE_API void UAECALL qemu_uae_version(int *major, int *minor, int *revision) {
    if (major) *major = 3; 
    if (minor) *minor = 8; 
    if (revision) *revision = 0;
}

UAE_API void UAECALL qemu_uae_init(void) {
    OutputDebugStringA("[QEMU-UAE] qemu_uae_init called\n");
    /* Standard QEMU-UAE initialization */
    qemu_uae_init_internal();
    /* Wait for background thread to finish qemu_init */
    qemu_uae_wait_until_started();
    OutputDebugStringA("[QEMU-UAE] qemu_uae_init wait finished, returning to UAE!\n");
}

UAE_API void UAECALL qemu_uae_start(void) {
    OutputDebugStringA("[QEMU-UAE] qemu_uae_start called\n");
    /* Standard QEMU-UAE start */
    qemu_uae_start_internal();
}

/* 
 * 2. PPC Subsystem Handshake (ppc.cpp: load_qemu_implementation)
 */
UAE_API void UAECALL ppc_cpu_version(int *major, int *minor, int *revision) {
    qemu_uae_ppc_version(major, minor, revision);
}

UAE_API bool UAECALL ppc_cpu_init(const char *model, uint32_t hid1) {
    char buf[256];
    sprintf(buf, "[QEMU-UAE] ppc_cpu_init called (model=%s)\n", model ? model : "NULL");
    OutputDebugStringA(buf);
    return qemu_uae_ppc_init(model, hid1);
}

UAE_API bool UAECALL ppc_cpu_init_pvr(uint32_t pvr) {
    /* Redirect to default init for now */
    return qemu_uae_ppc_init("604e", 0);
}

UAE_API void UAECALL ppc_cpu_close(void) {
    qemu_uae_ppc_close();
}

UAE_API void UAECALL ppc_cpu_stop(void) {
    qemu_uae_ppc_stop();
}

UAE_API void UAECALL ppc_cpu_atomic_raise_ext_exception(void) {
    qemu_uae_ppc_atomic_raise_ext_exception();
}

UAE_API void UAECALL ppc_cpu_atomic_cancel_ext_exception(void) {
    qemu_uae_ppc_atomic_cancel_ext_exception();
}

UAE_API void UAECALL ppc_cpu_map_memory(PPCMemoryRegion *regions, int count) {
    qemu_uae_ppc_map_memory(regions, count);
}

UAE_API void UAECALL ppc_cpu_set_pc(int cpu, uint32_t value) {
    qemu_uae_ppc_set_pc(cpu, value);
}

UAE_API void UAECALL ppc_cpu_run_continuous(void) {
    qemu_uae_ppc_run_continuous();
}

UAE_API void UAECALL ppc_cpu_run_single(int count) {
    qemu_uae_ppc_run_single(count);
}

UAE_API uint64_t UAECALL ppc_cpu_get_dec(void) {
    return qemu_uae_ppc_get_dec();
}

UAE_API void UAECALL ppc_cpu_do_dec(int value) {
    qemu_uae_ppc_do_dec(value);
}

UAE_API void UAECALL ppc_cpu_pause(int pause) {
    qemu_uae_ppc_pause(pause);
}

UAE_API void UAECALL ppc_cpu_reset(void) {
    qemu_uae_ppc_reset();
}

UAE_API void UAECALL ppc_cpu_set_state(int state) {
    qemu_uae_ppc_set_state(state);
}

UAE_API bool UAECALL ppc_cpu_check_state(int state) {
    return qemu_uae_ppc_check_state(state);
}

UAE_API bool UAECALL qemu_uae_ppc_in_cpu_thread(void) {
    return qemu_uae_ppc_in_cpu_thread_impl();
}

UAE_API void UAECALL qemu_uae_ppc_external_interrupt(bool enable) {
    qemu_uae_ppc_external_interrupt_impl(enable);
}

/* WinUAE loader aliases */
UAE_API int UAECALL qemu_uae_lock(int type) {
    return qemu_uae_lock_impl(type);
}

UAE_API void UAECALL qemu_uae_slirp_init(void) {}
UAE_API void UAECALL qemu_uae_slirp_input(const uint8_t *pkt, int pkt_len) {}

/* NEW: uaeppc_* aliases from skeleton.txt for modern WinUAE compatibility */
UAE_API const char* UAECALL uaeppc_get_version(void) {
    static char buf[128];
    int maj, min, rev;
    qemu_uae_version(&maj, &min, &rev);
    sprintf(buf, "%d.%d.%d-tcg-stabilized", maj, min, rev);
    return buf;
}

UAE_API int UAECALL uaeppc_init(void* config) {
    /* Note: original skeleton uses UAEPPC_CONFIG, we wrap for generic load */
    qemu_uae_init();
    return ppc_cpu_init("604e", 0);
}

UAE_API void UAECALL uaeppc_reset(void) {
    ppc_cpu_reset();
}

UAE_API void UAECALL uaeppc_execute(void) {
    ppc_cpu_run_continuous();
}

UAE_API void UAECALL uaeppc_shutdown(void) {
    ppc_cpu_close();
}

UAE_API uint64_t UAECALL uaeppc_get_capabilities(void) {
    return 0x07; /* Matches UAEPPC_CAP_PPC_970 | UAEPPC_CAP_SMP | UAEPPC_CAP_SLIRP */
}

/* Loader shim - WinUAE 6.0+ defines this internally, but old ones might call it */
UAE_API void* UAECALL uae_qemu_uae_init(void) { 
    return (void*)GetModuleHandleA("qemu-uae.dll"); 
}
