import collections
import csv
import hashlib
import io
import json
import sys

import integer_form_differential as reference
import integer_immediate_differential as immediate


REPORT = "Tests/Decoder/AARCH64/Results/bitmask_exhaustive.csv"
REPORT_SHA256 = "ffbd31d7d7a6ad071602fc44b7b8fc776e0dc33cf62eaed80374f595ab4d8a3c"
OPERATIONS = {"AND", "ORR", "EOR", "ANDS", "BFM", "SBFM", "UBFM"}


def oracle(width, n, immr, imms, logical):
    if width == 32 and n:
        return None, "N_WIDTH"
    selector = (n << 6) | ((~imms) & 63)
    length = selector.bit_length() - 1
    if length < 1:
        return None, "LEN"
    esize = 1 << length
    if esize > width:
        return None, "ESIZE"
    levels = esize - 1
    s, r = imms & levels, immr & levels
    if logical and s == levels:
        return None, "ALL_ONES"
    diff = (s - r) & 63
    d = diff & levels
    welem = sum(1 << bit for bit in range(s + 1))
    telem = sum(1 << bit for bit in range(d + 1))
    wmask = sum(((welem >> ((bit + r) % esize)) & 1) << bit
                for bit in range(width))
    tmask = sum(((telem >> (bit % esize)) & 1) << bit
                for bit in range(width))
    return (length, levels, s, r, diff, esize, welem, telem, wmask, tmask), "VALID"


def main(probe):
    with open(reference.CATALOG, encoding="utf-8") as stream:
        catalog = json.load(stream)
    forms = [form for form in catalog["instruction_forms"]
             if form["family_path"][1] in ("bitfield", "log_imm")]
    if len(forms) != 14:
        raise RuntimeError("bitmask form inventory changed")
    patterns = {item["id"]: item for item in catalog["encodings"]}
    requests = []
    meta = []
    for form in forms:
        if form["mnemonic"] not in OPERATIONS:
            raise RuntimeError(f"unexpected operation: {form['id']}")
        pattern = patterns[form["encoding_id"]]
        seed, fixed = int(pattern["value"], 16), int(pattern["mask"], 16)
        width = 64 if "_64" in form["id"] else 32
        logical = form["family_path"][1] == "log_imm"
        for n in range(2):
            for immr in range(64):
                for imms in range(64):
                    word = (seed & ~0x007FFC00) | (n << 22) | (immr << 16) | (imms << 10)
                    if (word & (fixed & ~0x00400000)) != (seed & ~0x00400000):
                        raise RuntimeError(f"fixed encoding changed: {form['id']}")
                    requests.append((form["id"], word))
                    meta.append((form, width, n, immr, imms, logical))
    if len(requests) != 114688 or len(set(requests)) != 114688:
        raise RuntimeError("exhaustive domain incomplete")
    responses = reference.probe_rows(probe, requests)
    llvm = reference.llvm_disassemble([word for _, word in requests])
    with open("Tests/Decoder/AARCH64/Results/integer_immediate_all_candidates.csv",
              newline="", encoding="utf-8") as stream:
        transferred = [row for row in csv.DictReader(stream)
                       if row["group"] == "log_imm"]
    if len(transferred) != 1016:
        raise RuntimeError("transferred cases changed")
    transferred_by_key = {(row["form"], int(row["encoding"], 16)): row
                          for row in transferred}
    witnessed = set()
    rejected_transfer = collections.Counter()
    counts = collections.Counter()
    coverage = collections.defaultdict(set)
    output = io.StringIO(newline="")
    writer = csv.writer(output)
    writer.writerow(("form", "operation", "width", "encoding", "N", "immr",
                     "imms", "status", "len", "levels", "S", "R", "diff",
                     "esize", "welem", "telem", "wmask", "tmask", "llvm"))
    for (identifier, word), response, text, item in zip(requests, responses, llvm, meta):
        form, width, n, immr, imms, logical = item
        result, status = oracle(width, n, immr, imms, logical)
        if not logical and n != (width == 64):
            result, status = None, "BITFIELD_N_WIDTH"
        elif not logical and width == 32 and (immr >= 32 or imms >= 32):
            result, status = None, "BITFIELD_IMM_BIT5"
        valid = result is not None
        key = identifier, word
        if key in transferred_by_key:
            old = transferred_by_key[key]
            if (old["probe"] == "REJECT") != (not valid):
                raise RuntimeError(f"transferred probe mismatch: {key} {status}")
            if old["probe"] == "REJECT":
                rejected_transfer[status] += 1
            witnessed.add(key)
        if (response is not None) != valid or (text != "<llvm-invalid>") != valid:
            raise RuntimeError(f"validity divergence: {identifier} {word:08X} {status} {text}")
        if valid:
            _, _, _, _, _, _, _, _, wmask, tmask = result
            operands = response[2]
            names = ("logical_immediate",) if logical else (
                "bitfield_write_mask", "bitfield_top_mask")
            expected = (wmask,) if logical else (wmask, tmask)
            for name, value in zip(names, expected):
                if name not in operands or operands[name]["value"] != value:
                    raise RuntimeError(f"mask mismatch: {identifier} {word:08X} {name}")
            if operands["Rd"]["width"] != width:
                raise RuntimeError(f"register width mismatch: {identifier} {word:08X}")
            if operands["immr"]["raw"] != immr or operands["imms"]["raw"] != imms:
                raise RuntimeError(f"raw parameter mismatch: {identifier} {word:08X}")
            if logical:
                printed = immediate.numbers(text)
                if not printed or (printed[-1] & ((1 << width) - 1)) != wmask:
                    raise RuntimeError(f"LLVM mask mismatch: {identifier} {word:08X} {text}")
            else:
                outcome, detail = immediate.check(form, word, operands, text)
                if outcome != "verified":
                    raise RuntimeError(f"LLVM bitfield alias mismatch: {identifier} {word:08X} {outcome} {detail}")
            coverage[(identifier, width)].add((immr, imms))
        counts[(form["mnemonic"], width, status)] += 1
        writer.writerow((identifier, form["mnemonic"], width, f"{word:08X}",
                         n, immr, imms, status, *(result if valid else ("",) * 10), text))
    if witnessed != set(transferred_by_key):
        raise RuntimeError(f"unresolved transfer: {len(set(transferred_by_key) - witnessed)}")
    if rejected_transfer != {"ALL_ONES": 40, "LEN": 16}:
        raise RuntimeError(f"56 rejected masks changed: {rejected_transfer}")
    data = output.getvalue().encode("utf-8")
    digest = hashlib.sha256(data).hexdigest()
    if digest != REPORT_SHA256:
        raise RuntimeError(f"exhaustive report hash changed: {digest}")
    with open(REPORT, "rb") as stream:
        if stream.read() != data:
            raise RuntimeError("exhaustive report differs from pinned baseline")
    print("Bitmask exhaustive:", len(requests), "transferred=", len(witnessed),
          "forms=", len(coverage), "rejected=", dict(rejected_transfer),
          "sha256=", digest)
    for key in sorted(counts):
        print("MASK", *key, counts[key])


if __name__ == "__main__":
    try:
        if len(sys.argv) != 2:
            raise ValueError("usage: bitmask_exhaustive.py PROBE")
        main(sys.argv[1])
    except (OSError, ValueError, RuntimeError, IndexError) as error:
        print("Bitmask exhaustive failed:", error, file=sys.stderr)
        raise SystemExit(1)