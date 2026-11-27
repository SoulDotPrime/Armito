import collections
import csv
import hashlib
import io
import json
import os
import shutil
import subprocess
import sys

import control_flow_validation as branch
import integer_form_differential as differential
import llvm_toolchain

CATALOG = "Decoder/AARCH64/Catalog/AARCH64InstructionCatalog.json"
REPORT = "Tests/Decoder/AARCH64/memory_operand_validation.csv"
REPORT_SHA256 = "e7893e5f0c2b6443f1df6d978ad7d31136d9a34dfadca708a4ec3fcccfa2de41"
PINNED = {
    "integer_register_baseline.csv": "739a129672c95570dddc8b256f2dc9bf9dae2957b470fa8c531130e3f2534015",
    "integer_immediate_all_candidates.csv": "ed292fcffc46970c987fddf22e2f73d7a7d7dd75831d5bb187f072b6b5e12fb9",
    "integer_immediate_rejections.csv": "d6545bbcd94cc1fbdb27958e96640f6c83ec5d75a24b136fcb8f4595d1403c20",
    "integer_register_rejections.csv": "3af651c064766b3d41778432e19134ab96e62d8e6e74decc0ccffd2f1bd94b32",
    "bitmask_exhaustive.csv": "ffbd31d7d7a6ad071602fc44b7b8fc776e0dc33cf62eaed80374f595ab4d8a3c",
    "integer_condition_audit.csv": "49b7cf96af57e6172f95bac7b25895d5013c4b5e7eca594a0bb5119f0105f8fe",
    "integer_alias_predicates.csv": "e9f412f73d15c3595bb844da9dd9519da76f7fc8cdc92a057763b12cfd0babeb",
    "integer_alias_coverage.csv": "05e1d7d7df3dde2c8aa1542d5a46a9e93ae0b62b9cee2b94a73b63d4bd0e5433",
    "integer_implicit_operands.csv": "6df6ade821c168b50a1f3846743be99700c66b0889375dca05ee86ac7e4734c0",
    "integer_invalid_encodings.csv": "550ba37f5e60486ee7ad1735403bc45d9d95c83f7d1353c6f923fcbb367e1d8f",
    "integer_comprehensive_validation.csv": "3f24c847e36c8df3cfa51db23146c11bcb903dbb6414afccedb21e06fcec2b49",
    "control_flow_validation.csv": "edddbdeab51b3491df3189d94445dbd19254a8e8e9443765adf0a1a61db4a4ff",
    "authenticated_control_flow_validation.csv": "acebf7ec98c12d14000310c15cc65ef86dd5ba4b2dfed38b3e9541f07ca6e982",
    "control_flow_comprehensive_validation.csv": "5a96fa23d0351af1660a1ffe46f2c0b0959368296882da5286503568c72dd62e",
}
CONVENTIONAL = frozenset((
    "ldst_pos", "ldst_regoff", "ldst_immpost", "ldst_immpre", "ldst_unscaled",
    "loadlit", "ldstpair_off", "ldstpair_post", "ldstpair_pre",
    "ldstnapair_offs"))
PC = 0x1000000000004000
MASK64 = (1 << 64) - 1
FEATURES = "+cmpbr,+hbc,+pauth,+pauth-lr,+tme,+el3"


def verify_pinned():
    root = "Tests/Decoder/AARCH64/"
    for name, expected in PINNED.items():
        digest = hashlib.sha256()
        with open(root + name, "rb") as stream:
            for chunk in iter(lambda: stream.read(65536), b""):
                digest.update(chunk)
        if digest.hexdigest() != expected:
            raise RuntimeError(f"historical report changed: {name}")


def field(word, lsb, width):
    return (word >> lsb) & ((1 << width) - 1)


def sxt(value, bits):
    return value - (1 << bits) if value & (1 << (bits - 1)) else value


