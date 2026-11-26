# AArch64 Decoder

Status: partial implementation. The decoder uses a byte-buffer source and
does not dereference guest virtual addresses.

## Input contract

`ArmitoDecoderSourceSet` provides a readable byte buffer, its size, guest base
address, and executable/readable flags. AArch64 instructions are 4 bytes,
little-endian, and require 4-byte alignment. Out-of-range and truncated reads
are reported as explicit decoder statuses.

`AARCH64Classify` and `AARCH64Decode` are stateless. The latter extracts
operands from supported encodings. Branch immediates are returned as signed
byte displacements; `AARCH64ToIR` resolves the direct `B` target using the
instruction address.

## Implemented classification and operand extraction

- `NOP` and `HINT`
- `B`, `BL`, `BR`, `BLR`, and `RET`
- `B.cond`, `CBZ`, `CBNZ`, `TBZ`, and `TBNZ`
- `MOV` register alias, `MOVN`, `MOVZ`, and `MOVK`
- `ADD`, `ADDS`, `SUB`, and `SUBS` immediate forms
- `ADD`, `ADDS`, `SUB`, and `SUBS` shifted-register forms

This list describes decoder classification and operand extraction, not
executable translation support.

## Translation to IR

Translated: `NOP`, direct `B`, register `BR`, and `RET`. `RET` produces an IR
return using the decoded target register; it is not represented as a `NOP`.

Other recognized instructions, including `BL` and `BLR`, are preserved as
`ARMITO_IR_OP_UNSUPPORTED`. Call/link-register side effects and conditional
branch semantics are not yet modeled. Encodings outside the implemented
classification are reported as unsupported and are preserved in IR when
`record_unsupported` is enabled.

## Validation

Run the host suite with:

```sh
make test
```

The decoder tests cover classification, operand extraction, source bounds,
unsupported-instruction recording, branch stopping, and the return IR
operation. The decoder is not a complete AArch64/ARM64E implementation.

The EDK2 X64 DEBUG and RELEASE builds were validated with the GCC toolchain;
both produced a PE32+ EFI application. This verifies compilation and linking,
not execution on UEFI firmware. Memory, SIMD, pointer authentication, system
instructions, a reference interpreter, native x86_64 code generation, and
Mach-O loading remain unimplemented or unverified.
