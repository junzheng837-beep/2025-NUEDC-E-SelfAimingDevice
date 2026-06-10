#!/usr/bin/env python3
"""OpenOCD debug and flash helper for TI MSPM0 projects.

This tool intentionally keeps recovery manual. If a target appears locked or
protected, it reports that state and stops instead of issuing a mass erase or
factory reset.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import socket
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Callable


DEFAULT_INTERFACE_CFG = "interface/cmsis-dap.cfg"
DEFAULT_TARGET_CFGS = ("target/ti/mspm0.cfg", "target/ti_mspm0.cfg")
DEFAULT_SPEEDS = (24000, 1000, 500)
PROGRAM_SUFFIXES = (".out", ".elf", ".axf", ".hex", ".bin")


@dataclass
class AttemptResult:
    ok: bool
    returncode: int
    output: str
    category: str
    guidance: str
    speed: int


def emit(event: str, **fields: object) -> None:
    print(json.dumps({"event": event, **fields}, ensure_ascii=False))


def find_existing(paths: list[Path], what: str) -> Path:
    for path in paths:
        if path.exists():
            return path.resolve()
    raise SystemExit(f"error: could not find {what}")


def find_openocd(explicit: str | None) -> Path:
    if explicit:
        return find_existing([Path(explicit)], "OpenOCD executable")

    candidates: list[Path] = []
    env_value = os.environ.get("OPENOCD")
    if env_value:
        candidates.append(Path(env_value))

    discovered = shutil.which("openocd")
    if discovered:
        candidates.append(Path(discovered))

    return find_existing(candidates, "OpenOCD executable; use --openocd")


def find_target_cfg(openocd_path: Path, explicit: str | None) -> str:
    if explicit:
        return explicit

    roots: list[Path] = []
    env_value = os.environ.get("OPENOCD_SCRIPTS")
    if env_value:
        roots.append(Path(env_value))
    prefix = openocd_path.resolve().parent.parent
    roots.extend((prefix / "share" / "openocd" / "scripts", prefix / "scripts"))
    for candidate in DEFAULT_TARGET_CFGS:
        if any((root / candidate).exists() for root in roots):
            return candidate
    return DEFAULT_TARGET_CFGS[0]


def find_gdb(explicit: str | None) -> Path:
    if explicit:
        return find_existing([Path(explicit)], "arm-none-eabi-gdb executable")

    env_value = os.environ.get("ARM_NONE_EABI_GDB")
    candidates = [Path(env_value)] if env_value else []
    discovered = shutil.which("arm-none-eabi-gdb")
    if discovered:
        candidates.append(Path(discovered))
    return find_existing(candidates, "arm-none-eabi-gdb executable; use --gdb")


def find_program(project_dir: Path, explicit: str | None) -> Path:
    if explicit:
        return find_existing([Path(explicit)], "program output")

    candidates: list[Path] = []
    build_dirs = [project_dir / name for name in ("Debug", "Release", "build")]
    build_dirs.extend(
        path
        for path in project_dir.iterdir()
        if path.is_dir() and path.name.lower().startswith("cmake-build-")
    )
    for root in build_dirs:
        if not root.exists():
            continue
        for suffix in PROGRAM_SUFFIXES:
            candidates.extend(sorted(root.rglob(f"*{suffix}")))
    for suffix in PROGRAM_SUFFIXES:
        candidates.extend(sorted(project_dir.glob(f"*{suffix}")))
    existing = sorted({path.resolve() for path in candidates if path.exists()}, key=lambda path: str(path).lower())
    if not existing:
        raise SystemExit("error: could not find program output (*.out, *.elf, *.axf, *.hex, *.bin); use --program")
    if len(existing) > 1:
        choices = "\n".join(f"  - {path}" for path in existing)
        raise SystemExit(f"error: found multiple program outputs; choose one explicitly with --program:\n{choices}")
    return existing[0]


def parse_speeds(value: str) -> list[int]:
    speeds: list[int] = []
    for item in value.split(","):
        item = item.strip()
        if not item:
            continue
        speed = int(item)
        if speed <= 0:
            raise argparse.ArgumentTypeError("adapter speeds must be positive integers")
        speeds.append(speed)
    if not speeds:
        raise argparse.ArgumentTypeError("provide at least one adapter speed")
    return speeds


def tcl_path(path: Path) -> str:
    value = str(path.resolve()).replace("\\", "/")
    return "{" + value.replace("}", "\\}") + "}"


def classify_failure(output: str, timed_out: bool = False) -> tuple[str, str]:
    text = output.lower()

    if timed_out:
        return (
            "transport_or_target_timeout",
            "OpenOCD timed out. Treat this as a possible wireless/probe interruption first; retry with a lower speed. If repeated target-access failures persist, ask the user to check wiring and whether the MSPM0 needs manual unlock.",
        )

    config_patterns = (
        "can't find",
        "couldn't open",
        "invalid command name",
        "unknown command",
        "no flash bank found",
        "can't open",
        "no such file",
    )
    if any(pattern in text for pattern in config_patterns):
        return (
            "configuration_error",
            "OpenOCD configuration or file lookup failed. Fix the command, cfg path, or program path before retrying.",
        )

    probe_patterns = (
        "unable to find a matching cmsis-dap device",
        "no cmsis-dap device found",
        "unable to open cmsis-dap device",
        "cmsis-dap: failed to open",
        "libusb_open() failed",
    )
    if any(pattern in text for pattern in probe_patterns):
        return (
            "probe_not_found",
            "No usable CMSIS-DAP probe was found. Check USB or wireless DAPLink connectivity, then retry. This is not evidence that the firmware command is wrong.",
        )

    probe_transport_patterns = (
        "cmsis-dap command mismatch",
        "cmd_connect failed",
        "cmsis-dap command",
    )
    if any(pattern in text for pattern in probe_transport_patterns):
        return (
            "probe_transport_or_contention_failure",
            "CMSIS-DAP communication was corrupted or the probe was busy. Do not rewrite the OpenOCD command after one failure. Close competing OpenOCD/debug sessions, wait for the wireless DAPLink link to recover, then retry.",
        )

    lock_patterns = (
        "target is locked",
        "device is locked",
        "debug access is locked",
        "authentication required",
        "access is protected",
        "device is protected",
        "secap cmd fail",
        "security violation",
    )
    if any(pattern in text for pattern in lock_patterns):
        return (
            "target_locked_or_protected",
            "The MSPM0 appears locked or protected. Stop automatic retries and tell the user to perform their manual unlock/recovery procedure. This helper will not mass erase or factory reset automatically.",
        )

    target_access_patterns = (
        "examination failed",
        "target not examined",
        "unable to halt",
        "timed out while waiting for target halted",
        "error executing cortex_m crc algorithm",
        "swd ack",
        "dap transaction failed",
        "error connecting dp",
        "failed to read memory",
        "failed to write memory",
    )
    if any(pattern in text for pattern in target_access_patterns):
        return (
            "transport_or_target_access_failure",
            "The probe was not reliably able to access the target. Retry because wireless DAPLink interruption is possible. If the failure repeats after reconnecting and lowering speed, ask the user to check wiring and manually unlock the MSPM0 if needed.",
        )

    if "verify failed" in text or "checksum mismatch" in text:
        return (
            "verify_failed",
            "Flash verification failed. Retry with explicit reset-halt sequencing and a lower adapter speed. Do not assume the command syntax is wrong after a single wireless-link failure.",
        )

    return (
        "openocd_error",
        "OpenOCD returned an error. Inspect the raw log before changing commands. Retry only if the log is consistent with an intermittent probe or wireless-link failure.",
    )


def normalize_subprocess_stream(value: str | bytes | None) -> str:
    if isinstance(value, bytes):
        return value.decode("utf-8", errors="replace")
    return value or ""


def breakpoint_was_hit(gdb_output: str) -> bool:
    created = re.search(r"(?m)^Breakpoint (\d+) at\b", gdb_output)
    if not created:
        return False
    return re.search(rf"(?m)^Breakpoint {re.escape(created.group(1))},\s+", gdb_output) is not None


def print_output(output: str) -> None:
    encoding = sys.stdout.encoding or "utf-8"
    safe_output = output.encode(encoding, errors="replace").decode(encoding, errors="replace")
    print(safe_output, end="" if safe_output.endswith("\n") else "\n")


def should_retry(category: str) -> bool:
    return category in {
        "probe_not_found",
        "probe_transport_or_contention_failure",
        "transport_or_target_timeout",
        "transport_or_target_access_failure",
        "verify_failed",
        "openocd_error",
    }


def openocd_args(args: argparse.Namespace, speed: int, commands: str) -> list[str]:
    return [
        str(args.openocd_path),
        "-f",
        args.interface_cfg,
        "-c",
        f"adapter speed {speed}",
        "-f",
        args.target_cfg,
        "-c",
        commands,
    ]


def run_openocd_once(args: argparse.Namespace, speed: int, commands: str) -> AttemptResult:
    command = openocd_args(args, speed, commands)
    emit("attempt", backend="openocd", speed_khz=speed, argv=command)
    try:
        completed = subprocess.run(
            command,
            cwd=args.project_dir,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=args.process_timeout,
        )
        output = completed.stdout + completed.stderr
        print_output(output)
        if completed.returncode == 0:
            return AttemptResult(True, 0, output, "success", "OpenOCD command completed.", speed)
        category, guidance = classify_failure(output)
        return AttemptResult(False, completed.returncode, output, category, guidance, speed)
    except subprocess.TimeoutExpired as exc:
        output = normalize_subprocess_stream(exc.stdout) + normalize_subprocess_stream(exc.stderr)
        print_output(output)
        category, guidance = classify_failure(output, timed_out=True)
        return AttemptResult(False, 124, output, category, guidance, speed)


def run_with_retries(
    args: argparse.Namespace,
    operation: str,
    command_builder: Callable[[int], str],
) -> int:
    attempts = 0
    result: AttemptResult | None = None
    for speed in args.speeds:
        for _ in range(args.attempts_per_speed):
            attempts += 1
            result = run_openocd_once(args, speed, command_builder(speed))
            emit(
                "attempt_result",
                operation=operation,
                attempt=attempts,
                speed_khz=speed,
                ok=result.ok,
                category=result.category,
                guidance=result.guidance,
            )
            if result.ok:
                emit("completed", operation=operation, attempts=attempts, speed_khz=speed)
                return 0
            if not should_retry(result.category):
                emit("stopped", operation=operation, attempts=attempts, category=result.category, guidance=result.guidance)
                return result.returncode or 1
            time.sleep(args.retry_delay)

    if result is None:
        guidance = "No adapter speeds were provided. Pass at least one positive speed with --speeds."
        emit("failed", operation=operation, attempts=attempts, category="invalid_arguments", guidance=guidance)
        return 2
    emit(
        "failed",
        operation=operation,
        attempts=attempts,
        category=result.category,
        guidance=result.guidance,
    )
    return result.returncode or 1


def flash_commands(program: Path, base_address: str | None, verify: bool) -> str:
    path = tcl_path(program)
    if program.suffix.lower() == ".bin":
        if not base_address:
            raise SystemExit("error: --base-address is required when flashing a .bin file")
        write = f"flash write_image erase {path} {base_address} bin"
        check = f"verify_image {path} {base_address} bin"
    else:
        write = f"flash write_image erase {path}"
        check = f"verify_image {path}"

    commands = f"init; reset init; {write}"
    if verify:
        commands += f"; {check}"
    commands += "; reset run; shutdown"
    return commands


def wait_for_port(host: str, port: int, process: subprocess.Popen[str], timeout: float) -> bool:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if process.poll() is not None:
            return False
        try:
            with socket.create_connection((host, port), timeout=0.2):
                return True
        except OSError:
            time.sleep(0.1)
    return False


def port_is_open(host: str, port: int) -> bool:
    try:
        with socket.create_connection((host, port), timeout=0.2):
            return True
    except OSError:
        return False


def terminate_process(process: subprocess.Popen[str]) -> str:
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=3)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=3)
    stdout, stderr = process.communicate()
    return (stdout or "") + (stderr or "")


def run_to_symbol_once(args: argparse.Namespace, speed: int, program: Path, symbol: str) -> AttemptResult:
    port = args.gdb_port
    if port_is_open("127.0.0.1", port):
        guidance = (
            f"GDB port {port} is already in use. Stop the existing OpenOCD/debug session or choose a free --gdb-port "
            "before running run-to-symbol. This helper will not attach to an unverified server."
        )
        emit("stopped", operation="run-to-symbol", category="gdb_port_in_use", guidance=guidance)
        return AttemptResult(False, 2, guidance, "gdb_port_in_use", guidance, speed)
    openocd_command = openocd_args(
        args,
        speed,
        f"gdb_port {port}; telnet_port disabled; tcl_port disabled; init; reset halt",
    )
    emit("attempt", backend="openocd-gdb", speed_khz=speed, argv=openocd_command, symbol=symbol)
    process = subprocess.Popen(
        openocd_command,
        cwd=args.project_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    try:
        if not wait_for_port("127.0.0.1", port, process, args.server_timeout):
            timed_out = process.poll() is None
            output = terminate_process(process)
            print_output(output)
            category, guidance = classify_failure(output, timed_out=timed_out)
            return AttemptResult(False, process.returncode or 1, output, category, guidance, speed)

        gdb_commands = [
            str(args.gdb_path),
            "-q",
            "-batch",
            str(program),
            "-ex",
            f"target extended-remote 127.0.0.1:{port}",
            "-ex",
            "monitor reset halt",
            "-ex",
            "delete breakpoints",
            "-ex",
            f"break {symbol}",
            "-ex",
            "continue",
            "-ex",
            "info registers",
            "-ex",
            "bt",
            "-ex",
            "delete breakpoints",
        ]
        gdb_commands.extend(["-ex", "monitor resume", "-ex", "detach", "-ex", "quit"])
        emit("gdb", argv=gdb_commands)
        try:
            completed = subprocess.run(
                gdb_commands,
                cwd=args.project_dir,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
                timeout=args.process_timeout,
            )
            gdb_output = completed.stdout + completed.stderr
        except subprocess.TimeoutExpired as exc:
            gdb_output = normalize_subprocess_stream(exc.stdout) + normalize_subprocess_stream(exc.stderr)
            server_output = terminate_process(process)
            output = server_output + gdb_output
            print_output(output)
            category, guidance = classify_failure(output, timed_out=True)
            return AttemptResult(False, 124, output, category, guidance, speed)

        server_output = terminate_process(process)
        output = server_output + gdb_output
        print_output(output)
        if completed.returncode == 0 and breakpoint_was_hit(gdb_output):
            return AttemptResult(True, 0, output, "success", "GDB hit the requested symbol breakpoint.", speed)
        category, guidance = classify_failure(output)
        return AttemptResult(False, completed.returncode or 1, output, category, guidance, speed)
    finally:
        if process.poll() is None:
            terminate_process(process)


def run_to_symbol(args: argparse.Namespace, program: Path, symbol: str) -> int:
    attempts = 0
    result: AttemptResult | None = None
    for speed in args.speeds:
        for _ in range(args.attempts_per_speed):
            attempts += 1
            result = run_to_symbol_once(args, speed, program, symbol)
            emit(
                "attempt_result",
                operation="run-to-symbol",
                attempt=attempts,
                speed_khz=speed,
                ok=result.ok,
                category=result.category,
                guidance=result.guidance,
            )
            if result.ok:
                emit("completed", operation="run-to-symbol", attempts=attempts, speed_khz=speed, symbol=symbol)
                return 0
            if not should_retry(result.category):
                emit("stopped", operation="run-to-symbol", attempts=attempts, category=result.category, guidance=result.guidance)
                return result.returncode or 1
            time.sleep(args.retry_delay)

    if result is None:
        guidance = "No adapter speeds were provided. Pass at least one positive speed with --speeds."
        emit("failed", operation="run-to-symbol", attempts=attempts, category="invalid_arguments", guidance=guidance)
        return 2
    emit("failed", operation="run-to-symbol", attempts=attempts, category=result.category, guidance=result.guidance)
    return result.returncode or 1


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("project_dir", help="MSPM0 project directory")
    parser.add_argument("--openocd", help="Path to openocd executable")
    parser.add_argument("--interface", dest="interface_cfg", default=DEFAULT_INTERFACE_CFG, help="OpenOCD interface cfg")
    parser.add_argument("--target", dest="target_cfg", help="OpenOCD target cfg; auto-detected by default")
    parser.add_argument(
        "--speeds",
        type=parse_speeds,
        default=list(DEFAULT_SPEEDS),
        help="Comma-separated SWD speeds in kHz, tried in order. Default: 24000,1000,500",
    )
    parser.add_argument(
        "--attempts-per-speed",
        type=int,
        default=1,
        help="Attempts for each SWD speed. Default: 1",
    )
    parser.add_argument("--process-timeout", type=float, default=30, help="Timeout per attempt in seconds. Default: 30")
    parser.add_argument("--retry-delay", type=float, default=1, help="Seconds to wait between retries. Default: 1")

    subparsers = parser.add_subparsers(dest="command", required=True)
    probe = subparsers.add_parser("probe", help="Connect, halt, report target state, then resume")

    flash = subparsers.add_parser("flash", help="Flash an ELF/OUT/AXF/HEX/BIN program and optionally verify")
    flash.add_argument("--program", help="Program output path; auto-detected from the project by default")
    flash.add_argument("--base-address", help="Required for raw .bin files, such as 0x0")
    flash.add_argument("--no-verify", action="store_true", help="Skip verify_image")

    registers = subparsers.add_parser("registers", help="Halt and print core registers")

    subparsers.add_parser("run", help="Resume the target")

    subparsers.add_parser("reset", help="Reset and run the target")

    symbol = subparsers.add_parser("run-to-symbol", help="Use OpenOCD + GDB to reset and run to a symbol breakpoint")
    symbol.add_argument("--program", help="Program output path; auto-detected from the project by default")
    symbol.add_argument("--symbol", default="main", help="Symbol breakpoint. Default: main")
    symbol.add_argument("--gdb", help="Path to arm-none-eabi-gdb")
    symbol.add_argument("--gdb-port", type=int, default=3333, help="OpenOCD GDB port. Default: 3333")
    symbol.add_argument("--server-timeout", type=float, default=5, help="Seconds to wait for OpenOCD GDB server")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    args.project_dir = Path(args.project_dir).resolve()
    if not args.project_dir.exists():
        parser.error(f"project directory does not exist: {args.project_dir}")
    if args.attempts_per_speed < 1:
        parser.error("--attempts-per-speed must be at least 1")
    if args.retry_delay < 0:
        parser.error("--retry-delay cannot be negative")
    args.openocd_path = find_openocd(args.openocd)
    args.target_cfg = find_target_cfg(args.openocd_path, args.target_cfg)
    emit(
        "wrapper",
        backend="openocd",
        openocd=str(args.openocd_path),
        project=str(args.project_dir),
        interface=args.interface_cfg,
        target=args.target_cfg,
        speeds_khz=args.speeds,
        automatic_unlock=False,
    )

    if args.command == "probe":
        return run_with_retries(args, "probe", lambda _speed: "init; halt; targets; resume; shutdown")
    if args.command == "flash":
        program = find_program(args.project_dir, args.program)
        emit("program", path=str(program), suffix=program.suffix.lower())
        return run_with_retries(
            args,
            "flash",
            lambda _speed: flash_commands(program, args.base_address, verify=not args.no_verify),
        )
    if args.command == "registers":
        return run_with_retries(
            args,
            "registers",
            lambda _speed: (
                f"init; halt; "
                f"echo [reg pc]; echo [reg sp]; echo [reg lr]; echo [reg xpsr]; resume; shutdown"
            ),
        )
    if args.command == "run":
        return run_with_retries(args, "run", lambda _speed: "init; resume; shutdown")
    if args.command == "reset":
        return run_with_retries(args, "reset", lambda _speed: "init; reset run; shutdown")
    if args.command == "run-to-symbol":
        if not re.fullmatch(r"[A-Za-z_.$][A-Za-z0-9_.$:]*", args.symbol):
            parser.error("--symbol contains unsupported characters")
        args.gdb_path = find_gdb(args.gdb)
        program = find_program(args.project_dir, args.program)
        emit("program", path=str(program), suffix=program.suffix.lower())
        return run_to_symbol(args, program, args.symbol)

    parser.error(f"unsupported command: {args.command}")
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
