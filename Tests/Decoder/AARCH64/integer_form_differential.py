import collections
import csv
import json
import re
import subprocess
import sys


CATALOG = "Decoder/AARCH64/Catalog/AARCH64InstructionCatalog.json"
LLVM_FEATURES = "+crc,+pauth,+mte,+cssc,+flagm,+pauth-lr,+cpa"
REGISTER_FIELDS = ("Rd", "Rn", "Rm", "Ra")
IMPLICIT_FLAG = 0x100
REGRESSION = "Tests/Decoder/AARCH64/integer_register_baseline.csv"


def bit_fields(form, groups, instruction_sets):
    fields = instruction_sets[form["instruction_set_id"]]["encoding_fields"][:]
    for group_id in form["group_ids"]:
        fields.extend(groups[group_id]["encoding_fields"])
    fields.extend(form["encoding_fields"])
    return {field["name"]: field for field in fields if field["name"]}


def field_mask(field):
    return ((1 << field["width"]) - 1) << field["lsb"]


def sample_words(pattern, fields):
    value = int(pattern["value"], 16)
    fixed = int(pattern["mask"], 16)
    words = [value]
    for field_name in REGISTER_FIELDS + (
        "imm12", "imm16", "immr", "imms", "imm6", "imm5", "imm8",
        "shift", "option", "cond", "nzcv", "hw", "sh",
    ):
        field = fields.get(field_name)
        if field is None:
            continue
        mask = field_mask(field) & ~fixed
        for number in (0, 1, 15, 30, 31, (1 << field["width"]) - 1):
            words.append((value & ~mask) | ((number << field["lsb"]) & mask))
    seed = value ^ 0x9E3779B9
    for _ in range(64):
        seed ^= (seed << 13) & 0xFFFFFFFF
        seed ^= seed >> 17
        seed ^= (seed << 5) & 0xFFFFFFFF
        words.append(value | (seed & ~fixed))
    return list(dict.fromkeys(words))


def probe_rows(path, requests):
    process = subprocess.run([path],
        input="".join(f"{identifier} {word:08x}\n" for identifier, word in requests),
        text=True, capture_output=True, check=True)
    rows = process.stdout.splitlines()
    if len(rows) != len(requests):
        raise RuntimeError(f"probe returned {len(rows)} of {len(requests)} responses")
    for (identifier, word), row in zip(requests, rows):
        columns = row.split("\t")
        if columns[:2] != [identifier, f"{word:08X}"]:
            raise RuntimeError(f"probe response misaligned: {row}")
    return [parse_probe(row) for row in rows]


def exhaustive_register_words(seed, pattern, fields):
    fixed = int(pattern["mask"], 16)
    words = []
    for name in REGISTER_FIELDS:
        field = fields.get(name)
        if field is None or field["width"] != 5:
            continue
        mask = field_mask(field) & ~fixed
        for number in range(32):
            words.append((seed & ~mask) | ((number << field["lsb"]) & mask))
    for left, right in (("Rd", "Rn"), ("Rd", "Rm"), ("Rd", "Ra"),
                        ("Rn", "Rm"), ("Rn", "Ra"), ("Rm", "Ra")):
        a, b = fields.get(left), fields.get(right)
        if a is None or b is None or a["width"] != 5 or b["width"] != 5:
            continue
        for x in (0, 15, 30, 31):
            for y in (0, 15, 30, 31):
                mask_a = field_mask(a) & ~fixed
                mask_b = field_mask(b) & ~fixed
                words.append((seed & ~(mask_a | mask_b)) |
                             ((x << a["lsb"]) & mask_a) |
                             ((y << b["lsb"]) & mask_b))
    return words


