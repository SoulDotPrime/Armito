import re
import subprocess
import sys

import llvm_toolchain


CASES = (
    (0xD2800000, "movz", "mov"),
    (0x8B020020, "add", "add"),
    (0xCB020020, "sub", "sub"),
    (0xD65F03C0, "ret", "ret"),
    (0xD61F0000, "br", "br"),
    (0xD63F0000, "blr", "blr"),
    (0x14000000, "b", "b"),
    (0x94000000, "bl", "bl"),
    (0xB4000040, "cbz", "cbz"),
    (0xB5000040, "cbnz", "cbnz"),
    (0x92800020, "movn", "mov"),
    (0x91003020, "add", "add"),
    (0xCB050083, "sub", "sub"),
    (0x8A020020, "and", "and"),
    (0xAA050083, "orr", "orr"),
    (0xCA0800E6, "eor", "eor"),
    (0xD37DF020, "ubfm", "lsl"),
    (0x10000080, "adr", "adr"),
    (0x9A820020, "csel", "csel"),
    (0x9B020C20, "madd", "madd"),
    (0x9AC608A4, "udiv", "udiv"),
    (0x39400020, "ldrb", "ldrb"),
    (0x39001062, "strb", "strb"),
    (0xA94107E0, "ldp", "ldp"),
    (0xA9BF0FE2, "stp", "stp"),
    (0x54000080, "b", "b.eq"),
    (0x36180080, "tbz", "tbz"),
    (0xD4000001, "svc", "svc"),
    (0xD503233F, "paciasp", "paciasp"),
    (0xD50323BF, "autiasp", "autiasp"),
    (0xD65F0FFF, "retab", "retab"),
    (0xD71F0801, "braa", "braa"),
    (0xC8A07C41, "cas", "cas"),
)


def llvm_bytes(encoding):
    return ", ".join(
        f"0x{(encoding >> (index * 8)) & 0xFF:02x}"
        for index in range(4)
    )


def disassemble(encoding):
    process = subprocess.run(
        [
            llvm_toolchain.executable(),
            "--disassemble",
            "--triple=aarch64",
            "--mattr=+pauth,+lse",
        ],
        input=llvm_bytes(encoding) + "\n",
        text=True,
        capture_output=True,
        check=False,
    )
    if process.returncode != 0 or process.stderr:
        raise RuntimeError(
            f"llvm-mc rejected 0x{encoding:08X}: {process.stderr.strip()}"
        )
    for line in process.stdout.splitlines():
        stripped = line.strip()
        if stripped and stripped != ".text":
            return re.split(r"[\s,]+", stripped, maxsplit=1)[0].lower()
    raise RuntimeError(f"llvm-mc returned no instruction for 0x{encoding:08X}")


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: catalog_llvm_differential.py <reference-binary>")

    process = subprocess.run(
        [sys.argv[1]],
        text=True,
        capture_output=True,
        check=False,
    )
    if process.returncode != 0:
        raise RuntimeError(process.stderr.strip() or "catalog reference failed")

    rows = {}
    for line in process.stdout.splitlines():
        columns = line.split("\t")
        if len(columns) != 3:
            raise RuntimeError(f"invalid catalog reference output: {line}")
        rows[int(columns[0], 16)] = (
            columns[1].lower(),
            {name.lower() for name in columns[2].split(",") if name},
        )

    for encoding, catalog_expected, llvm_expected in CASES:
        catalog_row, catalog_forms = rows.get(encoding, ("", set()))
        llvm_mnemonic = disassemble(encoding)
        if (
            catalog_row != catalog_expected
            or catalog_expected not in catalog_forms
            or llvm_mnemonic != llvm_expected
        ):
            raise RuntimeError(
                f"0x{encoding:08X}: catalog={catalog_row}/{catalog_forms}, "
                f"LLVM={llvm_mnemonic}, expected={catalog_expected}/{llvm_expected}"
            )
        print(
            f"0x{encoding:08X}: AARCHMRS {catalog_expected}, LLVM {llvm_expected}"
        )

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError) as error:
        print(f"AArch64 differential test failed: {error}", file=sys.stderr)
        raise SystemExit(1)
