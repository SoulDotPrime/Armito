import collections
import csv
import hashlib
import io
import json
import sys

import integer_form_differential as register
import integer_immediate_differential as immediate


REPORT = "Tests/Decoder/AARCH64/Results/integer_immediate_rejections.csv"
HISTORICAL = "Tests/Decoder/AARCH64/Results/integer_register_rejections.csv"
FULL_REPORT = "Tests/Decoder/AARCH64/Results/integer_immediate_all_candidates.csv"


def architectural_reason(form, word):
    group = form["family_path"][1]
    width = 64 if (word >> 31) & 1 else 32
    if group == "addsub_ext" and (word >> 10) & 7 > 4:
        return "ADD_SUB_EXT_IMM3_GT_4"
    if group == "addsub_shift":
        if (word >> 22) & 3 == 3:
            return "ADD_SUB_SHIFT_RESERVED"
        if width == 32 and (word >> 15) & 1:
            return "SHIFT_32_IMM6_BIT5"
    if group == "log_shift" and width == 32 and (word >> 15) & 1:
        return "SHIFT_32_IMM6_BIT5"
    if group == "bitfield":
        n, r, s = (word >> 22) & 1, (word >> 16) & 63, (word >> 10) & 63
        if n != (width == 64):
            return "BITFIELD_N_WIDTH"
        if width == 32 and (r >= 32 or s >= 32):
            return "BITFIELD_32_IMM_BIT5"
    if group == "setf":
        return "SETF_FIXED_MASK"
    if group == "log_imm":
        n = (word >> 22) & 1
        if width == 32 and n:
            return "DEFER_BITMASK_N_WIDTH"
        return "DECODE_BITMASK_PENDING"
    return "UNCLASSIFIED"


def fields_for(form, word):
    group = form["family_path"][1]
    if group == "addsub_ext":
        return f"imm3={(word >> 10) & 7};option={(word >> 13) & 7}"
    if group in ("addsub_shift", "log_shift"):
        return f"sf={(word >> 31) & 1};shift={(word >> 22) & 3};imm6={(word >> 10) & 63}"
    if group == "bitfield":
        return f"sf={(word >> 31) & 1};N={(word >> 22) & 1};immr={(word >> 16) & 63};imms={(word >> 10) & 63}"
    if group == "log_imm":
        return f"sf={(word >> 31) & 1};N={(word >> 22) & 1};immr={(word >> 16) & 63};imms={(word >> 10) & 63}"
    if group == "setf":
        return f"mask={word & 15}"
    return ""


def verify_report(path, rows):
    stream = io.StringIO(newline="")
    writer = csv.writer(stream)
    writer.writerow(("form", "operation", "group", "encoding", "fields",
                     "architectural_reason", "probe", "llvm", "category"))
    writer.writerows(rows)
    regenerated = stream.getvalue().encode("utf-8")
    with open(path, "rb") as stored:
        original = stored.read()
    if original != regenerated:
        raise RuntimeError(f"byte-for-byte corpus mismatch: {path}")
    return hashlib.sha256(original).hexdigest()


