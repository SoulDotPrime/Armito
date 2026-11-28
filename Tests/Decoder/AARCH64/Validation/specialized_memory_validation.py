import collections
import csv
import hashlib
import io
import json
import os
import re
import shutil
import subprocess

import control_flow_validation as branch
import integer_form_differential as differential
import llvm_toolchain

CATALOG = "Decoder/AARCH64/Catalog/AARCH64InstructionCatalog.json"
REPORT = "Tests/Decoder/AARCH64/Results/specialized_memory_validation.csv"
REPORT_SHA256 = "2979f0373dbfe08c89959fdcec602dedb9c0eaaf340f93ccdd93d455d7887285"
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
    "memory_operand_validation.csv": "313b773c73ff173519effcb6513dc2abf0b53a38f6c5071bf94a632770967606",
}


def verify_pinned():
    root = "Tests/Decoder/AARCH64/Results/"
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
    process = subprocess.run([probe, "--specialized"], input=source, text=True,
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
        if parts[2] != "COMPLETE" or not parts[4].startswith("MX:") or \
                not parts[5].startswith("SM:") or \
                not parts[6].startswith("ALIAS:"):
            raise RuntimeError("unexpected probe: " + line[:120])
        mem = tuple(map(int, parts[3][4:].split(":")))
        mx = tuple(map(int, parts[4][3:].split(":")))
        special = tuple(map(int, parts[5][3:].split(":")))
        if len(mem) != 15 or len(mx) != 19 or len(special) != 33:
            raise RuntimeError("memory metadata layout changed")
        operands = {}
        for item in parts[7:]:
            data = item.split(":")
            operands[data[0]] = tuple(map(int, data[1:]))
        results.append((mem, mx, special, parts[6][6:], operands))
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


def fields_for(form, groups, sets, encoding):
    fields = differential.bit_fields(form, groups, sets)
    return {name: (encoding >> field["lsb"]) & ((1 << field["width"]) - 1)
            for name, field in fields.items()}


def sign_extend(value, bits):
    return value - (1 << bits) if value & (1 << (bits - 1)) else value


def validate(form, encoding, result, groups, sets):
    mem, mx, special, alias, operands = result
    fid = form["id"]
    group = form["family_path"][1]
    raw = fields_for(form, groups, sets, encoding)
    name = form["operation_id"]
    for field, operand in operands.items():
        if operand[2] != 1 or (operand[0] == 0 and not operand[8] & 0x80):
            raise RuntimeError(f"uninterpreted {fid} {encoding:08X} {field}")
        if operand[0] in (1, 2, 3, 4, 5) and not operand[9]:
            raise RuntimeError(f"unknown architectural register width {fid} {field}")
    if (mx[12] != 42 or special[8:11] != (255, 0, 0) or
            mem[4] != raw.get("Rn", 255) and group != "memcms"):
        raise RuntimeError(f"specialized scope, VL/SVL, or base mismatch: {fid}")
    if "Rn" in operands and (operands["Rn"][0], operands["Rn"][9]) != (1, 64):
        raise RuntimeError(f"incorrect base register: {fid}")
    if group in ("asisdlse", "asisdlsep", "asisdlso", "asisdlsop"):
        count = int(name[2]) if name[2] in "234" else 1
        match = re.search(r"_[RI]([234])_", fid)
        if match:
            count = int(match.group(1))
        q = raw.get("Q", 0)
        size = raw["size"]
        vector = 64 << q
        element = 8 << size
        lane_group = group in ("asisdlso", "asisdlsop")
        replicate = False
        lane = 255
        if lane_group:
            scale = (raw["opcode"] >> 1) & 3
            replicate = scale == 3
            element = 8 << (size if replicate else scale)
            if not replicate:
                lane = ((q << 3) | (raw.get("S", 0) << 2) | size) if scale == 0 else (
                    (q << 2) | (raw.get("S", 0) << 1) | (size >> 1)) if scale == 1 else (
                    (q << 1) | raw.get("S", 0)) if size & 1 == 0 else q
        total = count * (element // 8 if lane_group else vector // 8)
        expected_kind = 4 if replicate else 3 if lane_group else 2
        expected_elements = count if lane_group else count * vector // element
        if (special[0], special[1], special[2], special[3],
                special[4], special[5], special[6], special[7],
                special[23], special[28], mem[0], mem[1],
                mx[0], mx[1], mx[2], mx[3]) != (
                expected_kind, 2, raw["Rt"],
                int(name[2]) if name[2] in "234" else 1,
                int(name[2]) if name[2] in "234" else 1,
                int(replicate), int(lane_group and not replicate and name[0] == "L"),
                int(lane_group and not replicate), 1, total,
                element, expected_elements, element, vector, lane, count):
            raise RuntimeError(f"AdvSIMD structure/lane mismatch {fid} {encoding:08X}")
        if (operands["Rt"][0], operands["Rt"][9]) != (2, vector):
            raise RuntimeError(f"AdvSIMD Vt bank/width mismatch {fid}")
        if group in ("asisdlsep", "asisdlsop"):
            expected_offset = total if raw["Rm"] == 31 else 0
            if (mem[3], mem[14], mx[13], special[22], special[29]) != (
                    2, expected_offset, 255 if raw["Rm"] == 31 else raw["Rm"],
                    1 if raw["Rm"] == 31 else 2,
                    total if raw["Rm"] == 31 else 0):
                raise RuntimeError(f"AdvSIMD post-index mismatch {fid}")
        if mem[5] != 255 or mem[6] != 255 or mx[17] or mx[18]:
            raise RuntimeError(f"AdvSIMD vector treated as GPR {fid}")
    elif group == "memcms":
        copy = fid.startswith("CPY")
        stage = raw.get("op1", 0) + 1 if copy else (raw.get("op2", 0) >> 2) + 1
        options = raw.get("op2", 0) if copy else raw.get("op2", 0) & 3
        if (special[0], special[11:18], special[22], special[24],
                special[27], special[28], special[31], special[32],
                mem[1:5], mx[3]) != (
                6 if copy else 7,
                (stage, raw["Rs"], raw["Rd"], raw["Rn"],
                 int(stage != 1), int(stage == 1), 1), 4, options,
                3 if copy else 2, 0, int(raw["Rd"] == 31),
                int(raw["Rs"] == 31),
                (0, 4 if copy else 2, 3, 255), 0):
            raise RuntimeError(f"MOPS stage/state mismatch {fid} {encoding:08X}")
        if any(operands[register][9] != 64 for register in ("Rd", "Rn", "Rs")):
            raise RuntimeError(f"MOPS register width mismatch {fid}")
        expected_nontemporal = bool(raw.get("op2", 0) &
                                    (12 if copy else 2))
        if mx[5] != int(expected_nontemporal) or mx[17] != int(len({
                raw["Rd"], raw["Rn"], raw["Rs"]}) != 3):
            raise RuntimeError(f"MOPS option/register overlap mismatch {fid}")
    elif group == "memop_unpriv":
        bits = 32 << raw["sz"]
        acquired = int(raw["A"] != 0 and raw["Rt"] != 31)
        if (special[0], special[26], mem[0], mem[2], mx[6:11],
                operands["Rt"][9], operands["Rs"][9]) != (
                9, int(raw["Rt"] != 31), bits, 4,
                (1, acquired, int(raw["R"] != 0), 0, 1), bits, bits):
            raise RuntimeError(f"LSUI RMW/ordering mismatch {fid} {encoding:08X}")
    elif group == "ldsttags" or name == "STGP":
        bulk = name in ("LDGM", "STGM", "STZGM")
        if name == "STGP":
            if (special[0], special[19], mx[11], mem[0], mem[1],
                    special[28], operands["Rt"][9], operands["Rt2"][9],
                    mem[14]) != (
                    8, 16, 1, 128, 2, 16, 64, 64,
                    sign_extend(raw["simm7"], 7) * 16 & ((1 << 64) - 1)):
                raise RuntimeError(f"MTE tag/data pair mismatch {fid}")
            return alias
        if (special[0], special[19], special[20], special[17],
                special[18], special[25], mx[11], operands["Rt"][9]) != (
                8, 16, int(bulk), int(bulk), int(bulk),
                int(name.startswith(("STZG", "STZ2G"))), 1, 64):
            raise RuntimeError(f"MTE tag dependency mismatch {fid}")
        if "imm9" in raw and mem[14] != sign_extend(raw["imm9"], 9) * 16 & ((1 << 64) - 1):
            raise RuntimeError(f"MTE tag offset mismatch {fid}")
    elif fid.startswith(("RPRFM_", "PRFUM_")):
        if (special[0], mem[0], mem[1], mem[2], mem[5],
                operands["Rt"][0]) != (10, 0, 0, 3, 255, 6):
            raise RuntimeError(f"RPRFM prefetch mismatch {fid}")
        if fid.startswith("RPRFM_") and operands["Rt"][3] != (raw["Rt"] & 7):
            raise RuntimeError(f"RPRFM prefetch operation mismatch {fid}")
    elif special[0] == 1:
        width_name = fid.split("_")[1][0]
        width = {"B": 8, "H": 16, "S": 32, "D": 64, "Q": 128}[width_name]
        pair = "Rt2" in operands
        if (special[1], mem[0], mx[0], mx[1], special[28],
                operands["Rt"][0], operands["Rt"][9], mem[5]) != (
                2, width, width, width, width // 8 * (2 if pair else 1),
                2, width, 255):
            raise RuntimeError(f"FP register/access width mismatch {fid}")
        if pair and (operands["Rt2"][0], operands["Rt2"][9]) != (2, width):
            raise RuntimeError(f"FP pair register mismatch {fid}")
    elif special[0] == 5:
        if not mx[6] or operands["Rt"][0] != 1:
            raise RuntimeError(f"unprivileged register bank mismatch {fid}")
    else:
        raise RuntimeError(f"unclassified specialized semantic group {fid} {group}")
    if group in ("ldst_unscaled", "ldst_unpriv", "ldst_immpre",
                 "ldst_immpost") and "imm9" in raw:
        if mem[14] != sign_extend(raw["imm9"], 9) & ((1 << 64) - 1):
            raise RuntimeError(f"unscaled/post-index offset mismatch {fid}")
    if group in ("ldstpair_off", "ldstpair_post", "ldstpair_pre",
                 "ldstnapair_offs") and "imm7" in raw:
        if mem[14] != sign_extend(raw["imm7"], 7) * (mem[0] // 8) & ((1 << 64) - 1):
            raise RuntimeError(f"pair offset scale mismatch {fid}")
    if group == "ldst_pos" and "imm12" in raw:
        if mem[14] != raw["imm12"] * (mem[0] // 8):
            raise RuntimeError(f"unsigned offset scale mismatch {fid}")
    if group == "loadlit":
        offset = sign_extend(raw["imm19"], 19) * 4
        if (mem[14], special[30], mx[16]) != (
                offset & ((1 << 64) - 1), (PC + offset) & ((1 << 64) - 1), 1):
            raise RuntimeError(f"PC-relative literal mismatch {fid}")
    if group == "ldst_regoff" and "Rm" in operands:
        width = (64 if fid.startswith("RPRFM_") or
                 raw["option"] & 1 else 32)
        if (operands["Rm"][9], mx[13]) != (width, raw["Rm"]):
            raise RuntimeError(f"register offset width/index mismatch {fid}")
    if group in ("ldstpair_pre", "ldstpair_post") and mem[3] != (
            1 if group == "ldstpair_pre" else 2):
        raise RuntimeError(f"pair writeback mismatch {fid}")
    return alias


def preferred_seed(form, pattern):
    word = int(pattern["value"], 16)
    group = form["family_path"][1]
    if group == "memcms":
        return word & ~((31 << 5) | (31 << 16)) | (2 << 5) | (1 << 16)
    if form["id"].startswith("RPRFM_"):
        return 0xF8A248DE
    if group == "ldst_regoff":
        return word | ((2 if form["id"].startswith(("LDR_B_", "STR_B_"))
                        else 3) << 13)
    return word


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
        gpr = "32" in fid or "64" in fid or fid.startswith("PRFM_")
        if (form["family_path"][1] in CONVENTIONAL and gpr and
                not form.get("feature_identifiers")):
            conv.append(form)
        else:
            specialized.append(form)
    if len(ldst) != 996 or len(conv) != 94 or len(specialized) != 902:
        raise RuntimeError("996-form reconciliation changed")
    scoped = [form for form in specialized if phase(form) == "3.4.2"]
    forwarded = [form for form in specialized if phase(form) != "3.4.2"]
    if len(scoped) != 433 or len(forwarded) != 469:
        raise RuntimeError("specialized/forwarded inventory changed")
    by_id = {form["id"]: form for form in scoped}
    seed_cases = [(form["id"], preferred_seed(form, patterns[form["encoding_id"]]),
                   "seed", "-") for form in scoped]
    seed_results = probe_rows(probe, seed_cases)
    if any(result is None for result in seed_results) or \
            len({case[1] for case in seed_cases}) != 433:
        raise RuntimeError("433 distinct COMPLETE seeds not demonstrated")
    exe = os.environ.get("LLVM_MC_CONTROL", shutil.which("llvm-mc-21") or
                         llvm_toolchain.executable())
    _, version = llvm_toolchain.require_qualified(exe)
    if version != "21.1.8":
        raise RuntimeError("qualified LLVM MC 21.1.8 required")
    reference = branch.llvm_rows(exe, seed_cases, FEATURES)
    seeds = {}
    for case, result, text in zip(seed_cases, seed_results, reference):
        fid, word, _, _ = case
        if text == "<invalid>" or text.split()[0] != by_id[fid]["mnemonic"].lower():
            raise RuntimeError(f"LLVM form identity mismatch {fid} {word:08X}: {text}")
        validate(by_id[fid], word, result, groups, sets)
        seeds[fid] = word
    cases = []
    parents = {a["parent_form_id"]: a for a in catalog["instruction_aliases"]
               if a["parent_form_id"] in seeds}
    for form in scoped:
        fid = form["id"]
        word = seeds[fid]
        pattern = patterns[form["encoding_id"]]
        mask = int(pattern["mask"], 16)
        fields = differential.bit_fields(form, groups, sets)
        cases.append((fid, word, "seed", "-"))
        for name in ("Rt", "Rn", "Rm", "Rs", "Rd"):
            if name not in fields or fields[name]["width"] != 5:
                continue
            lsb = fields[name]["lsb"]
            values = (0, 1, 29, 31) if name == "Rt" else (0, 1, 31)
            for value in values:
                candidate = (word & ~(31 << lsb)) | value << lsb
                if (candidate & mask) == (word & mask):
                    cases.append((fid, candidate, "register-" + name, "-"))
        if fid in parents and "Rt" in fields:
            lsb = fields["Rt"]["lsb"]
            cases.append((fid, (word & ~(31 << lsb)) | 31 << lsb,
                          "alias", "-"))
        if form["family_path"][1] == "memcms":
            fields_to_vary = ("op2",)
        elif form["family_path"][1] in (
                "asisdlse", "asisdlsep", "asisdlso", "asisdlsop"):
            fields_to_vary = ("Q", "size", "S")
        else:
            fields_to_vary = ()
        for name in fields_to_vary:
            if name not in fields or fields[name]["width"] > 4:
                continue
            lsb = fields[name]["lsb"]
            bits = (1 << fields[name]["width"]) - 1
            for value in range(bits + 1):
                candidate = (word & ~(bits << lsb)) | value << lsb
                if (candidate & mask) == (word & mask):
                    cases.append((fid, candidate, "domain-" + name, "-"))
        for name in ("imm9", "imm7", "simm7", "imm12", "imm19"):
            if name not in fields:
                continue
            lsb = fields[name]["lsb"]
            bits = fields[name]["width"]
            if bits in (7, 9):
                values = range(1 << bits)
            else:
                values = (0, 1, (1 << (bits - 1)) - 1,
                          1 << (bits - 1), (1 << bits) - 1)
            field_mask = ((1 << bits) - 1) << lsb
            for value in values:
                candidate = (word & ~field_mask) | value << lsb
                if (candidate & mask) == (word & mask):
                    cases.append((fid, candidate, "offset-" + name, "-"))
        for index in range(32):
            if mask & (1 << index):
                cases.append((fid, word ^ (1 << index),
                              "fixed-bit-negative", "-"))
        for feature in form.get("feature_identifiers", []):
            cases.append((fid, word, "feature-negative", feature))
    cases = list(dict.fromkeys(cases))
    results = probe_rows(probe, cases)
    reference_cases = []
    fixed_seen = collections.Counter()
    for case, result in zip(cases, results):
        fid, word, variant, _ = case
        form = by_id[fid]
        raw = fields_for(form, groups, sets, word)
        reference_needed = variant in ("seed", "alias") or \
            variant.startswith("domain-") or \
            result is None and variant != "feature-negative"
        if variant.startswith("register-"):
            register = variant[9:]
            reference_needed = result is None or raw.get(register) in (0, 31)
        if variant.startswith("offset-"):
            field = variant[7:]
            bits = differential.bit_fields(form, groups, sets)[field]["width"]
            reference_needed = result is None or raw[field] in (
                0, 1, 1 << (bits - 1), (1 << bits) - 1)
        if variant == "fixed-bit-negative":
            reference_needed = fixed_seen[fid] < 2
            fixed_seen[fid] += 1
        if reference_needed:
            reference_cases.append(case)
    reference_text = branch.llvm_rows(exe, reference_cases, FEATURES)
    reference = dict(zip(reference_cases, reference_text))
    output = io.StringIO(newline="")
    writer = csv.writer(output)
    writer.writerow(("class", "form", "operation", "group", "features",
                     "variant", "word", "result", "memory_metadata",
                     "address_metadata", "symbolic_metadata", "alias",
                     "operand_evidence", "reference"))
    counts = collections.Counter()
    complete_scoped = set()
    selected_aliases = set()
    for case, result in zip(cases, results):
        fid, word, variant, disabled = case
        form = by_id[fid]
        text = reference.get(case, "-")
        if variant in ("fixed-bit-negative", "feature-negative"):
            if result is not None:
                raise RuntimeError(f"undue form acceptance {fid} {word:08X}")
            state = "rejected-by-form"
        elif result is None:
            if text == "-":
                raise RuntimeError(f"unqualified positive rejection {fid} {word:08X}")
            state = "llvm-invalid" if text == "<invalid>" else "sibling-form"
        else:
            alias = validate(form, word, result, groups, sets)
            if text == "<invalid>":
                if (form["family_path"][1] != "memcms" or
                    not (result[1][17] or result[2][31] or
                         result[2][32] and fid.startswith("CPY"))):
                    raise RuntimeError(f"LLVM-invalid form accepted {fid} {word:08X}")
                state = ("constrained-unpredictable" if result[1][17]
                         else "implementation-context")
            else:
                state = "complete"
                if text != "-" and text.split()[0] not in (
                        form["mnemonic"].lower(), alias.lower()):
                    raise RuntimeError(f"LLVM mnemonic mismatch {fid} {word:08X}: {text}")
            if variant == "seed" and state == "complete":
                complete_scoped.add(fid)
            if variant == "alias" and fid in parents:
                if alias.lower() == parents[fid]["mnemonic"].lower():
                    selected_aliases.add(parents[fid]["id"])
        counts[(variant, state)] += 1
        mem, mx, sm, alias, operands = result if result is not None else (
            (), (), (), "-", {})
        evidence = (" ".join(
            f"{name}:k{op[0]}:r{op[1]}:raw{op[4]}:w{op[9]}:f{op[8]}"
            for name, op in sorted(operands.items())) if variant == "seed"
            else "-")
        writer.writerow(("specialized", fid, form["operation_id"],
                         form["family_path"][1],
                         ",".join(form.get("feature_identifiers", [])) or "-",
                         variant, f"{word:08X}", state,
                         ":".join(map(str, mem)) or "-",
                         ":".join(map(str, mx)) or "-",
                         ":".join(map(str, sm)) or "-", alias, evidence,
                         "llvm-verified" if text != "-" and state == "complete"
                         else text))
    for form in forwarded:
        writer.writerow(("forwarded", form["id"], form["operation_id"],
                         form["family_path"][1],
                         ",".join(form.get("feature_identifiers", [])) or "-",
                         "not-applicable", "-", "forwarded",
                         *(["-"] * 6)))
    missing = [form["id"] for form in scoped if form["id"] not in complete_scoped]
    if missing:
        raise RuntimeError("scoped COMPLETE missing: %d %s" %
                           (len(missing), ", ".join(missing[:20])))
    if len(selected_aliases) != len(parents) or len(parents) != 12:
        raise RuntimeError(f"alias coverage {len(selected_aliases)}/{len(parents)}")
    widths = {len(row) for row in csv.reader(io.StringIO(output.getvalue()))}
    if widths != {14}:
        raise RuntimeError(f"CSV columns misaligned: {widths}")
    data = output.getvalue().encode("utf-8")
    digest = hashlib.sha256(data).hexdigest()
    print("Specialized memory", len(scoped), "COMPLETE", len(forwarded),
          "forwarded", len(cases), "cases", len(reference_cases),
          "LLVM comparisons", len(selected_aliases), "aliases", dict(counts),
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