def parse_probe(line):
    columns = line.split("\t")
    if len(columns) == 3 and columns[2] == "REJECT":
        return None
    if len(columns) < 4:
        raise RuntimeError(f"invalid probe response: {line}")
    operands = {}
    for column in columns[4:]:
        name, kind, role, value, width, special, flags, modifier, raw, signed, scale = column.split(":")
        operands[name] = {
            "kind": int(kind), "role": int(role), "value": int(value),
            "width": int(width), "special": int(special),
            "flags": int(flags), "modifier": modifier.lower(),
            "raw": int(raw), "signed": int(signed), "scale": int(scale),
        }
    return columns[2].lower(), columns[3].lower(), operands


def register_name(operand):
    if operand["width"] not in (32, 64):
        raise RuntimeError("invalid architectural GPR width")
    prefix = "x" if operand["width"] == 64 else "w"
    if operand["special"] == 1:
        return "sp" if prefix == "x" else "wsp"
    if operand["special"] == 2:
        return prefix + "zr"
    return prefix + str(operand["value"])


def llvm_disassemble(words):
    if not words:
        return []
    source = "\n".join(
        ", ".join(f"0x{(word >> (8 * byte)) & 255:02x}" for byte in range(4))
        for word in words
    ) + "\n"
    process = subprocess.run(
        ["llvm-mc", "--disassemble", "--triple=aarch64",
         "--mattr=" + LLVM_FEATURES],
        input=source, text=True, capture_output=True, check=True,
    )
    lines = [line.strip().lower() for line in process.stdout.splitlines()
             if line.strip() and line.strip() != ".text"]
    invalid = {int(match) - 1 for match in re.findall(
        r"<stdin>:(\d+):\d+: warning: invalid instruction encoding",
        process.stderr)}
    if any(index < 0 or index >= len(words) for index in invalid):
        raise RuntimeError("LLVM diagnostic line outside input")
    if len(lines) + len(invalid) != len(words):
        raise RuntimeError(f"LLVM returned {len(lines)} instructions, {len(invalid)} invalid for {len(words)} words: {process.stderr[:200]}")
    iterator = iter(lines)
    return ["<llvm-invalid>" if index in invalid else next(iterator)
            for index in range(len(words))]


def register_fields(operands):
    return [(name, register_name(operands[name])) for name in REGISTER_FIELDS
            if name in operands and operands[name]["kind"] == 1 and
            operands[name]["role"] in (1, 2) and
            not operands[name]["flags"] & IMPLICIT_FLAG]


def llvm_registers(text):
    return re.findall(r"\b(?:wzr|xzr|wsp|sp|[wx][0-9]+)\b", text)


def compare(form, canonical, alias, operands, text):
    if text == "<llvm-invalid>":
        return "llvm-invalid", [], []
    mnemonic = text.split()[0] if text.split() else ""
    fields = register_fields(operands)
    printed = llvm_registers(text)
    if mnemonic != canonical and mnemonic != alias:
        if (canonical, alias, mnemonic) not in (
            ("autiasppcr", "-", "autiasppc"),
            ("autibsppcr", "-", "autibsppc"),
            ("ubfm", "ubfiz", "lsl"),
            ("bfm", "bfc", "bfi"),
        ):
            return "unrecognized-alias", fields, printed
    if mnemonic in ("cmn", "cmp", "tst"):
        if operands.get("Rd", {}).get("value") != 31:
            return "alias-constraint", fields, printed
        fields = [(name, value) for name, value in fields if name != "Rd"]
    elif mnemonic in ("neg", "negs", "ngc", "ngcs", "mvn") or (
            mnemonic == "mov" and canonical == "orr"):
        if operands.get("Rn", {}).get("value") != 31:
            return "alias-constraint", fields, printed
        fields = [(name, value) for name, value in fields if name != "Rn"]
    elif mnemonic in ("mul", "mneg", "smull", "smnegl", "umull", "umnegl"):
        if operands.get("Ra", {}).get("value") != 31:
            return "alias-constraint", fields, printed
        fields = [(name, value) for name, value in fields if name != "Ra"]
    elif mnemonic == "ror" and canonical == "extr":
        if operands.get("Rn", {}).get("value") != operands.get("Rm", {}).get("value"):
            return "alias-constraint", fields, printed
        fields = [(name, value) for name, value in fields if name != "Rm"]
    elif mnemonic in ("cinv", "cinc", "cneg"):
        if operands.get("Rn", {}).get("value") != operands.get("Rm", {}).get("value"):
            return "alias-constraint", fields, printed
        fields = [(name, value) for name, value in fields if name != "Rm"]
    elif mnemonic in ("cset", "csetm"):
        if any(operands.get(name, {}).get("value") != 31 for name in ("Rn", "Rm")):
            return "alias-constraint", fields, printed
        fields = [(name, value) for name, value in fields if name not in ("Rn", "Rm")]
    elif mnemonic == "irg" and len(printed) == 2 and len(fields) == 3:
        if operands.get("Rm", {}).get("value") != 31:
            return "alias-constraint", fields, printed
        fields = [(name, value) for name, value in fields if name != "Rm"]
    names = [value for _, value in fields]
    if (mnemonic in ("sxtb", "sxth", "sxtw", "uxtb", "uxth") and
            len(fields) == 2 and fields[1][1].startswith("x")):
        names[1] = "w" + fields[1][1][1:]
    if len(printed) != len(names):
        return "unmapped-registers", fields, printed
    if printed != names:
        return "register-divergence", fields, printed
    return "verified", register_fields(operands), printed


