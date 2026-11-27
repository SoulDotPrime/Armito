import collections
import csv
import hashlib
import io
import json
import os
import sys

import integer_form_differential as differential

REPORT = "Tests/Decoder/AARCH64/integer_invalid_encodings.csv"
REPORT_SHA256 = "550ba37f5e60486ee7ad1735403bc45d9d95c83f7d1353c6f923fcbb367e1d8f"


def classify(form, word, fields, fixed_mask, fixed_value):
    if (word & fixed_mask) != (fixed_value & fixed_mask):
        return "undefined"
    return "valid"


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
    baseline = [(form["id"], word) for form in forms.values()
                for word in differential.sample_words(
                    patterns[form["encoding_id"]],
                    differential.bit_fields(form, groups, sets))]
    seed_rows = differential.probe_rows(probe, baseline)
    seeds = {}
    for (identifier, word), result in zip(baseline, seed_rows):
        if result is not None:
            seeds.setdefault(identifier, word)
    if len(seeds) != 195:
        raise RuntimeError(f"positive distinguishing seed missing for "
                           f"{195 - len(seeds)} forms")
    requests = []
    meta = []
    for form in forms.values():
        pattern = patterns[form["encoding_id"]]
        seed = seeds[form["id"]]
        fixed_mask = int(pattern["mask"], 16)
        fields = differential.bit_fields(form, groups, sets)
        variants = [("seed", seed, "valid")]
        for index in range(32):
            if (fixed_mask >> index) & 1:
                variants.append((f"flip{index}", seed ^ (1 << index), "undefined"))
        for name, word, expect in variants:
            requests.append((form["id"], word))
            meta.append((form, word, name, expect, fixed_mask, seed, fields))
    responses = differential.probe_rows(probe, requests)
    disassembly = differential.llvm_disassemble([word for _, word in requests])
    output = io.StringIO(newline="")
    writer = csv.writer(output)
    writer.writerow(("form", "operation", "family", "variant", "encoding", "fixed_mask",
                     "expected", "probe", "llvm", "feature", "feature_gated",
                     "guest_has_feature", "result"))
    counts = collections.Counter()
    failures = []
    coverage = collections.defaultdict(lambda: collections.defaultdict(set))
    for (form, word, name, expect, fixed_mask, seed, fields), response, text in zip(
            meta, responses, disassembly):
        features = ",".join(form.get("feature_identifiers", [])) or "-"
        feature_gated = int(bool(form.get("feature_identifiers")))
        guest_has_feature = 1
        probe_state = "reject" if response is None else "accept"
        llvm_state = "invalid" if text == "<llvm-invalid>" else "valid"
        state = "verified"
        if expect == "undefined":
            if probe_state != "reject":
                state = "undue-accept"
        else:
            if probe_state != "accept":
                state = "runtime-partial"
        if state == "undue-accept":
            failures.append((form["id"], name, f"{word:08X}", state, probe_state, llvm_state))
        counts[(form["id"], name.split("flip")[0] if name.startswith("flip") else "seed", state)] += 1
        coverage[form["id"]][name].add(state)
        writer.writerow((form["id"], form["operation_id"], form["family_path"][1], name,
                         f"{word:08X}", f"{fixed_mask:08X}", expect, probe_state,
                         llvm_state, features, feature_gated, guest_has_feature, state))
    data = output.getvalue().encode("utf-8")
    digest = hashlib.sha256(data).hexdigest()
    states = collections.Counter(key[2] for key in counts if isinstance(key, tuple))
    print("Invalid encodings", len(requests), "cases", len(forms), "forms",
          "states", dict(states), "sha256", digest)
    if REPORT_SHA256 != "PENDING" and digest != REPORT_SHA256:
        raise RuntimeError(f"invalid report hash changed: {digest}")
    if os.environ.get("ARMITO_WRITE_REPORT") == "1":
        with open(REPORT, "wb") as stream:
            stream.write(data)
    else:
        with open(REPORT, "rb") as stream:
            if stream.read() != data:
                raise RuntimeError("invalid report mismatch")
    return digest, failures


if __name__ == "__main__":
    try:
        if len(sys.argv) != 2:
            raise ValueError("usage: integer_invalid_encodings.py PROBE")
        main(sys.argv[1])
    except (RuntimeError, OSError, ValueError, IndexError, KeyError) as error:
        print("Invalid encoding audit failed:", error, file=sys.stderr)
        raise SystemExit(1)