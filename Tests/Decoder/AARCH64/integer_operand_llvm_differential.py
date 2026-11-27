import re
import subprocess
import sys

import llvm_toolchain


CASES = {
    0x91000420: ("x0", "x1", "-", "-"),
    0x8B224820: ("x0", "x1", "w2", "uxtw"),
    0x9B220C20: ("x0", "w1", "w2", "-"),
    0x9AC24C20: ("w0", "w1", "x2", "-"),
    0x9A820020: ("x0", "x1", "x2", "-"),
    0xCB420C20: ("x0", "x1", "x2", "lsr"),
}


def main():
    output = subprocess.run([sys.argv[1]], capture_output=True, text=True, check=True)
    rows = {}
    for line in output.stdout.splitlines():
        word, registers, modifier = line.split("\t")
        rows[int(word, 16)] = (*registers.split(","), modifier)
    if set(rows) != set(CASES):
        raise RuntimeError("reference cases missing or unexpected")
    for word, expected in CASES.items():
        source = ", ".join(f"0x{(word >> (8 * i)) & 255:02x}" for i in range(4))
        result = subprocess.run(
            [llvm_toolchain.executable(), "--disassemble", "--triple=aarch64",
             "--mattr=+crc"],
            input=source + "\n", text=True, capture_output=True, check=True,
        )
        text = result.stdout.lower()
        registers = re.findall(r"\b(?:wzr|xzr|wsp|sp|[wx][0-9]+)\b", text)
        normalized = tuple((registers + ["-", "-", "-"])[:3])
        if normalized != expected[:3] or rows[word][:3] != expected[:3]:
            raise RuntimeError(f"0x{word:08X}: LLVM={registers}, Armito={rows[word]}")
        if expected[3] != "-" and (expected[3] not in text or rows[word][3].lower() != expected[3]):
            raise RuntimeError(f"0x{word:08X}: LLVM={text}, Armito={rows[word]}")
        print(f"Integer operands 0x{word:08X}: {rows[word]}")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Integer operand differential failed: {error}", file=sys.stderr)
        raise SystemExit(1)