#!/usr/bin/env python3
"""The single entry point for building and verifying the Lemuria.

Usage: py tools/dev.py <command>  (use python3 on Linux)

  verify     everything: format, build, tests, sanitizers, static analysis and checks
  quick      format, build, tests and checks (runs before each commit)
  configure  prepares the build folder and the third-party libraries
  build      compiles the engine
  test       runs the automated tests
  sanitize   compiles and runs the tests with the memory sanitizers
  format     checks the C++ formatting (--fix corrects it)
  tidy       runs the static analysis
  checks     runs the repository checks in tools/checks
  pytests    runs the tests of the checks and of the Git hooks
  gpu-test   opens the GPU, or a software renderer, without a window
  mutation   runs the mutation tests (Linux only)
  vcpkg-baseline  prints the vcpkg version pinned by this repository

This file is a protected "trava" (see docs/WORKFLOW.md).
"""

from __future__ import annotations

import argparse
import concurrent.futures
import json
import os
import platform
import shutil
import subprocess
import sys
import time
from collections.abc import Callable, Sequence
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
IS_WINDOWS = platform.system() == "Windows"
SYSTEM = "windows" if IS_WINDOWS else "linux"
TIDY_FOLDERS = ("src", "samples", "tools")
MUTATION_TEST_PROGRAMS = ("tests/core/core_tests",)


class StepFailed(Exception):
    """A verification step did not pass."""


def without_git_variables(environment: dict[str, str]) -> dict[str, str]:
    """Git hooks export variables that would redirect the Git commands of the tests to this repository."""
    return {name: value for name, value in environment.items() if not name.upper().startswith("GIT_")}


def visual_studio_path() -> Path:
    program_files = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
    vswhere = Path(program_files) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if not vswhere.exists():
        raise StepFailed("Visual Studio was not found. Install it with the C++ tools (docs/setup/DEV.md).")
    output = subprocess.run(
        [str(vswhere), "-latest", "-products", "*", "-requires",
         "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"],
        capture_output=True, text=True, check=False,
    ).stdout.strip()
    if not output:
        raise StepFailed("Visual Studio has no C++ tools. Install them (docs/setup/DEV.md).")
    return Path(output.splitlines()[0])


def load_visual_studio_environment() -> dict[str, str]:
    """Returns the environment of a Visual Studio developer prompt."""
    vcvars = visual_studio_path() / "VC" / "Auxiliary" / "Build" / "vcvars64.bat"
    completed = subprocess.run(
        f'call "{vcvars}" >nul 2>&1 && set', shell=True, capture_output=True, text=True, errors="replace", check=False
    )
    if completed.returncode != 0:
        raise StepFailed("Could not load the Visual Studio environment.")
    environment = {}
    for line in completed.stdout.splitlines():
        name, separator, value = line.partition("=")
        if separator and name:
            environment[name] = value
    return environment


def build_environment() -> dict[str, str]:
    environment = dict(os.environ)
    if IS_WINDOWS and shutil.which("cl") is None:
        environment = load_visual_studio_environment()
    if not environment.get("VCPKG_ROOT"):
        raise StepFailed("The VCPKG_ROOT variable is not set (docs/setup/DEV.md).")
    return environment


def run(command: Sequence[str], environment: dict[str, str] | None = None) -> None:
    completed = subprocess.run(list(command), cwd=ROOT, env=environment, check=False)
    if completed.returncode != 0:
        raise StepFailed(f"Command failed with exit code {completed.returncode}: {' '.join(command)}")


def find_llvm_tool(name: str, environment: dict[str, str]) -> str:
    found = shutil.which(name, path=environment.get("PATH"))
    if found:
        return found
    if IS_WINDOWS:
        bundled = visual_studio_path() / "VC" / "Tools" / "Llvm" / "x64" / "bin" / f"{name}.exe"
        if bundled.exists():
            return str(bundled)
    raise StepFailed(f"{name} was not found (docs/setup/DEV.md).")


def cpp_files(folders: Sequence[str] = (), sources_only: bool = False) -> list[str]:
    patterns = ["*.cpp"] if sources_only else ["*.cpp", "*.hpp"]
    listed = subprocess.run(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard", "--", *patterns],
        cwd=ROOT, capture_output=True, text=True, check=True, env=without_git_variables(dict(os.environ)),
    ).stdout.splitlines()
    existing = [path for path in listed if (ROOT / path).exists()]
    if not folders:
        return existing
    return [path for path in existing if path.split("/", 1)[0] in folders]


def preset(kind: str) -> str:
    return f"{SYSTEM}-{kind}"


def configure(kind: str = "dev") -> None:
    run(["cmake", "--preset", preset(kind)], build_environment())


def build(kind: str = "dev") -> None:
    configure(kind)
    run(["cmake", "--build", "--preset", preset(kind)], build_environment())


def test(kind: str = "dev") -> None:
    build(kind)
    run(["ctest", "--preset", preset(kind)], build_environment())


def sanitize() -> None:
    test("asan")


def format_code(fix: bool = False) -> None:
    environment = dict(os.environ) if not IS_WINDOWS else build_environment()
    tool = find_llvm_tool("clang-format", environment)
    files = cpp_files()
    mode = ["-i"] if fix else ["--dry-run", "--Werror"]
    for start in range(0, len(files), 40):
        run([tool, *mode, *files[start:start + 40]])


