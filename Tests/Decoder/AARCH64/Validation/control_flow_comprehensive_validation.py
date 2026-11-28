import collections
import csv
import hashlib
import io
import json
import os
import shutil
import subprocess
import sys

import authenticated_control_flow_validation as authenticated
import control_flow_validation as conventional
import llvm_toolchain

REPORT = "Tests/Decoder/AARCH64/Results/control_flow_comprehensive_validation.csv"
REPORT_SHA256 = "5a96fa23d0351af1660a1ffe46f2c0b0959368296882da5286503568c72dd62e"
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
}
COLUMNS = ("record_type", "id", "operation", "group", "status", "seed",
           "cases", "complete", "fixed_rejected", "feature_rejected",
           "sibling_valid", "llvm_verified", "operand_coverage",
           "dependencies", "restrictions", "features", "detail")
MASK64 = (1 << 64) - 1
FEATURES = "+cmpbr,+hbc,+pauth,+pauth-lr,+tme,+el3"


def verify_pinned():
    root = "Tests/Decoder/AARCH64/Results/"
    for name, expected in PINNED.items():
        digest = hashlib.sha256()
        with open(root + name, "rb") as stream:
            for chunk in iter(lambda: stream.read(65536), b""):
                digest.update(chunk)
        if digest.hexdigest() != expected:
            raise RuntimeError(f"historical report changed: {name}")


def read_prior(form_by_id):
    stats = collections.defaultdict(collections.Counter)
    witnesses = {}
    operands = collections.Counter()
    negative = []
    siblings = []
    deferred_conventional = set()
    deferred_future = set()
    extremes = collections.defaultdict(dict)
    for source, expected_class in (
            (conventional.REPORT, "conventional"),
            (authenticated.REPORT, "authenticated-or-exception")):
        with open(source, newline="", encoding="utf-8") as stream:
            for row in csv.DictReader(stream):
                category = row["class"]
                fid = row["form"]
                if category != expected_class:
                    if source == conventional.REPORT and category == "deferred":
                        deferred_conventional.add(fid)
                    elif (source == authenticated.REPORT and
                          category == "future-nonbranch-control"):
                        deferred_future.add(fid)
                    else:
                        raise RuntimeError(f"unknown inventory row: {category}")
                    continue
                if fid not in form_by_id:
                    raise RuntimeError(f"form missing from catalog: {fid}")
                form = form_by_id[fid]
                if (row["operation"] != form["operation_id"] or
                    row["group"] != form["family_path"][1]):
                    raise RuntimeError(f"catalog metadata mismatch: {fid}")
                features = ",".join(form.get("feature_identifiers", [])) or "-"
                if row["variant"] != "feature-negative" and row["feature"] != features:
                    raise RuntimeError(f"feature inventory mismatch: {fid}")
                if row["variant"] == "feature-negative" and row["feature"] not in (
                        form.get("feature_identifiers", [])):
                    raise RuntimeError(f"feature gate not declared: {fid}")
                result = row["result"]
                stats[fid]["cases"] += 1
                if result == "complete":
                    stats[fid]["complete"] += 1
                    if row["llvm"] == "llvm-verified":
                        stats[fid]["llvm_verified"] += 1
                elif row["variant"] == "fixed-bit-negative" and result in (
                        "rejected", "rejected-by-form"):
                    stats[fid]["fixed_rejected"] += 1
                    negative.append((fid, int(row.get("encoding", row.get("word")), 16),
                                     "fixed-bit-negative", "-"))
                elif row["variant"] == "feature-negative" and result in (
                        "rejected", "rejected-by-form"):
                    stats[fid]["feature_rejected"] += 1
                elif result == "sibling-form" and row["variant"] == "modifier":
                    stats[fid]["sibling_valid"] += 1
                    siblings.append((fid, int(row["word"], 16), row["llvm"]))
                else:
                    raise RuntimeError(f"unreconciled case {fid}: {row['variant']} {result}")
                if row["variant"] == "seed":
                    if fid in witnesses or result != "complete" or row["llvm"] != "llvm-verified":
                        raise RuntimeError(f"missing unique COMPLETE witness: {fid}")
                    word = int(row.get("encoding", row.get("word")), 16)
                    witnesses[fid] = (word, row["operands"])
                    if not row["operands"] or row["operands"] == "-":
                        raise RuntimeError(f"operand evidence missing: {fid}")
                    operands.update({item.split(":")[0] for item in
                                     row["operands"].split()})
                if source == conventional.REPORT and result == "complete" and (
                        form["family_path"][1] != "branch_reg"):
                    offset = int(row["offset"])
                    word = int(row["encoding"], 16)
                    if "min" not in extremes[fid] or offset < extremes[fid]["min"][0]:
                        extremes[fid]["min"] = (offset, word)
                    if "max" not in extremes[fid] or offset > extremes[fid]["max"][0]:
                        extremes[fid]["max"] = (offset, word)
    return stats, witnesses, operands, negative, siblings, extremes, deferred_conventional, deferred_future