def verify_baseline(probe_path, form_map):
    with open(REGRESSION, newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    if len(rows) != 67 or len({row["encoding"] + row["form"] for row in rows}) != 67:
        raise RuntimeError("67-entry historical divergence report changed")
    requests = [(row["form"], int(row["encoding"], 16)) for row in rows]
    responses = probe_rows(probe_path, requests)
    reference = llvm_disassemble([word for _, word in requests])
    for row, result, llvm in zip(rows, responses, reference):
        if result is None:
            raise RuntimeError(f"baseline encoding rejected: {row['encoding']}")
        canonical, alias, operands = result
        if llvm != row["llvm_text"].strip().lower():
            raise RuntimeError(f"LLVM historical regression: {row['encoding']}")
        status, fields, printed = compare(form_map[row["form"]], canonical,
                                          alias, operands, llvm)
        if status != "verified":
            raise RuntimeError(f"baseline regression {row['encoding']}: {status} "
                               f"Armito={fields} LLVM={printed}")
        if row["cause"] == "alias-width":
            if canonical != "sbfm" or llvm.split()[0] not in ("sxth", "sxtw"):
                raise RuntimeError("bitfield alias width regression")
        elif row["cause"] in ("special-register-field", "logical-immediate-destination-sp"):
            if "sp" not in printed and "wsp" not in printed:
                raise RuntimeError("SP architectural regression")
        else:
            raise RuntimeError("unknown historical cause")
    print(f"Historical divergence regressions: {len(rows)} resolved")


def main(probe_path):
    version = subprocess.run(["llvm-mc", "--version"], capture_output=True,
                             text=True, check=True).stdout
    if not re.search(r"\bLLVM version 18\.1\.3\b", version):
        raise RuntimeError("reference version mismatch: expected LLVM MC 18.1.3")
    with open(CATALOG, encoding="utf-8") as stream:
        catalog = json.load(stream)
    forms = catalog["instruction_forms"]
    groups = {group["id"]: group for group in catalog["groups"]}
    instruction_sets = {item["id"]: item for item in catalog["instruction_sets"]}
    patterns = {pattern["id"]: pattern for pattern in catalog["encodings"]}
    integer_forms = [form for form in forms
                     if form["family_path"][0] in ("dpimm", "dpreg")]
    by_id = {form["id"]: form for form in integer_forms}
    if (len(integer_forms) != 195 or len({f["operation_id"] for f in integer_forms}) != 112 or
            len(forms) != 4296 or len(patterns) != 3099 or
            catalog["metrics"]["condition_records"] != 599):
        raise RuntimeError("catalog inventory changed")
    verify_baseline(probe_path, by_id)
    baseline = [(form["id"], word) for form in integer_forms
                for word in sample_words(patterns[form["encoding_id"]],
                                         bit_fields(form, groups, instruction_sets))]
    if len(baseline) != 14116:
        raise RuntimeError(f"14,116-word regression corpus changed: {len(baseline)}")
    requests = list(baseline)
    seed_rows = probe_rows(probe_path, baseline)
    seeds = {}
    for (identifier, word), result in zip(baseline, seed_rows):
        if result is not None:
            seeds.setdefault(identifier, word)
    if len(seeds) != 195:
        raise RuntimeError(f"positive seed missing for {195 - len(seeds)} forms")
    for form in integer_forms:
        identifier = form["id"]
        fields = bit_fields(form, groups, instruction_sets)
        for word in exhaustive_register_words(
                seeds[identifier], patterns[form["encoding_id"]], fields):
            requests.append((identifier, word))
    requests = list(dict.fromkeys(requests))
    if len(requests) != 30212:
        raise RuntimeError(f"30,212-candidate register corpus changed: {len(requests)}")
    responses = probe_rows(probe_path, requests)
    accepted = [(identifier, word, result) for (identifier, word), result in
                zip(requests, responses) if result is not None]
    reference = llvm_disassemble([word for _, word, _ in accepted])
    verified = collections.Counter()
    checked_fields = collections.defaultdict(lambda: collections.defaultdict(set))
    counts = collections.Counter()
    unresolved = []
    evidence = {}
    for (identifier, word, (canonical, alias, operands)), text in zip(accepted, reference):
        status, fields, printed = compare(by_id[identifier], canonical, alias,
                                          operands, text)
        counts[status] += 1
        if status == "verified":
            verified[identifier] += 1
            for name, _ in fields:
                checked_fields[identifier][name].add(operands[name]["value"])
        elif status in ("register-divergence", "unmapped-registers", "unrecognized-alias"):
            unresolved.append((identifier, word, status, fields, printed, text))
        else:
            evidence.setdefault(identifier, (word, text))
    for identifier, word, status, fields, printed, text in unresolved[:20]:
        print(f"{status}: {identifier} {word:08X} Armito={fields} LLVM={printed} "
              f"text={text}", file=sys.stderr)
    missing = [form["id"] for form in integer_forms if not verified[form["id"]]]
    print(f"Integer register differential: baseline={len(baseline)} "
          f"requests={len(requests)} valid={len(accepted)} "
          f"LLVM-verified-forms={len(integer_forms)-len(missing)}/195 "
          f"outcomes={dict(counts)}")
    for identifier in missing:
        print(f"NO LLVM COMPARISON {identifier} {evidence.get(identifier)}")
    incomplete = []
    unproven = []
    for form in integer_forms:
        identifier = form["id"]
        for name in REGISTER_FIELDS:
            field = bit_fields(form, groups, instruction_sets).get(name)
            if field is None or field["width"] != 5:
                continue
            mask = field_mask(field) & ~int(patterns[form["encoding_id"]]["mask"], 16)
            if mask != field_mask(field):
                continue
            if name in checked_fields[identifier] and len(checked_fields[identifier][name]) < 32:
                incomplete.append((identifier, name, len(checked_fields[identifier][name])))
            elif name not in checked_fields[identifier]:
                unproven.append((identifier, name))
    print(f"Fields with incomplete LLVM value coverage: {len(incomplete)}")
    print("Examples:", incomplete[:20])
    print(f"Fields without LLVM evidence: {len(unproven)}", unproven[:20])
    print("LLVM rejected encodings (all forms combined):", counts["llvm-invalid"])
    if unresolved or missing or incomplete:
        raise RuntimeError(f"registers: {len(unresolved)} unresolved, "
                           f"{len(missing)} forms without LLVM comparison, "
                           f"{len(incomplete)} fields without full LLVM coverage")


if __name__ == "__main__":
    try:
        main(sys.argv[1])
    except (IndexError, OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Integer form differential failed: {error}", file=sys.stderr)
        raise SystemExit(1)