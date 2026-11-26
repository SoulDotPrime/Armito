import collections
import csv
import hashlib
import io
import itertools
import json
import sys

import integer_form_differential as differential


REPORT = "Tests/Decoder/AARCH64/integer_alias_predicates.csv"
REPORT_SHA256 = "e9f412f73d15c3595bb844da9dd9519da76f7fc8cdc92a057763b12cfd0babeb"


def mask(value, width):
    return value & ((1 << width) - 1)


def bitmask(sf, n, s, r):
    width = 32 << sf
    selector = (n << 6) | ((~s) & 63)
    length = selector.bit_length() - 1
    if length < 1 or (1 << length) > width:
        return None
    size = 1 << length
    count = s & (size - 1)
    if count == size - 1:
        return None
    return sum((((((1 << (count + 1)) - 1) >> ((position + r) % size)) & 1)
                << position) for position in range(width))


def preferred_bfx(sf, uns, s, r):
    width = 32 << sf
    if s < r or s == width - 1:
        return False
    if r == 0 and ((sf == 0 and s in (7, 15)) or
                   (sf == 1 and uns == 0 and s in (7, 15, 31))):
        return False
    return True


def preferred_movewide(sf, n, s, r):
    width = 32 << sf
    value = bitmask(sf, n, s, r)
    if value is None:
        return False
    for candidate in (value, mask(~value, width)):
        if sum(bool(candidate & (0xffff << shift))
               for shift in range(0, width, 16)) <= 1:
            return True
    return False


def interpret(node, fields):
    kind = node["_type"]
    if kind == "AST.Bool":
        return node["value"]
    if kind == "AST.Integer":
        return node["value"]
    if kind == "AST.Identifier":
        return fields[node["value"]][0]
    if kind == "Values.Value":
        bits = node["value"].strip("'")
        return (int(bits.replace("x", "0"), 2), len(bits), bits)
    if kind == "AST.Set":
        return [interpret(entry, fields) for entry in node["values"]]
    if kind == "AST.SquareOp":
        source = node["var"]["value"]
        bit = interpret(node["arguments"][0], fields)
        return (fields[source][0] >> bit) & 1
    if kind == "AST.UnaryOp":
        if node["op"] != "!":
            raise RuntimeError(f"unsupported unary operation {node['op']}")
        return not interpret(node["expr"], fields)
    if kind == "AST.Function":
        name = node["name"]
        if name == "IsFeatureImplemented":
            if node["arguments"][0]["value"] != "FEAT_ASMv8p2":
                raise RuntimeError("unexpected feature predicate")
            return True
        args = [interpret(argument, fields) for argument in node["arguments"]]
        if name == "UInt":
            return args[0]
        if name == "IsZero":
            return args[0] == 0
        if name == "IsOnes":
            return args[0] == (1 << fields[node["arguments"][0]["value"]][1]) - 1
        if name == "BFXPreferred":
            return preferred_bfx(*args)
        if name == "MoveWidePreferred":
            return preferred_movewide(*args)
        raise RuntimeError(f"unsupported predicate function: {name}")
    if kind == "AST.BinaryOp":
        op = node["op"]
        if op in ("&&", "||"):
            left = bool(interpret(node["left"], fields))
            return left and bool(interpret(node["right"], fields)) if op == "&&" else left or bool(interpret(node["right"], fields))
        left = interpret(node["left"], fields)
        right = interpret(node["right"], fields)
        if op == "IN":
            width = fields[node["left"]["value"]][1]
            return any(len(pattern) == width and
                       all(char == "x" or int(char) == ((left >> (width - 1 - i)) & 1)
                           for i, char in enumerate(pattern))
                       for _, _, pattern in right)
        if isinstance(left, tuple):
            left = left[0]
        if isinstance(right, tuple):
            right = right[0]
        return {"==": lambda: left == right, "!=": lambda: left != right,
                "<": lambda: left < right, ">=": lambda: left >= right,
                "+": lambda: left + right}[op]()
    raise RuntimeError(f"unsupported predicate AST: {kind}")


def raw_fields(form, groups, sets, word):
    result = {}
    for name, field in differential.bit_fields(form, groups, sets).items():
        result[name] = (mask(word >> field["lsb"], field["width"]), field["width"])
    return result


