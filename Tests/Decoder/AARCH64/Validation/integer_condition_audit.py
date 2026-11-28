import collections
import csv
import hashlib
import io
import json
import re
import sys

import integer_form_differential as differential


REPORT = "Tests/Decoder/AARCH64/Results/integer_condition_audit.csv"
REPORT_SHA256 = "49b7cf96af57e6172f95bac7b25895d5013c4b5e7eca594a0bb5119f0105f8fe"
NAMES = ("eq", "ne", "hs", "lo", "mi", "pl", "vs", "vc",
         "hi", "ls", "ge", "lt", "gt", "le", "al", "nv")


def holds(cond, nzcv):
    n, z, c, v = ((nzcv >> shift) & 1 for shift in (3, 2, 1, 0))
    base = (bool(z), bool(c), bool(n), bool(v), bool(c and not z),
            n == v, n == v and not z, True)[cond >> 1]
    return bool(base) ^ bool((cond & 1) and cond != 15)


def alias_for(mnemonic, rn, rm, cond):
    if cond >= 14:
        return "-"
    if mnemonic == "CSINC" and rn == rm:
        return "CSET" if rn == 31 else "CINC"
    if mnemonic == "CSINV" and rn == rm:
        return "CSETM" if rn == 31 else "CINV"
    if mnemonic == "CSNEG" and rn == rm:
        return "CNEG"
    return "-"


def main(probe):
    with open(differential.CATALOG, encoding="utf-8") as stream:
        catalog = json.load(stream)
    patterns = {p["id"]: p for p in catalog["encodings"]}
    forms = [f for f in catalog["instruction_forms"]
             if f["family_path"][0] in ("dpimm", "dpreg")]
    selected = [f for f in forms if f["family_path"][1] in
                ("condsel", "condcmp_imm", "condcmp_reg")]
    if len(forms) != 195 or len(selected) != 16:
        raise RuntimeError("integer condition inventory changed")
    cases = []
    for form in selected:
        pattern = patterns[form["encoding_id"]]
        seed = int(pattern["value"], 16)
        if form["family_path"][1] == "condsel":
            variant = {"CSEL": 0, "CSINC": 1, "CSINV": 2, "CSNEG": 3}[form["mnemonic"]]
            seed = (seed & ~((1 << 30) | (1 << 10))) | ((variant >> 1) << 30) | ((variant & 1) << 10)
        group = form["family_path"][1]
        for cond in range(16):
            for nzcv in (range(16) if group != "condsel" else (0,)):
                registers = ((0, 0), (31, 31), (0, 1), (1, 0))
                if group == "condcmp_imm":
                    registers = ((0, 0), (31, 0))
                for rn, rm in registers:
                    word = (seed & ~((15 << 12) | (31 << 5) | (31 << 16) | 15))
                    word |= (cond << 12) | (rn << 5)
                    if form["family_path"][1] == "condsel":
                        word |= rm << 16
                    elif form["family_path"][1] == "condcmp_reg":
                        word |= (rm << 16) | nzcv
                    else:
                        word |= nzcv
                    cases.append((form, word, cond, nzcv, rn, rm))
    requests = [(f["id"], word) for f, word, *_ in cases]
    if len(requests) != 6656 or len(set(requests)) != len(requests):
        raise RuntimeError("condition domain incomplete")
    responses = differential.probe_rows(probe, requests)
    disassembly = differential.llvm_disassemble([word for _, word in requests])
    output = io.StringIO(newline="")
    writer = csv.writer(output)
    writer.writerow(("form", "canonical", "encoding", "width", "cond", "condition",
                     "nzcv", "holds", "inverted", "Rn", "Rm", "preferred_alias",
                     "llvm", "result"))
    counts = collections.Counter()
    failures = []
    for (form, word, cond, nzcv, rn, rm), response, text in zip(
            cases, responses, disassembly):
        mnemonic = form["mnemonic"]
        alias = alias_for(mnemonic, rn, rm, cond)
        state = "verified"
        if response is None or response[0] != mnemonic.lower() or text == "<llvm-invalid>":
            state = "validity-difference"
        else:
            canonical, preferred, operands = response
            for field, expected in (("cond", cond), ("Rn", rn)):
                if field not in operands or operands[field]["raw"] != expected:
                    state = "field-difference"
            if mnemonic in ("CCMP", "CCMN") and operands.get("nzcv", {}).get("raw") != nzcv:
                state = "field-difference"
            if form["family_path"][1] != "condcmp_imm" and operands.get("Rm", {}).get("raw") != rm:
                state = "field-difference"
            if mnemonic in ("CSINC", "CSINV", "CSNEG") and preferred != alias.lower():
                state = "alias-difference"
            llvm_name = text.split()[0]
            accepted = {mnemonic.lower(), alias.lower()}
            if mnemonic == "CSNEG" and rn == rm == 31 and cond < 14:
                accepted.add("cneg")
            if mnemonic in ("CSINC", "CSINV", "CSNEG") and llvm_name not in accepted:
                state = "llvm-alias-difference"
            if mnemonic in ("CCMP", "CCMN") and llvm_name != mnemonic.lower():
                state = "llvm-operation-difference"
            if mnemonic == "CSEL" and llvm_name != "csel":
                state = "llvm-operation-difference"
            if mnemonic in ("CCMP", "CCMN"):
                match = re.search(r"#(?:0x[0-9a-f]+|\d+)\s*,\s*(\w+)\s*$", text)
                if not match or match.group(1) != NAMES[cond]:
                    state = "llvm-condition-difference"
        if state != "verified":
            failures.append((form["id"], f"{word:08X}", state, text, response))
        counts[(form["id"], cond, alias, state)] += 1
        writer.writerow((form["id"], mnemonic, f"{word:08X}",
                         64 if (word >> 31) & 1 else 32, cond, NAMES[cond], nzcv,
                         int(holds(cond, nzcv)),
                         NAMES[cond ^ 1] if cond < 14 else "not-invertible",
                         rn, rm, alias, text, state))
    if failures:
        raise RuntimeError(f"{len(failures)} discrepancies: {failures[:4]}")
    data = output.getvalue().encode("utf-8")
    digest = hashlib.sha256(data).hexdigest()
    if digest != REPORT_SHA256:
        raise RuntimeError(f"condition report hash changed: {digest}")
    with open(REPORT, "rb") as stream:
        if stream.read() != data:
            raise RuntimeError(f"condition report mismatch: {digest}")
    print("Integer conditions", len(cases), "forms", len(selected),
          "aliases", sorted({key[2] for key in counts}), "sha256", digest)


if __name__ == "__main__":
    try:
        if len(sys.argv) != 2:
            raise ValueError("usage: integer_condition_audit.py PROBE")
        main(sys.argv[1])
    except (RuntimeError, OSError, ValueError, IndexError) as error:
        print("Integer condition audit failed:", error, file=sys.stderr)
        raise SystemExit(1)