def rebase_probe(probe, queries):
    source = "".join(f"{fid} {word:08X} {pc:016X} -\n"
                     for fid, word, pc, _ in queries)
    process = subprocess.run([probe, "--auth"], input=source, text=True,
                             capture_output=True, check=True)
    lines = process.stdout.splitlines()
    if len(lines) != len(queries):
        raise RuntimeError("PC-rebase probe response count changed")
    results = []
    for (fid, word, pc, variant), line in zip(queries, lines):
        parts = line.split("\t")
        if parts[:3] != [fid, f"{word:08X}", "COMPLETE"]:
            raise RuntimeError(f"rebase {variant} not COMPLETE: {fid} {word:08X}")
        base = tuple(map(int, parts[3].split(":")))
        auth = tuple(map(int, parts[4][5:].split(":")))
        if len(base) != 10 or len(auth) != 20:
            raise RuntimeError("probe metadata ABI changed")
        if base[9] != ((pc + 4) & MASK64 if base[2] else 0):
            raise RuntimeError(f"fallthrough address mismatch: {fid} {word:08X}")
        operands = {part.split(":")[0]: tuple(map(int, part.split(":")[1:]))
                    for part in parts[5:]}
        results.append((base, auth, operands))
    return results


def displacement(group, word):
    lsb, bits = ((0, 26) if group == "branch_imm" else
                 (5, 14) if group == "testbranch" else
                 (5, 9) if group.startswith("compbranch_") else (5, 19))
    raw = (word >> lsb) & ((1 << bits) - 1)
    return (raw - (1 << bits) if raw & (1 << (bits - 1)) else raw) * 4


