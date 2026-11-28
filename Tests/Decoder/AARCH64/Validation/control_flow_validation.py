import collections
import csv
import hashlib
import io
import json
import os
import re
import shutil
import subprocess
import sys

import llvm_toolchain

CATALOG = "Decoder/AARCH64/Catalog/AARCH64InstructionCatalog.json"
REPORT = "Tests/Decoder/AARCH64/Results/control_flow_validation.csv"
REPORT_SHA256 = "edddbdeab51b3491df3189d94445dbd19254a8e8e9443765adf0a1a61db4a4ff"
PC = 0x1000000000004000
MASK64 = (1 << 64) - 1
CONVENTIONAL = frozenset(("branch_imm", "condbranch", "compbranch",
                          "compbranch_imm", "compbranch_regs",
                          "compbranch_regs2", "testbranch"))
INDIRECT = {"BR": 0xD61F0000, "BLR": 0xD63F0000,
            "RET": 0xD65F03C0}
FEATURES = "+cmpbr,+hbc"


def signed(value, bits):
    return value - (1 << bits) if value & (1 << (bits - 1)) else value


def field(word, lsb, width):
    return (word >> lsb) & ((1 << width) - 1)


def conventional(form):
    return form["family_path"][1] in CONVENTIONAL or (
        form["family_path"][1] == "branch_reg" and
        form["operation_id"] in INDIRECT)


def probe_rows(probe, cases):
    source = "".join(f"{fid} {word:08X} {PC:016X} {disabled}\n"
                     for fid, word, _, disabled in cases)
    process = subprocess.run([probe], input=source, text=True,
                             capture_output=True, check=True)
    lines = process.stdout.splitlines()
    if len(lines) != len(cases):
        raise RuntimeError(f"probe returned {len(lines)} of {len(cases)} lines")
    results = []
    for (fid, word, _, _), line in zip(cases, lines):
        columns = line.split("\t")
        if columns[:2] != [fid, f"{word:08X}"]:
            raise RuntimeError(f"probe misalignment: {line}")
        if columns[2] == "REJECT":
            results.append(None)
            continue
        if columns[2] != "COMPLETE":
            raise RuntimeError(f"unexpected probe status: {line}")
        metadata = tuple(map(int, columns[3].split(":")))
        operands = {}
        for column in columns[4:]:
            values = column.split(":")
            operands[values[0]] = tuple(map(int, values[1:]))
        results.append((metadata, operands))
    return results


def llvm_rows(executable, cases, features=FEATURES):
    words = [word for _, word, _, _ in cases]
    source = "\n".join(", ".join(f"0x{(word >> (8 * byte)) & 255:02x}"
                                     for byte in range(4)) for word in words) + "\n"
    process = subprocess.run(
        [executable, "--disassemble", "--triple=aarch64", "--mattr=" + features],
        input=source, text=True, capture_output=True, check=True)
    lines = [line.strip().lower() for line in process.stdout.splitlines()
             if line.strip() and line.strip() != ".text"]
    invalid = {int(item) - 1 for item in re.findall(
        r"<stdin>:(\d+):\d+: warning: invalid instruction encoding",
        process.stderr)}
    if len(lines) + len(invalid) != len(words):
        raise RuntimeError("LLVM reference result count mismatch: " +
                           process.stderr[:300])
    iterator = iter(lines)
    return ["<invalid>" if index in invalid else next(iterator)
            for index in range(len(words))]


