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
import llvm_toolchain

REPORT = "Tests/Decoder/AARCH64/authenticated_control_flow_validation.csv"
REPORT_SHA256 = "acebf7ec98c12d14000310c15cc65ef86dd5ba4b2dfed38b3e9541f07ca6e982"
FEATURES = "+pauth,+pauth-lr,+tme,+el3"
PC = branch.PC
SEEDS = {
    "BLRAAZ": 0xD63F081F, "BLRAA": 0xD73F0801,
    "BLRABZ": 0xD63F0C1F, "BLRAB": 0xD73F0C01,
    "BRAAZ": 0xD61F081F, "BRAA": 0xD71F0801,
    "BRABZ": 0xD61F0C1F, "BRAB": 0xD71F0C01,
    "ERETAA": 0xD69F0BFF, "ERETAB": 0xD69F0FFF,
    "ERET": 0xD69F03E0, "DRPS": 0xD6BF03E0,
    "RETAASPPCR": 0xD65F0BE0, "RETABSPPCR": 0xD65F0FE0,
    "RETAASPPC": 0x5500001F, "RETABSPPC": 0x5520001F,
    "RETAA": 0xD65F0BFF, "RETAB": 0xD65F0FFF,
}
NAMES = tuple(sorted(SEEDS, key=len, reverse=True))
ZERO_MODIFIER = {"BRAAZ", "BRABZ", "BLRAAZ", "BLRABZ"}
REGISTER_MODIFIER = {"BRAA", "BRAB", "BLRAA", "BLRAB",
                     "RETAASPPCR", "RETABSPPCR"}


def selected(form):
    group = form["family_path"][1]
    return group in ("exception", "miscbranch") or (
        group == "branch_reg" and
        form["operation_id"] not in ("BR", "BLR", "RET"))


def seed(form, patterns):
    for name in NAMES:
        if form["id"].startswith(name + "_"):
            return SEEDS[name]
    return int(patterns[form["encoding_id"]]["value"], 16)


def probe_rows(probe, cases):
    source = "".join(f"{fid} {word:08X} {PC:016X} {disabled}\n"
                     for fid, word, _, disabled in cases)
    process = subprocess.run([probe, "--auth"], input=source, text=True,
                             capture_output=True, check=True)
    lines = process.stdout.splitlines()
    if len(lines) != len(cases):
        raise RuntimeError(f"probe returned {len(lines)}/{len(cases)} cases")
    result = []
    for (fid, word, _, _), line in zip(cases, lines):
        parts = line.split("\t")
        if parts[:2] != [fid, f"{word:08X}"]:
            raise RuntimeError(f"probe response out of order: {line[:150]}")
        if parts[2] == "REJECT":
            result.append(None)
            continue
        if parts[2] != "COMPLETE" or not parts[4].startswith("AUTH:"):
            raise RuntimeError(f"unexpected probe response: {line[:150]}")
        base = tuple(map(int, parts[3].split(":")))
        auth = tuple(map(int, parts[4][5:].split(":")))
        if len(base) != 10 or len(auth) != 20:
            raise RuntimeError("runtime metadata layout changed")
        operands = {}
        for item in parts[5:]:
            data = item.split(":")
            operands[data[0]] = tuple(map(int, data[1:]))
        result.append((base, auth, operands))
    return result


def expected(form, word):
    name = form["id"].split("_")[0]
    operation = form["operation_id"]
    group = form["family_path"][1]
    key = 1 if name in ("BRAA", "BRAAZ", "BLRAA", "BLRAAZ", "RETAA",
                        "RETAASPPCR", "RETAASPPC", "ERETAA") else (
        2 if name in ("BRAB", "BRABZ", "BLRAB", "BLRABZ", "RETAB",
                      "RETABSPPCR", "RETABSPPC", "ERETAB") else 0)
    is_call = operation == "BLRA"
    is_branch = operation in ("BRA", "BLRA")
    is_return = operation in ("RETA", "RETASPPCR_reg", "RETASPPC_imm")
    is_eret = operation in ("ERET", "ERETA")
    is_drps = operation == "DRPS"
    is_dcps = name.startswith("DCPS")
    is_cancel = operation == "TCANCEL"
    kind = (5 if is_branch else 6 if is_return else
            8 if is_eret or is_drps else 10 if is_cancel else
            9 if is_dcps else 7)
    target_reg = (branch.field(word, 5, 5) if is_branch else
                  30 if is_return else 255)
    link = int(is_call)
    base = (kind, link, link, 255, 0, int(is_return), target_reg,
            0, 0, (PC + 4) & branch.MASK64 if link else 0)
    context = (2 if name in ZERO_MODIFIER else
               3 if is_return or operation == "ERETA" else
               1 if is_branch else 0)
    action = (5 if is_call else 4 if is_branch else
              6 if is_return or operation == "ERETA" else 0)
    modifier = branch.field(word, 0, 5) if name in REGISTER_MODIFIER else 255
    exception_kind = {"SVC": 1, "HVC": 2, "SMC": 3, "BRK": 4, "HLT": 5,
                      "DCPS1": 6, "DCPS2": 6, "DCPS3": 6,
                      "TCANCEL": 7, "ERET": 8, "ERETAA": 8,
                      "ERETAB": 8, "DRPS": 9}.get(name, 0)
    entry = kind == 7
    auth = (key, action, context, modifier,
            int(is_call or entry or operation in ("RETASPPCR_reg", "RETASPPC_imm")),
            int(is_return or operation == "ERETA"),
            int(entry or is_eret or is_drps or is_dcps),
            int(is_eret), int(is_eret), int(is_drps), int(is_drps),
            int(entry or is_eret or is_drps or is_dcps),
            int(entry), int(entry), exception_kind,
            int(name in ("HVC", "SMC") or is_eret),
            int(name[4]) if is_dcps else 0,
            branch.field(word, 5, 16) if group in ("exception", "miscbranch") else 0,
            int(name == "HVC"), int(name in ("SMC", "DCPS3")))
    return base, auth


