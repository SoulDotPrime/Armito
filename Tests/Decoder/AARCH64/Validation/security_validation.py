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

CATALOG = "Decoder/AARCH64/Catalog/AARCH64InstructionCatalog.json"
REPORT = "Tests/Decoder/AARCH64/Results/security_validation.csv"
REPORT_SHA256 = "PENDING"
INVENTORY_REPORT = "Tests/Decoder/AARCH64/Results/security_inventory.csv"
INVENTORY_SHA256 = "PENDING"
GROUPS = {"exception", "ldst_pac", "addsub_immtags"}
OPERATIONS = {"ERET", "DRPS", "PACGA", "IRG", "GMI", "SUBP", "SUBPS"}
FEATURES = "+pauth,+pauth-lr,+bti,+mte,+tme,+el3"
EXCEPTIONS = {"SVC": 1, "HVC": 2, "SMC": 3, "BRK": 4, "HLT": 5,
              "DCPS1": 6, "DCPS2": 6, "DCPS3": 6, "TCANCEL": 7,
              "ERET": 8, "DRPS": 9}
SECURITY_FEATURES = ("FEAT_PAuth", "FEAT_FPAC", "FEAT_BTI", "FEAT_MTE",
                     "FEAT_GCS", "FEAT_TME")
PRIOR_REPORTS = (
    ("Tests/Decoder/AARCH64/Results/authenticated_control_flow_validation.csv",
     "seed", "complete"),
    ("Tests/Decoder/AARCH64/Results/integer_comprehensive_validation.csv",
     "form", "COMPLETE"),
    ("Tests/Decoder/AARCH64/Results/specialized_memory_validation.csv",
     "seed", "complete"),
    ("Tests/Decoder/AARCH64/Results/system_validation.csv",
     "seed", "complete"),
)


def security_family(form):
    group = form["family_path"][1]
    name = form["id"].split("_")[0]
    operation = form["operation_id"]
    if group == "ldst_gcs":
        return "gcs-memory"
    if group == "ldst_pac":
        return "authenticated-load"
    if group == "ldsttags" or name == "STGP":
        return "mte-memory"
    if group == "addsub_immtags" or name in ("IRG", "GMI", "SUBP", "SUBPS"):
        return "mte-arithmetic"
    if group == "exception" or operation in ("ERET", "DRPS", "ERETA"):
        return "exception"
    if operation == "RETASPPC_imm" or (group == "branch_reg" and operation in (
            "BRA", "BLRA", "RETA", "RETASPPCR_reg")):
        return "authenticated-control"
    if group == "hints" and name == "BTI":
        return "bti"
    if group == "hints" and name.startswith(("PAC", "AUT", "XPAC")):
        return "pauth-hint"
    if name.startswith(("PAC", "AUT", "XPAC")):
        return "pauth-register"
    if name == "GCSB":
        return "gcs-hint"
    if any(feature.startswith(SECURITY_FEATURES)
           for feature in form.get("feature_identifiers", [])):
        return "related-extension"
    return ""


def prior_evidence():
    evidence = {}
    for path, variant, success in PRIOR_REPORTS:
        with open(path, newline="", encoding="utf-8") as stream:
            for row in csv.DictReader(stream):
                if row.get("variant", row.get("record_type")) != variant:
                    continue
                if (row.get("extraction") if variant == "form" else
                        row.get("result")) != success:
                    continue
                evidence.setdefault(row["form"],
                                    (path, int(row.get("word", row.get("encoding")), 16)))
    return evidence


