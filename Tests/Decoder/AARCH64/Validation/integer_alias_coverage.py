import collections
import csv
import hashlib
import io
import json
import sys

CATALOG = "Decoder/AARCH64/Catalog/AARCH64InstructionCatalog.json"
SOURCE = "Tests/Decoder/AARCH64/Results/integer_alias_predicates.csv"
REPORT = "Tests/Decoder/AARCH64/Results/integer_alias_coverage.csv"
REPORT_SHA256 = "05e1d7d7df3dde2c8aa1542d5a46a9e93ae0b62b9cee2b94a73b63d4bd0e5433"


def is_constant(node, value):
    return node["_type"] == "AST.Bool" and node["value"] is value


def uses(node, name):
    if node["_type"] == "AST.Function" and node["name"] == name:
        return True
    for key in ("left", "right", "expr", "var"):
        child = node.get(key)
        if isinstance(child, dict) and uses(child, name):
            return True
    for key in ("arguments", "values"):
        for child in node.get(key, []):
            if isinstance(child, dict) and uses(child, name):
                return True
    return False


def main():
    with open(CATALOG, encoding="utf-8") as stream:
        catalog = json.load(stream)
    forms = {form["id"]: form for form in catalog["instruction_forms"]
             if form["family_path"][0] in ("dpimm", "dpreg")}
    aliases = [alias for alias in catalog["instruction_aliases"]
               if alias["parent_form_id"] in forms]
    conditions = {condition["id"]: condition["ast"] for condition in catalog["conditions"]}
    if len(forms) != 195 or len(aliases) != 93 or len({a["parent_form_id"] for a in aliases}) != 66:
        raise RuntimeError("integer alias inventory changed")
    aggregate = collections.defaultdict(collections.Counter)
    with open(SOURCE, newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            counters = aggregate[row["alias"]]
            counters["rows"] += 1
            counters["pred" + row["predicate"]] += 1
            if row["result"] == "invalid":
                counters["reject"] += 1
            else:
                counters["verified"] += 1
                if row["selected"] == row["alias"].split("::alias::")[1].lower():
                    counters["selected"] += 1
    output = io.StringIO(newline="")
    writer = csv.writer(output)
    writer.writerow(("alias", "parent_form", "operation", "mnemonic", "predicate_class",
                     "pred_true", "pred_false", "verified", "selected", "reject",
                     "has_pos", "has_neg", "selected_ever", "predicate_functions"))
    problems = []
    for alias in aliases:
        condition = conditions[alias["condition_id"]]
        preferred = conditions[alias["preferred_condition_id"]]
        if is_constant(condition, True):
            pclass = "constant-true"
        elif is_constant(condition, False):
            pclass = "constant-false"
        else:
            pclass = "variable"
        counters = aggregate[alias["id"]]
        has_pos = counters["pred1"] > 0
        has_neg = counters["pred0"] > 0
        selected_ever = counters["selected"] > 0
        if pclass == "constant-true" and (has_neg or not has_pos):
            problems.append((alias["id"], "constant-true polarity"))
        elif pclass == "constant-false" and has_pos:
            problems.append((alias["id"], "constant-false polarity"))
        elif pclass == "variable" and not (has_pos and has_neg):
            problems.append((alias["id"], "variable missing polarity"))
        if is_constant(preferred, False) and selected_ever:
            problems.append((alias["id"], "preferred=false selected"))
        functions = ";".join(name for name in ("BFXPreferred", "MoveWidePreferred")
                             if uses(condition, name) or uses(preferred, name))
        writer.writerow((alias["id"], alias["parent_form_id"],
                         forms[alias["parent_form_id"]]["operation_id"], alias["mnemonic"],
                         pclass, counters["pred1"], counters["pred0"], counters["verified"],
                         counters["selected"], counters["reject"], int(has_pos), int(has_neg),
                         int(selected_ever), functions))
    if problems:
        raise RuntimeError(f"alias coverage classification failed: {problems}")
    data = output.getvalue().encode("utf-8")
    digest = hashlib.sha256(data).hexdigest()
    if digest != REPORT_SHA256:
        raise RuntimeError(f"alias coverage hash changed: {digest}")
    with open(REPORT, "rb") as stream:
        if stream.read() != data:
            raise RuntimeError("alias coverage report mismatch")
    classes = collections.Counter(row[4] for row in list(csv.reader(io.StringIO(output.getvalue())))[1:])
    print("Alias coverage", len(aliases), "aliases", dict(classes), "sha256", digest)


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, OSError, KeyError, ValueError) as error:
        print("Alias coverage failed:", error, file=sys.stderr)
        raise SystemExit(1)