import argparse
import hashlib
import json
import sys
from collections import Counter, defaultdict
from pathlib import Path


TOOLS_DIR = Path(__file__).resolve().parent
ROOT_DIR = TOOLS_DIR.parents[1]
CATALOG_DIR = ROOT_DIR / "Decoder" / "AARCH64" / "Catalog"
CATALOG_PATH = CATALOG_DIR / "AARCH64InstructionCatalog.json"
PATTERNS_PATH = (
    ROOT_DIR / "Tests" / "Fixtures" / "AARCH64" / "AARCH64CatalogPatterns.inc"
)
CATALOG_C_PATH = CATALOG_DIR / "AARCH64CatalogData.inc"
SOURCE_SHA256 = "2234d548ad50a95c3119166fad7077f333c4f9fe4cf87a274ed80abc066054b2"
SOURCE_COMMIT = "66d0604c6b800745edfbd4891ba450add54ede25"
GUEST_FEATURE_WORDS = 2
SOURCE_URL = (
    "https://raw.githubusercontent.com/fooptrvoid/"
    "arm-mra-2024-12-aarchmrs-bsd/"
    + SOURCE_COMMIT
    + "/Instructions.json"
)
FAMILY_MODULES = {
    "control": "control_system",
    "dpimm": "data_processing",
    "dpreg": "data_processing",
    "ldst": "load_store_atomic",
    "reserved": "reserved_or_unallocated",
    "simd_dp": "simd_fp_crypto",
    "sme": "matrix_sme",
    "sve": "vector_sve",
}


def parse_bits(text, width):
    if not isinstance(text, str) or len(text) != width:
        raise ValueError("encoding bit string width mismatch")
    if any(bit not in "01x" for bit in text):
        raise ValueError("encoding bit string contains an unsupported symbol")
    mask = 0
    value = 0
    for offset, bit in enumerate(reversed(text)):
        if bit != "x":
            mask |= 1 << offset
        if bit == "1":
            value |= 1 << offset
    return mask, value


def parse_encoding(encoding):
    width = encoding.get("width")
    if not isinstance(width, int) or width <= 0 or width > 64:
        raise ValueError("invalid encoding width")
    mask = 0
    value = 0
    occupied = 0
    fields = []
    for item in encoding.get("values", []):
        bit_range = item.get("range", {})
        start = bit_range.get("start")
        field_width = bit_range.get("width")
        pattern = item.get("value", {}).get("value", "").strip("'")
        should_be_mask = item.get("should_be_mask")
        if should_be_mask is not None:
            should_be_mask = should_be_mask.get("value", "").strip("'")
        if (
            not isinstance(start, int)
            or not isinstance(field_width, int)
            or start < 0
            or field_width <= 0
            or start + field_width > width
        ):
            raise ValueError("encoding field range is invalid")
        field_mask, field_value = parse_bits(pattern, field_width)
        if occupied & (((1 << field_width) - 1) << start):
            raise ValueError("encoding fields overlap")
        occupied |= ((1 << field_width) - 1) << start
        mask |= field_mask << start
        value |= field_value << start
        fields.append(
            {
                "kind": item.get("_type", "").rsplit(".", 1)[-1],
                "name": item.get("name"),
                "lsb": start,
                "width": field_width,
                "value_pattern": pattern,
                "should_be_mask": should_be_mask,
            }
        )
    return width, mask, value, fields


def feature_identifiers(condition):
    found = set()

    def visit(node):
        if isinstance(node, dict):
            if (
                node.get("_type") == "AST.Function"
                and node.get("name") == "IsFeatureImplemented"
            ):
                for argument in node.get("arguments", []):
                    if (
                        isinstance(argument, dict)
                        and argument.get("_type") == "AST.Identifier"
                        and argument.get("value", "").startswith("FEAT_")
                    ):
                        found.add(argument["value"])
            for value in node.values():
                visit(value)
        elif isinstance(node, list):
            for value in node:
                visit(value)

    visit(condition)
    return sorted(found)


def assembly_mnemonic(assembly):
    if not isinstance(assembly, dict):
        return None
    for symbol in assembly.get("symbols", []):
        if symbol.get("_type") == "Instruction.Symbols.Literal":
            return symbol.get("value")
    return None