def write_inventory(catalog, patterns, probe, target_forms):
    evidence = prior_evidence()
    target = {form["id"] for form in target_forms}
    with open(REPORT, newline="", encoding="utf-8") as stream:
        target_rows = list(csv.DictReader(stream))
    target_words = {row["form"]: int(row["word"], 16)
                    for row in target_rows if row["variant"] == "seed"}
    alias_rows = {row["preferred_alias"]: row for row in target_rows
                  if row["variant"] == "alias" and row["state"] == "OK"}
    validated_aliases = {row["preferred_alias"] for row in target_rows
                         if row["variant"] == "alias" and row["state"] == "OK"}
    forms = [form for form in catalog["instruction_forms"]
             if security_family(form)]
    cases = [(form, target_words.get(form["id"],
                    evidence[form["id"]][1] if form["id"] in evidence else
                    int(patterns[form["encoding_id"]]["value"], 16)),
              "inventory", "-") for form in forms]
    results = probe_rows(probe, cases)
    exe = os.environ.get("LLVM_MC_SECURITY", shutil.which("llvm-mc-21") or
                         llvm_toolchain.executable())
    llvm_toolchain.require_qualified(exe)
    llvm_texts = branch.llvm_rows(exe, [(form["id"], word, "inventory", "-")
                                       for form, word, _, _ in cases], FEATURES)
    if len(llvm_texts) != len(cases):
        raise RuntimeError("inventory LLVM result count mismatch")
    feature_cases = [(form, word, "feature-negative", feature)
                     for form, word, _, _ in cases
                     for feature in form.get("feature_identifiers", [])]
    feature_results = probe_rows(probe, feature_cases)
    feature_status = collections.defaultdict(dict)
    for (form, word, _, feature), (state, _, _, _) in zip(
            feature_cases, feature_results):
        feature_status[form["id"]][feature] = state
    aliases = [alias for alias in catalog["instruction_aliases"]
               if alias["parent_form_id"] in {form["id"] for form in forms} or
               alias["name"].startswith(("GCS", "CMPP"))]
    counts = collections.Counter()
    output = io.StringIO(newline="")
    writer = csv.writer(output)
    writer.writerow(("record_type", "instruction_id", "form_id", "mnemonic",
                     "family", "encoding", "features", "classification",
                     "expected_operands", "actual_operands", "expected_status",
                     "actual_status", "validation_source", "outcome",
                     "deferred_phase", "technical_justification",
                     "feature_negative_status"))
    for (form, word, _, _), (state, metadata, operands, alias), llvm_text in zip(
            cases, results, llvm_texts):
        family = security_family(form)
        source = evidence[form["id"]][0] if form["id"] in evidence else "-"
        destination = ""
        if family == "gcs-memory":
            classification = "DEFERRED"
            destination = "3.6"
            reason = "GCS backing-store address and access validation is forwarded"
        elif family == "related-extension" or family == "gcs-hint":
            classification = "NOT_APPLICABLE"
            reason = "Adjacent architectural extension, not a 3.5.2 operand target"
        elif state == "OK" and (form["id"] in target or source != "-"):
            classification = "COMPLETE"
            if form["id"] in target:
                source = REPORT
            reason = "Seed decoded; target or prior family validation supplies evidence"
        elif variant == "feature-negative" and state != "FEATURE_UNAVAILABLE":
            raise RuntimeError(f"feature gate not conclusive: {form['id']} {disabled} {state}")
        elif state == "OK":
            classification = "PARTIAL"
            reason = "Seed decoded but operand-domain evidence is not yet available"
        else:
            classification = "MISSING"
            reason = "Canonical seed does not decode to the expected form"
        gated = feature_status[form["id"]]
        if classification == "COMPLETE" and any(
                status != "FEATURE_UNAVAILABLE" and not (
                    family in ("pauth-hint", "bti") and
                    status == "FEATURE_FALLBACK:HINT_HM_hints")
                for status in gated.values()):
            classification = "PARTIAL"
            reason = "Feature-disabled catalog result is not conclusive"
        elif classification == "COMPLETE" and family in ("pauth-hint", "bti"):
            reason = "Feature-disabled encoding resolves to generic HINT"
        if classification == "COMPLETE" and llvm_text == "<invalid>":
            classification = "PARTIAL"
            reason = "LLVM seed unavailable; independent operand oracle required"
        if classification == "COMPLETE" and (
                not llvm_text or
                llvm_text.split()[0] != form["mnemonic"].lower()):
            classification = "PARTIAL"
            reason = "LLVM canonical mnemonic differs from catalog form"
        if classification == "COMPLETE" and any(
                operand[-1] != 1 or
                (operand[0] == 0 and not operand[7] & 0x80)
                for operand in operands.values()):
            classification = "PARTIAL"
            reason = "Raw or uninterpreted operand in COMPLETE form"
        if family in ("pauth-register", "pauth-hint") and classification == "COMPLETE":
            name = form["mnemonic"]
            if name != "PACM":
                expected_key = (5 if name == "PACGA" else
                                4 if name.startswith(("PACDB", "AUTDB",
                                                      "PACDZB", "AUTDZB")) else
                                3 if name.startswith(("PACDA", "AUTDA",
                                                      "PACDZA", "AUTDZA", "XPACD")) else
                                2 if name.startswith(("PACIB", "AUTIB",
                                                      "PACIZB", "AUTIZB", "PACNBIB")) else 1)
                expected_action = (3 if name.startswith("XPAC") else
                                   2 if name.startswith("AUT") else 1)
                if metadata[:2] != (expected_key, expected_action):
                    classification = "PARTIAL"
                    reason = "PAuth key or action mismatch"
        if (family == "pauth-register" and classification == "COMPLETE" and
                "FEAT_PAuth_LR" in form.get("feature_identifiers", [])):
            classification = "PARTIAL"
            reason = "PAuth_LR implicit PC/SP/X15 modifiers lack validated operand representation"
        if family == "pauth-hint" and classification == "COMPLETE":
            name = form["mnemonic"]
            if name == "PACM":
                if metadata[20] != 1 or metadata[0:2] != (0, 0) or any(
                        operand[7] & 0x100 for operand in operands.values()):
                    classification = "PARTIAL"
                    reason = "PACM PSTATE.PACM metadata mismatch"
                else:
                    reason = "PSTATE.PACM write; Arm A64 PACM description"
            else:
                destination = 17 if name.endswith("1716") else 30
                modifier = 16 if name.endswith("1716") else 31
                implicit = operands.get("implicit_destination")
                source_operand = operands.get("implicit_modifier")
                if (implicit is None or implicit[2] != destination or
                        implicit[8] != 64 or not implicit[7] & 0x100 or
                        (name != "XPACLRI" and
                         (source_operand is None or
                          source_operand[2] != modifier or
                          source_operand[8] != 64 or
                          not source_operand[7] & 0x100 or
                          (modifier == 31 and source_operand[9] != (
                              1 if name.endswith("SP") else 2)))) or
                        (name == "XPACLRI" and source_operand is not None)):
                    classification = "PARTIAL"
                    reason = "PAuth HINT implicit register identity mismatch"
        counts[(family, classification)] += 1
        writer.writerow(("form", form["operation_id"], form["id"],
                         form["mnemonic"], family, f"{word:08X}",
                         " ".join(form.get("feature_identifiers", [])) or "-",
                         classification, llvm_text,
                         " ".join(f"{name}:{':'.join(map(str, values))}"
                                  for name, values in sorted(operands.items()))
                         if state == "OK" else "-",
                         "COMPLETE" if classification == "COMPLETE" else "-",
                         state, source, "verified" if classification == "COMPLETE"
                         else "open", destination, reason,
                         " ".join(f"{feature}:{status}" for feature, status
                                  in sorted(gated.items())) or "-"))
    for alias in aliases:
        family = security_family(next(
            (form for form in forms if form["id"] == alias["parent_form_id"]),
            {"family_path": ["control", "systeminstrs"], "id": alias["name"],
             "operation_id": alias["operation_id"], "feature_identifiers": []}))
        destination = "3.6" if alias["name"].startswith("GCS") else ""
        classification = ("DEFERRED" if destination else "COMPLETE"
                          if alias["name"] in validated_aliases else "PARTIAL")
        counts[("alias", classification)] += 1
        writer.writerow(("alias", alias["operation_id"],
                         alias["parent_form_id"], alias["mnemonic"],
                         family or "gcs-system-alias", "-",
                         " ".join(alias.get("feature_identifiers", [])) or "-",
                         classification, "cmpp x0, x0" if
                         classification == "COMPLETE" else "-",
                         alias_rows[alias["name"]]["operands"] if
                         classification == "COMPLETE" else "-", "COMPLETE" if
                         classification == "COMPLETE" else "-", "OK" if
                         classification == "COMPLETE" else "-",
                         REPORT if classification == "COMPLETE" else "-",
                         "verified" if classification == "COMPLETE" else "open",
                         destination, "Alias-specific predicate and operands need"
                         " independent validation" if classification == "PARTIAL" else
                         "Alias encoding independently assembled with LLVM MC"
                         if classification == "COMPLETE" else
                         "GCS architectural side effects deferred to 3.6", "-"))
    data = output.getvalue().encode("utf-8")
    digest = hashlib.sha256(data).hexdigest()
    print("Security catalog inventory", len(forms), "forms",
          len({form["operation_id"] for form in forms}), "operation IDs",
          len(aliases),
          "aliases", len(feature_cases), "profile feature negatives",
          sum(text != "<invalid>" for text in llvm_texts),
          "LLVM form comparisons",
          dict(sorted(counts.items())), "sha256", digest)
    if INVENTORY_SHA256 != "PENDING" and digest != INVENTORY_SHA256:
        raise RuntimeError("security inventory hash changed")
    if os.environ.get("ARMITO_WRITE_REPORT") == "1":
        with open(INVENTORY_REPORT, "wb") as stream:
            stream.write(data)
    else:
        with open(INVENTORY_REPORT, "rb") as stream:
            if stream.read() != data:
                raise RuntimeError("security inventory mismatch")


