import collections
import csv
import hashlib
import io
import json
import os
import sys

import integer_form_differential as differential

REPORT = "Tests/Decoder/AARCH64/integer_implicit_operands.csv"
REPORT_SHA256 = "6df6ade821c168b50a1f3846743be99700c66b0889375dca05ee86ac7e4734c0"
IMPLICIT = 0x100
SPECIAL_SP = 1
SPECIAL_ZERO = 2
REGISTER_FIELDS = ("Rd", "Rn", "Rm", "Ra")


def contract(form):
    family = form["family_path"][1]
    mnemonic = form["mnemonic"]
    reads = set()
    writes = set()
    implicit_reads = set()
    nzcv_read = False
    nzcv_write = False
    carry_read = False
    cond_read = False
    partial = False
    if family == "addsub_carry":
        carry_read = True
        nzcv_write = mnemonic in ("ADCS", "SBCS")
        reads.update(("Rd", "Rn", "Rm"))
        writes.add("Rd")
    elif family in ("addsub_shift", "addsub_imm", "addsub_ext", "addsub_immtags", "addsub_pt"):
        nzcv_write = mnemonic in ("ADDS", "SUBS")
        reads.update(("Rn", "Rm"))
        writes.add("Rd")
        partial = mnemonic in ("ADDS", "SUBS")
    elif family in ("log_shift", "log_imm"):
        nzcv_write = mnemonic in ("ANDS", "BICS")
        reads.update(("Rn", "Rm"))
        writes.add("Rd")
        partial = mnemonic in ("ANDS", "BICS")
    elif family == "dp_2src":
        reads.update(("Rn", "Rm"))
        writes.add("Rd")
    elif family == "dp_3src":
        reads.update(("Rn", "Rm", "Ra"))
        writes.add("Rd")
    elif family in ("dp_1src", "dp_1src_imm"):
        reads.add("Rn")
        writes.add("Rd")
    elif family == "movewide":
        if mnemonic == "MOVK":
            reads.add("Rd")
            implicit_reads.add("Rd")
            partial = True
        writes.add("Rd")
    elif family == "bitfield":
        reads.add("Rn")
        writes.add("Rd")
        implicit_reads.add("Rn")
        if mnemonic == "BFM":
            reads.add("Rd")
            implicit_reads.add("Rd")
            partial = True
    elif family == "extract":
        reads.update(("Rn", "Rm"))
        writes.add("Rd")
    elif family == "condsel":
        cond_read = True
        reads.update(("Rn", "Rm"))
        writes.add("Rd")
    elif family in ("condcmp_imm", "condcmp_reg"):
        cond_read = True
        nzcv_read = True
        nzcv_write = True
        reads.add("Rn")
        if family == "condcmp_reg":
            reads.add("Rm")
    elif family == "setf":
        nzcv_write = True
        reads.add("Rn")
        partial = True
    elif family == "rmif":
        nzcv_write = True
        reads.add("Rn")
        partial = True
    elif family == "minmax_imm":
        reads.add("Rn")
        writes.add("Rd")
    elif family == "pcreladdr":
        writes.add("Rd")
    return {"reads": reads, "writes": writes, "implicit_reads": implicit_reads,
            "nzcv_read": nzcv_read, "nzcv_write": nzcv_write,
            "carry_read": carry_read, "cond_read": cond_read, "partial": partial}


