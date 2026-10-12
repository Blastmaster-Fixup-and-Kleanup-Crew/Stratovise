# Stratovise

Stratovise is an independent Windows desktop virtualization manager inspired by the straightforward workflow of early workstation-era VM software.

## Platform and stack

- Host: Windows 10/11 x64 (first-class compatibility target) and Linux x86-64
- Language: C++20
- UI: Qt 6 Widgets, using a classic menu bar, toolbars, dialogs, and VM inventory on both platforms
- Build: CMake
- Virtualization backend: QEMU, integrated through a dedicated backend layer; QEMU executable discovery is platform-aware

Stratovise is intended to run on Windows and Linux hosts and host guest operating systems such as Windows and Linux, subject to their requirements and licenses.

## Goals

- Create, configure, start, stop, pause, and resume VMs.
- Attach bootable ISO images and configure virtual disks.
- Configure guest memory, processor count, boot order, and networking.
- Present VM state, console output, and actionable backend errors.
- Maintain a compact late-1990s workstation-style workflow without copying proprietary VMware code, branding, or artwork.

## Milestones

1. Qt application shell and VM inventory.
2. QEMU detection, version checks, and backend error reporting.
3. Persistent VM definitions and validated configuration.
4. VM lifecycle controls and console/log handling.
5. Virtual disk, ISO, and networking workflows.
6. Windows packaging, dependency checks, and clean-install validation.

## Development prerequisites

- Windows 10/11 x64 or Linux x86-64
- CMake
- A C++20 compiler toolchain (MSVC or MinGW on Windows; GCC or Clang on Linux)
- Qt 6 Widgets development package
- QEMU for the host platform

## Build

Windows (PowerShell):

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Linux:

```sh
cmake -S . -B build
cmake --build build -j
```

## Licensing

Stratovise is an independent implementation inspired by historical workstation virtualization workflows. It does not use VMware proprietary source code, logos, or artwork. Guest OSes and third-party dependencies retain their own license terms.

## Status

Initial Qt shell and QEMU detection are implemented. The current New VM dialog adds an in-memory draft only. Persistent machine definitions, virtual disk creation, and VM launch/lifecycle controls remain future milestones. No successful compile or runtime test is claimed yet.