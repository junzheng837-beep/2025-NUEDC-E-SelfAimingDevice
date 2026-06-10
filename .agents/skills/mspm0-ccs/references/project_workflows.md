# SysConfig And Project Workflows

Use this when editing `.syscfg` or `system.syscfg`, validating a CCS, Keil, or CMake/GCC/OpenOCD project, building, or flashing.

## SysConfig Editing

Treat `.syscfg` as the editable source for device metadata, pinmux, peripheral instances, clocks, DMA, interrupts, and generated initialization.

Preserve metadata:

```text
@cliArgs
@v2CliArgs
@versions
--device
--package
--product
```

Keep the original metadata comment syntax valid. Some empty CCS projects use `//@cliArgs` line comments. Do not rewrite those as `* @cliArgs` unless the line is inside an active `/* ... */` block; a real failure from this mistake was `SyntaxError: Unexpected token '*'`.

Editing strategy:

1. Find an existing local instance or an example with the same device/package/peripheral.
2. Copy local style instead of inventing fields.
3. Change only the requested module, pin, clock, or runtime behavior.
4. Preserve `$suggestSolution` / `$assign` lines unless you know the solver impact.
5. Run SysConfig CLI or rebuild.
6. Inspect generated `ti_msp_dl_config.h` for names.

## CCS Project Rules

Editable surfaces are normally `.syscfg`, user source files, user headers, and project docs.

Generated or build outputs are inspection-only:

```text
Debug/ti_msp_dl_config.c
Debug/ti_msp_dl_config.h
Release/ti_msp_dl_config.c
Release/ti_msp_dl_config.h
Debug/device.opt
Debug/device_linker.cmd
Debug/device.cmd.genlibs
Debug/*.mk
*.o
*.d
*.out
*.map
```

Avoid unnecessary edits to `.project`, `.cproject`, `.ccsproject`, `.settings/`, `targetConfigs/*.ccxml`, and Keil `*.uvoptx` files. These files can change SDK discovery, compiler options, debug probe, and linker behavior.

## Keil / uVision Project Rules

- Editable surfaces are normally `system.syscfg`, user source files, user headers, and the Keil project only when build settings truly need to change.
- Treat a Keil `*.uvprojx` as the project entrypoint when the active project is Keil-based.
- Treat the project's scatter file as the linker source of truth. If memory layout changes, update it deliberately rather than guessing from CCS defaults.
- Treat `keil/Objects/`, `keil/Listings/`, `*.uvoptx`, logs, maps, and generated outputs as inspection-only.
- Keil projects do not use `targetConfigs/*.ccxml`; do not require a CCS debug-config file when the active project is Keil-based.

## CMake / GCC / OpenOCD Project Rules

- Editable surfaces are normally `.syscfg`, user source files, user headers, `CMakeLists.txt`, and toolchain/OpenOCD config files only when the requested feature requires build-system changes.
- Treat `cmake-build-*`, `build/`, generated binaries, maps, object files, and generated SysConfig outputs as inspection-only.
- Detect the active target from the existing CMake project instead of assuming `Debug/<project>.out`.
- Use the existing OpenOCD config files such as `daplink.cfg`, `stlink.cfg`, or `xds110.cfg` when present.
- MSPM0 OpenOCD support commonly depends on a TI MSPM0-capable OpenOCD build or TI extension branch. Mainline or unrelated OpenOCD builds may not recognize the target or adapter flow.
- If OpenOCD fails with `unable to find a matching CMSIS-DAP device`, treat it as no matching probe/adapter found, not as proof that the firmware, SysConfig, or linker setup is wrong.
- Do not require CCS `targetConfigs/*.ccxml` for an OpenOCD-based project.

## Framework-Style Project Rules

- Identify whether the project is simple or framework-style before editing. Framework projects often contain directories such as `app/`, `bsp/`, `components/`, `core/`, `drivers/`, `hal/`, `middleware/`, or `tasks/`.
- Do not move code between layers just to make an example fit. Follow the project's existing ownership boundaries.
- For multi-module projects, find the existing peripheral wrapper, board file, or application module that owns similar behavior before adding new code.
- For timing/control features, confirm whether the period is controlled by timer ISR, RTOS task delay, hardware PWM/ADC trigger chain, or main-loop polling.

## Validation Chain

Run the static checker first:

```powershell
python scripts\check_syscfg.py <project-dir>
```

Before flashing or debugging, add the optional connected-probe check:

```powershell
python scripts\detect_probe.py
python scripts\check_syscfg.py <project-dir> --probe
```

The probe detector is read-only. It should not open the target, erase flash, or reset the MCU. If the physical probe conflicts with CCS `.ccxml` or OpenOCD hints, stop and ask the user to confirm the intended backend before flashing.

Run SysConfig CLI when available. Prefer the exact command generated in `Debug/subdir_rules.mk` when it exists for CCS projects. A fresh project may not have generated makefiles yet; SysConfig CLI can still validate `.syscfg` into a temporary output directory.

