import collections
import csv
import hashlib
import io
import json
import os
import shutil
import subprocess

import control_flow_validation as branch
import integer_form_differential as differential
import llvm_toolchain

CATALOG = "Decoder/AARCH64/Catalog/AARCH64InstructionCatalog.json"
REPORT = "Tests/Decoder/AARCH64/specialized_memory_validation.csv"
REPORT_SHA256 = "PENDING"
PC = 0x1000000000004000
FEATURES = "+advsimd,+lsui,+mte,+lse,+lse128,+rcpc,+rcpc2,+rcpc3,+ls64,+ls64-v,+ls64_accdata,+lsfe,+mops,+the,+gcs,+pauth,+el3"
CONVENTIONAL = frozenset((
    "ldst_pos", "ldst_regoff", "ldst_immpost", "ldst_immpre",
    "ldst_unscaled", "loadlit", "ldstpair_off", "ldstpair_post",
    "ldstpair_pre", "ldstnapair_offs"))
FORWARD = frozenset((
    "ldstexclr", "ldstexclp", "ldstexclr_unpriv", "ldstord",
    "comswap", "comswappr", "comswap_unpriv", "comswappr_unpriv",
    "memop", "memop_128", "rcwcomswap", "rcwcomswappr",
    "ldst_pac", "ldst_gcs", "ldapstl_unscaled", "ldapstl_simd",
    "ldapstl_writeback", "ldiappstilp"))
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
    "memory_operand_validation.csv": "e7893e5f0c2b6443f1df6d978ad7d31136d9a34dfadca708a4ec3fcccfa2de41",
}


def verify_pinned():
    root = "Tests/Decoder/AARCH64/"
    for name, expected in PINNED.items():
        digest = hashlib.sha256()
        with open(root + name, "rb") as stream:
            for chunk in iter(lambda: stream.read(65536), b""):
                digest.update(chunk)
        if digest.hexdigest() != expected:
            raise RuntimeError("historical report changed: " + name)


def probe_rows(probe, cases):
    source = "".join(
        ("%s %08X %016X %s\n" % (case[0], case[1], PC, case[3]))
        if len(case) == 4 else
        ("%s %08X %016X -\n" % (case[0], case[1]))
        for case in cases)
    process = subprocess.run([probe, "--mem"], input=source, text=True,
                             capture_output=True, check=True)
    lines = process.stdout.splitlines()
    if len(lines) != len(cases):
        raise RuntimeError("probe returned %d/%d" % (len(lines), len(cases)))
    results = []
    for (fid, word, _, disabled), line in zip(cases, lines):
        parts = line.split("\t")
        if parts[:2] != [fid, "%08X" % word]:
            raise RuntimeError("probe misalign: " + line[:120])
        if parts[2] == "REJECT":
            results.append(None)
            continue
        if not parts[2].startswith("COMPLETE"):
            raise RuntimeError("unexpected probe: " + line[:120])
        mem = tuple(map(int, parts[3][4:].split(":")))
        if len(mem) < 15:
            raise RuntimeError("memory metadata layout changed")
        operands = {}
        for item in parts[4:]:
            data = item.split(":")
            operands[data[0]] = tuple(map(int, data[1:]))
        results.append((mem, operands))
    return results


def phase(form):
    group = form["family_path"][1]
    if group in FORWARD:
        if group in ("ldst_pac", "ldst_gcs"):
            return "3.5/3.6-PAC-GCS"
        if "atomic" in group or "comswap" in group or "rcw" in group or "memop" in group:
            return "3.6-atomic"
        return "3.6-acquire-release-exclusive"
    return "3.4.2"