def tidy() -> None:
    environment = build_environment()
    configure("dev")
    tool = find_llvm_tool("clang-tidy", environment)
    build_folder = ROOT / "build" / preset("dev")
    files = cpp_files(TIDY_FOLDERS, sources_only=True)

    def analyze(path: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [tool, "--quiet", "-p", str(build_folder), "--extra-arg=-Wno-unused-command-line-argument", path],
            cwd=ROOT, env=environment, capture_output=True, text=True, errors="replace", check=False,
        )

    with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count() or 2) as pool:
        results = list(pool.map(analyze, files))

    failed = [result for result in results if result.returncode != 0]
    for result in failed:
        print(result.stdout, result.stderr, sep="\n")
    if failed:
        raise StepFailed(f"Static analysis found problems in {len(failed)} file(s).")
    print(f"Static analysis: {len(files)} file(s) analyzed, no problems.")


def checks(names: Sequence[str] = ("all",)) -> None:
    run([sys.executable, "tools/checks/run.py", *names])


def pytests() -> None:
    environment = without_git_variables(dict(os.environ))
    environment["PYTHONDONTWRITEBYTECODE"] = "1"
    for folder in ("tools/checks/tests", "tests/hooks"):
        run([sys.executable, "-m", "unittest", "discover", "-s", folder, "-p", "test_*.py"], environment)
    if IS_WINDOWS:
        for script in sorted((ROOT / "tests" / "hooks").glob("*.tests.ps1")):
            run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(script)], environment)


def gpu_test() -> None:
    build("dev")
    run(["ctest", "--test-dir", f"build/{preset('dev')}", "-L", "gpu", "--output-on-failure"], build_environment())


def mutation() -> None:
    if IS_WINDOWS:
        raise StepFailed("The mutation tests only run on Linux (docs/adr/0011-ferramentas-de-verificacao.md).")
    environment = build_environment()
    if not environment.get("MULL_IR_FRONTEND"):
        raise StepFailed("The MULL_IR_FRONTEND variable must point to the Mull compiler plugin.")
    environment["MULL_CONFIG"] = str(ROOT / "cmake" / "mull.yml")
    run(["cmake", "--preset", preset("mutation")], environment)
    run(["cmake", "--build", "--preset", preset("mutation")], environment)
    # Informative for now: mutants that survive are reported, but do not fail the job.
    for program in MUTATION_TEST_PROGRAMS:
        run(["mull-runner-19", "--reporters=IDE", "--allow-surviving", f"build/{preset('mutation')}/{program}"],
            environment)


def vcpkg_baseline() -> None:
    configuration = json.loads((ROOT / "vcpkg-configuration.json").read_text(encoding="utf-8"))
    print(configuration["default-registry"]["baseline"])


def run_steps(steps: Sequence[tuple[str, Callable[[], None]]]) -> int:
    finished: list[tuple[str, float]] = []
    for name, step in steps:
        print(f"\n==> {name}", flush=True)
        started = time.monotonic()
        try:
            step()
        except StepFailed as failure:
            print(f"\nFAILED: {name}\n{failure}")
            return 1
        finished.append((name, time.monotonic() - started))
    print("\nResult:")
    for name, seconds in finished:
        print(f"  OK  {name} ({seconds:.1f}s)")
    return 0


QUICK_STEPS: list[tuple[str, Callable[[], None]]] = [
    ("Formatting", format_code),
    ("Build and tests", test),
    ("Repository checks", checks),
    ("Tests of the checks and hooks", pytests),
]

BUILD_ONLY_STEPS: list[tuple[str, Callable[[], None]]] = [
    ("Build and tests", test),
    ("Build and tests with sanitizers", sanitize),
    ("Repository checks", checks),
    ("Tests of the checks and hooks", pytests),
]

VERIFY_STEPS: list[tuple[str, Callable[[], None]]] = [
    ("Formatting", format_code),
    ("Build and tests", test),
    ("Build and tests with sanitizers", sanitize),
    ("Static analysis", tidy),
    ("Repository checks", checks),
    ("Tests of the checks and hooks", pytests),
]


def main(arguments: Sequence[str]) -> int:
    parser = argparse.ArgumentParser(description="Builds and verifies the Lemuria.")
    parser.add_argument("command", choices=[
        "verify", "quick", "configure", "build", "test", "sanitize", "format", "tidy", "checks", "pytests",
        "gpu-test", "mutation", "vcpkg-baseline",
    ])
    parser.add_argument("--fix", action="store_true", help="format: correct the files")
    parser.add_argument("--no-static", action="store_true", help="verify: skip formatting and static analysis")
    parser.add_argument("names", nargs="*", help="checks: which checks to run")
    options = parser.parse_args(arguments)

    simple: dict[str, Callable[[], None]] = {
        "configure": configure, "build": build, "test": test, "sanitize": sanitize, "tidy": tidy,
        "pytests": pytests, "gpu-test": gpu_test, "mutation": mutation,
    }
    if options.command == "verify":
        return run_steps(BUILD_ONLY_STEPS if options.no_static else VERIFY_STEPS)
    if options.command == "quick":
        return run_steps(QUICK_STEPS)
    if options.command == "vcpkg-baseline":
        vcpkg_baseline()
        return 0
    if options.command == "format":
        return run_steps([("Formatting", lambda: format_code(options.fix))])
    if options.command == "checks":
        return run_steps([("Repository checks", lambda: checks(options.names or ("all",)))])
    return run_steps([(options.command, simple[options.command])])


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