Build through the active project's generated build flow when present:

```powershell
gmake -C <project-dir>\Debug clean all
```

If `Debug/makefile` references both `../device_linker.cmd` and `-l"./device_linker.cmd"`, treat that as a CCS generated build-file state issue, not an application or SysConfig failure. Regenerate/rebuild in CCS when possible. For one-off CLI validation, avoid linking the same generated linker script twice.

For Keil projects, validate by opening or building the `.uvprojx` in Keil/uVision and checking the generated `ti_msp_dl_config.c` / `ti_msp_dl_config.h`, `Objects/`, and `Listings/` outputs rather than expecting CCS makefiles.

For CMake/GCC/OpenOCD projects, prefer the existing build directory and target:

```powershell
cmake --build <project-dir>\cmake-build-debug --target <target>
cmake --build <project-dir>\cmake-build-debug --target <flash-target>
```

If no configured build directory exists, configure one using the project's documented preset/toolchain. Do not invent compiler paths when the project README or toolchain file already declares them.

## DSLite / J-Link Flash

For a vague request such as "flash this project", detect the connected probe before choosing DSLite, J-Link tools, or OpenOCD. Do not assume the user's previous probe is still connected.

The verified flash path is DSLite / UniFlash with J-Link:

```powershell
dslite -c <project-dir>\targetConfigs\MSPM0G3507.ccxml -N
dslite -c <project-dir>\targetConfigs\MSPM0G3507.ccxml -e -r 2 -u <project-dir>\Debug\<project>.out
```

The `.ccxml` must match the physical probe. A project configured for XDS110 can build successfully and still fail to flash through J-Link.

Use `-r 2 -u` for automated flashing after clock-tree changes. This performs a System Reset after programming. If manual flashing appears to start with the wrong clock speed, press the board reset button before judging the firmware.

If `dslite -N` hangs or cannot list the core, stop stale CCS/DSLite/J-Link sessions, reconnect if needed, and retry detection before erase/program operations.

## CCS-DSS Debug

For interactive debug actions on CCS / CCS Theia projects, use the CCS-DSS section in `debug_backends.md`. This path uses `targetConfigs/*.ccxml`, so the file must match the connected probe such as J-Link or XDS110.

## OpenOCD Flash

Use the project-provided OpenOCD target when available. Otherwise the explicit shape is:

```powershell
openocd -f <probe-or-board.cfg> -c "program <firmware.elf|firmware.hex|firmware.bin> verify reset exit"
```

Keep the flash backend explicit, for example `--backend dslite` or `--backend openocd`, when writing wrappers or documentation.

## OpenOCD / GDB Debug

For an MSPM0-capable OpenOCD installation, use the OpenOCD/GDB section in `debug_backends.md`. Keep one operation active per probe and do not automatically mass erase, factory reset, or unlock a target.

## Hardware Claims

Report validation levels separately:

- source/static inspection
- SysConfig generation
- compile/link
- flash tool success
- physical board behavior
- serial/logic analyzer observation

Do not report hardware behavior as verified unless it was observed on connected hardware.

## SDK Schema Lookup

Use evidence before authoring unfamiliar `.syscfg` fields or enum values. There is no single friendly MSPM0 field manual listing every module field, enum, solver rule, and clock option.

Use sources in this order:

1. The user's existing `.syscfg`.
2. Packaged examples under `examples/`.
3. Local TI MSPM0 SDK `.syscfg` examples.
4. Local SDK metadata under `source/ti/driverlib/.meta/*.syscfg.js`.
5. SysConfig GUI or standalone SysConfig output for the same device, package, SDK, and tool version.
6. Small scaffolds under `assets/snippets/`.

Search local SDK examples and module metadata with:

```powershell
python scripts\index_syscfg_examples.py C:\ti\mspm0_sdk_2_10_00_04 --board LP_MSPM0G3507 --module UART
```

Useful SDK paths:

```text
<mspm0_sdk>/examples/**/*.syscfg
<mspm0_sdk>/source/ti/driverlib/.meta/GPIO.syscfg.js
<mspm0_sdk>/source/ti/driverlib/.meta/UART.syscfg.js
<mspm0_sdk>/source/ti/driverlib/.meta/SYSCTL.syscfg.js
<mspm0_sdk>/source/ti/driverlib/.meta/PWM.syscfg.js
<mspm0_sdk>/source/ti/driverlib/.meta/TIMER.syscfg.js
<mspm0_sdk>/source/ti/driverlib/.meta/ADC12.syscfg.js
<mspm0_sdk>/source/ti/driverlib/.meta/DMA.syscfg.js
```

TI `LP_MSPM0G3507` examples are often useful for Tianmengxing MSPM0G3507 work, but board pin maps still need Tianmengxing verification. Do not invent device, package, product, board, version metadata, module fields, or enum values.
