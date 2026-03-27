/*
 * QEMU integration code for use with UAE
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
#include "target/ppc/cpu-models.h"
#include "target/ppc/helper_regs.h"
#include "system/system.h"
#include "system/cpus.h"
#include <glib.h>
#include "qemu-uae.h"
#include "uae/qemu-uae.h"

#include "uae/log.h"
#include "uae/ppc.h"
#include "uae/qemu.h"

#ifdef UAE
#error UAE should not be defined here
#endif

/* Increase this when changes are not backwards compatible */
#define VERSION_MAJOR 3
#define VERSION_MINOR 8
#define VERSION_REVISION 2

#if QEMU_UAE_VERSION_MAJOR != VERSION_MAJOR
#error Major version mismatch between UAE and QEMU-UAE
#endif

#if QEMU_UAE_VERSION_MINOR != VERSION_MINOR
#warning Minor version mismatch between UAE and QEMU-UAE
#endif

static struct {
    volatile bool started;
    bool exit_main_loop;
} state;

static bool qemu_initialized = false;
static GCond init_cond;
static GMutex init_mutex;
static bool init_done = false;



extern void qemu_uae_stubs_log_impl(const char *format, ...) G_GNUC_PRINTF(1, 2);

static void qemu_uae_debug_log(const char *msg)
{
    FILE *f = fopen("e:\\git\\qemu-uae\\winuae\\qemu-uae-debug.log", "a");
    if (f) {
        fprintf(f, "[QEMU-UAE] %s\n", msg);
        fclose(f);
    }
    OutputDebugStringA(msg);
}

static DWORD WINAPI main_thread_function(LPVOID arg)
{
    qemu_uae_debug_log("Main thread entry point reached.");
    
    /* Boot QEMU 10.x Headless via minimal initialization */
    const char *argv[] = {
        "qemu-system-ppc",
        "-M", "uae",
        "-m", "16",
        "-display", "none",
        NULL
    };

    qemu_uae_debug_log("Calling qemu_init...");
    qemu_init(ARRAY_SIZE(argv) - 1, (char **)argv);
    qemu_uae_debug_log("qemu_init returned successfully!");

    /* Signal that initialization is complete */
    g_mutex_lock(&init_mutex);
    init_done = true;
    g_cond_signal(&init_cond);
    g_mutex_unlock(&init_mutex);

    qemu_uae_debug_log("Entering main loop...");
    qemu_main_loop();
    qemu_uae_debug_log("Main loop exited unexpectedly!");
    return 0;
}

static HANDLE main_thread_handle;

void qemu_uae_init_internal(void)
{
    if (qemu_initialized) {
        return;
    }
    qemu_initialized = true;

    g_mutex_init(&init_mutex);
    g_cond_init(&init_cond);

    qemu_uae_debug_log("Spawning main_thread_function...");
    main_thread_handle = CreateThread(NULL, 0, main_thread_function, NULL, 0, NULL);
    if (!main_thread_handle) {
        qemu_uae_debug_log("FAILED to spawn thread!");
    }

    if (uae_log == NULL) {
        uae_log = qemu_uae_stubs_log_impl;
    }
}

void qemu_uae_start_internal(void)
{
    OutputDebugStringA("[QEMU-UAE] qemu_uae_start called\n");
    qemu_uae_ppc_run_continuous();
}

void qemu_uae_set_started(void)
{
    state.started = true;
}

void qemu_uae_wait_until_started(void)
{
    g_mutex_lock(&init_mutex);
    while (!init_done) {
        g_cond_wait(&init_cond, &init_mutex);
    }
    g_mutex_unlock(&init_mutex);
}

bool qemu_uae_main_loop_should_exit(void)
{
    return state.exit_main_loop;
}