def candidates(form, pattern, groups, sets, conditions):
    seed = int(pattern["value"], 16)
    fixed = int(pattern["mask"], 16)
    for entry in form["encoding_fields"]:
        bits = entry["value_pattern"]
        if entry["kind"] == "Bits" and set(bits) <= {"0", "1"}:
            part = ((1 << entry["width"]) - 1) << entry["lsb"]
            seed = (seed & ~part) | (int(bits, 2) << entry["lsb"])
    if form["family_path"][1] == "condsel":
        variant = {"CSEL": 0, "CSINC": 1, "CSINV": 2, "CSNEG": 3}[form["mnemonic"]]
        seed = (seed & ~((1 << 30) | (1 << 10))) | ((variant >> 1) << 30) | ((variant & 1) << 10)
    def fixed_conditions(node):
        if node["_type"] == "AST.BinaryOp":
            if (node["op"] == "==" and node["left"]["_type"] == "AST.Identifier" and
                    node["right"]["_type"] == "Values.Value"):
                return [(node["left"]["value"], int(node["right"]["value"].strip("'"), 2))]
            if node["op"] == "&&":
                return fixed_conditions(node["left"]) + fixed_conditions(node["right"])
        return []
    for name, value in fixed_conditions(conditions[form["condition_id"]]):
        field = differential.bit_fields(form, groups, sets).get(name)
        if field is not None:
            part = ((1 << field["width"]) - 1) << field["lsb"]
            seed = (seed & ~part) | (value << field["lsb"])
    if form["family_path"][1] == "dp_2src" and form["mnemonic"] in ("ASRV", "LSLV", "LSRV", "RORV"):
        seed = (seed & ~(3 << 10)) | ({"LSLV": 0, "LSRV": 1, "ASRV": 2, "RORV": 3}[form["mnemonic"]] << 10)
    if form["family_path"][1] == "dp_1src" and form["mnemonic"] == "REV":
        seed = (seed & ~(3 << 10)) | (3 << 10)
    if form["family_path"][1] == "dp_3src" and form["mnemonic"] in ("UMADDL", "UMSUBL"):
        seed |= 1 << 23
    fields = differential.bit_fields(form, groups, sets)
    words = {seed}
    mutable = {}
    for name in ("Rd", "Rn", "Rm", "Ra", "immr", "imms", "imm16", "hw",
                 "sh", "imm12", "shift", "imm6", "cond", "N"):
        if name not in fields:
            continue
        entry = fields[name]
        field_mask = differential.field_mask(entry) & ~fixed
        if field_mask:
            mutable[name] = (entry["lsb"], field_mask, entry["width"])
    def change(word, name, value):
        position, field_mask, _ = mutable[name]
        return (word & ~field_mask) | ((value << position) & field_mask)
    registers = {name: (0, 1, 30, 31) for name in ("Rd", "Rn", "Rm", "Ra") if name in mutable}
    for name, values in registers.items():
        words.update(change(seed, name, value) for value in values)
    for left, right in itertools.combinations(registers, 2):
        for x, y in itertools.product(registers[left], registers[right]):
            words.add(change(change(seed, left, x), right, y))
    for name in ("immr", "imms", "imm16", "hw", "sh", "imm12", "shift", "imm6", "cond", "N"):
        if name in mutable:
            width = mutable[name][2]
            values = range(1 << width) if name in ("immr", "imms", "cond", "hw") else (0, 1, 7, 15, 16, 31, (1 << width) - 1)
            words.update(change(seed, name, value) for value in values)
    if "immr" in mutable and "imms" in mutable:
        for r, s in itertools.product(range(64), repeat=2):
            words.add(change(change(seed, "immr", r), "imms", s))
    for rname in ("Rn", "Rd", "Ra"):
        if rname in mutable:
            for field in ("immr", "imms", "imm16", "hw", "imm12", "shift", "imm6", "sh", "cond"):
                if field in mutable:
                    size = mutable[field][2]
                    values = range(1 << size) if field in ("immr", "imms", "cond", "hw") else (0, 1, 7, 15, (1 << size) - 1)
                    for v in values:
                        for r in (0, 31):
                            words.add(change(change(seed, field, v), rname, r))
    if "immr" in mutable and "imms" in mutable and "Rn" in mutable:
        for r, s in itertools.product((0, 1, 7, 8, 15, 16, 30, 31, 32, 47, 62, 63), repeat=2):
            words.add(change(change(change(seed, "immr", r), "imms", s), "Rn", 31))
    if "Rn" in mutable and "Rm" in mutable and "cond" in mutable:
        for cond, reg in itertools.product(range(16), (0, 1, 31)):
            words.add(change(change(change(seed, "cond", cond), "Rn", reg), "Rm", reg))
    return sorted(words)