def check(form, word, response):
    group = form["family_path"][1]
    operation = form["operation_id"]
    metadata, operands = response
    for name, operand in operands.items():
        if operand[10] != 1 or (operand[0] == 0 and not operand[7] & 0x80):
            raise RuntimeError(f"uninterpreted operand {form['id']} {word:08X} {name}")
    kind, link, fall, cond, nzcv, lr, target_reg, compare_width, target, after = metadata
    is_register = group == "branch_reg"
    is_direct = group == "branch_imm"
    expected_kind = (4 if operation == "RET" else 2) if is_register else (
        1 if is_direct else 3)
    expected_link = int(operation in ("BL", "BLR"))
    always = group == "condbranch" and (word & 15) >= 14
    expected_fall = int((not is_direct and not is_register and not always)
                        or expected_link)
    expected_nzcv = int(group == "condbranch" and not always)
    expected_cond = field(word, 0, 4) if group == "condbranch" else (
        field(word, 21, 3) if group.startswith("compbranch_") else 255)
    if group == "compbranch_regs2":
        expected_width = 16 if word & (1 << 14) else 8
    elif group.startswith("compbranch") or group == "testbranch":
        expected_width = 64 if word & (1 << 31) else 32
    else:
        expected_width = 0
    if (kind, link, fall, cond, nzcv, compare_width, after) != (
            expected_kind, expected_link, expected_fall, expected_cond,
            expected_nzcv, expected_width, (PC + 4) & MASK64 if fall else 0):
        raise RuntimeError(f"metadata mismatch {form['id']} {word:08X}: {metadata}")
    if is_register:
        reg = field(word, 5, 5)
        expected_lr = int(operation == "RET" and reg == 30)
        if (target_reg, lr, target) != (reg, expected_lr, 0):
            raise RuntimeError(f"indirect branch mismatch {form['id']} {word:08X}")
        rn = operands["Rn"]
        if (rn[0], rn[1], rn[2], rn[8], rn[9], rn[10]) != (
                1, 2, reg, 64, 2 if reg == 31 else 0, 1):
            raise RuntimeError(f"target register mismatch {form['id']} {word:08X}")
        if operands["Rm"][0] != 0 or not operands["Rm"][7] & 0x80:
            raise RuntimeError("structural Rm exposed as architectural register")
        return 0

    offset_name, lsb, bits = (
        ("imm26", 0, 26) if group == "branch_imm" else
        ("imm14", 5, 14) if group == "testbranch" else
        ("imm9", 5, 9) if group.startswith("compbranch_") else
        ("imm19", 5, 19))
    raw = field(word, lsb, bits)
    displacement = signed(raw, bits) * 4
    address = (PC + displacement) & MASK64
    op = operands[offset_name]
    if (op[0] != 6 or op[1] != 4 or
            op[2] != displacement & MASK64 or op[3] != raw or
            op[4] != displacement or op[5] != address or op[6] != 4 or
            op[7] & 0x11 != 0x11 or target != address or target_reg != 255):
        raise RuntimeError(f"offset mismatch {form['id']} {word:08X}: {op} {metadata}")
    if group in ("compbranch", "compbranch_imm", "compbranch_regs",
                 "compbranch_regs2", "testbranch"):
        rt = operands["Rt"]
        width = 32 if group == "compbranch_regs2" else expected_width
        if (rt[0], rt[1], rt[2], rt[8], rt[9]) != (
                1, 2, word & 31, width, 2 if (word & 31) == 31 else 0):
            raise RuntimeError(f"source register mismatch {form['id']} {word:08X}")
    if group in ("compbranch_regs", "compbranch_regs2"):
        rm = operands["Rm"]
        width = 32 if group == "compbranch_regs2" else expected_width
        if (rm[0], rm[1], rm[2], rm[8], rm[9]) != (
                1, 2, (word >> 16) & 31, width,
                2 if ((word >> 16) & 31) == 31 else 0):
            raise RuntimeError(f"comparison register mismatch {form['id']} {word:08X}")
    if group == "compbranch_imm" and (operands["imm6"][1] != 2 or
                                        operands["imm6"][2] !=
                                        (word >> 15) & 63 or
                                        operands["imm6"][7] & 1):
        raise RuntimeError("comparison immediate confused with branch target")
    if group == "testbranch" and (operands["bit_index"][2] !=
                                   ((word >> 31) << 5 | (word >> 19) & 31)):
        raise RuntimeError("bit index mismatch")
    return displacement


