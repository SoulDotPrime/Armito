import collections
import json
import re
import subprocess
import sys

import integer_form_differential as register
import llvm_toolchain


IMMEDIATE_GROUPS = {
    "addsub_imm": ("imm12",),
    "addsub_immtags": ("imm6", "imm4"),
    "movewide": ("imm16", "hw"),
    "minmax_imm": ("imm8",),
    "condcmp_imm": ("imm5", "nzcv"),
    "condcmp_reg": ("nzcv",),
    "rmif": ("imm6", "mask"),
    "addsub_ext": ("imm3",),
    "addsub_pt": ("imm3",),
    "addsub_shift": ("imm6",),
    "log_shift": ("imm6",),
    "extract": ("imms",),
    "dp_1src_imm": ("imm16",),
    "pcreladdr": ("immhi", "immlo"),
    "bitfield": ("immr", "imms"),
    "log_imm": ("immr", "imms"),
}
NUMBER = re.compile(r"#(-?0x[0-9a-f]+|-?[0-9]+)\b")


def immediate_fields(form, fields):
    group = form["family_path"][1]
    if group == "setf":
        return ("mask",) if "mask" in fields else ()
    return tuple(name for name in IMMEDIATE_GROUPS.get(group, ()) if name in fields)


def samples(seed, fields, pattern, names, group):
    fixed = int(pattern["mask"], 16)
    words = []
    for name in names:
        field = fields[name]
        mask = register.field_mask(field) & ~fixed
        size = 1 << field["width"]
        if size <= 4096:
            values = range(size)
        else:
            values = (0, 1, 2, 15, 16, 31, 63, 127, 128, 255,
                      size // 4, size // 2 - 1, size // 2, size - 2, size - 1)
        for value in values:
            words.append((seed & ~mask) | ((value << field["lsb"]) & mask))
    if "imm6" in names and "imm4" in names:
        a, b = fields["imm6"], fields["imm4"]
        for x in (0, 1, 31, 63):
            for y in (0, 1, 7, 15):
                ma, mb = register.field_mask(a), register.field_mask(b)
                words.append((seed & ~(ma | mb)) | (x << a["lsb"]) |
                             (y << b["lsb"]))
    if group == "bitfield":
        a, b = fields["immr"], fields["imms"]
        ma, mb = register.field_mask(a), register.field_mask(b)
        for r in range(64):
            for s in range(64):
                words.append((seed & ~(ma | mb)) | (r << a["lsb"]) |
                             (s << b["lsb"]))
    if "imm12" in names and "sh" in fields:
        field = fields["sh"]
        mask = register.field_mask(field) & ~fixed
        for raw in (0, 1, 4095):
            imm = fields["imm12"]
            ma = register.field_mask(imm) & ~fixed
            words.append(((seed & ~(ma | mask)) |
                          ((raw << imm["lsb"]) & ma) | mask))
    if "imm16" in names and "hw" in fields:
        field = fields["hw"]
        mask = register.field_mask(field) & ~fixed
        for raw in (0, 1, 65535):
            imm = fields["imm16"]
            ma = register.field_mask(imm) & ~fixed
            for hw in range(1 << field["width"]):
                words.append((seed & ~(ma | mask)) |
                             ((raw << imm["lsb"]) & ma) |
                             ((hw << field["lsb"]) & mask))
    if "immhi" in names and "immlo" in names:
        a, b = fields["immhi"], fields["immlo"]
        ma, mb = register.field_mask(a), register.field_mask(b)
        for raw in (0, 1, 3, 0x7ffff, 0x80000, 0xffffe, 0xfffff):
            words.append((seed & ~(ma | mb)) |
                         ((raw >> 2) << a["lsb"]) | ((raw & 3) << b["lsb"]))
    return list(dict.fromkeys(words))


def numbers(text):
    return [int(token, 0) for token in NUMBER.findall(text.split("//")[0])]


def check(form, word, operands, text):
    group = form["family_path"][1]
    if text == "<llvm-invalid>":
        return "llvm-invalid", ""
    values = numbers(text)
    def field(name):
        return operands[name]["raw"]
    def require(actual, expected):
        if actual != expected:
            return "mismatch", f"Armito={actual} LLVM={expected}"
        return "verified", ""
    def independent_raw(lsb, width):
        return (word >> lsb) & ((1 << width) - 1)
    if group == "addsub_imm":
        if not values:
            return "unmapped-alias", text
        sh = (word >> 22) & 1
        if operands["scaled_immediate"]["value"] != (field("imm12") << (12 * sh)):
            return "mismatch", "scaled immediate"
        if sh and "lsl #12" not in text:
            return "unmapped-alias", text
        if not sh and "lsl #12" in text:
            return "mismatch", "unexpected shift"
        if operands["sh"]["value"] != 12 * sh:
            return "mismatch", "ADD/SUB shift interpretation"
        return require(operands["scaled_immediate"]["value"], values[0] << (12 * sh))
    if group == "addsub_immtags":
        if len(values) < 2:
            return "unmapped-alias", text
        if operands["imm6"]["raw"] != independent_raw(16, 6) or operands["imm4"]["raw"] != independent_raw(10, 4):
            return "mismatch", "raw ADDG/SUBG fields"
        if operands["imm6"]["value"] != independent_raw(16, 6) * 16 or operands["imm6"]["scale"] != 4 or operands["imm4"]["value"] != independent_raw(10, 4):
            return "mismatch", "independent tag offset and tag immediate"
        return require((operands["imm6"]["value"], operands["imm4"]["value"]),
                       tuple(values[:2]))
    if group == "movewide":
        if not values:
            return "unmapped-alias", text
        raw, shift = field("imm16"), field("hw") * 16
        if operands["hw"]["value"] != shift or operands["move_wide_immediate"]["value"] != raw << shift:
            return "mismatch", "move-wide composite"
        if text.split()[0] == "mov" and form["mnemonic"] in ("MOVZ", "MOVN"):
            result = (raw << shift) if form["mnemonic"] == "MOVZ" else (~(raw << shift)) & ((1 << operands["Rd"]["width"]) - 1)
            return require(result, values[0] & ((1 << operands["Rd"]["width"]) - 1))
        if text.split()[0] != form["mnemonic"].lower():
            return "unmapped-alias", text
        if len(values) > 1 and values[1] != shift:
            return "mismatch", "move-wide shift"
        if len(values) == 1 and shift and "lsl" not in text:
            return "unmapped-alias", text
        return require(raw, values[0])
    if group == "minmax_imm":
        if not values:
            return "unmapped-alias", text
        raw = field("imm8")
        if raw != independent_raw(10, 8):
            return "mismatch", "raw imm8"
        actual = operands["imm8"]["signed"] if form["mnemonic"].startswith("S") else operands["imm8"]["value"]
        expected = raw - 256 if form["mnemonic"].startswith("S") and raw >= 128 else raw
        if actual != expected or operands["imm8"]["value"] != expected & ((1 << 64) - 1):
            return "mismatch", "independent signed/unsigned extension of imm8"
        return require(actual, values[0])
    if group == "dp_1src_imm":
        if not values:
            return "unmapped-alias", text
        raw = field("imm16")
        if raw != independent_raw(5, 16):
            return "mismatch", "raw PAuth imm16"
        if operands["imm16"]["signed"] != -(raw * 4) or operands["imm16"]["scale"] != 2:
            return "mismatch", "independent PAuth-LR negative byte offset"
        return require(operands["imm16"]["signed"], values[0])
    if group == "pcreladdr":
        if not values:
            return "unmapped-alias", text
        raw = (field("immhi") << 2) | field("immlo")
        if operands["pc_relative_address"]["raw"] != raw:
            return "mismatch", "composite PC-relative raw"
        return require(operands["pc_relative_address"]["signed"], values[0])
    if group == "bitfield":
        width = operands["Rd"]["width"]
        r, s = field("immr"), field("imms")
        mnemonic = text.split()[0]
        if mnemonic in ("bfm", "sbfm", "ubfm"):
            return require((r, s), tuple(values[:2]))
        if mnemonic in ("bfxil", "sbfx", "ubfx"):
            return require((r, s - r + 1), tuple(values[:2]))
        if mnemonic in ("bfi", "bfc", "sbfiz", "ubfiz"):
            return require(((width - r) % width, s + 1), tuple(values[:2]))
        if mnemonic in ("sxtb", "uxtb", "sxth", "uxth", "sxtw"):
            expected = {"sxtb": 7, "uxtb": 7, "sxth": 15, "uxth": 15, "sxtw": 31}[mnemonic]
            return require((r, s), (0, expected))
        if mnemonic in ("asr", "lsr"):
            return require((r, s), (values[0], width - 1))
        if mnemonic == "lsl":
            return require((r, s), ((width - values[0]) % width, width - 1 - values[0]))
        return "unmapped-alias", text
    if group == "log_imm":
        return "deferred-bitmask", ""
    if group == "setf":
        if field("mask") != 13 or (word & 15) != 13:
            return "mismatch", "raw SETF fixed mask"
        return "implicit-mask", ""
    if group in ("condcmp_imm", "condcmp_reg"):
        if len(values) < (2 if group == "condcmp_imm" else 1):
            return "unmapped-alias", text
        actual = (field("imm5"), field("nzcv")) if group == "condcmp_imm" else (field("nzcv"),)
        return require(actual, tuple(values[:len(actual)]))
    if group in ("rmif", "addsub_ext", "addsub_pt", "addsub_shift", "log_shift", "extract"):
        name = "imm3" if group in ("addsub_ext", "addsub_pt") else "imms" if group == "extract" else "imm6"
        raw = field(name)
        if group in ("addsub_ext", "addsub_pt", "addsub_shift", "log_shift") and raw == 0 and not values:
            return "verified", ""
        if group == "rmif":
            if len(values) < 2:
                return "unmapped-alias", text
            return require((raw, field("mask")), tuple(values[:2]))
        if not values:
            return "unmapped-alias", text
        return require(raw, values[-1])
    return "not-implemented", group


def main(probe):
    exe, ver = llvm_toolchain.require_qualified()
    print(f"LLVM MC toolchain: {exe} version {ver}")
    with open(register.CATALOG, encoding="utf-8") as stream:
        catalog = json.load(stream)
    forms = [form for form in catalog["instruction_forms"]
             if form["family_path"][0] in ("dpimm", "dpreg")]
    groups = {group["id"]: group for group in catalog["groups"]}
    sets = {item["id"]: item for item in catalog["instruction_sets"]}
    patterns = {item["id"]: item for item in catalog["encodings"]}
    requests = []
    inventory = {}
    for form in forms:
        fields = register.bit_fields(form, groups, sets)
        names = immediate_fields(form, fields)
        unclassified = [name for name in fields if name.startswith("imm") or
                        name in ("hw", "sh", "mask", "nzcv")]
        if set(unclassified) != set(names) | ({"hw"} if "hw" in fields and
                                       form["family_path"][1] == "movewide" else set()) | (
                                           {"sh"} if "sh" in fields and
                                           form["family_path"][1] == "addsub_imm" else set()):
            raise RuntimeError(f"unclassified immediate fields in {form['id']}: {unclassified}")
        if not names:
            continue
        inventory[form["id"]] = (form, names)
        seed = int(patterns[form["encoding_id"]]["value"], 16)
        for word in samples(seed, fields, patterns[form["encoding_id"]], names,
                            form["family_path"][1]):
            requests.append((form["id"], word))
    if len(forms) != 195 or len(inventory) != 89 or sum(len(names) for _, names in inventory.values()) != 118:
        raise RuntimeError("Integer immediate inventory changed")
    requests = list(dict.fromkeys(requests))
    responses = register.probe_rows(probe, requests)
    if len(requests) != 62851:
        raise RuntimeError("62,851-candidate immediate regression corpus changed")
    accepted = [(identifier, word, result) for (identifier, word), result
                in zip(requests, responses) if result is not None]
    disassembly = register.llvm_disassemble([word for _, word, _ in accepted])
    counts = collections.Counter()
    coverage = collections.defaultdict(lambda: collections.defaultdict(set))
    failures = []
    for (identifier, word, (_, _, operands)), text in zip(accepted, disassembly):
        form, names = inventory[identifier]
        fields = register.bit_fields(form, groups, sets)
        for name in names:
            if name not in operands:
                failures.append((identifier, word, "missing-field", name, text))
            elif operands[name]["raw"] != ((word >> fields[name]["lsb"]) & ((1 << fields[name]["width"]) - 1)):
                failures.append((identifier, word, "raw", name, text))
        status, detail = check(form, word, operands, text)
        counts[status] += 1
        if status == "verified":
            for name in names:
                coverage[identifier][name].add(operands[name]["raw"])
        elif status not in ("llvm-invalid", "deferred-bitmask", "implicit-mask"):
            failures.append((identifier, word, status, detail, text))
    print(f"Immediate inventory: forms={len(inventory)} fields={sum(len(v[1]) for v in inventory.values())} "
          f"requests={len(requests)} accepted={len(accepted)} "
          f"rejected-by-Armito={len(requests)-len(accepted)} outcomes={dict(counts)}")
    for form, names in inventory.values():
        print(f"FIELD {form['id']} operation={form['operation_id']} "
              + " ".join(f"{name}={len(coverage[form['id']][name])}"
                         for name in names))
    print("OPERATION COVERAGE", dict(collections.Counter(
        form["operation_id"] for form, names in inventory.values()
        if any(coverage[form["id"]][name] for name in names))))
    missing = [(form["id"], name) for form, names in inventory.values()
               if form["family_path"][1] not in ("log_imm", "setf")
               for name in names if not coverage[form["id"]][name]]
    incomplete = []
    for form, names in inventory.values():
        if form["family_path"][1] in ("minmax_imm", "bitfield", "addsub_immtags", "addsub_imm"):
            for name in names:
                field = register.bit_fields(form, groups, sets)[name]
                domain = 1 << field["width"]
                if form["family_path"][1] == "bitfield":
                    domain = 32 if "_32" in form["id"] else 64
                if len(coverage[form["id"]][name]) != domain:
                    incomplete.append((form["id"], name, domain,
                                       len(coverage[form["id"]][name])))
    for row in failures[:30]:
        print("IMMEDIATE GAP", *row, file=sys.stderr)
    if failures or missing or incomplete:
        raise RuntimeError(f"{len(failures)} immediate mismatches/unmapped cases; "
                           f"{len(missing)} unverified fields: {missing[:12]}; "
                           f"{len(incomplete)} incomplete domains: {incomplete[:12]}")


if __name__ == "__main__":
    try:
        main(sys.argv[1])
    except (OSError, ValueError, IndexError, RuntimeError, subprocess.CalledProcessError) as error:
        print("Immediate differential failed:", error, file=sys.stderr)
        raise SystemExit(1)