def build_catalog(source_path):
    source_bytes = source_path.read_bytes()
    source_sha256 = hashlib.sha256(source_bytes).hexdigest()
    if source_sha256 != SOURCE_SHA256:
        raise ValueError("source SHA-256 does not match the pinned AARCHMRS release")
    source = json.loads(source_bytes)
    metadata = source.get("_meta", {}).get("version", {})
    if metadata.get("architecture") != "v9Ap6-A":
        raise ValueError("source architecture release does not match the pinned profile")
    if metadata.get("schema") != "2.5.3":
        raise ValueError("source schema version does not match the pinned release")
    if "BSD 3-clause" not in source["_meta"]["license"]["info"]:
        raise ValueError("source license does not match the documented terms")

    operation_records = []
    for operation_id, operation in sorted(source["operations"].items()):
        operation_records.append(
            {
                "id": operation_id,
                "kind": (
                    "operation_alias"
                    if operation.get("_type") == "Instruction.OperationAlias"
                    else "operation"
                ),
                "operation_id": operation.get("operation_id"),
            }
        )
    operation_ids = {item["id"] for item in operation_records}
    forms = []
    aliases = []
    groups = {}
    instruction_sets = []
    encoding_groups = {}
    alias_id_counts = defaultdict(int)
    condition_records = {}

    def condition_id(condition):
        if condition is None:
            return None
        content = json.dumps(
            condition, sort_keys=True, separators=(",", ":"), ensure_ascii=True
        )
        identifier = "condition_" + hashlib.sha256(content.encode()).hexdigest()[:16]
        if (
            identifier in condition_records
            and condition_records[identifier] != condition
        ):
            raise ValueError("condition ID hash collision")
        condition_records[identifier] = condition
        return identifier

    def merged_encoding(parts):
        if not parts:
            raise ValueError("instruction form has no inherited encoding")
        width = parts[-1]["width"]
        mask = 0
        value = 0
        fields = []
        leaf_fields = []
        for part in parts:
            if part["width"] != width:
                raise ValueError("inherited encoding widths do not match")
            part_mask = part["mask"]
            part_value = part["value"]
            if ((value ^ part_value) & mask & part_mask) != 0:
                raise ValueError("inherited encoding bits conflict")
            value = (value & ~part_mask) | part_value
            mask |= part_mask
            for field in part["fields"]:
                field_entry = dict(field)
                field_entry["source_node"] = part["source_node"]
                fields.append(field_entry)
                if part["source_node_kind"] == "instruction":
                    leaf_fields.append(dict(field))
        return width, mask, value, fields, leaf_fields

    def part_from_node(node, node_kind):
        width, mask, value, fields = parse_encoding(node["encoding"])
        return {
            "width": width,
            "mask": mask,
            "value": value,
            "fields": fields,
            "source_node": node.get("name", node_kind),
            "source_node_kind": node_kind,
        }

    def visit(
        node,
        family_path,
        conditions,
        parent_form_id,
        instruction_set_id,
        group_ids,
        encoding_parts,
    ):
        node_type = node.get("_type")
        current_path = family_path
        current_conditions = conditions
        current_parent = parent_form_id
        current_group_ids = group_ids
        current_parts = encoding_parts

        if node_type == "Instruction.InstructionGroup":
            current_path = family_path + [node["name"]]
            if node.get("condition") is not None:
                current_conditions = conditions + [node["condition"]]
            group_id = (
                "group::"
                + instruction_set_id
                + "::"
                + "/".join(current_path)
            )
            if group_id in groups:
                raise ValueError(f"duplicate instruction group path {group_id}")
            group_fields = []
            if node.get("encoding") is not None:
                group_part = part_from_node(node, "group")
                current_parts = encoding_parts + [group_part]
                group_fields = group_part["fields"]
            groups[group_id] = {
                "id": group_id,
                "name": node["name"],
                "parent_group_id": group_ids[-1] if group_ids else None,
                "instruction_set_id": instruction_set_id,
                "family_path": current_path,
                "feature_identifiers": feature_identifiers(node.get("condition")),
                "condition_id": condition_id(node.get("condition")),
                "encoding_fields": group_fields,
            }
            current_group_ids = group_ids + [group_id]
        elif node_type == "Instruction.Instruction":
            current_parts = encoding_parts + [part_from_node(node, "instruction")]
            encoding_width, mask, value, fields, leaf_fields = merged_encoding(
                current_parts
            )
            form_id = node["name"]
            encoding_id = (
                f"A64_{encoding_width}_{mask:0{(encoding_width + 3) // 4}X}_"
                f"{value:0{(encoding_width + 3) // 4}X}"
            )
            form_conditions = conditions[:]
            if node.get("condition") is not None:
                form_conditions.append(node["condition"])
            form = {
                "id": form_id,
                "operation_id": node["operation_id"],
                "mnemonic": assembly_mnemonic(node.get("assembly")),
                "family_path": family_path,
                "group_ids": group_ids,
                "instruction_set_id": instruction_set_id,
                "decoder_module": FAMILY_MODULES.get(
                    family_path[0] if family_path else "", "unassigned"
                ),
                "encoding_id": encoding_id,
                "encoding_width": encoding_width,
                "feature_identifiers": sorted(
                    {
                        feature
                        for condition in form_conditions
                        for feature in feature_identifiers(condition)
                    }
                ),
                "condition_id": condition_id(node.get("condition")),
                "encoding_fields": leaf_fields,
            }
            if node["operation_id"] not in operation_ids:
                raise ValueError(f"unknown operation ID {node['operation_id']}")
            forms.append(form)
            pattern_key = (encoding_width, mask, value)
            group = encoding_groups.setdefault(
                pattern_key,
                {
                    "id": encoding_id,
                    "width": encoding_width,
                    "mask": f"0x{mask:0{(encoding_width + 3) // 4}X}",
                    "value": f"0x{value:0{(encoding_width + 3) // 4}X}",
                    "specificity": mask.bit_count(),
                    "priority": mask.bit_count(),
                    "form_ids": [],
                },
            )
            if group["id"] != encoding_id:
                raise ValueError("encoding ID collision")
            group["form_ids"].append(form_id)
            current_parent = form_id
            current_conditions = form_conditions
        elif node_type == "Instruction.InstructionAlias":
            if parent_form_id is None:
                raise ValueError("instruction alias has no parent instruction form")
            alias_base = f"{parent_form_id}::alias::{node['name']}"
            alias_id_counts[alias_base] += 1
            occurrence = alias_id_counts[alias_base]
            alias_id = alias_base if occurrence == 1 else f"{alias_base}::{occurrence}"
            alias_conditions = conditions[:]
            if node.get("condition") is not None:
                alias_conditions.append(node["condition"])
            aliases.append(
                {
                    "id": alias_id,
                    "name": node["name"],
                    "parent_form_id": parent_form_id,
                    "operation_id": node["operation_id"],
                    "mnemonic": assembly_mnemonic(node.get("assembly")),
                    "family_path": family_path,
                    "instruction_set_id": instruction_set_id,
                    "group_ids": group_ids,
                    "feature_identifiers": sorted(
                        {
                            feature
                            for condition in alias_conditions
                            for feature in feature_identifiers(condition)
                        }
                    ),
                    "condition_id": condition_id(node.get("condition")),
                    "preferred_condition_id": condition_id(node.get("preferred")),
                }
            )
            if node["operation_id"] not in operation_ids:
                raise ValueError(f"unknown alias operation ID {node['operation_id']}")

        for child in node.get("children", []):
            visit(
                child,
                current_path,
                current_conditions,
                current_parent,
                instruction_set_id,
                current_group_ids,
                current_parts,
            )

    for instruction_set in source["instructions"]:
        instruction_set_id = f"instruction_set::{instruction_set['name']}"
        base_conditions = []
        if instruction_set.get("condition") is not None:
            base_conditions.append(instruction_set["condition"])
        base_parts = []
        if instruction_set.get("encoding") is not None:
            base_parts.append(part_from_node(instruction_set, "instruction_set"))
        instruction_set_fields = (
            base_parts[0]["fields"] if base_parts else []
        )
        instruction_sets.append(
            {
                "id": instruction_set_id,
                "name": instruction_set["name"],
                "feature_identifiers": feature_identifiers(
                    instruction_set.get("condition")
                ),
                "condition_id": condition_id(instruction_set.get("condition")),
                "encoding_width": base_parts[0]["width"] if base_parts else 32,
                "encoding_fields": instruction_set_fields,
            }
        )
        for child in instruction_set.get("children", []):
            visit(
                child,
                [],
                base_conditions,
                None,
                instruction_set_id,
                [],
                base_parts,
            )

    forms.sort(key=lambda item: item["id"])
    aliases.sort(key=lambda item: item["id"])
    groups = dict(sorted(groups.items()))
    instruction_sets.sort(key=lambda item: item["id"])
    encodings = sorted(encoding_groups.values(), key=lambda item: item["id"])
    for encoding in encodings:
        encoding["form_ids"].sort()

    overlap_pairs = 0
    equal_priority_pairs = 0
    numeric_patterns = []
    for encoding in encodings:
        numeric_patterns.append(
            (
                encoding["width"],
                int(encoding["mask"], 16),
                int(encoding["value"], 16),
                encoding["priority"],
            )
        )
    for index, first in enumerate(numeric_patterns):
        width_a, mask_a, value_a, priority_a = first
        for width_b, mask_b, value_b, priority_b in numeric_patterns[index + 1 :]:
            if width_a == width_b and ((value_a ^ value_b) & mask_a & mask_b) == 0:
                overlap_pairs += 1
                if priority_a == priority_b:
                    equal_priority_pairs += 1

    referenced_operation_ids = {form["operation_id"] for form in forms}
    instruction_alias_operation_ids = {
        alias["operation_id"] for alias in aliases
    }
    return {
        "catalog_version": 1,
        "source": {
            "name": "Arm AARCHMRS A-profile machine-readable specification",
            "profile": metadata["architecture"],
            "schema": metadata["schema"],
            "build": metadata["build"],
            "timestamp": metadata["timestamp"],
            "commit": SOURCE_COMMIT,
            "url": SOURCE_URL,
            "sha256": source_sha256,
            "license": "BSD-3-Clause",
        },
        "conflict_policy": {
            "exact_patterns": "deduplicate by width, mask, and value",
            "priority": "number of fixed bits in the mask",
            "higher_priority": "test the more-specific pattern first",
            "equal_priority_overlap": (
                "ambiguous until an explicit semantic guard or alias rule "
                "resolves it; never choose implicitly"
            ),
            "scope": (
                "bit-pattern overlap is a candidate conflict; feature and "
                "operand conditions are retained separately"
            ),
        },
        "metrics": {
            "operation_definitions": len(operation_records),
            "operation_alias_definitions": sum(
                item["kind"] == "operation_alias" for item in operation_records
            ),
            "referenced_operation_ids": len(referenced_operation_ids),
            "operation_ids_used_by_instruction_aliases": len(
                instruction_alias_operation_ids
            ),
            "instruction_forms": len(forms),
            "instruction_alias_records": len(aliases),
            "instruction_groups": len(groups),
            "instruction_sets": len(instruction_sets),
            "unique_encoding_patterns": len(encodings),
            "unique_alias_names": len({alias["name"] for alias in aliases}),
            "forms_by_family": dict(
                sorted(
                    Counter(form["family_path"][0] for form in forms).items()
                )
            ),
            "forms_by_decoder_module": dict(
                sorted(Counter(form["decoder_module"] for form in forms).items())
            ),
            "condition_records": len(condition_records),
            "unique_mnemonics": len(
                {form["mnemonic"] for form in forms if form["mnemonic"] is not None}
            ),
            "feature_identifiers": len(
                {
                    feature
                    for form in forms
                    for feature in form["feature_identifiers"]
                }
            ),
            "potential_overlapping_pattern_pairs": overlap_pairs,
            "equal_priority_overlapping_pattern_pairs": equal_priority_pairs,
        },
        "operations": operation_records,
        "instruction_sets": instruction_sets,
        "groups": list(groups.values()),
        "conditions": [
            {"id": identifier, "ast": condition_records[identifier]}
            for identifier in sorted(condition_records)
        ],
        "encodings": encodings,
        "instruction_forms": forms,
        "instruction_aliases": aliases,
    }


def catalog_bytes(catalog):
    return (json.dumps(catalog, indent=2, ensure_ascii=True) + "\n").encode()