def main(probe):
    verify_pinned()
    with open(conventional.CATALOG, encoding="utf-8") as stream:
        catalog = json.load(stream)
    if len(catalog["encodings"]) != 3099 or len(catalog["instruction_forms"]) != 4296:
        raise RuntimeError("catalog baseline changed")
    control = {form["id"]: form for form in catalog["instruction_forms"]
               if form["family_path"][0] == "control"}
    conventional_ids = {fid for fid, form in control.items()
                        if conventional.conventional(form)}
    authenticated_ids = {fid for fid, form in control.items()
                         if authenticated.selected(form)}
    future = set(control) - conventional_ids - authenticated_ids
    if (len(control), len(conventional_ids), len(authenticated_ids), len(future)) != (
            129, 49, 27, 53) or conventional_ids & authenticated_ids:
        raise RuntimeError("129-form control inventory changed")
    (stats, witnesses, operand_names, negative, siblings, extremes,
     deferred_conventional, deferred_future) = read_prior(control)
    if (set(witnesses) != conventional_ids | authenticated_ids or
        deferred_conventional != authenticated_ids | future or
        deferred_future != future or len(negative) != 1113 or
        len(siblings) != 2 or len(extremes) != 46):
        raise RuntimeError("prior campaigns do not reconcile with inventory")
    totals = collections.Counter()
    for item in stats.values():
        totals.update(item)
    if any((totals[key] != value) for key, value in (
            ("cases", 23385), ("complete", 22216),
            ("fixed_rejected", 1113), ("feature_rejected", 54),
            ("sibling_valid", 2), ("llvm_verified", 1336))):
        raise RuntimeError(f"prior case reconciliation changed: {dict(totals)}")
    seed_words = {word for word, _ in witnesses.values()}
    if len(seed_words) != 76:
        raise RuntimeError("colliding positive witness encodings")

    exe = os.environ.get("LLVM_MC_CONTROL", shutil.which("llvm-mc-21") or
                         llvm_toolchain.executable())
    _, version = llvm_toolchain.require_qualified(exe)
    if version != "21.1.8":
        raise RuntimeError("control extensions require qualified LLVM MC 21.1.8")
    reference = conventional.llvm_rows(exe, negative, FEATURES)
    llvm_invalid = sum(text == "<invalid>" for text in reference)
    llvm_other = len(reference) - llvm_invalid
    if llvm_invalid + llvm_other != totals["fixed_rejected"]:
        raise RuntimeError("negative reference campaign incomplete")
    overlaps = {
        "RETAASPPCR_64M_branch_reg": ("RETAA_64E_branch_reg", "retaa"),
        "RETABSPPCR_64M_branch_reg": ("RETAB_64E_branch_reg", "retab"),
    }
    overlap_queries = []
    for fid, word, text in siblings:
        target, mnemonic = overlaps[fid]
        if word != witnesses[target][0] or text.split()[0] != mnemonic:
            raise RuntimeError(f"sibling identity mismatch: {fid} {word:08X}")
        overlap_queries.append((target, word, conventional.PC, "sibling"))
    rebase_probe(probe, overlap_queries)

    combined = sorted(fid for fid in authenticated_ids if
                      "FEAT_PAuth_LR" in control[fid].get("feature_identifiers", []))
    if len(combined) != 4:
        raise RuntimeError("PAuth-LR combination inventory changed")
    combined_cases = [(fid, witnesses[fid][0], "combined-PAuth-LR", "FEAT_PAuth")
                      for fid in combined]
    combined_responses = authenticated.probe_rows(probe, combined_cases)
    combined_llvm = conventional.llvm_rows(
        exe, combined_cases, "+pauth-lr,-pauth,+el3,+tme")
    for case, response, text in zip(combined_cases, combined_responses, combined_llvm):
        fid, word, _, _ = case
        if response is None or text == "<invalid>" or (
                text.split()[0] != control[fid]["mnemonic"].lower()):
            raise RuntimeError(f"PAuth-LR-only profile mismatch: {fid}")
        authenticated.validate(control[fid], word, response)

    queries = []
    for fid, (word, _) in sorted(witnesses.items()):
        for pc in (conventional.PC, 0, MASK64 - 3):
            queries.append((fid, word, pc, "seed"))
        for boundary in ("min", "max"):
            if fid in extremes:
                _, boundary_word = extremes[fid][boundary]
                for pc in (0, MASK64 - 3):
                    queries.append((fid, boundary_word, pc, boundary))
    rebased = rebase_probe(probe, queries)
    if len(queries) != 412:
        raise RuntimeError("PC boundary inventory changed")
    for (fid, word, pc, _), (base, auth, operands) in zip(queries, rebased):
        form = control[fid]
        group = form["family_path"][1]
        if fid in conventional_ids:
            expected_pc_read = (group != "branch_reg" or
                                form["operation_id"] == "BLR")
        else:
            expected_pc_read = authenticated.expected(form, word)[1][4]
        if auth[4] != expected_pc_read:
            raise RuntimeError(f"PC dependency mismatch: {fid} {word:08X}")
        if pc == conventional.PC:
            if fid in conventional_ids:
                conventional.check(form, word, (base, operands))
            else:
                authenticated.validate(form, word, (base, auth, operands))
        if fid in conventional_ids and group != "branch_reg":
            offset = displacement(group, word)
            name = ("imm26" if group == "branch_imm" else
                    "imm14" if group == "testbranch" else
                    "imm9" if group.startswith("compbranch_") else "imm19")
            target = (pc + offset) & MASK64
            op = operands[name]
            if (base[8], op[4], op[5], op[6], op[1]) != (
                    target, offset, target, 4, 4) or target & 3:
                raise RuntimeError(f"PC wrap or alignment mismatch: {fid} {word:08X}")
        elif base[8] != 0:
            raise RuntimeError(f"invented static branch target: {fid}")
        if fid in authenticated_ids and base[0] not in (5, 6, 7, 8, 9, 10):
            raise RuntimeError(f"authenticated/exception kind lost: {fid}")

    output = io.StringIO(newline="")
    writer = csv.writer(output)
    writer.writerow(COLUMNS)
    by_operation = collections.defaultdict(set)
    by_feature = collections.defaultdict(set)
    by_dependency = collections.defaultdict(set)
    by_restriction = collections.defaultdict(set)
    for fid in sorted(witnesses):
        form = control[fid]
        counts = stats[fid]
        group = form["family_path"][1]
        word, operand_coverage = witnesses[fid]
        features = form.get("feature_identifiers", [])
        deps = set()
        if fid in conventional_ids:
            if group != "branch_reg" or form["operation_id"] in ("BLR",):
                deps.add("PC")
            if form["operation_id"] in ("BL", "BLR", "RET"):
                deps.add("LR")
            if group == "condbranch":
                deps.add("NZCV")
        else:
            (_, auth, _) = rebased[queries.index((fid, word, 0, "seed"))]
            for name, value in zip(("PC", "SP", "PSTATE", "ELR", "SPSR",
                                    "DEBUG_LINK", "DEBUG_STATUS"), auth[4:11]):
                if value:
                    deps.add(name)
            if auth[14] in (1, 2, 3, 4, 5):
                deps.update(("PC", "PSTATE", "ELR", "SPSR"))
            if auth[12] or auth[13]:
                deps.update(("ELR", "SPSR"))
            if form["operation_id"] in ("BLRA", "RETA", "RETASPPCR_reg",
                                        "RETASPPC_imm"):
                deps.add("LR")
            if auth[0] in (1, 2):
                deps.add("PAC_IA" if auth[0] == 1 else "PAC_IB")
        constraints = {"fixed-bits"}
        if group != "branch_reg" and fid in conventional_ids:
            constraints.update(("signed-offset", "alignment-4"))
        if group == "testbranch":
            constraints.add("bit-index")
        if group == "compbranch_imm":
            constraints.add("compare-imm6")
        if features:
            constraints.add("feature-gate")
        if counts["sibling_valid"]:
            constraints.add("valid-sibling-overlap")
        if fid in authenticated_ids:
            (_, auth, _) = rebased[queries.index((fid, word, 0, "seed"))]
            if auth[18]:
                constraints.add("EL2-required")
            if auth[19]:
                constraints.add("EL3-required")
            if auth[15]:
                constraints.add("min-EL1")
            if auth[14]:
                constraints.add("current-EL-unknown")
        by_operation[form["operation_id"]].add(fid)
        for feature in features:
            by_feature[feature].add(fid)
        for dependency in deps:
            by_dependency[dependency].add(fid)
        for constraint in constraints:
            by_restriction[constraint].add(fid)
        writer.writerow(("form", fid, form["operation_id"], group,
                         "COMPLETE", f"{word:08X}", counts["cases"],
                         counts["complete"], counts["fixed_rejected"],
                         counts["feature_rejected"], counts["sibling_valid"],
                         counts["llvm_verified"], operand_coverage,
                         ",".join(sorted(deps)) or "-",
                         ",".join(sorted(constraints)),
                         ",".join(features) or "-", "Arm/public"))
    for fid in sorted(future):
        form = control[fid]
        writer.writerow(("phase-3.6", fid, form["operation_id"],
                         form["family_path"][1], "deferred", "-", 0, 0, 0, 0,
                         0, 0, "-", "-", "-",
                         ",".join(form.get("feature_identifiers", [])) or "-",
                         "hints/barriers/PSTATE/system"))
    for category, groups in (("operation", by_operation),
                             ("feature", by_feature),
                             ("dependency", by_dependency),
                             ("restriction", by_restriction)):
        for name, ids in sorted(groups.items()):
            writer.writerow((category, name, "-", "-", "covered", "-",
                             len(ids), 0, 0, 0, 0, 0, "-", "-", "-", "-",
                             ",".join(sorted(ids))))
    for name, count in sorted(operand_names.items()):
        writer.writerow(("operand", name, "-", "-", "covered", "-",
                         count, 0, 0, 0, 0, 0, "-", "-", "-", "-", "-"))
    for fid, word, pc, variant in queries:
        writer.writerow(("PC-boundary", fid, control[fid]["operation_id"],
                         control[fid]["family_path"][1], "COMPLETE",
                         f"{word:08X}", 1, 1, 0, 0, 0, 0, "-", "PC", "-",
                         "-", f"{variant}:PC={pc:016X}"))
    for fid in combined:
        writer.writerow(("combined-feature-profile", fid,
                         control[fid]["operation_id"],
                         control[fid]["family_path"][1], "COMPLETE",
                         f"{witnesses[fid][0]:08X}", 1, 1, 0, 0, 0, 1,
                         "-", "SP,PC,LR", "FEAT_PAuth_LR", "FEAT_PAuth_LR",
                         "FEAT_PAuth disabled; LLVM verified"))
    writer.writerow(("total", "phase-3.3", "-", "-", "COMPLETE", "-",
                     totals["cases"], totals["complete"],
                     totals["fixed_rejected"], totals["feature_rejected"],
                     totals["sibling_valid"], totals["llvm_verified"],
                     len(operand_names), len(by_dependency),
                     len(by_restriction), len(by_feature),
                     f"76 forms;53 to 3.6;{len(queries)} PC-boundary;"
                     f"{len(combined)} PAuth-LR-only;"
                     f"{llvm_invalid} LLVM-unavailable;{llvm_other} LLVM-other"))
    lengths = {len(row) for row in csv.reader(io.StringIO(output.getvalue()))}
    if lengths != {len(COLUMNS)}:
        raise RuntimeError(f"CSV columns misaligned: {lengths}")
    data = output.getvalue().encode("utf-8")
    digest = hashlib.sha256(data).hexdigest()
    print("Control-flow comprehensive", len(witnesses), "COMPLETE", len(future),
          "to phase 3.6", totals["cases"], "historical cases", len(queries),
          "PC-boundary cases", totals["complete"], "positive",
          totals["fixed_rejected"], "fixed-bit rejected-by-form",
          totals["feature_rejected"], "feature-gated",
          totals["sibling_valid"], "valid sibling overlaps",
          totals["llvm_verified"], "LLVM-verified", llvm_invalid,
          "LLVM-unavailable negatives", llvm_other, "LLVM-valid-other negatives",
          len(combined), "PAuth-LR-only",
          "sha256", digest)
    print("Coverage", len(by_operation), "operations", len(operand_names),
          "operand names", len(by_dependency), "dependencies",
          len(by_restriction), "restrictions", len(by_feature), "features")
    if REPORT_SHA256 != "PENDING" and digest != REPORT_SHA256:
        raise RuntimeError(f"comprehensive report hash changed: {digest}")
    if os.environ.get("ARMITO_WRITE_REPORT") == "1":
        with open(REPORT, "wb") as stream:
            stream.write(data)
    else:
        with open(REPORT, "rb") as stream:
            if stream.read() != data:
                raise RuntimeError("comprehensive report mismatch")


if __name__ == "__main__":
    try:
        if len(sys.argv) != 2:
            raise ValueError("usage: control_flow_comprehensive_validation.py PROBE")
        main(sys.argv[1])
    except (RuntimeError, OSError, ValueError, KeyError, IndexError,
            subprocess.CalledProcessError) as error:
        print("Control-flow comprehensive failed:", error, file=sys.stderr)
        raise SystemExit(1)
