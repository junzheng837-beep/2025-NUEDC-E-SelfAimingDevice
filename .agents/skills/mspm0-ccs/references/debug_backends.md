# Debug Backends

Use this reference when the user asks the agent to flash or debug a connected MSPM0 board. Keep CCS-DSS and OpenOCD/GDB as separate backends.

Before selecting a backend for an unspecified probe, run:

```powershell
python scripts\detect_probe.py
python scripts\check_syscfg.py <project-dir> --probe
```

Probe detection is read-only. Do not flash when multiple probes are connected, detection is unknown, or the physical probe conflicts with project configuration until the user confirms the intended backend.

## CCS-DSS Backend

CCS Debug Server Scripting is abbreviated as `ccs-dss` in this skill. Use it for CCS / CCS Theia / UniFlash tooling. Do not apply these commands to a CMake/OpenOCD project unless that project also has a valid CCS `.ccxml` and the user explicitly wants CCS DSS.

## Scope

- Requires TI CCS / CCS Theia or UniFlash scripting components.
- Requires a valid `targetConfigs/*.ccxml` for the active board and probe.
- Requires a built CCS `.out` file when loading or reloading firmware.
- Uses the debug probe selected inside `.ccxml`, so it is not limited to J-Link. It can also work with CCS-supported probes such as XDS110 when the `.ccxml` matches the connected hardware.
- Does not cover OpenOCD/GDB debugging. Use the separate backend below.

## Safety Rules

- Debug actions can halt the CPU and disturb real-time behavior. Warn the user before halting a motor, power stage, or time-sensitive control loop.
- Prefer UART logging, logic analyzer capture, or scoped register reads when non-intrusive observation is enough.
- Use symbol breakpoints before source-line breakpoints when possible. Source-line breakpoints require valid debug info and a line that maps to generated code.
- If a source-line breakpoint fails but symbol breakpoints work, treat it as a debug-info/source-line mapping issue first, not proof that the board or probe failed.
- If connect/list-core operations hang, close stale CCS, DSLite, UniFlash, J-Link, or debug-server processes before retrying.

## Script

From this skill package:

```powershell
python scripts\ccs_dss_debug.py <project-dir> probe --leave-running
```

When running from the repository root:

```powershell
python skills\mspm0-ccs\scripts\ccs_dss_debug.py <project-dir> probe --leave-running
```

Common commands:

```powershell
# Connect, read reset types and registers, then continue the target before disconnecting.
python scripts\ccs_dss_debug.py <project-dir> probe --leave-running

# Program the current Debug/Release .out and use System Reset after loading.
python scripts\ccs_dss_debug.py <project-dir> load --reset "System Reset" --leave-running

# Program, reset, and halt at main.
python scripts\ccs_dss_debug.py <project-dir> run-to-symbol --symbol main --load --reset "System Reset"

# Program, reset, and halt at a source line.
python scripts\ccs_dss_debug.py <project-dir> break-line --source empty.c --line 5 --load --reset "System Reset"

# Load debug symbols only; do not program flash.
python scripts\ccs_dss_debug.py <project-dir> load-symbols --symbol main --symbol UART_DMADoneTxCallback

# Load symbols only, reset, and halt at a source line without reprogramming flash.
python scripts\ccs_dss_debug.py <project-dir> break-line --source BSP/UART.c --line 75 --symbols --reset "System Reset"

# Load symbols only, reset, and halt at an address breakpoint.
python scripts\ccs_dss_debug.py <project-dir> break-address --address 0x2564 --symbols --reset "System Reset"

# Continue a currently connected target and disconnect the debug session.
python scripts\ccs_dss_debug.py <project-dir> run

# Halt and print PC/SP/LR.
python scripts\ccs_dss_debug.py <project-dir> halt
```

Useful options:

- `--ccs-run <path>`: explicit CCS scripting `run.bat`.
- `--ccxml <path>`: explicit target configuration.
- `--out <path>`: explicit program output file.
- `--timeout-ms <n>`: DSS script timeout.
- `--keep-js`: keep the temporary JavaScript for diagnosis.
- `--symbols`: load debug symbols from `.out` without programming flash for commands that support it.
- `--leave-running`: remove breakpoints and continue target execution before disconnecting, where supported by the chosen command.

## Verified Notes

Validated on LCKFB Tianmengxing MSPM0G3507 + CCS / CCS Theia + J-Link with a CCS project containing `targetConfigs/MSPM0G3507.ccxml` and `Debug/<project>.out`.

Observed working operations:

- `ds.configure(ccxml)` and `ds.openSession(/cortex|m0|MSPM0/i)`.
- `session.target.connect()`.
- `session.target.getResets()` returning reset types including Board Reset, CPU Reset, Core Reset, and System Reset.
- `session.registers.read("PC")`, `SP`, and `LR`.
- `session.memory.loadProgram(<project>.out)`.
- `session.symbols.load(<project>.out)` for no-flash symbol loading.
- `session.symbols.getAddress(<symbol>)` and `session.symbols.lookupSymbols(<address>)`.
- Symbol breakpoint at `main`.
- System Reset followed by `target.run()` halting at `main`.
- Source-line breakpoint at a line with generated code.
- Address breakpoint at a known function address.
- `target.run(false)` to leave the target running before disconnect.