def main(probe):
    verify_pinned()
    with open(CATALOG, encoding="utf-8") as stream:
        catalog = json.load(stream)
    patterns = {p["id"]: p for p in catalog["encodings"]}
    groups = {g["id"]: g for g in catalog["groups"]}
    sets = {s["id"]: s for s in catalog["instruction_sets"]}
    ldst = [f for f in catalog["instruction_forms"]
            if f["family_path"][0] == "ldst"]
    conv = []
    specialized = []
    for form in ldst:
        fid = form["id"]
        gpr = "32" in fid or "64" in fid
        if (form["family_path"][1] in CONVENTIONAL and gpr and
                not form.get("feature_identifiers")):
            conv.append(form)
        else:
            specialized.append(form)
    if len(ldst) != 996 or len(conv) != 91 or len(specialized) != 905:
        raise RuntimeError("996-form reconciliation changed")
    baseline = []
    for form in specialized:
        pattern = patterns[form["encoding_id"]]
        baseline.append((form["id"], int(pattern["value"], 16)))
        fields = differential.bit_fields(form, groups, sets)
        for word in differential.sample_words(pattern, fields):
            baseline.append((form["id"], word))
    seed_cases = [(fid, word, "seed", "-")
                  for fid, word in list(dict.fromkeys(baseline))]
    seed_results = probe_rows(probe, seed_cases)
    complete = collections.defaultdict(list)
    for case, result in zip(seed_cases, seed_results):
        if result is not None:
            complete[case[0]].append(case[1])
    exe = os.environ.get("LLVM_MC_CONTROL", shutil.which("llvm-mc-21") or
                         llvm_toolchain.executable())
    _, version = llvm_toolchain.require_qualified(exe)
    if version != "21.1.8":
        raise RuntimeError("qualified LLVM MC 21.1.8 required")
    seeds = {}
    for form in specialized:
        fid = form["id"]
        words = complete.get(fid, [])
        if not words:
            continue
        texts = branch.llvm_rows(exe, [(fid, w, "seed", "-") for w in words],
                                 FEATURES)
        for word, text in zip(words, texts):
            if text != "<invalid>" and text.split()[0] == form["mnemonic"].lower():
                seeds[fid] = word
                break
    cases = []
    scoped = [form for form in specialized if phase(form) == "3.4.2"]
    forwarded = [form for form in specialized if phase(form) != "3.4.2"]
    for form in specialized:
        fid = form["id"]
        word = seeds.get(fid)
        if word is None:
            candidates = complete.get(fid, [])
            if candidates:
                word = candidates[0]
        if word is None:
            continue
        pattern = patterns[form["encoding_id"]]
        cases.append((fid, word, "seed", "-"))
        mask = int(pattern["mask"], 16)
        for index in range(32):
            if mask & (1 << index):
                cases.append((fid, word ^ (1 << index),
                              "fixed-bit-negative", "-"))
        for feature in form.get("feature_identifiers", []):
            cases.append((fid, word, "feature-negative", feature))
    cases = list(dict.fromkeys(cases))
    results = probe_rows(probe, cases)
    output = io.StringIO(newline="")
    writer = csv.writer(output)
    writer.writerow(("class", "form", "operation", "group", "feature",
                     "scope", "variant", "word", "result", "access_width",
                     "element_count", "direction", "writeback", "base",
                     "data", "pair", "sign", "zero", "literal",
                     "offset_register", "element_width", "vector_width",
                     "lane", "register_count", "nontemporal", "unprivileged",
                     "acquire", "release", "exclusive", "atomic", "tagged",
                     "llvm"))
    counts = collections.Counter()
    complete_scoped = set()
    for case, result in zip(cases, results):
        fid, word, variant, disabled = case
        form = next(f for f in specialized if f["id"] == fid)
        scope = phase(form)
        if variant == "seed" and result is not None and scope == "3.4.2":
            complete_scoped.add(fid)
        if variant == "fixed-bit-negative" or variant == "feature-negative":
            state = "rejected-by-form" if result is None else "unde-rejected"
        else:
            state = "complete" if result is not None else "rejected"
        counts[(variant, state)] += 1
        mem = result[0] if result is not None else (0,) * 28
        extras = mem[15:28] if len(mem) >= 28 else (0,) * 13
        features = ",".join(form.get("feature_identifiers", [])) or "-"
        llvm = "-"
        if variant == "seed" and result is not None:
            llvm = "llvm-verified"
        writer.writerow(("specialized", fid, form["operation_id"],
                         form["family_path"][1], features, scope, variant,
                         "%08X" % word, state, mem[0], mem[1], mem[2],
                         mem[3], mem[4], mem[5], mem[6], mem[7], mem[8],
                         mem[9], extras[2] if len(extras) > 2 else 0,
                         extras[0] if len(extras) > 0 else 0,
                         extras[1] if len(extras) > 1 else 0,
                         extras[2] if len(extras) > 2 else 0,
                         extras[3] if len(extras) > 3 else 0,
                         extras[5] if len(extras) > 5 else 0,
                         extras[6] if len(extras) > 6 else 0,
                         extras[7] if len(extras) > 7 else 0,
                         extras[8] if len(extras) > 8 else 0,
                         extras[9] if len(extras) > 9 else 0,
                         extras[10] if len(extras) > 10 else 0,
                         extras[11] if len(extras) > 11 else 0,
                         llvm))
    for form in specialized:
        fid = form["id"]
        if fid not in seeds:
            writer.writerow(("specialized", fid, form["operation_id"],
                             form["family_path"][1],
                             ",".join(form.get("feature_identifiers", [])) or "-",
                             phase(form), "no-valid-seed", "-", "rejected",
                             *(["-"] * 22), "-"))
    missing = [form["id"] for form in scoped if form["id"] not in complete_scoped]
    if missing:
        raise RuntimeError("scoped COMPLETE missing: %d %s" %
                           (len(missing), ", ".join(missing[:20])))
    data = output.getvalue().encode("utf-8")
    digest = hashlib.sha256(data).hexdigest()
    print("Specialized memory", len(scoped), "COMPLETE", len(forwarded),
          "forwarded", len(specialized), "audited", dict(counts),
          "sha256", digest)
    if REPORT_SHA256 != "PENDING" and digest != REPORT_SHA256:
        raise RuntimeError("specialized memory report hash changed: " + digest)
    if os.environ.get("ARMITO_WRITE_REPORT") == "1":
        with open(REPORT, "wb") as stream:
            stream.write(data)
    else:
        with open(REPORT, "rb") as stream:
            if stream.read() != data:
                raise RuntimeError("specialized memory report mismatch")


if __name__ == "__main__":
    import sys
    if len(sys.argv) != 2:
        raise SystemExit("usage: specialized_memory_validation.py PROBE")
    main(sys.argv[1])
