import os
import re
import subprocess

HISTORICAL_REFERENCE = "18.1.3"
QUALIFIED = {
    "18.1.3": "historical reference; pinned baselines generated with it",
    "18.1.8": "qualified by full reproduction of every pinned baseline",
    "21.1.8": "qualified by full reproduction of every pinned baseline",
}


def executable():
    return os.environ.get("LLVM_MC", "llvm-mc")


def version(exe=None):
    exe = exe or executable()
    output = subprocess.run([exe, "--version"], capture_output=True, text=True,
                            check=True).stdout
    match = re.search(r"LLVM version (\d+\.\d+\.\d+)", output)
    if not match:
        raise RuntimeError(f"cannot determine LLVM version from {exe!r}: "
                           f"{output.strip()!r}")
    return match.group(1)


def describe(exe=None):
    exe = exe or executable()
    return f"{exe} (LLVM {version(exe)})"


def require_qualified(exe=None):
    exe = exe or executable()
    ver = version(exe)
    if ver not in QUALIFIED:
        raise RuntimeError(
            f"unqualified LLVM MC toolchain {exe!r} version {ver}; qualified "
            f"versions: {', '.join(sorted(QUALIFIED))}; qualify a new version "
            f"by reproducing every pinned baseline, then record it here")
    return exe, ver