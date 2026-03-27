#include "qemu/osdep.h"
#include <stdarg.h>
#include <stdio.h>

#include "uae/log.h"
#include "qemu/main-loop.h"
#include "qemu-uae.h"
#include "uae/qemu-uae.h"

/* Prototypes to satisfy -Werror=missing-prototypes */
void uae_log_reset_timestamp(void);
void qemu_uae_stubs_log_impl(const char *format, ...) G_GNUC_PRINTF(1, 2);

void uae_log_reset_timestamp(void)
{
}

/* 
 * Implementation for the uae_log pointer defined in qemu-uae-cpu.c.
 */
void qemu_uae_stubs_log_impl(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

/* Mutex stubs - prototypes are in qemu-uae.h */
void qemu_uae_mutex_lock(void)
{
    bql_lock_uae();
}

void uae_qemu_mutex_lock(void) { qemu_uae_mutex_lock(); }

void qemu_uae_mutex_unlock(void)
{
    bql_unlock_uae();
}

void uae_qemu_mutex_unlock(void) { qemu_uae_mutex_unlock(); }

int qemu_uae_mutex_trylock(void)
{
    return bql_trylock_uae();
}

void qemu_uae_mutex_trylock_cancel(void)
{
    bql_trylock_cancel_uae();
}