def probe_rows(probe, cases):
    source = "".join(f"{form['id']} {word:08X} 1000 {disabled}\n"
                     for form, word, _, disabled in cases)
    process = subprocess.run([probe], input=source, text=True,
                             capture_output=True, check=True)
    lines = process.stdout.splitlines()
    if len(lines) != len(cases):
        raise RuntimeError(f"probe returned {len(lines)}/{len(cases)} cases")
    result = []
    for (form, word, _, _), line in zip(cases, lines):
        parts = line.split("\t")
        if parts[:2] != [form["id"], f"{word:08X}"]:
            raise RuntimeError(f"probe response out of order: {line}")
        if parts[2] != "OK":
            result.append((parts[2], (), {}, "-"))
            continue
        if parts[3] != "COMPLETE" or not parts[4].startswith("SEC:"):
            raise RuntimeError(f"incomplete security result: {line}")
        metadata = tuple(map(int, parts[4][4:].split(":")))
        if len(metadata) != 24:
            raise RuntimeError("security metadata layout changed")
        if not parts[5].startswith("ALIAS:"):
            raise RuntimeError(f"missing alias result: {line}")
        alias = parts[5][6:]
        operands = {}
        for part in parts[6:]:
            name, *values = part.split(":")
            if name in operands:
                raise RuntimeError(f"duplicate operand {form['id']} {name}")
            operands[name] = tuple(map(int, values))
        result.append(("OK", metadata, operands, alias))
    return result


