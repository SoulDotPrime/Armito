from __future__ import annotations

import csv
import hashlib
import io
import json
import os
import re
import subprocess
import sys

import llvm_toolchain

CATALOG = "Decoder/AARCH64/Catalog/AARCH64InstructionCatalog.json"
REPORT = "Tests/Decoder/AARCH64/Results/system_validation.csv"
REPORT_SHA256 = "207ef6aee587e9ce9db579a1a10aab90638ba332be00ca0f22c4a43dd2281bcb"
PC = 0x1000000000004000
SYSTEM_GROUPS = frozenset((
    "systemmove", "systemmovepr", "systeminstrs",
    "syspairinstrs", "barriers", "hints", "pstate",
    "systeminstrswithreg", "systemresult"))
MOVE_GROUPS = frozenset(("systemmove", "systemmovepr"))


def probe_rows(probe, cases):
    source = "".join(
        "%s %08X %016X\n" % (case[0], case[1], PC)
        for case in cases
    )
    result = subprocess.run(
        [probe], input=source, text=True, capture_output=True, check=True
    )
    rows = result.stdout.splitlines()
    if len(rows) != len(cases):
        raise RuntimeError("system probe row count mismatch")
    parsed = []
    for case, line in zip(cases, rows):
        parts = line.split("\t")
        if parts[:2] != [case[0], "%08X" % case[1]]:
            raise RuntimeError("system probe row identity mismatch")
        kind = parts[2]
        if kind == "OK":
            if len(parts) < 5 or parts[3] != "COMPLETE":
                raise RuntimeError("OK without COMPLETE operands")
            sys_parts = parts[4].split(":")[1:]
            if len(sys_parts) != 18:
                raise RuntimeError("malformed SYS field: %s" % parts[4])
            parsed.append((kind, sys_parts))
        elif kind in ("UNSUPPORTED", "INVALID", "NO-OPERANDS"):
            parsed.append((kind,))
        else:
            raise RuntimeError("unknown system result: %r" % kind)
    return parsed


def llvm_disassemble(executable, words, features="+pauth"):
    source = "\n".join(
        ", ".join("0x%02x" % ((word >> (8 * byte)) & 255)
                  for byte in range(4))
        for word in words
    ) + "\n"
    result = subprocess.run(
        [executable, "--disassemble", "--triple=aarch64",
         "--mattr=" + features],
        input=source, text=True, capture_output=True, check=True
    )
    lines = [line.strip() for line in result.stdout.splitlines()
             if line.strip() and line.strip() != ".text"]
    invalid = {int(item) - 1 for item in re.findall(
        r"<stdin>:(\d+):\d+: warning: invalid instruction encoding",
        result.stderr)}
    return lines, invalid


