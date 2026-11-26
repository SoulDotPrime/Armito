#!/usr/bin/env python3
"""Gera Tests/Data/aarch64_golden.txt a partir dos dumps do firmware do Mac mini M2.

Fonte da verdade: os bytes reais dos segmentos extraidos (Docs/FIRMWARE/extracted/
*.bin), nao o .dis. Para cada word de 32 bits, o llvm-mc (triple aarch64, o mesmo
alvo ARM64E subtipo 2) e chamado uma vez para obter o mnemonico. Words que o
llvm-mc recusa ("invalid instruction encoding") sao marcadas como
`invalid-encoding` e servem de caso negativo para o decoder.

Saida: Tests/Data/aarch64_golden.txt
    <hex> <mnemonico> <ocorrencias>
"""
import os, sys, struct, subprocess, collections

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
PAIRS = [
    ("sptm_TEXT_EXEC", 0xfffffff0270b4000),
    ("txm_TEXT_EXEC", 0xfffffff017034000),
    ("txm_TEXT_BOOT_EXEC", 0xfffffff017088000),
]

def disasm_words(words):
    """Devolve {word: mnemonico}, com 'invalid-encoding' para os recusados."""
    txt = "".join(
        "0x%02x 0x%02x 0x%02x 0x%02x\n" % (w & 0xff, (w >> 8) & 0xff, (w >> 16) & 0xff, (w >> 24) & 0xff)
        for w in words
    )
    r = subprocess.run(["llvm-mc", "--disassemble", "--triple=aarch64"],
                       input=txt, capture_output=True, text=True)
    bad_lines = set()
    for line in r.stderr.splitlines():
        if "invalid instruction encoding" in line:
            bad_lines.add(int(line.split(":")[1].split(":")[0]))
    good = [l.strip() for l in r.stdout.splitlines() if l.strip() and l.strip() != ".text"]
    if len(good) + len(bad_lines) != len(words):
        sys.exit("contagem inconsistente: %d bons + %d ruins != %d words"
                 % (len(good), len(bad_lines), len(words)))
    result = {}
    gi = 0
    for i, w in enumerate(words, start=1):
        if i in bad_lines:
            result.setdefault(w, "invalid-encoding")
        else:
            result.setdefault(w, good[gi].split()[0])
            gi += 1
    return result

def main():
    out_dir = os.path.join(ROOT, "Tests", "Data")
    os.makedirs(out_dir, exist_ok=True)
    counter = collections.Counter()
    mnems = {}
    total = 0
    for name, base in PAIRS:
        with open(os.path.join(HERE, "extracted", name + ".bin"), "rb") as f:
            data = f.read()
        words = struct.unpack("<%dI" % (len(data) // 4), data)
        total += len(words)
        mapping = disasm_words(words)
        for w, m in mapping.items():
            mnems[w] = m
        for w in words:
            counter[w] += 1
    path = os.path.join(out_dir, "aarch64_golden.txt")
    with open(path, "w") as f:
        f.write("# codificacao mnemonico ocorrencias  (Mac mini M2, ARM64E subtipo 2)\n")
        f.write("# gerado por Docs/FIRMWARE/gen_golden.py a partir de %d instrucoes reais\n" % total)
        for w in sorted(counter):
            f.write("%08x %s %d\n" % (w, mnems[w], counter[w]))
    inval = sum(1 for w in mnems if mnems[w] == "invalid-encoding")
    print("instrucoes lidas:", total)
    print("codificacoes distintas:", len(counter))
    print("invalidas para o llvm-mc:", inval)
    print("arquivo:", path)

if __name__ == "__main__":
    main()