def validate(form, word, metadata, operands):
    name = form["id"].split("_")[0]
    group = form["family_path"][1]
    key, action, context, diversity, tagged, granule, block, checked, zero, \
        kind, min_el, level, el2, el3, immediate, flow, width, base, data, \
        writeback, pacm_write, offset, base_sp, data_zero = metadata
    if kind != EXCEPTIONS.get(name, 0):
        raise RuntimeError(f"exception kind mismatch: {form['id']}")
    if group == "exception":
        if immediate != (word >> 5) & 0xFFFF:
            raise RuntimeError(f"exception immediate mismatch: {form['id']}")
        if (el2, el3, level) != (
                int(name == "HVC"), int(name in ("SMC", "DCPS3")),
                int(name[4]) if name.startswith("DCPS") else 0):
            raise RuntimeError(f"exception level mismatch: {form['id']}")
    if name in ("ERET", "DRPS") and (flow, min_el) != (
            8, int(name == "ERET")):
        raise RuntimeError(f"exception return mismatch: {form['id']}")
    if name == "PACGA" and (key, action) != (5, 1):
        raise RuntimeError("PACGA key/action mismatch")
    if group == "ldst_pac" and (key, action, diversity) != (
            4 if name == "LDRAB" else 3, 7, 1):
        raise RuntimeError(f"authenticated load mismatch: {form['id']}")
    if group == "ldst_pac" and (width, base, data, writeback) != (
            64, (word >> 5) & 31, word & 31, int("64W" in form["id"])):
        raise RuntimeError(f"authenticated load address mismatch: {form['id']}")
    if group == "ldst_pac":
        raw = ((word >> 12) & 0x1FF) | (((word >> 22) & 1) << 9)
        signed = raw - 1024 if raw & 0x200 else raw
        immediate_operand = operands.get("auth_offset")
        if ((offset, base_sp, data_zero) !=
                ((signed * 8) & ((1 << 64) - 1), int(base == 31),
                 int(data == 31)) or
                immediate_operand is None or immediate_operand[3] != raw or
                immediate_operand[4] != signed * 8 or
                immediate_operand[6] != 8 or
                not immediate_operand[7] & 0x10):
            raise RuntimeError(f"authenticated load offset mismatch: {form['id']}")
    if (group == "addsub_immtags" or name in ("IRG", "GMI", "SUBP", "SUBPS")) \
            and (tagged, granule) != (1, 16):
        raise RuntimeError(f"MTE metadata mismatch: {form['id']}")
    if group == "addsub_immtags":
        raw6 = (word >> 16) & 63
        raw4 = (word >> 10) & 15
        imm6 = operands.get("imm6")
        imm4 = operands.get("imm4")
        if (imm6 is None or imm4 is None or imm6[3] != raw6 or
                imm6[2] != raw6 << 4 or imm6[6] != 4 or
                imm4[3] != raw4):
            raise RuntimeError(f"MTE tag immediate mismatch: {form['id']}")
    if any(op[-1] != 1 or (op[0] == 1 and op[8] != 64)
           for op in operands.values()):
        raise RuntimeError(f"incomplete or invalid operands: {form['id']}")
    if not operands and name not in ("ERET", "DRPS"):
        raise RuntimeError(f"missing operands: {form['id']}")