One tested source line failed because no code was associated with that exact line. Another source line in the same file worked. For agents, that means source-line breakpoint failure should be reported precisely and retried with a symbol, nearby executable line, or address.

For non-invasive diagnosis after firmware has already been flashed, prefer `load-symbols`, `break-line --symbols`, or `break-address --symbols` over `--load`. These commands load debug information from the `.out` file without rewriting flash.

## When To Stop

Stop and ask the user before continuing if:

- The `.ccxml` probe does not match the connected hardware.
- The board controls motors, high-power outputs, or moving mechanisms and the next step will halt the CPU.
- The script can connect but loading a new `.out` would overwrite firmware the user did not ask to replace.
- OpenOCD files are present and the user appears to be using an OpenOCD workflow instead of CCS DSS.

## OpenOCD / GDB Backend

Use this backend when the detected probe and interface configuration are compatible with an MSPM0-capable OpenOCD installation.

### Verified Scope

The packaged helper was verified with:

- MSPM0G3507 hardware
- CMSIS-DAP / DAPLink probe
- an MSPM0-capable OpenOCD build containing `target/ti/mspm0.cfg` or `target/ti_mspm0.cfg`
- `interface/cmsis-dap.cfg`
- `arm-none-eabi-gdb`
- a TI Arm Clang-generated CCS `.out` ELF file

The helper can also use `.elf`, `.axf`, `.hex`, and `.bin` outputs. A raw `.bin` requires `--base-address`.

OpenOCD MSPM0 support commonly requires a TI MSPM0-capable build or TI extension branch. Do not assume an unrelated mainline OpenOCD installation can access MSPM0 correctly.

### Commands

```powershell
python scripts\openocd_debug.py <project-dir> probe
python scripts\openocd_debug.py <project-dir> flash
python scripts\openocd_debug.py <project-dir> registers
python scripts\openocd_debug.py <project-dir> run-to-symbol --symbol main
```

Other available actions:

```powershell
python scripts\openocd_debug.py <project-dir> run
python scripts\openocd_debug.py <project-dir> reset
```

Default config and fallback speeds:

```text
interface/cmsis-dap.cfg
target/ti/mspm0.cfg or target/ti_mspm0.cfg, auto-detected from the OpenOCD installation
24000,1000,500 kHz
```

Override them only when the connected probe, target support package, or project requires a different choice:

```powershell
python scripts\openocd_debug.py <project-dir> --interface <interface.cfg> --target <target.cfg> --speeds 24000,1000,500 probe
```

### Flash Behavior

`flash` auto-detects a compatible program output under the project directory, then performs:

```text
init
reset init
flash write_image erase <program>
verify_image <program>
reset run
shutdown
```

The helper searches CCS `Debug`/`Release`, generic `build`, CLion-style `cmake-build-*`, and the project root. Use `--program <path>` when several outputs exist and the automatic choice is ambiguous. Use `--no-verify` only when the user explicitly accepts losing verification.

### Connection Failures And Retries

The helper separates probe and transport failures from firmware failures. Its default retry order is `24000`, `1000`, then `500` kHz.

- `unable to find a matching CMSIS-DAP device`: probe discovery failure. Check USB or wireless DAPLink connectivity.
- `CMSIS-DAP command mismatch` or `CMD_CONNECT failed`: likely link corruption, wireless interruption, or another process holding the probe.
- target halt, SWD ACK, or DAP access failures: retry after reconnecting and lowering speed; if repeated, check wiring and ask whether the target needs manual unlock.
- verify failure: retry with a stable connection and lower speed before changing commands.

Do not run parallel OpenOCD operations against one probe. Flash, register reads, and GDB sessions can contend with each other.

### Locked Or Protected Targets

The helper intentionally does not perform automatic unlock, mass erase, or factory reset operations.

If it reports `target_locked_or_protected`, stop retries and ask the user to run their known manual unlock or recovery procedure. This avoids destructive recovery when a transient wireless failure merely resembles a target-access problem.

### OpenOCD Debug Safety

`probe` and `registers` briefly halt the current CPU state without resetting the target, then restore execution before the one-shot OpenOCD server exits. `run-to-symbol` intentionally resets and runs to the requested breakpoint. Before using debug actions on motors, power electronics, or other real-time control systems:

1. Warn the user that debug actions may pause control loops.
2. Put actuators into a safe state when possible.
3. Do not promise that a one-shot OpenOCD command can leave the target halted after the server exits. Use a separately managed persistent OpenOCD session when a paused target must remain under debugger control.

`run-to-symbol` refuses to attach when its GDB port is already occupied. Close the existing OpenOCD/debug session or choose an unused `--gdb-port`; do not attach to an unverified listener.
