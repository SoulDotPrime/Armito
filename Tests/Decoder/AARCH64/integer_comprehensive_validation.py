import collections
import csv
import hashlib
import io
import json
import sys

import integer_form_differential as differential
import integer_implicit_operands as implicit
import integer_alias_predicates as aliasmod

REPORT = "Tests/Decoder/AARCH64/integer_comprehensive_validation.csv"
REPORT_SHA256 = "3f24c847e36c8df3cfa51db23146c11bcb903dbb6414afccedb21e06fcec2b49"


def historical_text(text):
    if text == "autiasppcr\tx0":
        return "autiasppc\tx0"
    if text == "autibsppcr\tx0":
        return "autibsppc\tx0"
    return text



def operand_summary(operands):
    parts = []
    for name in operands:
        operand = operands[name]
        parts.append(f"{name}:k{operand['kind']}:r{operand['role']}:"
                     f"w{operand['width']}:s{operand['special']}:f{operand['flags']}")
    return " ".join(parts)


def main(probe):
    with open(differential.CATALOG, encoding="utf-8") as stream:
        catalog = json.load(stream)
    patterns = {p["id"]: p for p in catalog["encodings"]}
    groups = {g["id"]: g for g in catalog["groups"]}
    sets = {s["id"]: s for s in catalog["instruction_sets"]}
    conditions = {c["id"]: c["ast"] for c in catalog["conditions"]}
    forms = {f["id"]: f for f in catalog["instruction_forms"]
             if f["family_path"][0] in ("dpimm", "dpreg")}
    aliases = [a for a in catalog["instruction_aliases"] if a["parent_form_id"] in forms]
    operations = {f["operation_id"] for f in forms.values()}
    descriptors = sum(len(f["encoding_fields"]) for f in catalog["instruction_forms"])
    if (len(forms) != 195 or len(operations) != 112 or len(aliases) != 93 or
            len({a["parent_form_id"] for a in aliases}) != 66 or
            len(catalog["instruction_forms"]) != 4296 or
            len(catalog["encodings"]) != 3099 or
            catalog["metrics"]["condition_records"] != 599):
        raise RuntimeError("catalog inventory changed")
    by_form = collections.defaultdict(list)
    for alias in aliases:
        by_form[alias["parent_form_id"]].append(alias)

    baseline = [(form["id"], word) for form in forms.values()
                for word in differential.sample_words(
                    patterns[form["encoding_id"]],
                    differential.bit_fields(form, groups, sets))]
    seed_rows = differential.probe_rows(probe, baseline)
    seeds = {}
    seed_response = {}
    for (identifier, word), result in zip(baseline, seed_rows):
        if result is not None and identifier not in seeds:
            seeds[identifier] = word
            seed_response[identifier] = result
    if len(seeds) != 195:
        raise RuntimeError(f"positive COMPLETE seed missing for "
                           f"{195 - len(seeds)} forms")
    ordered = list(forms.values())
    seed_words = [seeds[form["id"]] for form in ordered]
    seed_llvm = differential.llvm_disassemble(seed_words)

    output = io.StringIO(newline="")
    writer = csv.writer(output)
    writer.writerow(("record_type", "form", "operation", "family", "encoding",
                     "canonical", "alias", "extraction", "operands", "reads",
                     "writes", "implicit_reads", "nzcv_read", "nzcv_write",
                     "carry_read", "cond_read", "partial", "features", "llvm",
                     "result"))
    verified_forms = 0
    operand_kinds = collections.Counter()
    operand_roles = collections.Counter()
    dependency_coverage = collections.Counter()
    feature_coverage = collections.Counter()
    restriction_coverage = collections.Counter()
    failures = []
    for form, word, text in zip(ordered, seed_words, seed_llvm):
        identifier = form["id"]
        canonical, observed, operands = seed_response[identifier]
        status, fields, printed = differential.compare(form, canonical, observed,
                                                       operands, text)
        spec = implicit.contract(form)
        features = ",".join(form.get("feature_identifiers", [])) or "-"
        for feature in form.get("feature_identifiers", []):
            feature_coverage[feature] += 1
        for name in spec["reads"]:
            dependency_coverage[f"read:{name}"] += 1
        for name in spec["writes"]:
            dependency_coverage[f"write:{name}"] += 1
        for name in spec["implicit_reads"]:
            dependency_coverage[f"implicit:{name}"] += 1
        for flag, label in ((spec["nzcv_read"], "nzcv_read"),
                            (spec["nzcv_write"], "nzcv_write"),
                            (spec["carry_read"], "carry_read"),
                            (spec["cond_read"], "cond_read")):
            if flag:
                dependency_coverage[label] += 1
        if spec["partial"]:
            restriction_coverage["partial"] += 1
        if form.get("feature_identifiers"):
            restriction_coverage["feature_gated"] += 1
        for name in operands:
            operand = operands[name]
            operand_kinds[operand["kind"]] += 1
            operand_roles[operand["role"]] += 1
        result = "verified" if status == "verified" else status
        if result != "verified":
            failures.append((identifier, status, text))
        else:
            verified_forms += 1
        writer.writerow(("form", identifier, form["operation_id"],
                         form["family_path"][1], f"{word:08X}", canonical, observed,
                         "COMPLETE", operand_summary(operands),
                         ",".join(sorted(spec["reads"])) or "-",
                         ",".join(sorted(spec["writes"])) or "-",
                         ",".join(sorted(spec["implicit_reads"])) or "-",
                         int(spec["nzcv_read"]), int(spec["nzcv_write"]),
                         int(spec["carry_read"]), int(spec["cond_read"]),
                         int(spec["partial"]), features,
                         historical_text(text), result))
    if verified_forms != 195:
        raise RuntimeError(f"COMPLETE extraction proven for {verified_forms}/195 forms: "
                           f"{failures[:6]}")

    selected = set()
    negative_only = set()
    alias_failures = []
    for form in ordered:
        identifier = form["id"]
        entries = by_form.get(identifier, [])
        if not entries:
            continue
        candidates = aliasmod.candidates(form, patterns[form["encoding_id"]],
                                         groups, sets, conditions)
        responses = differential.probe_rows(
            probe, [(identifier, word) for word in candidates])
        llvm = differential.llvm_disassemble(candidates)
        witnesses = {}
        for word, response, text in zip(candidates, responses, llvm):
            fields = aliasmod.raw_fields(form, groups, sets, word)
            winners = []
            for alias in entries:
                pred = bool(aliasmod.interpret(
                    conditions[alias["condition_id"]], fields))
                pref = bool(aliasmod.interpret(
                    conditions[alias["preferred_condition_id"]], fields))
                if pred and pref:
                    winners.append(alias)
            for alias in winners:
                if alias["id"] not in witnesses:
                    witnesses[alias["id"]] = (word, text, response)
        for alias in entries:
            features = ",".join(form.get("feature_identifiers", [])) or "-"
            if alias["id"] in witnesses:
                word, text, response = witnesses[alias["id"]]
                selected.add(alias["id"])
                observed = response[1] if response is not None else "-"
                writer.writerow(("alias", identifier, form["operation_id"],
                                 form["family_path"][1], f"{word:08X}", "-", alias["id"],
                                 "COMPLETE", "-", "-", "-", "-", 0, 0, 0, 0, 0,
                                 features, historical_text(text),
                                 f"selected:{observed}"))
            else:
                negative_only.add(alias["id"])
                writer.writerow(("alias", identifier, form["operation_id"],
                                 form["family_path"][1], f"{seeds[identifier]:08X}",
                                 "-", alias["id"], "COMPLETE", "-", "-", "-", "-",
                                 0, 0, 0, 0, 0, features, "-", "negative-only"))
    if len(selected) != 92 or negative_only != {"REV_64_dp_1src::alias::REV64"}:
        raise RuntimeError(f"alias coverage changed: selected={len(selected)} "
                           f"negative_only={sorted(negative_only)}")

    data = output.getvalue().encode("utf-8")
    digest = hashlib.sha256(data).hexdigest()
    print("Comprehensive validation", len(forms), "forms", len(operations),
          "operations", len(aliases), "aliases", "descriptors", descriptors)
    print("COMPLETE forms", verified_forms, "aliases selected", len(selected),
          "negative-only", sorted(negative_only))
    print("operand kinds", dict(sorted(operand_kinds.items())),
          "roles", dict(sorted(operand_roles.items())))
    print("dependencies", dict(sorted(dependency_coverage.items())))
    print("restrictions", dict(sorted(restriction_coverage.items())),
          "features", len(feature_coverage))
    print("sha256", digest)
    if REPORT_SHA256 != "PENDING" and digest != REPORT_SHA256:
        raise RuntimeError(f"comprehensive report hash changed: {digest}")
    import os
    if os.environ.get("ARMITO_WRITE_REPORT") == "1":
        with open(REPORT, "wb") as stream:
            stream.write(data)
    else:
        with open(REPORT, "rb") as stream:
            if stream.read() != data:
                raise RuntimeError("comprehensive report mismatch")
    return digest


if __name__ == "__main__":
    try:
        if len(sys.argv) != 2:
            raise ValueError("usage: integer_comprehensive_validation.py PROBE")
        main(sys.argv[1])
    except (RuntimeError, OSError, ValueError, IndexError, KeyError) as error:
        print("Comprehensive validation failed:", error, file=sys.stderr)
        raise SystemExit(1)