def validate(form, word, result):
    base, auth, operands = result
    if (base, auth) != expected(form, word):
        raise RuntimeError(f"metadata mismatch {form['id']} {word:08X}: "
                           f"{base} {auth} != {expected(form, word)}")
    name = form["id"].split("_")[0]
    group = form["family_path"][1]
    for field_name, op in operands.items():
        if op[10] != 1 or (op[0] == 0 and not op[7] & 0x80):
            raise RuntimeError(f"uninterpreted operand {form['id']} "
                               f"{word:08X} {field_name}")
    if group == "branch_reg":
        if "Rn" in operands:
            rn = operands["Rn"]
            target = form["operation_id"] in ("BRA", "BLRA")
            if (rn[0], rn[1], rn[8]) != (1 if target else 0,
                                        2 if target else 0, 64):
                raise RuntimeError(f"incorrect Rn classification {form['id']}")
            if target and (rn[2], rn[9]) != (
                    branch.field(word, 5, 5),
                    2 if branch.field(word, 5, 5) == 31 else 0):
                raise RuntimeError(f"incorrect target register {form['id']}")
        if "Rm" in operands:
            rm = operands["Rm"]
            source = name in REGISTER_MODIFIER
            if (rm[0], rm[1], rm[8]) != (1 if source else 0,
                                        2 if source else 0, 64):
                raise RuntimeError(f"incorrect modifier register {form['id']}")
            if source and (rm[2], rm[9]) != (
                    word & 31, 2 if (word & 31) == 31 else 0):
                raise RuntimeError(f"incorrect modifier register value {form['id']}")
    if "imm16" in operands:
        immediate = operands["imm16"]
        if (immediate[0], immediate[1], immediate[2], immediate[3]) != (
                6, 5 if group == "exception" else 2,
                branch.field(word, 5, 16), branch.field(word, 5, 16)):
            raise RuntimeError(f"exception immediate mismatch {form['id']}")


