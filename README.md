# QEMU-UAE PowerPC Integration

This project provides a high-performance PowerPC emulation bridge for the UAE (Ubiquitous Amiga Emulator) framework, powered by QEMU 10.x.

## Overview
The integration allows UAE to leverage QEMU's TCG (Tiny Code Generator) for efficient emulation of PowerPC processors (e.g., 603e, 604e, 750) used in Amiga PowerUP and WarpOS accelerators.

### Key Features
- **Direct RAM Mapping**: Optimized host memory bridge (`memory_region_init_ram_ptr`) for low-latency guest RAM access.
- **External Interrupts**: Robust wiring of UAE host interrupts to the emulated PPC core.
- **Modern QEMU Core**: Based on QEMU 10.x with Meson build system support.
- **Production Ready**: Clean API surface with standardized symbol visibility for Windows (DLL) and Linux (SO).

## Build Instructions (MSYS2 / Windows)

### Prerequisites
1. [MSYS2](https://www.msys2.org/) installed.
2. Necessary packages: `mingw-w64-ucrt-x86_64-toolchain`, `mingw-w64-ucrt-x86_64-meson`, `mingw-w64-ucrt-x86_64-ninja`, `mingw-w64-ucrt-x86_64-glib2`, `diffutils`.

### Compilation
Open a **UCRT64** terminal and run:
```bash
git clone <repository_url> qemu-uae
cd qemu-uae/qemu-latest
mkdir build && cd build
meson setup .. --buildtype=release
ninja
```

## Usage
The build produces two main artifacts:
1.  **`qemu-system-ppc.exe`**: A standalone executable for testing and diagnostics.
2.  **`qemu-uae.dll`**: A shared library intended to be loaded by UAE as a PPC plugin.

### Architecture Support
- **Current Build**: 64-bit (UCRT64).
- **32-bit Support**: To build a 32-bit version for older UAE hosts, use the `MINGW32` environment in MSYS2 and the corresponding `i686` toolchain. Note: QEMU 10.x performance is highly optimized for 64-bit architectures.

## API Surface
The `qemu-uae.dll` exports the following key symbols:
- `uae_qemu_init`: Initialize the QEMU environment.
- `uae_qemu_execute`: Start the PPC emulation loop.
- `ppc_cpu_map_memory`: Map Amiga host memory regions into the PPC address space.
- `qemu_uae_ppc_external_interrupt`: Signal an external interrupt to the PPC core.


## License
This project is licensed under the same terms as QEMU (GPLv2).
