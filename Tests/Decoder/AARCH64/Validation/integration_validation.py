from __future__ import annotations

import csv
import hashlib
import json
import pathlib
import subprocess
import sys

CATALOG = pathlib.Path("Decoder/AARCH64/Catalog/AARCH64InstructionCatalog.json")
REPORT = pathlib.Path("Tests/Decoder/AARCH64/Results/integration_validation.csv")
REPORT_SHA256 = "0133254688480534f895dda7c3bf9e94ff9d4335ca73017264dcd2bf73828263"
PC = 0x1000000000004000

FORWARD = frozenset((
    "ldstexclr", "ldstexclp", "ldstexclr_unpriv", "ldstord",
    "comswap", "comswappr", "comswap_unpriv", "comswappr_unpriv",
    "memop", "memop_128", "rcwcomswap", "rcwcomswappr",
    "ldst_gcs", "ldapstl_unscaled", "ldapstl_simd",
    "ldapstl_writeback", "ldiappstilp"))


def phase(form):
    if form["family_path"][1] in FORWARD:
        return "3.5/3.6"
    return "3.4.2"


def preferred_seed(form, patterns):
    pattern = patterns[form["encoding_id"]]
    word = int(pattern["value"], 16)
    group = form["family_path"][1]
    if group == "memcms":
        return word & ~((31 << 5) | (31 << 16)) | (2 << 5) | (1 << 16)
    if form["id"].startswith("RPRFM_"):
        return 0xF8A248DE
    if group == "ldst_regoff":
        return word | ((2 if form["id"].startswith(("LDR_B_", "STR_B_"))
                        else 3) << 13)
    return word & 0xFFFFFFFF


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
        raise RuntimeError("integration probe row count mismatch")
    parsed = []
    for case, line in zip(cases, rows):
        parts = line.split("\t")
        if parts[:2] != [case[0], "%08X" % case[1]]:
            raise RuntimeError("integration probe row identity mismatch")
        kind = parts[2]
        if kind == "OK":
            if len(parts) < 4 or parts[3] != "COMPLETE":
                raise RuntimeError("OK without COMPLETE operands")
            parsed.append((kind, parts))
        elif kind in ("UNSUPPORTED", "INVALID", "NO-OPERANDS"):
            parsed.append((kind,))
        elif kind.startswith("ERROR"):
            parsed.append((kind,))
        else:
            raise RuntimeError("unknown integration result: %r" % kind)
    return parsed


def main():
    args = [a for a in sys.argv[1:] if a != "--write-report"]
    if len(args) != 1:
        raise SystemExit("usage: integration_validation.py PROBE [--write-report]")
    probe = args[0]
    write_report = "--write-report" in sys.argv[1:]

    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    patterns = {p["id"]: p for p in catalog["encodings"]}
    ldst = [form for form in catalog["instruction_forms"]
            if form["family_path"][0] == "ldst"]

    scoped = [f for f in ldst if phase(f) != "3.5/3.6"]
    forwarded = [f for f in ldst if phase(f) == "3.5/3.6"]

    seed_cases = [(f["id"], preferred_seed(f, patterns)) for f in scoped]
    results = probe_rows(probe, seed_cases)

    complete_count = 0
    for form, result in zip(scoped, results):
        if result[0] != "OK":
            raise RuntimeError("COMPLETE form %s not OK: %r" %
                               (form["id"], result))
        complete_count += 1

    fwd_cases = [(f["id"], preferred_seed(f, patterns)) for f in forwarded]
    fwd_results = probe_rows(probe, fwd_cases)
    forwarded_count = 0
    for form, result in zip(forwarded, fwd_results):
        if result[0] != "UNSUPPORTED":
            raise RuntimeError("forwarded form %s not UNSUPPORTED: %r" %
                               (form["id"], result))
        forwarded_count += 1

    rows = []
    for form, case, result in zip(scoped, seed_cases, results):
        rows.append((
            "complete", form["id"], form["operation_id"],
            form["family_path"][1],
            ",".join(form["feature_identifiers"]),
            "seed", "%08X" % case[1], "ok", str(result[0])))
    for form, case, result in zip(forwarded, fwd_cases, fwd_results):
        rows.append((
            "forwarded", form["id"], form["operation_id"],
            form["family_path"][1],
            ",".join(form["feature_identifiers"]),
            "seed", "%08X" % case[1], "unsupported", str(result[0])))

    if write_report:
        REPORT.parent.mkdir(parents=True, exist_ok=True)
        with REPORT.open("w", encoding="utf-8", newline="") as stream:
            writer = csv.writer(stream, lineterminator="\n")
            writer.writerow((
                "class", "form", "operation", "group", "features",
                "variant", "word", "result", "decoder_result"))
            writer.writerows(rows)
    else:
        if not REPORT.is_file():
            raise SystemExit("missing integration report")
        actual = hashlib.sha256(REPORT.read_bytes()).hexdigest()
        if REPORT_SHA256 == "PENDING":
            raise SystemExit("report hash not pinned: %s" % actual)
        if actual != REPORT_SHA256:
            raise SystemExit("integration report hash mismatch")

    print(
        "integration: %d complete, %d forwarded, %d cases" %
        (complete_count, forwarded_count, len(rows)))
    print("integration: all scoped forms OK, all forwarded UNSUPPORTED")


if __name__ == "__main__":
    main()