def main(probe):
    with open(differential.CATALOG, encoding="utf-8") as stream:
        catalog = json.load(stream)
    patterns = {p["id"]: p for p in catalog["encodings"]}
    groups = {g["id"]: g for g in catalog["groups"]}
    sets = {s["id"]: s for s in catalog["instruction_sets"]}
    forms = {f["id"]: f for f in catalog["instruction_forms"]
             if f["family_path"][0] in ("dpimm", "dpreg")}
    if len(forms) != 195:
        raise RuntimeError("integer form inventory changed")
    requests = []
    meta = []
    for form in forms.values():
        seed = int(patterns[form["encoding_id"]]["value"], 16)
        fields = differential.bit_fields(form, groups, sets)
        for reg in REGISTER_FIELDS:
            field = fields.get(reg)
            if field is None or field["width"] != 5:
                continue
            fixed = int(patterns[form["encoding_id"]]["mask"], 16)
            mask = differential.field_mask(field) & ~fixed
            if mask != differential.field_mask(field):
                continue
            for value in (0, 1, 30, 31):
                word = (seed & ~mask) | ((value << field["lsb"]) & mask)
                requests.append((form["id"], word))
                meta.append((form, word, reg, value))
    responses = differential.probe_rows(probe, requests)
    accepted_forms = set()
    output = io.StringIO(newline="")
    writer = csv.writer(output)
    writer.writerow(("form", "operation", "family", "encoding", "register", "value",
                     "implicit_read", "read", "write", "special", "width",
                     "nzcv_read", "nzcv_write", "carry_read", "cond_read",
                     "partial", "result"))
    counts = collections.Counter()
    failures = []
    coverage = collections.defaultdict(lambda: collections.defaultdict(set))
    for (form, word, reg, value), response in zip(meta, responses):
        spec = contract(form)
        if response is None:
            counts["rejected"] += 1
            continue
        accepted_forms.add(form["id"])
        canonical, alias, operands = response
        operand = operands.get(reg)
        if operand is None:
            failures.append((form["id"], f"{word:08X}", reg, "missing-operand"))
            counts["failure"] += 1
            continue
        special = operand["special"]
        width = operand["width"]
        state = "verified"
        if value == 31 and reg in REGISTER_FIELDS and width in (32, 64):
            if special not in (SPECIAL_ZERO, SPECIAL_SP):
                state = "special-context"
        if width not in (32, 64):
            state = "width"
        if state != "verified":
            failures.append((form["id"], f"{word:08X}", reg, state, special, width))
        counts[(form["id"], reg, state)] += 1
        coverage[form["id"]][reg].add(value)
        writer.writerow((form["id"], form["operation_id"], form["family_path"][1],
                         f"{word:08X}", reg, value, int(reg in spec["implicit_reads"]),
                         int(reg in spec["reads"]), int(reg in spec["writes"]), special,
                         width, int(spec["nzcv_read"]), int(spec["nzcv_write"]),
                         int(spec["carry_read"]), int(spec["cond_read"]),
                         int(spec["partial"]), state))
    for form in forms.values():
        if form["id"] in accepted_forms:
            continue
        spec = contract(form)
        seed = int(patterns[form["encoding_id"]]["value"], 16)
        reg = "Rd" if "Rd" in spec["reads"] or "Rd" in spec["writes"] else "Rn"
        writer.writerow((form["id"], form["operation_id"], form["family_path"][1],
                         f"{seed:08X}", reg, 0, int(reg in spec["implicit_reads"]),
                         int(reg in spec["reads"]), int(reg in spec["writes"]),
                         SPECIAL_ZERO if reg == "Rd" else 0, 0,
                         int(spec["nzcv_read"]), int(spec["nzcv_write"]),
                         int(spec["carry_read"]), int(spec["cond_read"]),
                         int(spec["partial"]), "rejected"))
        counts["contract-only"] += 1
    if failures:
        raise RuntimeError(f"{len(failures)} implicit-operand divergences: {failures[:6]}")
    data = output.getvalue().encode("utf-8")
    digest = hashlib.sha256(data).hexdigest()
    states = collections.Counter(key[2] for key in counts if isinstance(key, tuple))
    print("Implicit operands", len(requests), "cases", len(forms), "forms",
          "states", dict(states), "sha256", digest)
    if REPORT_SHA256 != "PENDING" and digest != REPORT_SHA256:
        raise RuntimeError(f"implicit report hash changed: {digest}")
    if os.environ.get("ARMITO_WRITE_REPORT") == "1":
        with open(REPORT, "wb") as stream:
            stream.write(data)
    else:
        with open(REPORT, "rb") as stream:
            if stream.read() != data:
                raise RuntimeError("implicit report mismatch")
    return digest


if __name__ == "__main__":
    try:
        if len(sys.argv) != 2:
            raise ValueError("usage: integer_implicit_operands.py PROBE")
        main(sys.argv[1])
    except (RuntimeError, OSError, ValueError, IndexError, KeyError) as error:
        print("Implicit operand audit failed:", error, file=sys.stderr)
        raise SystemExit(1)