def main(probe):
    with open(CATALOG, encoding="utf-8") as stream:
        catalog = json.load(stream)
    patterns = {pattern["id"]: pattern for pattern in catalog["encodings"]}
    control = [form for form in catalog["instruction_forms"]
               if form["family_path"][0] == "control"]
    conventional_forms = [form for form in control if conventional(form)]
    deferred = [form for form in control if not conventional(form)]
    if (len(control), len(conventional_forms), len(deferred),
            len(catalog["encodings"]), len(catalog["instruction_forms"])) != (
            129, 49, 80, 3099, 4296):
        raise RuntimeError("catalog control inventory changed")
    if len({form["operation_id"] for form in conventional_forms}) != 15:
        raise RuntimeError("conventional operation inventory changed")
    cases = []
    for form in conventional_forms:
        identifier = form["id"]
        group = form["family_path"][1]
        pattern = patterns[form["encoding_id"]]
        seed = INDIRECT[form["operation_id"]] if group == "branch_reg" else int(
            pattern["value"], 16)
        cases.append((identifier, seed, "seed", "-"))
        if group != "branch_reg":
            name, lsb, bits = (
                ("imm26", 0, 26) if group == "branch_imm" else
                ("imm14", 5, 14) if group == "testbranch" else
                ("imm9", 5, 9) if group.startswith("compbranch_") else
                ("imm19", 5, 19))
            for value in (range(1 << bits) if bits == 9 else
                          (0, 1, 2, (1 << (bits - 1)) - 1,
                           1 << (bits - 1), (1 << bits) - 1)):
                mask = ((1 << bits) - 1) << lsb
                cases.append((identifier, (seed & ~mask) | (value << lsb),
                              name, "-"))
        register_lsb = 5 if group == "branch_reg" else 0
        if group == "branch_reg" or group.startswith("compbranch") or group == "testbranch":
            for index in range(32):
                cases.append((identifier, seed & ~(31 << register_lsb) |
                              index << register_lsb, "register", "-"))
        if group in ("compbranch_regs", "compbranch_regs2"):
            for index in range(32):
                cases.append((identifier, seed & ~(31 << 16) |
                              index << 16, "register_rm", "-"))
        if group == "compbranch_imm":
            for immediate in range(64):
                word = seed & ~(63 << 15) | immediate << 15
                if (word & int(pattern["mask"], 16)) == (
                        int(pattern["value"], 16) & int(pattern["mask"], 16)):
                    cases.append((identifier, word, "compare_immediate", "-"))
        if group == "testbranch":
            for index in range(64):
                cases.append((identifier,
                              seed & ~((1 << 31) | (31 << 19)) |
                              (index >> 5) << 31 | (index & 31) << 19,
                              "bit_index", "-"))
        if group == "condbranch":
            for code in range(16):
                cases.append((identifier, (seed & ~15) | code, "condition", "-"))
        for index in range(32):
            if int(pattern["mask"], 16) & (1 << index):
                cases.append((identifier, seed ^ (1 << index),
                              "fixed-bit-negative", "-"))
        for feature in form.get("feature_identifiers", []):
            cases.append((identifier, seed, "feature-negative", feature))
    cases = list(dict.fromkeys(cases))
    responses = probe_rows(probe, cases)
    forms = {form["id"]: form for form in control}
    reference_cases = []
    for case, result in zip(cases, responses):
        fid, word, variant, _ = case
        if result is None or variant in ("fixed-bit-negative", "feature-negative"):
            continue
        if variant == "imm9" and field(word, 5, 9) not in (0, 1, 255, 256, 511):
            continue
        if variant in ("register", "register_rm"):
            lsb = (5 if forms[fid]["family_path"][1] == "branch_reg"
                   else 16 if variant == "register_rm" else 0)
            if field(word, lsb, 5) not in (0, 1, 30, 31):
                continue
        if variant == "compare_immediate" and field(word, 15, 6) not in (
                0, 1, 30, 31, 62, 63):
            continue
        reference_cases.append(case)
    exe = os.environ.get("LLVM_MC_CONTROL", shutil.which("llvm-mc-21") or
                         llvm_toolchain.executable())
    _, version = llvm_toolchain.require_qualified(exe)
    if version != "21.1.8":
        raise RuntimeError("FEAT_CMPBR requires qualified LLVM MC 21.1.8")
    llvm = llvm_rows(exe, reference_cases)
    reference = {}
    for case, text in zip(reference_cases, llvm):
        fid, word, _, _ = case
        if text == "<invalid>":
            raise RuntimeError(f"LLVM rejects accepted {fid} {word:08X}")
        group = forms[fid]["family_path"][1]
        name = text.split()[0]
        if group == "condbranch":
            if not name.startswith("b.") and not name.startswith("bc."):
                raise RuntimeError(f"LLVM conditional mnemonic mismatch: {text}")
        elif name != forms[fid]["mnemonic"].lower():
            raise RuntimeError(f"LLVM mnemonic mismatch {fid}: {text}")
        if group != "branch_reg":
            expected = check(forms[fid], word,
                             responses[cases.index(case)])
            literals = re.findall(r"#(-?(?:0x[0-9a-f]+|[0-9]+))", text)
            if not literals or int(literals[-1], 0) != expected:
                raise RuntimeError(f"LLVM displacement mismatch {fid}: {text} {expected}")
        reference[(fid, word)] = "llvm-verified"
    output = io.StringIO(newline="")
    writer = csv.writer(output)
    writer.writerow(("class", "form", "operation", "group", "feature",
                     "variant", "encoding", "result", "target", "offset",
                     "kind", "link", "fallthrough", "nzcv", "lr", "width",
                     "condition", "operands", "llvm"))
    counts = collections.Counter()
    positive = set()
    for (fid, word, variant, disabled), result in zip(cases, responses):
        form = forms[fid]
        group = form["family_path"][1]
        expected_reject = variant in ("fixed-bit-negative", "feature-negative")
        if expected_reject and result is not None:
            raise RuntimeError(f"undue acceptance {fid} {word:08X} {variant}")
        if not expected_reject and result is None:
            raise RuntimeError(f"undue rejection {fid} {word:08X} {variant}")
        offset = check(form, word, result) if result is not None else 0
        metadata = result[0] if result is not None else (0,) * 10
        operand_coverage = (" ".join(
            f"{name}:k{op[0]}:r{op[1]}:w{op[8]}:s{op[9]}:f{op[7]}"
            for name, op in sorted(result[1].items()))
            if variant == "seed" and result is not None else "-")
        state = "rejected" if expected_reject else "complete"
        if variant == "seed" and not expected_reject:
            positive.add(fid)
        counts[(variant, state)] += 1
        writer.writerow(("conventional", fid, form["operation_id"], group,
                         disabled if disabled != "-" else
                         ",".join(form.get("feature_identifiers", [])) or "-",
                         variant, f"{word:08X}", state,
                         f"{metadata[8]:016X}", offset,
                         metadata[0], metadata[1], metadata[2], metadata[4],
                         metadata[5], metadata[7], metadata[3], operand_coverage,
                         reference.get((fid, word), "-")))
    for form in deferred:
        writer.writerow(("deferred", form["id"], form["operation_id"],
                         form["family_path"][1],
                         ",".join(form.get("feature_identifiers", [])) or "-",
                         "-", "-", "deferred", "-", "-", "-", "-", "-",
                         "-", "-", "-", "-", "-", "-"))
    if len(positive) != 49:
        raise RuntimeError(f"COMPLETE witnesses missing: {49 - len(positive)}")
    lengths = [len(row) for row in csv.reader(io.StringIO(output.getvalue()))]
    if len(set(lengths)) != 1 or lengths[0] != 19:
        raise RuntimeError("control-flow CSV columns misaligned")
    data = output.getvalue().encode("utf-8")
    digest = hashlib.sha256(data).hexdigest()
    print("Control-flow validation", len(control), "catalog forms", len(positive),
          "COMPLETE", len(deferred), "deferred", len(cases), "cases",
          len(reference_cases), "LLVM comparisons", dict(counts), "sha256", digest)
    if REPORT_SHA256 != "PENDING" and digest != REPORT_SHA256:
        raise RuntimeError(f"control-flow report hash changed: {digest}")
    if os.environ.get("ARMITO_WRITE_REPORT") == "1":
        with open(REPORT, "wb") as stream:
            stream.write(data)
    else:
        with open(REPORT, "rb") as stream:
            if stream.read() != data:
                raise RuntimeError("control-flow report mismatch")


if __name__ == "__main__":
    try:
        if len(sys.argv) != 2:
            raise ValueError("usage: control_flow_validation.py PROBE")
        main(sys.argv[1])
    except (RuntimeError, OSError, ValueError, KeyError, IndexError,
            subprocess.CalledProcessError) as error:
        print("Control-flow audit failed:", error, file=sys.stderr)
        raise SystemExit(1)