def patterns_bytes(catalog):
    samples = [
        (
            f"{{0x{int(item['value'], 16):0{item['width'] // 4}X}u, "
            f"{len(item['form_ids'])}u}}"
        )
        for item in catalog["encodings"]
    ]
    return (",\n".join(f"    {sample}" for sample in samples) + "\n").encode()


def c_string_literal(value):
    encoded = value.encode("ascii")
    escaped = []
    for byte in encoded:
        if byte == 34:
            escaped.append('\\"')
        elif byte == 92:
            escaped.append("\\\\")
        elif 32 <= byte <= 126:
            escaped.append(chr(byte))
        else:
            escaped.append(f"\\{byte:03o}")
    return '"' + "".join(escaped) + '"'


def walk_condition(node):
    if isinstance(node, dict):
        yield node
        for value in node.values():
            if isinstance(value, (dict, list)):
                yield from walk_condition(value)
    elif isinstance(node, list):
        for value in node:
            if isinstance(value, (dict, list)):
                yield from walk_condition(value)


def catalog_c_bytes(catalog):
    validate_catalog(catalog)
    strings = []
    string_offsets = {}

    def intern(value):
        if value is None:
            return None
        if value not in string_offsets:
            string_offsets[value] = sum(len(item.encode("ascii")) + 1 for item in strings)
            strings.append(value)
        return string_offsets[value]

    groups_by_id = {group["id"]: group for group in catalog["groups"]}
    conditions_by_id = {
        condition["id"]: condition["ast"] for condition in catalog["conditions"]
    }
    condition_ids = sorted(conditions_by_id)
    condition_indices = {
        identifier: index for index, identifier in enumerate(condition_ids)
    }
    feature_names = sorted(
        {
            argument["value"]
            for condition in catalog["conditions"]
            for node in walk_condition(condition["ast"])
            if node.get("_type") == "AST.Function"
            and node.get("name") == "IsFeatureImplemented"
            for argument in node.get("arguments", [])
            if argument.get("_type") == "AST.Identifier"
            and argument.get("value", "").startswith("FEAT_")
        }
    )
    feature_indices = {
        name: index for index, name in enumerate(feature_names)
    }
    field_names = {
        field["name"]
        for owner in (
            catalog["instruction_sets"]
            + catalog["groups"]
            + catalog["instruction_forms"]
        )
        for field in owner["encoding_fields"]
        if field["name"] is not None
    }
    condition_symbols = {
        node["value"]
        for condition in catalog["conditions"]
        for node in walk_condition(condition["ast"])
        if node.get("_type") == "AST.Identifier"
        and not node.get("value", "").startswith("FEAT_")
    }
    symbol_names = sorted(field_names | condition_symbols)
    symbol_indices = {name: index for index, name in enumerate(symbol_names)}
    if len(feature_names) > GUEST_FEATURE_WORDS * 64:
        raise ValueError("guest feature profile capacity exceeded")
    if len(symbol_names) > 0xFFFF or len(condition_ids) > 0xFFFF:
        raise ValueError("catalog symbol or condition index capacity exceeded")
    instruction_sets = catalog["instruction_sets"]
    instruction_set_indices = {
        item["id"]: index for index, item in enumerate(instruction_sets)
    }
    group_indices = {
        item["id"]: index for index, item in enumerate(catalog["groups"])
    }
    forms_by_id = {
        form["id"]: form for form in catalog["instruction_forms"]
    }
    field_rows = []
    operand_rows = []
    feature_rows = []
    guard_rows = []
    group_path_rows = []
    group_rows = []
    instruction_set_rows = []
    form_rows = []
    alias_rows = []
    form_indices = {}
    pattern_rows = []
    condition_code_rows = []
    condition_value_rows = []
    condition_set_rows = []
    condition_rows = []

    def field_range(fields):
        first = len(field_rows)
        for field in fields:
            value_mask, value = parse_bits(
                field["value_pattern"], field["width"]
            )
            should_pattern = field["should_be_mask"]
            should_mask = 0
            if should_pattern is not None:
                for bit, required in enumerate(reversed(should_pattern)):
                    if required == "1":
                        should_mask |= 1 << bit
            field_rows.append(
                (
                    intern(field["name"]),
                    field["lsb"],
                    field["width"],
                    1 if field["kind"] == "Bits" else 0,
                    symbol_indices.get(field["name"], 0xFFFF),
                    value_mask,
                    value,
                    should_mask,
                    value & should_mask,
                )
            )
        return first, len(field_rows) - first

    def operand_kind(field_name, form):
        family = form["family_path"][0] if form["family_path"] else ""
        path_names = set(form["family_path"])
        operation = form["operation_id"]
        gpr_names = {
            "Rd", "Rn", "Rm", "Ra", "Rs", "Rt", "Rt2", "Rdn", "Rv"
        }
        if field_name in gpr_names:
            return 1
        if field_name.startswith("ZA") or field_name in {"Zda", "ZAt", "Zt"}:
            return 4
        if field_name.startswith("Z"):
            return 3
        if field_name.startswith("PN") or (
            field_name.startswith("P") and len(field_name) > 1
        ):
            return 5
        if field_name.startswith("V") and len(field_name) > 1:
            return 2
        if field_name in {"cond", "cc"}:
            return 7
        if (
            field_name in {"op0", "op1", "op2", "CRn", "CRm", "Cn", "Cm"}
            and family == "control"
            and path_names.intersection(
                {"systemmove", "systemmovepr", "systeminstrs",
                 "systeminstrswithreg", "syspairinstrs"}
            )
        ):
            return 8
        if field_name in {"option", "opt"}:
            return 10
        if field_name in {"shift", "sh"}:
            return 9
        if field_name.startswith(("imm", "simm", "off")):
            if (
                family == "simd_dp"
                and path_names.intersection({"asimdelem", "asisdelem", "asimdins"})
                and field_name not in {"immr", "imms"}
            ):
                return 11
            return 6
        if (
            family in {"control", "dpimm", "dpreg", "ldst"}
            and len(field_name) > 1
            and field_name[0] == "i"
            and field_name[1].isdigit()
        ):
            return 6
        return 0

    def operand_role(field_name, form, kind):
        family = form["family_path"][0] if form["family_path"] else ""
        path_names = set(form["family_path"])
        if field_name in {"Rdn", "Zdn", "Zda", "ZAda"}:
            return 6
        if field_name in {
            "Rd", "Rt", "Vd", "Zd", "Pd", "PNd", "Zt", "ZAt"
        }:
            return 1
        if field_name == "Rn" and family == "ldst":
            return 3
        if field_name in {
            "Rn", "Rm", "Ra", "Rs", "Rt2", "Vn", "Vm", "Zn", "Zm",
            "Pn", "Pm", "Pg", "PNg", "Zk"
        }:
            return 2
        if kind == 6:
            return 4 if field_name.startswith(("off", "simm")) else 0
        if kind == 7:
            return 5
        if kind in {8, 9, 10}:
            return 5
        if family == "ldst" and field_name == "Rt":
            return 2
        return 0

    def operand_flags(field_name, form, kind, role):
        flags = 0
        path_names = set(form["family_path"])
        is_pc_relative = bool(
            path_names.intersection(
                {"branch_imm", "compbranch_imm", "condbranch",
                 "testbranch", "loadlit", "pcreladdr", "sve_int_adr"}
            )
        )
        if field_name.startswith("simm"):
            flags |= 1
        if is_pc_relative and kind == 6:
            flags |= 2
            if field_name in {"imm14", "imm19", "imm26"}:
                flags |= 1
        if (
            kind == 1
            and path_names.intersection({"addsub_imm", "addsub_ext"})
            and field_name in {"Rd", "Rn"}
        ):
            flags |= 4
        if (
            kind == 1
            and field_name in {"Rt", "Rt2"}
            and path_names.intersection({"asisdlse", "asisdlsep", "asisdlsop"})
        ):
            flags |= 8
        return flags

    def register_width(form):
        for component in form["id"].split("_"):
            if component in {"32", "64"}:
                return int(component)
        if form["operation_id"] in {"MRS", "MSR"}:
            return 64
        return 0

    def operand_range(form):
        fields = []
        instruction_set = instruction_sets[
            instruction_set_indices[form["instruction_set_id"]]
        ]
        fields.extend(instruction_set["encoding_fields"])
        for group_id in form["group_ids"]:
            fields.extend(groups_by_id[group_id]["encoding_fields"])
        fields.extend(form["encoding_fields"])
        resolved = {}
        for field in fields:
            if field["name"] is not None:
                resolved[field["name"]] = field
        first = len(operand_rows)
        width = register_width(form)
        for name, field in sorted(
            resolved.items(), key=lambda item: (item[1]["lsb"], item[0])
        ):
            kind = operand_kind(name, form)
            role = operand_role(name, form, kind)
            operand_rows.append(
                (
                    symbol_indices[name],
                    field["lsb"],
                    field["width"],
                    kind,
                    role,
                    operand_flags(name, form, kind, role),
                    width if kind == 1 else 0,
                )
            )
        return first, len(operand_rows) - first

    family_ids = {
        "control": 0,
        "dpimm": 1,
        "dpreg": 2,
        "ldst": 3,
        "simd_dp": 4,
        "sve": 5,
        "sme": 6,
        "reserved": 7,
    }

    def addressing_mode(form):
        path_names = set(form["family_path"])
        if "loadlit" in path_names:
            return 9
        if path_names.intersection(
            {"branch_imm", "compbranch_imm", "condbranch", "testbranch",
             "pcreladdr", "sve_int_adr"}
        ):
            return 8
        if path_names.intersection({"ldstpair_pre", "ldst_immpre"}):
            return 6 if "ldstpair_pre" in path_names else 2
        if path_names.intersection({"ldstpair_post", "ldst_immpost"}):
            return 7 if "ldstpair_post" in path_names else 3
        if "ldst_regoff" in path_names:
            return 4
        if "ldstpair_off" in path_names or "ldstnapair_offs" in path_names:
            return 5
        if path_names.intersection(
            {"ldst_pos", "ldst_unscaled", "ldst_unpriv",
             "ldapstl_unscaled", "ldapstl_writeback"}
        ):
            return 1
        return 0

    def feature_range(features):
        first = len(feature_rows)
        feature_rows.extend(intern(feature) for feature in features)
        return first, len(feature_rows) - first

    def guard_range(guards):
        first = len(guard_rows)
        guard_rows.extend(guards)
        return first, len(guard_rows) - first

    def condition_value(value):
        text = value.strip("'")
        if not text or len(text) > 32:
            return None
        mask, bits = parse_bits(text, len(text))
        entry = (bits, mask, len(text))
        try:
            return condition_value_rows.index(entry)
        except ValueError:
            condition_value_rows.append(entry)
            return len(condition_value_rows) - 1

    def compile_condition_ast(node, output):
        node_type = node.get("_type") if isinstance(node, dict) else None
        if node_type == "AST.Identifier":
            symbol = symbol_indices.get(node.get("value"))
            output.append(
                (1, 0, symbol, 0) if symbol is not None else (0, 0, 0, 0)
            )
            return
        if node_type == "Values.Value":
            value_index = condition_value(node.get("value", ""))
            output.append(
                (2, 0, value_index, 0)
                if value_index is not None
                else (0, 0, 0, 0)
            )
            return
        if node_type == "AST.Bool":
            output.append((13, 0, 1 if node.get("value") else 0, 0))
            return
        if node_type == "AST.Integer":
            value = node.get("value")
            if not isinstance(value, int) or value < 0 or value > 0xFFFFFFFF:
                output.append((0, 0, 0, 0))
                return
            entry = (value, 0xFFFFFFFF, 32)
            try:
                value_index = condition_value_rows.index(entry)
            except ValueError:
                condition_value_rows.append(entry)
                value_index = len(condition_value_rows) - 1
            output.append((2, 0, value_index, 0))
            return
        if node_type == "AST.Function":
            name = node.get("name")
            arguments = node.get("arguments", [])
            preferred_functions = {
                "BFXPreferred": 1,
                "MoveWidePreferred": 2,
                "SVEMoveMaskPreferred": 3,
            }
            if name in preferred_functions:
                for argument in arguments:
                    compile_condition_ast(argument, output)
                output.append(
                    (14, len(arguments), preferred_functions[name], 0)
                )
                return
            if name == "IsFeatureImplemented" and len(arguments) == 1:
                argument = arguments[0]
                feature_index = feature_indices.get(argument.get("value"))
                if (
                    argument.get("_type") == "AST.Identifier"
                    and feature_index is not None
                ):
                    output.append((3, 0, feature_index, 0))
                    return
            supported_unary = {
                "BitCount": 9,
                "UInt": 10,
                "IsZero": 11,
                "IsOnes": 12,
            }
            if name in supported_unary and len(arguments) == 1:
                compile_condition_ast(arguments[0], output)
                output.append((supported_unary[name], 1, 0, 0))
                return
            if name in ("SysOp", "SysOp128") and arguments:
                for argument in arguments:
                    compile_condition_ast(argument, output)
                output.append((7, len(arguments), 0, 0))
                return
            for argument in arguments:
                compile_condition_ast(argument, output)
            output.append((0, len(arguments), 0, 0))
            return
        if node_type == "AST.BinaryOp":
            operator = node.get("op")
            if operator == "IN":
                compile_condition_ast(node.get("left", {}), output)
                values = node.get("right", {}).get("values", [])
                if not values or any(value.get("_type") != "Values.Value" for value in values):
                    output.append((0, 1, 0, 0))
                    return
                first = len(condition_set_rows)
                for value in values:
                    value_index = condition_value(value.get("value", ""))
                    if value_index is None:
                        output.append((0, 1, 0, 0))
                        return
                    condition_set_rows.append(value_index)
                output.append((6, 1, first, len(values)))
                return
            operators = {
                "==": 1,
                "!=": 2,
                "&&": 3,
                "||": 4,
                ">": 5,
                ">=": 6,
                "<": 7,
                "<=": 8,
                "+": 9,
                "-": 10,
                "&": 11,
                "|": 12,
                "^": 13,
            }
            compile_condition_ast(node.get("left", {}), output)
            compile_condition_ast(node.get("right", {}), output)
            operator_id = operators.get(operator)
            output.append(
                (4, 2, operator_id, 0)
                if operator_id is not None
                else (0, 2, 0, 0)
            )
            return
        if node_type == "AST.UnaryOp":
            compile_condition_ast(node.get("expr", {}), output)
            output.append(
                (5, 1, 0, 0) if node.get("op") == "!" else (0, 1, 0, 0)
            )
            return
        if node_type == "AST.Concat":
            values = node.get("values", [])
            for value in values:
                compile_condition_ast(value, output)
            output.append((7, len(values), 0, 0))
            return
        if node_type == "AST.SquareOp":
            variable = node.get("var", {})
            arguments = node.get("arguments", [])
            if len(arguments) != 1:
                output.append((0, 0, 0, 0))
                return
            compile_condition_ast(variable, output)
            compile_condition_ast(arguments[0], output)
            output.append((8, 2, 0, 0))
            return
        output.append((0, 0, 0, 0))

    for identifier in condition_ids:
        code = []
        compile_condition_ast(conditions_by_id[identifier], code)
        depth = 0
        maximum = 0
        for opcode, argument_count, _, _ in code:
            if opcode in (1, 2, 3, 13, 0):
                depth += 1 - argument_count
            elif opcode in (4, 8):
                depth -= 1
            elif opcode in (7, 14):
                depth += 1 - argument_count
            if depth < 0:
                raise ValueError(f"invalid condition stack in {identifier}")
            maximum = max(maximum, depth)
        if maximum > 64 or depth != 1 or len(code) > 0xFFFF:
            raise ValueError(f"condition program exceeds limits: {identifier}")
        first = len(condition_code_rows)
        condition_code_rows.extend(code)
        condition_rows.append((first, len(code), maximum))

    unsupported_condition_ids = {
        identifier
        for identifier, (first, count, _) in zip(condition_ids, condition_rows)
        if any(
            opcode == 0
            for opcode, _, _, _ in condition_code_rows[first:first + count]
        )
    }
    guarded_condition_ids = set()
    instruction_set_condition_ids = {
        item["condition_id"]
        for item in catalog["instruction_sets"]
        if item["condition_id"] is not None
    }
    guarded_condition_ids.update(instruction_set_condition_ids)
    guarded_condition_ids.update(
        group["condition_id"]
        for group in catalog["groups"]
        if group["condition_id"] is not None
    )
    guarded_condition_ids.update(
        form["condition_id"]
        for form in catalog["instruction_forms"]
        if form["condition_id"] is not None
    )
    unsupported_guard_condition_count = len(
        guarded_condition_ids & unsupported_condition_ids
    )

    for group in catalog["groups"]:
        field_first, field_count = field_range(group["encoding_fields"])
        feature_first, feature_count = feature_range(
            group["feature_identifiers"]
        )
        parent_index = (
            group_indices[group["parent_group_id"]]
            if group["parent_group_id"] is not None
            else 0xFFFF
        )
        group_rows.append(
            (
                intern(group["id"]),
                intern(group["name"]),
                parent_index,
                intern(group["condition_id"]),
                condition_indices.get(group["condition_id"], 0xFFFF),
                field_first,
                field_count,
                feature_first,
                feature_count,
            )
        )

    instruction_set_rows = []
    for instruction_set in instruction_sets:
        field_first, field_count = field_range(
            instruction_set["encoding_fields"]
        )
        feature_first, feature_count = feature_range(
            instruction_set["feature_identifiers"]
        )
        instruction_set_rows.append(
            (
                intern(instruction_set["id"]),
                intern(instruction_set["name"]),
                intern(instruction_set["condition_id"]),
                condition_indices.get(
                    instruction_set["condition_id"], 0xFFFF
                ),
                field_first,
                field_count,
                feature_first,
                feature_count,
            )
        )

    for encoding in catalog["encodings"]:
        form_first = len(form_rows)
        for form_id in encoding["form_ids"]:
            form = forms_by_id[form_id]
            form_indices[form_id] = len(form_rows)
            field_first, field_count = field_range(form["encoding_fields"])
            feature_first, feature_count = feature_range(
                form["feature_identifiers"]
            )
            ordered_guards = []
            instruction_set = next(
                item for item in instruction_sets
                if item["id"] == form["instruction_set_id"]
            )
            if instruction_set["condition_id"] is not None:
                ordered_guards.append(
                    (
                        condition_indices[instruction_set["condition_id"]],
                        instruction_set_indices[instruction_set["id"]],
                        0,
                        0,
                    )
                )
            for group_id in form["group_ids"]:
                condition = groups_by_id[group_id]["condition_id"]
                if condition is not None:
                    ordered_guards.append(
                        (
                            condition_indices[condition],
                            group_indices[group_id],
                            1,
                            0,
                        )
                    )
            if form["condition_id"] is not None:
                ordered_guards.append(
                    (
                        condition_indices[form["condition_id"]],
                        len(form_rows),
                        2,
                        0,
                    )
                )
            guard_first, guard_count = guard_range(ordered_guards)
            group_first = len(group_path_rows)
            group_path_rows.extend(
                group_indices[group_id] for group_id in form["group_ids"]
            )
            operand_first, operand_count = operand_range(form)
            family = form["family_path"][0] if form["family_path"] else ""
            form_rows.append(
                (
                    intern(form["id"]),
                    intern(form["operation_id"]),
                    intern(form["mnemonic"]),
                    intern(form["decoder_module"]),
                    intern(form["condition_id"]),
                    condition_indices.get(form["condition_id"], 0xFFFF),
                    field_first,
                    field_count,
                    feature_first,
                    feature_count,
                    guard_first,
                    guard_count,
                    group_first,
                    len(form["group_ids"]),
                    instruction_set_indices[form["instruction_set_id"]],
                    operand_first,
                    operand_count,
                    family_ids.get(family, family_ids["reserved"]),
                    addressing_mode(form),
                )
            )
        is_reserved = all(
            forms_by_id[form_id]["decoder_module"] == "reserved_or_unallocated"
            for form_id in encoding["form_ids"]
        )
        pattern_rows.append(
            (
                int(encoding["mask"], 16),
                int(encoding["value"], 16),
                encoding["priority"],
                form_first,
                len(encoding["form_ids"]),
                1 if is_reserved else 0,
            )
        )
    for alias in catalog["instruction_aliases"]:
        alias_rows.append(
            (
                intern(alias["name"]),
                intern(alias["operation_id"]),
                intern(alias["mnemonic"]),
                form_indices[alias["parent_form_id"]],
                condition_indices.get(alias["condition_id"], 0xFFFF),
                condition_indices.get(
                    alias["preferred_condition_id"], 0xFFFF
                ),
            )
        )

    bucket_rows = []
    bucket_offsets = [0]
    for bucket in range(64):
        compatible = [
            index
            for index, encoding in enumerate(catalog["encodings"])
            if (
                (
                    (bucket << 26) ^ int(encoding["value"], 16)
                )
                & (int(encoding["mask"], 16) & 0xFC000000)
            ) == 0
        ]
        compatible.sort(
            key=lambda index: (
                -catalog["encodings"][index]["priority"],
                index,
            )
        )
        bucket_rows.extend(compatible)
        bucket_offsets.append(len(bucket_rows))

    def pointer(offset):
        return "NULL" if offset is None else f"&AARCH64CatalogStringPool[{offset}u]"

    def pointer_array(name, rows):
        values = [pointer(offset) for offset in rows]
        if not values:
            values = ["NULL"]
        return (
            f"static const char *const {name}[] = {{\n"
            + ",\n".join(f"    {value}" for value in values)
            + "\n};\n"
        )

    feature_name_offsets = [intern(name) for name in feature_names]
    symbol_name_offsets = [intern(name) for name in symbol_names]

    output = []
    output.append(f"#define AARCH64_CATALOG_PATTERN_COUNT {len(pattern_rows)}u\n")
    output.append(f"#define AARCH64_CATALOG_FORM_COUNT {len(form_rows)}u\n")
    output.append(f"#define AARCH64_CATALOG_ALIAS_COUNT {len(alias_rows)}u\n")
    output.append(f"#define AARCH64_CATALOG_FIELD_COUNT {len(field_rows)}u\n")
    output.append(
        f"#define AARCH64_CATALOG_OPERAND_DESCRIPTOR_COUNT "
        f"{len(operand_rows)}u\n"
    )
    output.append(f"#define AARCH64_CATALOG_GROUP_COUNT {len(group_rows)}u\n")
    output.append(
        f"#define AARCH64_CATALOG_INSTRUCTION_SET_COUNT "
        f"{len(instruction_set_rows)}u\n"
    )
    output.append(f"#define AARCH64_CATALOG_BUCKET_INDEX_COUNT {len(bucket_rows)}u\n")
    output.append(f"#define AARCH64_CATALOG_CONDITION_COUNT {len(condition_rows)}u\n")
    output.append(
        f"#define AARCH64_CATALOG_CONDITION_CODE_COUNT "
        f"{len(condition_code_rows)}u\n"
    )
    output.append(
        f"#define AARCH64_CATALOG_CONDITION_VALUE_COUNT "
        f"{len(condition_value_rows)}u\n"
    )
    output.append(
        f"#define AARCH64_CATALOG_CONDITION_SET_COUNT "
        f"{len(condition_set_rows)}u\n"
    )
    output.append(f"#define AARCH64_CATALOG_FEATURE_COUNT {len(feature_names)}u\n")
    output.append(f"#define AARCH64_CATALOG_SYMBOL_COUNT {len(symbol_names)}u\n")
    output.append(
        f"#define AARCH64_CATALOG_UNSUPPORTED_CONDITION_COUNT "
        f"{len(unsupported_condition_ids)}u\n"
    )
    output.append(
        f"#define AARCH64_CATALOG_UNSUPPORTED_GUARD_CONDITION_COUNT "
        f"{unsupported_guard_condition_count}u\n"
    )
    output.append(
        f"#define AARCH64_CATALOG_GENERATED_MAX_CONDITION_STACK "
        f"{max(row[2] for row in condition_rows)}u\n"
    )
    output.append(
        "static const char AARCH64CatalogStringPool[] =\n"
        + "\n".join(
            f"    {c_string_literal(value)} \"\\000\"" for value in strings
        )
        + ";\n"
    )
    output.append(
        "static const AARCH64CatalogCondition "
        "AARCH64CatalogConditions[] = {\n"
    )
    output.append(
        ",\n".join(
            "    {%uu, %uu, %uu}"
            % (first, count, maximum)
            for first, count, maximum in condition_rows
        )
    )
    output.append("\n};\n")
    output.append(
        "static const AARCH64CatalogExpressionInstruction "
        "AARCH64CatalogConditionCode[] = {\n"
    )
    output.append(
        ",\n".join(
            "    {%uu, %uu, %uu, %uu}"
            % (opcode, argument_count, operand or 0, auxiliary)
            for opcode, argument_count, operand, auxiliary
            in condition_code_rows
        )
    )
    output.append("\n};\n")
    output.append(
        "static const AARCH64CatalogExpressionValue "
        "AARCH64CatalogConditionValues[] = {\n"
    )
    output.append(
        ",\n".join(
            "    {0x%08Xu, 0x%08Xu, %uu, {0u, 0u, 0u}}"
            % (value, mask, width)
            for value, mask, width in condition_value_rows
        )
    )
    output.append("\n};\n")
    output.append("static const uint16_t AARCH64CatalogConditionSetValues[] = {\n")
    output.append(
        ",\n".join(f"    {index}u" for index in condition_set_rows)
        if condition_set_rows else "    0u"
    )
    output.append("\n};\n")
    output.append("static const AARCH64CatalogField AARCH64CatalogFields[] = {\n")
    output.append(
        ",\n".join(
            "    {%s, %uu, %uu, %uu, 0u, %uu, 0x%08Xu, 0x%08Xu, "
            "0x%08Xu, 0x%08Xu}"
            % (
                pointer(name),
                lsb,
                width,
                kind,
                symbol_index,
                value_mask,
                value,
                should_mask,
                should_value,
            )
            for name, lsb, width, kind, symbol_index, value_mask, value,
            should_mask, should_value
            in field_rows
        )
    )
    output.append("\n};\n")
    output.append(
        "static const AARCH64CatalogOperandDescriptor "
        "AARCH64CatalogOperandDescriptors[] = {\n"
    )
    output.append(
        ",\n".join(
            "    {%uu, %uu, %uu, %uu, %uu, %uu, %uu}"
            % (symbol, lsb, width, kind, role, flags, register_width)
            for symbol, lsb, width, kind, role, flags, register_width
            in operand_rows
        )
    )
    output.append("\n};\n")
    output.append(
        pointer_array(
            "AARCH64CatalogFeatureNames",
            feature_name_offsets,
        )
    )
    output.append(
        pointer_array(
            "AARCH64CatalogSymbolNames",
            symbol_name_offsets,
        )
    )
    output.append(pointer_array("AARCH64CatalogFeatureStrings", feature_rows))
    output.append("static const AARCH64CatalogGuard AARCH64CatalogGuards[] = {\n")
    output.append(
        ",\n".join(
            "    {%uu, %uu, %uu, 0u}"
            % (condition_index, scope_index, scope_kind)
            for condition_index, scope_index, scope_kind, _ in guard_rows
        )
        if guard_rows else "    {65535u, 0u, 0u, 0u}"
    )
    output.append("\n};\n")
    output.append("static const uint16_t AARCH64CatalogFormGroupIndices[] = {\n")
    output.append(
        ",\n".join(f"    {index}u" for index in group_path_rows)
        if group_path_rows else "    0u"
    )
    output.append("\n};\n")
    output.append("static const AARCH64CatalogGroup AARCH64CatalogGroups[] = {\n")
    output.append(
        ",\n".join(
            "    {%s, %s, %uu, %s, %uu, &AARCH64CatalogFields[%uu], %uu, "
            "&AARCH64CatalogFeatureStrings[%uu], %uu}"
            % (
                pointer(identifier),
                pointer(name),
                parent,
                pointer(condition),
                condition_index,
                field_first,
                field_count,
                feature_first,
                feature_count,
            )
            for identifier, name, parent, condition, condition_index,
            field_first, field_count, feature_first, feature_count in group_rows
        )
    )
    output.append("\n};\n")
    output.append(
        "static const AARCH64CatalogInstructionSet "
        "AARCH64CatalogInstructionSets[] = {\n"
    )
    output.append(
        ",\n".join(
            "    {%s, %s, %s, %uu, &AARCH64CatalogFields[%uu], %uu, "
            "&AARCH64CatalogFeatureStrings[%uu], %uu}"
            % (
                pointer(identifier),
                pointer(name),
                pointer(condition),
                condition_index,
                field_first,
                field_count,
                feature_first,
                feature_count,
            )
            for identifier, name, condition, condition_index, field_first,
            field_count, feature_first, feature_count in instruction_set_rows
        )
    )
    output.append("\n};\n")
    output.append("static const AARCH64CatalogForm AARCH64CatalogForms[] = {\n")
    output.append(
        ",\n".join(
            "    {%s, %s, %s, %s, %s, %uu, %s, %uu, %s, %uu, "
            "%s, %uu, %s, %uu, %uu, %s, %uu, %uu, %uu}"
            % (
                pointer(identifier),
                pointer(operation),
                pointer(mnemonic),
                pointer(module),
                pointer(condition),
                condition_index,
                "NULL" if field_count == 0 else
                    f"&AARCH64CatalogFields[{field_first}u]",
                field_count,
                "NULL" if feature_count == 0 else
                    f"&AARCH64CatalogFeatureStrings[{feature_first}u]",
                feature_count,
                "NULL" if guard_count == 0 else
                    f"&AARCH64CatalogGuards[{guard_first}u]",
                guard_count,
                "NULL" if group_count == 0 else
                    f"&AARCH64CatalogFormGroupIndices[{group_first}u]",
                group_count,
                instruction_set_index,
                "NULL" if operand_count == 0 else
                    f"&AARCH64CatalogOperandDescriptors[{operand_first}u]",
                operand_count,
                family,
                addressing_mode_value,
            )
            for identifier, operation, mnemonic, module, condition,
            condition_index, field_first, field_count, feature_first,
            feature_count, guard_first, guard_count, group_first, group_count,
            instruction_set_index, operand_first, operand_count, family,
            addressing_mode_value
            in form_rows
        )
    )
    output.append("\n};\n")
    output.append("static const AARCH64CatalogAlias AARCH64CatalogAliases[] = {\n")
    output.append(
        ",\n".join(
            "    {%s, %s, %s, %uu, %uu, %uu}"
            % (
                pointer(name),
                pointer(operation),
                pointer(mnemonic),
                form_index,
                condition_index,
                preferred_condition_index,
            )
            for name, operation, mnemonic, form_index, condition_index,
            preferred_condition_index in alias_rows
        )
        if alias_rows else "    {NULL, NULL, NULL, 0u, 65535u, 65535u}"
    )
    output.append("\n};\n")
    output.append("static const AARCH64CatalogPattern AARCH64CatalogPatterns[] = {\n")
    output.append(
        ",\n".join(
            "    {0x%08Xu, 0x%08Xu, %uu, %uu, %uu, %uu, "
            "&AARCH64CatalogForms[%uu]}"
            % (mask, value, priority, first_form, form_count, flags, first_form)
            for mask, value, priority, first_form, form_count, flags in pattern_rows
        )
    )
    output.append("\n};\n")
    output.append("static const uint32_t AARCH64CatalogBucketOffsets[65] = {\n")
    output.append(",\n".join(f"    {offset}u" for offset in bucket_offsets))
    output.append("\n};\n")
    output.append("static const uint16_t AARCH64CatalogBucketPatterns[] = {\n")
    output.append(
        ",\n".join(f"    {index}u" for index in bucket_rows)
        if bucket_rows else "    0u"
    )
    output.append("\n};\n")
    output.append(
        "static const uint32_t AARCH64CatalogGeneratedStorageBytes = "
        "(uint32_t)(sizeof(AARCH64CatalogStringPool) + "
        "sizeof(AARCH64CatalogConditions) + "
        "sizeof(AARCH64CatalogConditionCode) + "
        "sizeof(AARCH64CatalogConditionValues) + "
        "sizeof(AARCH64CatalogConditionSetValues) + "
        "sizeof(AARCH64CatalogFields) + "
        "sizeof(AARCH64CatalogOperandDescriptors) + "
        "sizeof(AARCH64CatalogFeatureNames) + "
        "sizeof(AARCH64CatalogSymbolNames) + "
        "sizeof(AARCH64CatalogFeatureStrings) + "
        "sizeof(AARCH64CatalogGuards) + "
        "sizeof(AARCH64CatalogFormGroupIndices) + "
        "sizeof(AARCH64CatalogGroups) + "
        "sizeof(AARCH64CatalogInstructionSets) + "
        "sizeof(AARCH64CatalogForms) + "
        "sizeof(AARCH64CatalogAliases) + "
        "sizeof(AARCH64CatalogPatterns) + "
        "sizeof(AARCH64CatalogBucketOffsets) + "
        "sizeof(AARCH64CatalogBucketPatterns));\n"
    )
    return "".join(output).encode("ascii")