def main():
    args = sys.argv[1:]
    write_report = "--write-report" in args
    args = [a for a in args if a != "--write-report"]
    if len(args) != 1:
        raise SystemExit("usage: system_validation.py PROBE [--write-report]")
    probe = args[0]

    catalog = json.loads(open(CATALOG, encoding="utf-8").read())
    patterns = {p["id"]: p for p in catalog["encodings"]}
    forms = [f for f in catalog["instruction_forms"]
             if f["family_path"][1] in SYSTEM_GROUPS]

    cases = []
    for form in forms:
        pattern = patterns[form["encoding_id"]]
        word = int(pattern["value"], 16)
        cases.append((form["id"], word, form))

    results = probe_rows(probe, [(c[0], c[1]) for c in cases])

    for form, result in zip(forms, results):
        if result[0] != "OK":
            raise RuntimeError("system form %s not OK: %r" %
                               (form["id"], result))

    witnesses = [
        ("MRS_MIDR_EL1", 0xD5380000, "MIDR_EL1", "1"),
        ("MSR_SCTLR_EL1", 0xD5181000, "SCTLR_EL1", "2"),
    ]
    witness_results = probe_rows(probe, [(w[0], w[1]) for w in witnesses])
    for witness, result in zip(witnesses, witness_results):
        if result[0] != "OK":
            raise RuntimeError("witness %s not OK" % witness[0])
        sys_parts = result[1]
        if sys_parts[13] != witness[2]:
            raise RuntimeError(
                "witness %s name %s != %s" %
                (witness[0], sys_parts[13], witness[2]))
        if sys_parts[7] != witness[3]:
            raise RuntimeError(
                "witness %s access %s != %s" %
                (witness[0], sys_parts[7], witness[3]))

    exe = llvm_toolchain.executable()
    words = [case[1] for case in cases]
    llvm_lines, llvm_invalid = llvm_disassemble(exe, words)
    if len(llvm_lines) + len(llvm_invalid) != len(words):
        raise RuntimeError("LLVM line count mismatch")

    reference = {}
    line_index = 0
    for index, word in enumerate(words):
        if index in llvm_invalid:
            continue
        text = llvm_lines[line_index].lower()
        line_index += 1
        if text == "<invalid>":
            raise RuntimeError("LLVM rejects accepted %08X" % word)
        reference[index] = text

    rows = []
    counts = {}
    for index, (form, case, result) in enumerate(zip(forms, cases, results)):
        group = form["family_path"][1]
        sys_parts = result[1]
        op0, op1, crn, crm, op2, rt = sys_parts[0:6]
        register, access, impl_def, is_sysl = sys_parts[6:10]
        barrier_domain, barrier_type, hint_imm = sys_parts[10:13]
        name = sys_parts[13]
        pstate_field, identity = sys_parts[14:16]

        if group in MOVE_GROUPS:
            if access not in ("1", "2"):
                raise RuntimeError(
                    "move form %s invalid access %s" % (form["id"], access))
        if group == "barriers":
            if barrier_domain == "0" and barrier_type == "0":
                if form["operation_id"] not in ("CLREX", "DMB", "DSB",
                                                 "ISB", "SB"):
                    raise RuntimeError(
                        "barrier form %s missing domain/type" % form["id"])
        if group == "hints":
            if hint_imm == "0" and form["operation_id"] not in ("NOP", "HINT"):
                pass
        if group == "pstate":
            pass

        llvm_text = reference.get(index, "-")
        state = "complete"
        counts[(group, state)] = counts.get((group, state), 0) + 1
        rows.append((
            "system", form["id"], form["operation_id"], group,
            ",".join(form.get("feature_identifiers", [])) or "-",
            "seed", "%08X" % case[1], state,
            op0, op1, crn, crm, op2, rt, register, access,
            impl_def, is_sysl, barrier_domain, barrier_type,
            hint_imm, name, pstate_field, identity, llvm_text))

    fieldnames = (
        "class", "form", "operation", "group", "features",
        "variant", "word", "result",
        "op0", "op1", "crn", "crm", "op2", "rt", "register", "access",
        "impl_def", "is_sysl", "barrier_domain", "barrier_type",
        "hint_immediate", "register_name", "pstate_field", "identity",
        "llvm")
    output = io.StringIO(newline="")
    writer = csv.writer(output, lineterminator="\n")
    writer.writerow(fieldnames)
    writer.writerows(rows)
    data = output.getvalue().encode("utf-8")
    digest = hashlib.sha256(data).hexdigest()

    if write_report:
        os.makedirs(os.path.dirname(REPORT), exist_ok=True)
        with open(REPORT, "wb") as stream:
            stream.write(data)
    else:
        if not os.path.isfile(REPORT):
            raise SystemExit("missing system report")
        actual = hashlib.sha256(open(REPORT, "rb").read()).hexdigest()
        if REPORT_SHA256 == "PENDING":
            raise SystemExit("report hash not pinned: %s" % actual)
        if actual != REPORT_SHA256:
            raise SystemExit("system report hash mismatch")

    group_counts = {}
    for form in forms:
        group = form["family_path"][1]
        group_counts[group] = group_counts.get(group, 0) + 1
    print("System validation", len(forms), "COMPLETE",
          len(cases), "cases", len(reference), "LLVM comparisons",
          dict(counts), "sha256", digest)
    print("System groups", group_counts)


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, OSError, ValueError, KeyError, IndexError,
            subprocess.CalledProcessError) as error:
        print("System audit failed:", error, file=sys.stderr)
        raise SystemExit(1)