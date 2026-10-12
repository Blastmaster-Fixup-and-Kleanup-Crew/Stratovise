# Stratovise

Stratovise is an independent Windows and Linux desktop virtualization manager inspired by the straightforward workflow of early workstation-era VM software.

## Platform and stack

- Host: Windows 10/11 x64 and Linux x86-64
- Language: C++20
- UI: Qt 6 Widgets, using a classic menu bar, toolbar, dialogs, and VM inventory on both platforms
- Build: CMake
- Virtualization backend: QEMU, integrated through a dedicated backend layer with host-specific executable discovery

Stratovise is intended to run on Windows and Linux hosts and host guest operating systems such as Windows and Linux, subject to their requirements and licenses.

## Current functionality

- Create VM definitions with a name, guest OS label, RAM, CPU count, and optional ISO path.
- Save and reload VM definitions in a JSON file under the current user's application configuration directory.
- Start QEMU with the selected VM's RAM and CPU settings and a bootable ISO.
- Stop a running QEMU process and remove saved VM definitions.
- Detect QEMU and query its version.
- Show VM state and basic launch/runtime output in the status bar.

VM definitions are configuration records, not complete virtual machines yet. This build does not create virtual disks, edit hardware after creation, provide snapshots, or offer an integrated graphical console. VM startup currently requires an existing ISO file. These are known limitations, not completed features.

## Goals

- Create, configure, start, stop, pause, and resume VMs.
- Attach bootable ISO images and create/manage virtual disks.
- Configure guest memory, processor count, boot order, networking, and device options.
- Present VM state, console output, and actionable backend errors.
- Maintain a compact late-1990s workstation-style workflow without copying proprietary VMware code, branding, or artwork.

## Milestones

1. Qt application shell and VM inventory — implemented.
2. QEMU detection, version checks, and backend error reporting — implemented.
3. Persistent VM definitions — implemented.
4. Basic start/stop lifecycle for ISO-booted guests — implemented, pending build/runtime validation.
5. Virtual disk creation, editable hardware settings, and stronger guest lifecycle controls.
6. Console integration, networking/device configuration, snapshots, and usability improvements.
7. Windows/Linux packaging, dependency checks, automated tests, and clean-install validation.

## Development prerequisites

- Windows 10/11 x64 or Linux x86-64
- CMake 3.21 or newer
- A C++20 compiler toolchain (MSVC or MinGW on Windows; GCC or Clang on Linux)
- Qt 6.4+ Widgets development package
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

Version 0.2.0 development milestone. Persistent definitions and a basic QEMU start/stop path have been added. A successful compile and runtime test have not yet been verified, so build validation is the next required gate before treating this milestone as stable.