def overlap_kind(first, second):
    width_a, mask_a, value_a, priority_a = first
    width_b, mask_b, value_b, priority_b = second
    if width_a != width_b:
        return "disjoint"
    if mask_a == mask_b and value_a == value_b:
        return "exact"
    if ((value_a ^ value_b) & mask_a & mask_b) != 0:
        return "disjoint"
    if priority_a != priority_b:
        return "ordered"
    return "ambiguous"


def fields_pattern(fields, width):
    mask = 0
    value = 0
    occupied = 0
    for field in fields:
        start = field["lsb"]
        field_width = field["width"]
        if start < 0 or field_width <= 0 or start + field_width > width:
            raise ValueError("encoding field range is invalid")
        field_range = ((1 << field_width) - 1) << start
        if occupied & field_range:
            raise ValueError("encoding fields overlap within a source node")
        occupied |= field_range
        field_mask, field_value = parse_bits(
            field["value_pattern"], field_width
        )
        mask |= field_mask << start
        value |= field_value << start
        should_be_mask = field["should_be_mask"]
        if should_be_mask is not None and (
            len(should_be_mask) != field_width
            or any(bit not in "01x" for bit in should_be_mask)
        ):
            raise ValueError("invalid should-be mask")
    return mask, value


def validate_catalog(catalog):
    test_conflicts = (
        (overlap_kind((32, 0xF0, 0xA0, 4), (32, 0xF0, 0xA0, 4)) == "exact")
        and (overlap_kind((32, 0xF0, 0xA0, 4), (32, 0xF0, 0xB0, 4)) == "disjoint")
        and (overlap_kind((32, 0xF0, 0xA0, 4), (32, 0xF8, 0xA0, 5)) == "ordered")
        and (overlap_kind((32, 0xC0, 0x00, 2), (32, 0x30, 0x00, 2)) == "ambiguous")
    )
    if not test_conflicts:
        raise ValueError("synthetic encoding conflict checks failed")

    required_top_level = {
        "catalog_version",
        "source",
        "conflict_policy",
        "metrics",
        "operations",
        "instruction_sets",
        "groups",
        "conditions",
        "encodings",
        "instruction_forms",
        "instruction_aliases",
    }
    if set(catalog) != required_top_level:
        raise ValueError("catalog top-level schema is incomplete or unexpected")
    if catalog["catalog_version"] != 1:
        raise ValueError("unsupported catalog format version")
    if (
        catalog["source"]["commit"] != SOURCE_COMMIT
        or catalog["source"]["sha256"] != SOURCE_SHA256
        or catalog["source"]["license"] != "BSD-3-Clause"
    ):
        raise ValueError("catalog provenance does not match the pinned source")

    operation_ids = [item["id"] for item in catalog["operations"]]
    if len(operation_ids) != len(set(operation_ids)):
        raise ValueError("duplicate operation ID")
    operation_id_set = set(operation_ids)

    instruction_set_ids = [
        item["id"] for item in catalog["instruction_sets"]
    ]
    if len(instruction_set_ids) != len(set(instruction_set_ids)):
        raise ValueError("duplicate instruction-set ID")
    instruction_set_map = {
        item["id"]: item for item in catalog["instruction_sets"]
    }

    group_ids = [item["id"] for item in catalog["groups"]]
    if len(group_ids) != len(set(group_ids)):
        raise ValueError("duplicate instruction-group ID")
    group_map = {item["id"]: item for item in catalog["groups"]}
    for group in catalog["groups"]:
        if group["instruction_set_id"] not in instruction_set_map:
            raise ValueError(f"unknown instruction set in group {group['id']}")
        parent_id = group["parent_group_id"]
        if parent_id is not None and parent_id not in group_map:
            raise ValueError(f"unknown parent group in {group['id']}")
        if parent_id is not None:
            if (
                group_map[parent_id]["instruction_set_id"]
                != group["instruction_set_id"]
            ):
                raise ValueError(f"instruction set mismatch in group {group['id']}")
        fields_pattern(
            group["encoding_fields"],
            instruction_set_map[group["instruction_set_id"]]["encoding_width"],
        )

    condition_ids = [item["id"] for item in catalog["conditions"]]
    if len(condition_ids) != len(set(condition_ids)):
        raise ValueError("duplicate condition ID")
    condition_id_set = set(condition_ids)
    if catalog["metrics"]["condition_records"] != len(condition_ids):
        raise ValueError("condition metric mismatch")

    def validate_condition_reference(identifier, owner):
        if identifier is not None and identifier not in condition_id_set:
            raise ValueError(f"unknown condition ID in {owner}")

    for instruction_set in catalog["instruction_sets"]:
        validate_condition_reference(
            instruction_set["condition_id"], instruction_set["id"]
        )
    for group in catalog["groups"]:
        validate_condition_reference(group["condition_id"], group["id"])

    encoding_ids = [item["id"] for item in catalog["encodings"]]
    if len(encoding_ids) != len(set(encoding_ids)):
        raise ValueError("duplicate encoding ID")
    encoding_map = {item["id"]: item for item in catalog["encodings"]}
    form_ids = [item["id"] for item in catalog["instruction_forms"]]
    if len(form_ids) != len(set(form_ids)):
        raise ValueError("duplicate instruction form ID")
    form_map = {item["id"]: item for item in catalog["instruction_forms"]}
    forms_by_encoding = defaultdict(set)
    referenced_operation_ids = set()

    for instruction_set in catalog["instruction_sets"]:
        fields_pattern(
            instruction_set["encoding_fields"],
            instruction_set["encoding_width"],
        )

    for encoding in catalog["encodings"]:
        width = encoding.get("width")
        if not isinstance(width, int) or width <= 0 or width > 64:
            raise ValueError(f"invalid width for encoding {encoding['id']}")
        mask = int(encoding["mask"], 16)
        value = int(encoding["value"], 16)
        limit = (1 << width) - 1
        if mask & ~limit or value & ~mask:
            raise ValueError(f"mask/value out of range for encoding {encoding['id']}")
        if encoding["specificity"] != mask.bit_count():
            raise ValueError(f"incorrect specificity for encoding {encoding['id']}")
        if encoding["priority"] != encoding["specificity"]:
            raise ValueError(f"incorrect priority for encoding {encoding['id']}")
        if not encoding["form_ids"]:
            raise ValueError(f"encoding {encoding['id']} has no instruction forms")
        for form_id in encoding["form_ids"]:
            if form_id not in form_map:
                raise ValueError(f"unknown form {form_id} in encoding {encoding['id']}")

    for form in catalog["instruction_forms"]:
        if form["operation_id"] not in operation_id_set:
            raise ValueError(f"unknown operation ID in form {form['id']}")
        if form["encoding_id"] not in encoding_map:
            raise ValueError(f"unknown encoding ID in form {form['id']}")
        encoding = encoding_map[form["encoding_id"]]
        if form["id"] not in encoding["form_ids"]:
            raise ValueError(f"form {form['id']} is missing from its encoding")
        if form["encoding_width"] != encoding["width"]:
            raise ValueError(f"width mismatch for form {form['id']}")
        instruction_set = instruction_set_map.get(form["instruction_set_id"])
        if instruction_set is None:
            raise ValueError(f"unknown instruction set in form {form['id']}")
        validate_condition_reference(form["condition_id"], form["id"])
        expected_parent = None
        path_names = []
        for group_id in form["group_ids"]:
            group = group_map.get(group_id)
            if group is None:
                raise ValueError(f"unknown group {group_id} in form {form['id']}")
            if (
                group["instruction_set_id"] != form["instruction_set_id"]
                or group["parent_group_id"] != expected_parent
            ):
                raise ValueError(f"invalid group path in form {form['id']}")
            expected_parent = group_id
            path_names.append(group["name"])
        if path_names != form["family_path"]:
            raise ValueError(f"family path mismatch in form {form['id']}")
        mask = 0
        value = 0
        parts = [instruction_set["encoding_fields"]]
        for group_id in form["group_ids"]:
            group = group_map.get(group_id)
            if group is None:
                raise ValueError(f"unknown group {group_id} in form {form['id']}")
            parts.append(group["encoding_fields"])
        parts.append(form["encoding_fields"])
        for fields in parts:
            part_mask, part_value = fields_pattern(
                fields, form["encoding_width"]
            )
            if ((value ^ part_value) & mask & part_mask) != 0:
                raise ValueError(f"conflicting inherited bits in form {form['id']}")
            value = (value & ~part_mask) | part_value
            mask |= part_mask
        if (
            mask != int(encoding["mask"], 16)
            or value != int(encoding["value"], 16)
        ):
            raise ValueError(f"inherited mask/value mismatch in form {form['id']}")
        forms_by_encoding[form["encoding_id"]].add(form["id"])
        referenced_operation_ids.add(form["operation_id"])

    for encoding in catalog["encodings"]:
        if set(encoding["form_ids"]) != forms_by_encoding[encoding["id"]]:
            raise ValueError(f"form membership mismatch in encoding {encoding['id']}")

    alias_ids = [item["id"] for item in catalog["instruction_aliases"]]
    if len(alias_ids) != len(set(alias_ids)):
        raise ValueError("duplicate instruction alias ID")
    for alias in catalog["instruction_aliases"]:
        if alias["parent_form_id"] not in form_map:
            raise ValueError(f"unknown parent form in alias {alias['id']}")
        if alias["operation_id"] not in operation_id_set:
            raise ValueError(f"unknown operation ID in alias {alias['id']}")
        if alias["instruction_set_id"] not in instruction_set_map:
            raise ValueError(f"unknown instruction set in alias {alias['id']}")
        expected_parent = None
        path_names = []
        for group_id in alias["group_ids"]:
            group = group_map.get(group_id)
            if group is None:
                raise ValueError(f"unknown group in alias {alias['id']}")
            if (
                group["instruction_set_id"] != alias["instruction_set_id"]
                or group["parent_group_id"] != expected_parent
            ):
                raise ValueError(f"invalid group path in alias {alias['id']}")
            expected_parent = group_id
            path_names.append(group["name"])
        if path_names != alias["family_path"]:
            raise ValueError(f"family path mismatch in alias {alias['id']}")
        parent_form = form_map[alias["parent_form_id"]]
        if (
            parent_form["instruction_set_id"] != alias["instruction_set_id"]
            or parent_form["group_ids"] != alias["group_ids"]
        ):
            raise ValueError(f"parent path mismatch in alias {alias['id']}")
        validate_condition_reference(alias["condition_id"], alias["id"])
        validate_condition_reference(
            alias["preferred_condition_id"], alias["id"]
        )

    metrics = catalog["metrics"]
    if metrics["operation_definitions"] != len(operation_ids):
        raise ValueError("operation definition metric mismatch")
    if metrics["operation_alias_definitions"] != sum(
        item["kind"] == "operation_alias" for item in catalog["operations"]
    ):
        raise ValueError("operation alias definition metric mismatch")
    if metrics["referenced_operation_ids"] != len(referenced_operation_ids):
        raise ValueError("referenced operation ID metric mismatch")
    if metrics["operation_ids_used_by_instruction_aliases"] != len(
        {alias["operation_id"] for alias in catalog["instruction_aliases"]}
    ):
        raise ValueError("instruction alias operation ID metric mismatch")
    if metrics["instruction_forms"] != len(form_ids):
        raise ValueError("instruction form metric mismatch")
    if metrics["instruction_alias_records"] != len(alias_ids):
        raise ValueError("instruction alias metric mismatch")
    if metrics["instruction_groups"] != len(group_ids):
        raise ValueError("instruction group metric mismatch")
    if metrics["instruction_sets"] != len(instruction_set_ids):
        raise ValueError("instruction set metric mismatch")
    if metrics["unique_encoding_patterns"] != len(encoding_ids):
        raise ValueError("encoding pattern metric mismatch")
    if metrics["unique_alias_names"] != len(
        {alias["name"] for alias in catalog["instruction_aliases"]}
    ):
        raise ValueError("unique alias name metric mismatch")
    if metrics["forms_by_family"] != dict(
        sorted(
            Counter(
                form["family_path"][0]
                for form in catalog["instruction_forms"]
            ).items()
        )
    ):
        raise ValueError("family coverage metric mismatch")
    if metrics["forms_by_decoder_module"] != dict(
        sorted(
            Counter(
                form["decoder_module"]
                for form in catalog["instruction_forms"]
            ).items()
        )
    ):
        raise ValueError("decoder module coverage metric mismatch")

    potential_overlaps = 0
    equal_priority_overlaps = 0
    patterns = [
        (
            item["width"],
            int(item["mask"], 16),
            int(item["value"], 16),
            item["priority"],
        )
        for item in catalog["encodings"]
    ]
    for index, first in enumerate(patterns):
        for second in patterns[index + 1 :]:
            kind = overlap_kind(first, second)
            if kind in ("ordered", "ambiguous"):
                potential_overlaps += 1
            if kind == "ambiguous":
                equal_priority_overlaps += 1
    if (
        metrics["potential_overlapping_pattern_pairs"] != potential_overlaps
        or metrics["equal_priority_overlapping_pattern_pairs"]
        != equal_priority_overlaps
    ):
        raise ValueError("encoding conflict metrics mismatch")
    if not catalog["conflict_policy"]["equal_priority_overlap"].startswith(
        "ambiguous until"
    ):
        raise ValueError("equal-priority conflict policy must reject ambiguity")
    return potential_overlaps, equal_priority_overlaps


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "mode", choices=("generate", "generate-c", "check", "validate")
    )
    parser.add_argument("--source", type=Path)
    parser.add_argument("--catalog", type=Path, default=CATALOG_PATH)
    parser.add_argument("--patterns", type=Path, default=PATTERNS_PATH)
    parser.add_argument("--c-output", type=Path, default=CATALOG_C_PATH)
    arguments = parser.parse_args()

    if arguments.mode in ("generate", "check"):
        if arguments.source is None:
            parser.error("--source is required for generate and check")
        catalog = build_catalog(arguments.source)
        catalog_content = catalog_bytes(catalog)
        patterns_content = patterns_bytes(catalog)
        c_content = catalog_c_bytes(catalog)
        validate_catalog(catalog)
        if arguments.mode == "generate":
            arguments.catalog.write_bytes(catalog_content)
            arguments.patterns.write_bytes(patterns_content)
            arguments.c_output.write_bytes(c_content)
        else:
            if arguments.catalog.read_bytes() != catalog_content:
                raise ValueError("catalog differs from pinned source")
            if arguments.patterns.read_bytes() != patterns_content:
                raise ValueError("catalog pattern fixture differs from pinned source")
            if arguments.c_output.read_bytes() != c_content:
                raise ValueError("generated C catalog differs from pinned source")
    elif arguments.mode == "generate-c":
        catalog = json.loads(arguments.catalog.read_text())
        validate_catalog(catalog)
        if arguments.patterns.read_bytes() != patterns_bytes(catalog):
            raise ValueError("catalog pattern fixture is stale")
        arguments.c_output.write_bytes(catalog_c_bytes(catalog))
    else:
        catalog = json.loads(arguments.catalog.read_text())
        potential_overlaps, equal_priority_overlaps = validate_catalog(catalog)
        if arguments.patterns.read_bytes() != patterns_bytes(catalog):
            raise ValueError("catalog pattern fixture is stale")
        if arguments.c_output.read_bytes() != catalog_c_bytes(catalog):
            raise ValueError("generated C catalog is stale")
        print(
            "AARCHMRS catalog: "
            f"{catalog['metrics']['instruction_forms']} instruction forms, "
            f"{catalog['metrics']['instruction_alias_records']} aliases, "
            f"{catalog['metrics']['unique_encoding_patterns']} encoding patterns, "
            f"{len(catalog['instruction_forms'])} generated forms"
        )
        print(
            f"Potential pattern overlaps: {potential_overlaps}; "
            f"equal-priority ambiguities: {equal_priority_overlaps}"
        )

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"catalog validation failed: {error}", file=sys.stderr)
        raise SystemExit(1)
