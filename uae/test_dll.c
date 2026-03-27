#include <windows.h>
#include <stdio.h>

typedef void (*uae_qemu_init_t)(void);

int main() {
    printf("[Test] Attempting to load qemu-uae.dll...\n");
    HMODULE hDll = LoadLibrary("qemu-uae.dll");
    if (!hDll) {
        DWORD err = GetLastError();
        printf("[Test] FAILED to load DLL. Error code: %lu\n", err);
        return 1;
    }
    printf("[Test] DLL loaded successfully.\n");

    uae_qemu_init_t uae_qemu_init = (uae_qemu_init_t)GetProcAddress(hDll, "uae_qemu_init");
    if (!uae_qemu_init) {
        printf("[Test] FAILED to find uae_qemu_init export.\n");
        FreeLibrary(hDll);
        return 1;
    }
    printf("[Test] Found uae_qemu_init at %p. Calling it...\n", uae_qemu_init);

    /* This should trigger the "QEMU: Initializing" logs */
    uae_qemu_init();

    printf("[Test] Call returned. DLL verified.\n");
    FreeLibrary(hDll);
    return 0;
}