def main(probe):
    with open(branch.CATALOG, encoding="utf-8") as stream:
        catalog = json.load(stream)
    patterns = {pattern["id"]: pattern for pattern in catalog["encodings"]}
    control = [form for form in catalog["instruction_forms"]
               if form["family_path"][0] == "control"]
    forms = [form for form in control if selected(form)]
    others = [form for form in control if form["family_path"][1] in (
        "hints", "pstate", "barriers", "systemmovepr", "systemmove",
        "systeminstrs", "syspairinstrs", "systemresult",
        "systeminstrswithreg")]
    if (len(forms), len(others), len(control), len(catalog["encodings"]),
            len(catalog["instruction_forms"])) != (27, 53, 129, 3099, 4296):
        raise RuntimeError("authenticated/exception inventory changed")
    by_id = {form["id"]: form for form in forms}
    cases = []
    for form in forms:
        fid = form["id"]
        group = form["family_path"][1]
        word = seed(form, patterns)
        cases.append((fid, word, "seed", "-"))
        if form["operation_id"] in ("BRA", "BLRA"):
            for index in range(32):
                cases.append((fid, word & ~(31 << 5) | index << 5,
                              "target", "-"))
        if fid.split("_")[0] in REGISTER_MODIFIER:
            for index in range(32):
                cases.append((fid, word & ~31 | index, "modifier", "-"))
        if group in ("exception", "miscbranch"):
            for immediate in (0, 1, 2, 0x7fff, 0x8000, 0xffff):
                cases.append((fid, word & ~(0xffff << 5) | immediate << 5,
                              "imm16", "-"))
        mask = int(patterns[form["encoding_id"]]["mask"], 16)
        for index in range(32):
            if mask & (1 << index):
                cases.append((fid, word ^ (1 << index), "fixed-bit-negative", "-"))
        for feature in form.get("feature_identifiers", []):
            cases.append((fid, word, "feature-negative", feature))
    cases = list(dict.fromkeys(cases))
    results = probe_rows(probe, cases)
    exe = os.environ.get("LLVM_MC_CONTROL", shutil.which("llvm-mc-21") or
                         llvm_toolchain.executable())
    _, version = llvm_toolchain.require_qualified(exe)
    if version != "21.1.8":
        raise RuntimeError("PAuth-LR and EL3 require qualified LLVM MC 21.1.8")
    reference_cases = [case for case in cases if case[2] in (
        "seed", "target", "modifier", "imm16")]
    reference_text = branch.llvm_rows(exe, reference_cases, FEATURES)
    reference = {case: text for case, text in zip(reference_cases, reference_text)}
    el3_cases = [case for case in reference_cases if case[2] == "seed" and
                 case[0] in ("SMC_EX_exception", "DCPS3_DC_exception")]
    if branch.llvm_rows(exe, el3_cases, "+pauth,+pauth-lr,+tme") != [
            "<invalid>"] * len(el3_cases):
        raise RuntimeError("EL3-dependent reference encodings not gated")
    output = io.StringIO(newline="")
    writer = csv.writer(output)
    writer.writerow(("class", "form", "operation", "group", "feature",
                     "variant", "word", "result", "flow", "key", "action",
                     "context", "target_register", "modifier_register",
                     "pc", "sp", "lr", "pstate", "elr", "spsr", "debug",
                     "exception_kind", "required_el", "level_selector",
                     "immediate", "requires_el2", "requires_el3",
                     "operands", "llvm"))
    counts = collections.Counter()
    witnesses = set()
    llvm_matches = 0
    for case, result in zip(cases, results):
        fid, word, variant, disabled = case
        form = by_id[fid]
        name = fid.split("_")[0]
        text = reference.get(case, "-")
        if variant in ("fixed-bit-negative", "feature-negative"):
            if result is not None:
                raise RuntimeError(f"undue form acceptance {fid} {word:08X}")
            state = "rejected-by-form"
        elif text == "<invalid>":
            if result is not None:
                raise RuntimeError(f"LLVM invalid accepted {fid} {word:08X}")
            state = "llvm-invalid"
        elif result is None and text != "-" and (
                text.split()[0] != form["mnemonic"].lower()):
            state = "sibling-form"
        else:
            if result is None:
                raise RuntimeError(f"valid form rejected {fid} {word:08X}: {text}")
            if text == "-" or text.split()[0] != form["mnemonic"].lower():
                raise RuntimeError(f"LLVM mnemonic mismatch {fid}: {text}")
            validate(form, word, result)
            llvm_matches += 1
            state = "complete"
            if variant == "seed":
                witnesses.add(fid)
        counts[(variant, state)] += 1
        base, auth, operands = result if result is not None else (
            (0,) * 10, (0,) * 20, {})
        summary = (" ".join(f"{operand}:k{op[0]}:r{op[1]}:w{op[8]}:f{op[7]}"
                            for operand, op in sorted(operands.items()))
                   if variant == "seed" else "-")
        writer.writerow(("authenticated-or-exception", fid,
                         form["operation_id"], form["family_path"][1],
                         disabled if disabled != "-" else
                         ",".join(form.get("feature_identifiers", [])) or "-",
                         variant, f"{word:08X}", state, base[0], auth[0],
                         auth[1], auth[2], base[6], auth[3], auth[4], auth[5],
                         base[5], auth[6], auth[7], auth[8], auth[9], auth[14],
                         auth[15], auth[16], auth[17], auth[18], auth[19], summary,
                         "llvm-verified" if state == "complete" else text))
    for form in others:
        writer.writerow(("future-nonbranch-control", form["id"],
                         form["operation_id"], form["family_path"][1],
                         ",".join(form.get("feature_identifiers", [])) or "-",
                         *(["-"] * 24)))
    if len(witnesses) != 27:
        raise RuntimeError(f"COMPLETE witnesses missing: {27 - len(witnesses)}")
    widths = {len(row) for row in csv.reader(io.StringIO(output.getvalue()))}
    if widths != {29}:
        raise RuntimeError(f"CSV columns misaligned: {widths}")
    data = output.getvalue().encode("utf-8")
    digest = hashlib.sha256(data).hexdigest()
    print("Authenticated/exception validation", len(witnesses), "COMPLETE",
          len(others), "future", len(cases), "cases", llvm_matches,
          "LLVM-verified", dict(counts), "sha256", digest)
    if REPORT_SHA256 != "PENDING" and digest != REPORT_SHA256:
        raise RuntimeError(f"authenticated report hash changed: {digest}")
    if os.environ.get("ARMITO_WRITE_REPORT") == "1":
        with open(REPORT, "wb") as stream:
            stream.write(data)
    else:
        with open(REPORT, "rb") as stream:
            if stream.read() != data:
                raise RuntimeError("authenticated report mismatch")


if __name__ == "__main__":
    try:
        if len(sys.argv) != 2:
            raise ValueError("usage: authenticated_control_flow_validation.py PROBE")
        main(sys.argv[1])
    except (RuntimeError, OSError, ValueError, KeyError, IndexError,
            subprocess.CalledProcessError) as error:
        print("Authenticated control audit failed:", error, file=sys.stderr)
        raise SystemExit(1)