def main(probe):
    with open(register.CATALOG, encoding="utf-8") as stream:
        catalog = json.load(stream)
    forms = [form for form in catalog["instruction_forms"]
             if form["family_path"][0] in ("dpimm", "dpreg")]
    by_id = {form["id"]: form for form in forms}
    groups = {group["id"]: group for group in catalog["groups"]}
    sets = {item["id"]: item for item in catalog["instruction_sets"]}
    patterns = {item["id"]: item for item in catalog["encodings"]}
    requests = list(dict.fromkeys(
        (form["id"], word) for form in forms
        for word in immediate.samples(
            int(patterns[form["encoding_id"]]["value"], 16),
            register.bit_fields(form, groups, sets), patterns[form["encoding_id"]],
            immediate.immediate_fields(form, register.bit_fields(form, groups, sets)),
            form["family_path"][1])
        if immediate.immediate_fields(form, register.bit_fields(form, groups, sets))))
    if len(requests) != 62851:
        raise RuntimeError(f"immediate corpus changed: {len(requests)}")
    results = register.probe_rows(probe, requests)
    disassembly = register.llvm_disassemble([word for _, word in requests])
    counters = collections.Counter()
    report = []
    all_rows = []
    failures = []
    by_encoding = {}
    for (identifier, word), result, llvm in zip(requests, results, disassembly):
        form = by_id[identifier]
        group = form["family_path"][1]
        reason = architectural_reason(form, word)
        if group == "log_imm":
            category = "deferred-bitmask" if result is not None else "deferred-bitmask-reject"
        elif group == "setf":
            category = "implicit-setf"
        elif reason != "UNCLASSIFIED":
            category = "architecturally-invalid"
        else:
            category = "llvm-verified"
        counters[category] += 1
        if (category == "architecturally-invalid" and (result is not None or llvm != "<llvm-invalid>")) or (
                category == "llvm-verified" and (result is None or llvm == "<llvm-invalid>")) or (
                category == "implicit-setf" and (result is None or llvm == "<llvm-invalid>")) or (
                category == "deferred-bitmask-reject" and llvm != "<llvm-invalid>"):
            failures.append((identifier, word, reason, llvm, result is not None))
        if category == "implicit-setf" and (word & 15) != 13:
            failures.append((identifier, word, "SETF mask must be 1101"))
        row = (identifier, form["operation_id"], group,
               f"{word:08X}", fields_for(form, word), reason,
               "COMPLETE" if result else "REJECT", llvm, category)
        all_rows.append(row)
        if category != "llvm-verified" and category != "deferred-bitmask":
            report.append(row)
            by_encoding[(identifier, f"{word:08X}")] = row
    expected = {"llvm-verified": 52209, "architecturally-invalid": 9624,
                "deferred-bitmask": 960, "deferred-bitmask-reject": 56,
                "implicit-setf": 2}
    if counters != expected or failures:
        raise RuntimeError(f"immediate reconciliation {dict(counters)} != {expected}: {failures[:10]}")
    if len(report) != 9682:
        raise RuntimeError("immediate audit rows changed")
    full_hash = verify_report(FULL_REPORT, all_rows)
    rejection_hash = verify_report(REPORT, report)
    with open(FULL_REPORT, newline="", encoding="utf-8") as stream:
        full_stored = list(csv.DictReader(stream))
    if len(full_stored) != len(all_rows) or any(
            tuple(row.values()) != actual for row, actual in zip(full_stored, all_rows)):
        raise RuntimeError("full per-candidate immediate report changed")
    with open(REPORT, newline="", encoding="utf-8") as stream:
        stored = list(csv.DictReader(stream))
    if len(stored) != 9682 or len(by_encoding) != len(stored):
        raise RuntimeError("immediate per-encoding report changed")
    recorded = collections.Counter()
    for row in stored:
        key = row["form"], row["encoding"]
        if key not in by_encoding or tuple(row.values()) != by_encoding[key]:
            raise RuntimeError(f"immediate per-encoding regression: {key}")
        recorded[row["category"]] += 1
    if recorded != {"architecturally-invalid": 9624,
                    "deferred-bitmask-reject": 56, "implicit-setf": 2}:
        raise RuntimeError("immediate audit category count changed")
    print("Immediate audit:", dict(counters), "rows=", len(report),
          "all-candidate-rows=", len(full_stored))
    print("Immutable immediate corpora SHA-256:", full_hash, rejection_hash)
    baseline = [(form["id"], word) for form in forms
                for word in register.sample_words(patterns[form["encoding_id"]],
                                                  register.bit_fields(form, groups, sets))]
    if len(baseline) != 14116:
        raise RuntimeError("register baseline changed")
    seed_results = register.probe_rows(probe, baseline)
    seeds = {}
    for (identifier, word), result in zip(baseline, seed_results):
        if result is not None:
            seeds.setdefault(identifier, word)
    if len(seeds) != 195:
        raise RuntimeError("missing register seeds")
    historical = list(dict.fromkeys(baseline + [
        (form["id"], word) for form in forms
        for word in register.exhaustive_register_words(
            seeds[form["id"]], patterns[form["encoding_id"]],
            register.bit_fields(form, groups, sets))]))
    if len(historical) != 30212:
        raise RuntimeError("register corpus changed")
    results = register.probe_rows(probe, historical)
    with open(HISTORICAL, newline="", encoding="utf-8") as stream:
        stored = list(csv.DictReader(stream))
    if len(stored) != 880:
        raise RuntimeError("historical rejection report changed")
    rejected = {(identifier, word) for (identifier, word), result in
                zip(historical, results) if result is None}
    candidates = set(historical)
    expected_historical = {(row["form"], int(row["encoding"], 16)) for row in stored}
    if len(expected_historical) != 880:
        raise RuntimeError("duplicate historical rejection witness")
    if not expected_historical <= rejected or not expected_historical <= candidates:
        raise RuntimeError("historical witness absent from corpus or accepted")
    historical_llvm = register.llvm_disassemble(
        [int(row["encoding"], 16) for row in stored])
    reasons = collections.Counter()
    for row, llvm in zip(stored, historical_llvm):
        identifier, word = row["form"], int(row["encoding"], 16)
        form = by_id[identifier]
        reason = architectural_reason(form, word)
        if (reason != row["architectural_reason"] or reason == "UNCLASSIFIED" or
                llvm != "<llvm-invalid>" or row["llvm"] != llvm or
                row["probe"] != "REJECT" or row["category"] != "architecturally-invalid" or
                row["group"] != form["family_path"][1] or
                row["operation"] != form["operation_id"] or
                row["fields"] != fields_for(form, word)):
            raise RuntimeError(f"historical witness changed: {identifier} {word:08X}")
        reasons[reason] += 1
    with open(REPORT, newline="", encoding="utf-8") as stream:
        immediate_rows = list(csv.DictReader(stream))
    new_invalid = {(row["form"], int(row["encoding"], 16)) for row in immediate_rows
                   if row["category"] == "architecturally-invalid"}
    overlap = expected_historical & new_invalid
    print("Cross-corpus architectural invalid overlap:", len(overlap),
          "immediate-only=", len(new_invalid - expected_historical),
          "register-only=", len(expected_historical - new_invalid))
    print("Register historical audit:", dict(reasons), "rows=", len(stored))


if __name__ == "__main__":
    try:
        if len(sys.argv) != 2:
            raise ValueError("usage: integer_immediate_rejection_audit.py PROBE")
        main(sys.argv[1])
    except (OSError, ValueError, IndexError, RuntimeError) as error:
        print("Rejection audit failed:", error, file=sys.stderr)
        raise SystemExit(1)