def main(probe):
    with open(CATALOG, encoding="utf-8") as stream:
        catalog = json.load(stream)
    patterns = {p["id"]: p for p in catalog["encodings"]}
    forms = [f for f in catalog["instruction_forms"] if
             f["family_path"][1] in GROUPS or f["operation_id"] in OPERATIONS]
    if len(forms) != 22:
        raise RuntimeError(f"security inventory changed: {len(forms)}")
    cases = []
    for form in forms:
        pattern = patterns[form["encoding_id"]]
        word = int(pattern["value"], 16)
        cases.append((form, word, "seed", "-"))
        mask = int(pattern["mask"], 16)
        for bit in range(32):
            if mask & (1 << bit):
                cases.append((form, word ^ (1 << bit), "fixed-bit", "-"))
        for feature in form.get("feature_identifiers", []):
            cases.append((form, word, "feature-negative", feature))
        group = form["family_path"][1]
        if group in ("ldst_pac", "addsub_immtags", "dp_2src"):
            fields = (("Rt" if group == "ldst_pac" else "Rd", 0),
                      ("Rn", 5))
            if group == "dp_2src":
                fields += (("Rm", 16),)
            for name, shift in fields:
                field_mask = 31 << shift
                if not mask & field_mask:
                    for value in (1, 30, 31):
                        cases.append((form, (word & ~field_mask) | (value << shift),
                                      f"register-{name}-{value}", "-"))
        if group == "exception" and not mask & (0xFFFF << 5):
            for value in (1, 0x7FFF, 0xFFFF):
                cases.append((form, (word & ~(0xFFFF << 5)) | (value << 5),
                              f"immediate-imm16-{value}", "-"))
        if group == "ldst_pac" and not mask & (0x1FF << 12):
            for raw in (1, 255, 511):
                cases.append((form, (word & ~(0x1FF << 12)) | (raw << 12),
                              f"immediate-auth-{raw}", "-"))
            if not mask & (1 << 22):
                for raw in (512, 1023):
                    cases.append((form, (word & ~((0x1FF << 12) | (1 << 22))) |
                                  ((raw & 511) << 12) | ((raw >> 9) << 22),
                                  f"immediate-auth-{raw}", "-"))
        if group == "addsub_immtags":
            if not mask & (15 << 10):
                for tag in range(16):
                    cases.append((form, (word & ~(15 << 10)) | (tag << 10),
                                  f"tag-domain-{tag}", "-"))
            if not mask & (63 << 16):
                for raw in (1, 31, 63):
                    cases.append((form, (word & ~(63 << 16)) | (raw << 16),
                                  f"tag-offset-{raw}", "-"))
        cases.append((form, word, "repeat", "-"))
        if form["id"] == "SUBPS_64S_dp_2src":
            cases.append((form, word | 31, "alias", "-"))
    cases.append((forms[0], 0, "reserved", "-"))
    bti = next(form for form in catalog["instruction_forms"]
               if form["id"] == "BTI_HB_hints")
    for name, op2 in (("c", 2), ("j", 4), ("jc", 6)):
        cases.append((bti, 0xD503241F | (op2 << 5), f"bti-{name}", "-"))
    for op2 in (1, 3, 5, 7):
        cases.append((bti, 0xD503241F | (op2 << 5),
                      f"bti-domain-{op2}", "-"))
    results = probe_rows(probe, cases)
    exe = os.environ.get("LLVM_MC_SECURITY", shutil.which("llvm-mc-21") or
                         llvm_toolchain.executable())
    llvm_toolchain.require_qualified(exe)
    seeds = [case for case in cases if case[2] in ("seed", "alias") or
             case[2].startswith(("bti-", "register-", "immediate-", "tag-"))]
    llvm = branch.llvm_rows(exe, [(f["id"], w, v, d)
                                  for f, w, v, d in seeds], FEATURES)
    if len(llvm) != len(seeds):
        raise RuntimeError("LLVM seed count mismatch")
    llvm_by_case = {(case[0]["id"], case[2]): text
                    for case, text in zip(seeds, llvm)}
    counts = collections.Counter()
    initial = {}
    output = io.StringIO(newline="")
    writer = csv.writer(output)
    writer.writerow(("form", "operation", "group", "variant", "word",
                     "feature", "state", "key", "action", "diversity",
                     "memory_tagged", "granule", "exception_kind",
                     "level", "immediate", "flow", "memory_width",
                     "memory_base", "memory_data", "writeback", "pacm_write",
                     "offset", "base_sp", "data_zero",
                     "operands", "preferred_alias", "llvm"))
    for (form, word, variant, disabled), (state, metadata, operands, alias) in zip(
            cases, results):
        if variant == "seed":
            initial[form["id"]] = (state, metadata, operands, alias)
        if variant == "repeat" and (
                state, metadata, operands, alias) != initial[form["id"]]:
            raise RuntimeError(f"nondeterministic decode: {form['id']}")
        if variant == "reserved" and state != "RESERVED":
            raise RuntimeError(f"permanently undefined encoding accepted: {state}")
        if variant.startswith("bti-domain-"):
            text = llvm_by_case[(form["id"], variant)]
            if state == "OK" or (text != "<invalid>" and
                                 text.split()[0] == "bti"):
                raise RuntimeError(f"invalid BTI class accepted: {variant}")
        elif variant in ("seed", "alias") or variant.startswith((
                "bti-", "register-", "immediate-", "tag-")):
            text = llvm_by_case[(form["id"], variant)]
            if text == "<invalid>" and variant != "seed":
                if state == "OK":
                    raise RuntimeError(f"LLVM rejects decoded variant: {form['id']} {word:08X}")
            else:
                if state != "OK":
                    raise RuntimeError(f"LLVM-valid variant not COMPLETE: {form['id']} {state}")
                validate(form, word, metadata, operands)
                if variant.startswith("register-"):
                    _, name, number = variant.split("-")
                    operand = operands.get(name)
                    if (operand is None or operand[3] != int(number) or
                            operand[8] != 64):
                        raise RuntimeError(f"register identity mismatch: {form['id']} {variant}")
                    if int(number) == 31:
                        expected_special = None
                        if form["family_path"][1] == "addsub_immtags" and name in ("Rd", "Rn"):
                            expected_special = 1
                        if form["family_path"][1] == "ldst_pac":
                            expected_special = 1 if name == "Rn" else 2
                        if form["operation_id"] == "PACGA":
                            expected_special = 1 if name == "Rm" else 2
                        if expected_special is not None and operand[9] != expected_special:
                            raise RuntimeError(f"SP/ZR mismatch: {form['id']} {variant}")
                if variant.startswith(("tag-domain-", "tag-offset-")):
                    name = "imm4" if variant.startswith("tag-domain-") else "imm6"
                    expected = int(variant.rsplit("-", 1)[1])
                    operand = operands.get(name)
                    if operand is None or operand[3] != expected:
                        raise RuntimeError(f"MTE tag domain mismatch: {form['id']} {variant}")
            mnemonic = "cmpp" if variant == "alias" else form["mnemonic"].lower()
            if variant == "alias" and alias != "CMPP":
                raise RuntimeError(f"CMPP preferred alias mismatch: {alias}")
            if variant == "alias":
                assembled = subprocess.run(
                    [exe, "-triple=aarch64", "-mattr=+mte", "-show-encoding"],
                    input="cmpp x0, x0\n", text=True, capture_output=True,
                    check=True).stdout
                encoded = "[" + ",".join(f"0x{value:02x}"
                    for value in word.to_bytes(4, "little")) + "]"
                if encoded not in assembled or text.split()[0] != "subps":
                    raise RuntimeError(f"LLVM CMPP alias encoding mismatch: {assembled}")
            elif text != "<invalid>" and text.split()[0] != mnemonic:
                raise RuntimeError(f"LLVM mnemonic mismatch {form['id']}: {text}")
            if variant in ("bti-c", "bti-j", "bti-jc") and text.strip() != (
                    "bti\t" + variant[4:]):
                raise RuntimeError(f"BTI class mismatch: {variant} {text}")
            if variant == "seed" and text == "<invalid>":
                raise RuntimeError(f"LLVM rejects seed: {form['id']}")
        elif variant == "feature-negative" and state != "FEATURE_UNAVAILABLE":
            raise RuntimeError(f"feature gate not conclusive: {form['id']} {disabled} {state}")
        elif variant not in ("repeat", "reserved") and state == "OK":
            raise RuntimeError(f"unexpected form acceptance: {form['id']} {word:08X}")
        counts[(variant, state)] += 1
        fields = metadata if metadata else (0,) * 24
        writer.writerow((form["id"], form["operation_id"],
                         form["family_path"][1], variant, f"{word:08X}",
                         disabled, state, fields[0], fields[1], fields[3],
                         fields[4], fields[5], fields[9], fields[11],
                         fields[14], fields[15], fields[16], fields[17],
                         fields[18], fields[19], fields[20], fields[21],
                         fields[22], fields[23],
                         " ".join(f"{name}:k{op[0]}:w{op[8]}"
                                  for name, op in sorted(operands.items()))
                          if variant in ("seed", "alias") or
                          variant.startswith(("bti-", "register-", "immediate-", "tag-"))
                          else "-",
                          alias,
                          llvm_by_case[(form["id"], variant)]
                          if variant in ("seed", "alias") or
                          variant.startswith(("bti-", "register-", "immediate-", "tag-"))
                          else "-"))
    data = output.getvalue().encode("utf-8")
    digest = hashlib.sha256(data).hexdigest()
    print("Security validation", len(forms), "target forms", len(seeds),
          "positive cases", len(cases),
          "cases", len(llvm) + 1, "LLVM comparisons", dict(counts),
          "sha256", digest)
    if REPORT_SHA256 != "PENDING" and digest != REPORT_SHA256:
        raise RuntimeError("security report hash changed")
    if os.environ.get("ARMITO_WRITE_REPORT") == "1":
        with open(REPORT, "wb") as stream:
            stream.write(data)
    else:
        with open(REPORT, "rb") as stream:
            if stream.read() != data:
                raise RuntimeError("security report mismatch")
    write_inventory(catalog, patterns, probe, forms)


if __name__ == "__main__":
    try:
        main(sys.argv[1])
    except (IndexError, RuntimeError, OSError, ValueError,
            subprocess.CalledProcessError) as error:
        print("Security audit failed:", error, file=sys.stderr)
        raise SystemExit(1)