def probe_rows(probe, cases):
    source = "".join(f"{fid} {word:08X} {PC:016X} {disabled}\n"
                     for fid, word, _, disabled in cases)
    process = subprocess.run([probe, "--mem"], input=source, text=True,
                             capture_output=True, check=True)
    lines = process.stdout.splitlines()
    if len(lines) != len(cases):
        raise RuntimeError(f"probe returned {len(lines)}/{len(cases)}")
    results = []
    for (fid, word, _, _), line in zip(cases, lines):
        parts = line.split("\t")
        if parts[:2] != [fid, f"{word:08X}"]:
            raise RuntimeError(f"probe misalign: {line[:120]}")
        if parts[2] == "REJECT":
            results.append(None)
            continue
        if parts[2] != "COMPLETE" or not parts[3].startswith("MEM:"):
            raise RuntimeError(f"unexpected probe: {line[:120]}")
        mem = tuple(map(int, parts[3][4:].split(":")))
        if len(mem) < 15:
            raise RuntimeError("memory metadata layout changed")
        operands = {}
        for item in parts[4:]:
            data = item.split(":")
            if data[0] == "MX":
                continue
            operands[data[0]] = tuple(map(int, data[1:]))
        results.append((mem, operands))
    return results


def validate(form, word, result):
    mem, operands = result
    (access, count, direction, writeback, base, data, pair, sxt_flag,
     zxt_flag, literal, base_sp, base_zero, data_sp, data_zero, offset) = mem
    group = form["family_path"][1]
    name = form["id"]
    is_store = name.startswith(("STR", "STUR", "STP", "STNP"))
    is_prefetch = name.startswith("PRFM")
    exp_dir = 2 if is_store else 3 if is_prefetch else 1
    if direction != exp_dir:
        raise RuntimeError(f"direction mismatch {name} {word:08X}")
    is_pair = group in ("ldstpair_off", "ldstpair_post", "ldstpair_pre",
                        "ldstnapair_offs")
    exp_count = 2 if is_pair else 1
    if count != exp_count:
        raise RuntimeError(f"element count mismatch {name}")
    if group == "loadlit":
        if not literal or base != 255:
            raise RuntimeError(f"literal metadata mismatch {name}")
        imm19 = field(word, 5, 19)
        exp = (PC + sxt(imm19, 19) * 4) & MASK64
        if offset != sxt(imm19, 19) * 4 & MASK64:
            raise RuntimeError(f"literal offset mismatch {name}")
    else:
        if literal:
            raise RuntimeError(f"unexpected literal {name}")
        if base != field(word, 5, 5):
            raise RuntimeError(f"base register mismatch {name}")
        if base_sp != (1 if field(word, 5, 5) == 31 else 0):
            raise RuntimeError(f"base SP flag mismatch {name}")
        if group == "ldst_pos":
            imm12 = field(word, 10, 12)
            exp_off = imm12 * (access // 8)
            if offset != exp_off:
                raise RuntimeError(f"imm12 offset mismatch {name} {word:08X}")
        elif group in ("ldst_unscaled", "ldst_immpost", "ldst_immpre"):
            imm9 = field(word, 12, 9)
            if offset != sxt(imm9, 9) & MASK64:
                raise RuntimeError(f"imm9 offset mismatch {name}")
        elif is_pair:
            imm7 = field(word, 15, 7)
            if offset != (sxt(imm7, 7) * (access // 8)) & MASK64:
                raise RuntimeError(f"imm7 offset mismatch {name}")
    exp_wb = 1 if group == "ldst_immpre" else 2 if group == "ldst_immpost" else 0
    if writeback != exp_wb:
        raise RuntimeError(f"writeback mismatch {name}")
    if data != field(word, 0, 5):
        raise RuntimeError(f"data register mismatch {name}")
    if data_sp or (data_zero != (1 if field(word, 0, 5) == 31 else 0)):
        raise RuntimeError(f"data ZR flag mismatch {name}")
    rt = operands["Rt"]
    exp_rt_w = 64 if (name.startswith(("LDR_64", "STR_64", "LDRSW", "LDPSW",
                                        "LDUR_64", "STUR_64", "LDP_64", "STP_64",
                                        "LDNP_64", "STNP_64"))
                      or (name.startswith(("LDRSB", "LDRSH")) and
                          field(word, 30, 2) == 1)) else 32
    if rt[9] != exp_rt_w:
        raise RuntimeError(f"Rt width mismatch {name} {word:08X}: {rt[9]}!={exp_rt_w}")
    if is_pair and operands["Rt2"][9] != exp_rt_w:
        raise RuntimeError(f"Rt2 width mismatch {name}")
    if "Rn" in operands and operands["Rn"][9] != 64:
        raise RuntimeError(f"Rn width mismatch {name}")
    for op_name, op in operands.items():
        if op[2] != 1 or (op[0] == 0 and not op[8] & 0x80):
            raise RuntimeError(f"uninterpreted operand {name} {op_name}")
    return access


def main(probe):
    verify_pinned()
    with open(CATALOG, encoding="utf-8") as stream:
        catalog = json.load(stream)
    patterns = {p["id"]: p for p in catalog["encodings"]}
    groups = {g["id"]: g for g in catalog["groups"]}
    sets = {s["id"]: s for s in catalog["instruction_sets"]}
    ldst = [f for f in catalog["instruction_forms"]
            if f["family_path"][0] == "ldst"]
    if len(ldst) != 996 or len(catalog["encodings"]) != 3099 or \
            len(catalog["instruction_forms"]) != 4296:
        raise RuntimeError("catalog baseline changed")
    conv = []
    spec = []
    for form in ldst:
        fid = form["id"]
        is_gpr = "32" in fid or "64" in fid
        feature_gated = bool(form.get("feature_identifiers"))
        if (form["family_path"][1] in CONVENTIONAL and is_gpr and
                not feature_gated):
            conv.append(form)
        else:
            spec.append(form)
    if len(conv) != 91 or len(spec) != 905:
        raise RuntimeError(f"split changed: {len(conv)} conv {len(spec)} spec")
    baseline = []
    for form in conv:
        pattern = patterns[form["encoding_id"]]
        baseline.append((form["id"], int(pattern["value"], 16)))
        fields = differential.bit_fields(form, groups, sets)
        for word in differential.sample_words(pattern, fields):
            baseline.append((form["id"], word))
    seed_rows = differential.probe_rows(
        "Build/Host/aarch64integerformprobe", baseline)
    complete = collections.defaultdict(list)
    for (fid, word), result in zip(baseline, seed_rows):
        if result is not None:
            complete[fid].append(word)
    if len(complete) != 91:
        raise RuntimeError(f"missing COMPLETE candidates: {91 - len(complete)}")
    exe = os.environ.get("LLVM_MC_CONTROL", shutil.which("llvm-mc-21") or
                         llvm_toolchain.executable())
    _, version = llvm_toolchain.require_qualified(exe)
    if version != "21.1.8":
        raise RuntimeError("qualified LLVM MC 21.1.8 required")
    seeds = {}
    for form in conv:
        fid = form["id"]
        words = complete[fid]
        texts = branch.llvm_rows(exe, [(fid, w, "seed", "-") for w in words],
                                 FEATURES)
        for word, text in zip(words, texts):
            if text != "<invalid>" and text.split()[0] == form["mnemonic"].lower():
                seeds[fid] = word
                break
        if fid not in seeds:
            raise RuntimeError(f"no LLVM-valid seed for {fid}")
    cases = []
    for form in conv:
        fid = form["id"]
        word = seeds[fid]
        pattern = patterns[form["encoding_id"]]
        mask = int(pattern["mask"], 16)
        cases.append((fid, word, "seed", "-"))
        for index in range(32):
            if mask & (1 << index):
                cases.append((fid, word ^ (1 << index), "fixed-bit-negative", "-"))
        for feature in form.get("feature_identifiers", []):
            cases.append((fid, word, "feature-negative", feature))
        cases.append((fid, word, "boundary-zero", "-"))
    cases = list(dict.fromkeys(cases))
    results = probe_rows(probe, cases)
    exe = os.environ.get("LLVM_MC_CONTROL", shutil.which("llvm-mc-21") or
                         llvm_toolchain.executable())
    _, version = llvm_toolchain.require_qualified(exe)
    if version != "21.1.8":
        raise RuntimeError("qualified LLVM MC 21.1.8 required")
    ref_cases = [c for c in cases if c[2] in ("seed", "boundary-zero")]
    ref_text = branch.llvm_rows(exe, ref_cases, FEATURES)
    reference = {c: t for c, t in zip(ref_cases, ref_text)}
    if len(ref_cases) != len(ref_text):
        raise RuntimeError(f"llvm ref count {len(ref_cases)}!={len(ref_text)}")
    output = io.StringIO(newline="")
    writer = csv.writer(output)
    writer.writerow(("class", "form", "operation", "group", "feature", "variant",
                     "word", "result", "access_width", "elements", "direction",
                     "writeback", "base", "data", "pair", "sign", "zero",
                     "literal", "base_sp", "data_zero", "offset", "llvm"))
    counts = collections.Counter()
    witnesses = set()
    access_by_form = {}
    for case, result in zip(cases, results):
        fid, word, variant, disabled = case
        form = next(f for f in conv if f["id"] == fid)
        text = reference.get(case, "-")
        if variant in ("fixed-bit-negative", "feature-negative"):
            if result is not None:
                raise RuntimeError(f"undue accept {fid} {word:08X} {variant}")
            state = "rejected-by-form"
        else:
            if result is None:
                raise RuntimeError(f"valid rejected {fid} {word:08X}: {text}")
            if text == "-" or text.split()[0] != form["mnemonic"].lower():
                raise RuntimeError(f"LLVM mismatch {fid}: {text}")
            access_by_form[fid] = validate(form, word, result)
            state = "complete"
            if variant == "seed":
                witnesses.add(fid)
        counts[(variant, state)] += 1
        mem = result[0] if result is not None else (0,) * 15
        writer.writerow(("conventional", fid, form["operation_id"],
                         form["family_path"][1],
                         disabled if disabled != "-" else
                         ",".join(form.get("feature_identifiers", [])) or "-",
                         variant, f"{word:08X}", state, mem[0], mem[1], mem[2],
                         mem[3], mem[4], mem[5], mem[6], mem[7], mem[8], mem[9],
                         mem[10], mem[13], mem[14],
                         "llvm-verified" if state == "complete" else text))
    for form in spec:
        writer.writerow(("specialized-3.4.2", form["id"], form["operation_id"],
                         form["family_path"][1],
                         ",".join(form.get("feature_identifiers", [])) or "-",
                         *(["-"] * 17)))
    if len(witnesses) != 91:
        raise RuntimeError(f"COMPLETE witnesses missing: {91 - len(witnesses)}")
    lengths = {len(row) for row in csv.reader(io.StringIO(output.getvalue()))}
    if lengths != {22}:
        raise RuntimeError(f"CSV columns misaligned: {lengths}")
    data = output.getvalue().encode("utf-8")
    digest = hashlib.sha256(data).hexdigest()
    print("Memory validation", len(witnesses), "COMPLETE", len(spec),
          "specialized", len(cases), "cases", dict(counts), "sha256", digest)
    if REPORT_SHA256 != "PENDING" and digest != REPORT_SHA256:
        raise RuntimeError(f"memory report hash changed: {digest}")
    if os.environ.get("ARMITO_WRITE_REPORT") == "1":
        with open(REPORT, "wb") as stream:
            stream.write(data)
    else:
        with open(REPORT, "rb") as stream:
            if stream.read() != data:
                raise RuntimeError("memory report mismatch")


if __name__ == "__main__":
    try:
        if len(sys.argv) != 2:
            raise ValueError("usage: memory_operand_validation.py PROBE")
        main(sys.argv[1])
    except (RuntimeError, OSError, ValueError, KeyError, IndexError,
            subprocess.CalledProcessError) as error:
        print("Memory validation failed:", error, file=sys.stderr)
        raise SystemExit(1)