def main(probe):
    with open(differential.CATALOG, encoding="utf-8") as stream:
        catalog = json.load(stream)
    forms = {f["id"]: f for f in catalog["instruction_forms"]
             if f["family_path"][0] in ("dpimm", "dpreg")}
    aliases = [a for a in catalog["instruction_aliases"] if a["parent_form_id"] in forms]
    groups = {g["id"]: g for g in catalog["groups"]}
    sets = {s["id"]: s for s in catalog["instruction_sets"]}
    patterns = {p["id"]: p for p in catalog["encodings"]}
    conditions = {c["id"]: c["ast"] for c in catalog["conditions"]}
    if len(forms) != 195 or len(aliases) != 93 or len({a["parent_form_id"] for a in aliases}) != 66:
        raise RuntimeError("integer alias inventory changed")
    by_form = collections.defaultdict(list)
    for alias in aliases:
        by_form[alias["parent_form_id"]].append(alias)
    cases = [(form, word) for form_id, entries in by_form.items()
             for form in (forms[form_id],)
             for word in candidates(form, patterns[form["encoding_id"]], groups, sets, conditions)]
    responses = differential.probe_rows(probe, [(form["id"], word) for form, word in cases])
    llvm = differential.llvm_disassemble([word for _, word in cases])
    output = io.StringIO(newline="")
    writer = csv.writer(output)
    writer.writerow(("form", "operation", "encoding", "alias", "predicate", "preferred",
                     "candidate_count", "selected", "probe", "llvm", "result"))
    evidence = collections.defaultdict(collections.Counter)
    failures = []
    textual = collections.Counter()
    overlaps = collections.Counter()
    for (form, word), response, assembly in zip(cases, responses, llvm):
        fields = raw_fields(form, groups, sets, word)
        evaluated = []
        for alias in by_form[form["id"]]:
            predicate = bool(interpret(conditions[alias["condition_id"]], fields))
            preferred = bool(interpret(conditions[alias["preferred_condition_id"]], fields))
            evaluated.append((alias, predicate, preferred))
        winners = [alias for alias, pred, pref in evaluated if pred and pref]
        overlap = len(winners) > 1
        if overlap and form["mnemonic"] == "UBFM" and {a["mnemonic"] for a in winners} == {"LSL", "UBFIZ"}:
            selected = "ubfiz"
        else:
            selected = winners[0]["mnemonic"].lower() if len(winners) == 1 else "-"
        if overlap and response is not None:
            overlaps[(form["id"], tuple(alias["mnemonic"] for alias in winners), selected)] += 1
        if response is not None:
            canonical, observed, _ = response
            if canonical != form["mnemonic"].lower() or observed != selected or (overlap and selected == "-"):
                failures.append((form["id"], f"{word:08X}", selected, observed, canonical, len(winners)))
            if assembly == "<llvm-invalid>":
                textual[(form["id"], "LLVM-invalid")] += 1
            elif assembly.split()[0] not in (selected, canonical):
                textual[(form["id"], assembly.split()[0], selected)] += 1
        for alias, pred, pref in evaluated:
            if response is not None:
                evidence[alias["id"]][(int(pred), int(pref), int(alias in winners))] += 1
            writer.writerow((form["id"], form["operation_id"], f"{word:08X}",
                             alias["id"], int(pred), int(pref), len(winners),
                             selected, "REJECT" if response is None else response[1], assembly,
                             "invalid" if response is None else "verified" if not failures or failures[-1][:2] != (form["id"], f"{word:08X}") else "difference"))
    if failures:
        summary = collections.Counter((item[0], item[2], item[3], item[5]) for item in failures)
        raise RuntimeError(f"{len(failures)} predicate/priority divergences: {summary.most_common(18)}")
    missing = [alias["id"] for alias in aliases if not any(key[0] == 1 for key in evidence[alias["id"]])
               or not any(key[0] == 0 for key in evidence[alias["id"]])]
    selected_aliases = {alias["id"] for alias in aliases if
                        any(key[0] == 1 and key[1] == 1 for key in evidence[alias["id"]])}
    if len(selected_aliases) != 92 or set(aliases[index]["id"] for index in range(len(aliases))) - selected_aliases != {"REV_64_dp_1src::alias::REV64"}:
        raise RuntimeError("alias positive coverage changed")
    if sum(overlaps.values()) != 106 or set(overlaps) != {
            ("UBFM_32M_bitfield", ("LSL", "UBFIZ"), "ubfiz"),
            ("UBFM_64M_bitfield", ("LSL", "UBFIZ"), "ubfiz")}:
        raise RuntimeError(f"overlap classification changed: {overlaps}")
    data = output.getvalue().encode("utf-8")
    digest = hashlib.sha256(data).hexdigest()
    print("Alias campaign", len(cases), "rows", sum(evidence[alias["id"]].total() for alias in aliases),
          "missing positive/negative", missing, "sha256", digest)
    print("Overlaps", overlaps, "LLVM textual differences", textual.most_common(12))
    if digest != REPORT_SHA256:
        raise RuntimeError(f"alias report hash changed: {digest}")
    with open(REPORT, "rb") as stream:
        if stream.read() != data:
            raise RuntimeError("alias report mismatch")


if __name__ == "__main__":
    try:
        if len(sys.argv) != 2:
            raise ValueError("usage: integer_alias_predicates.py PROBE")
        main(sys.argv[1])
    except (RuntimeError, ValueError, OSError, KeyError) as error:
        print("Alias campaign failed:", error, file=sys.stderr)
        raise SystemExit(1)