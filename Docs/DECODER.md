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
- ARM64E: `PACIA/B`, `PACDA/DB`, `PACIZA/ZB`, `PACDZA/DZB`,
  `AUTIA/B`, `AUTDA/DB`, `AUTIZA/ZB`, `AUTDZA/DZB`, `XPACI/XPACD`,
  `PACIASP/PACIBSP`, `AUTIASP/AUTIBSP`, `RETAA/RETAB`, authenticated
  `BRAA/BRAB` and `BLRAA/BLRAB` forms (including zero-modifier variants),
  and `LDRAA/LDRAB` offset-addressing forms. Their pre-index/writeback
  encodings are not decoded as these instructions; with
  `record_unsupported` enabled they are retained as generic unsupported IR.

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

The listed ARM64E instructions are decoded with operands, key, modifier,
and memory/flow metadata, then emitted as dedicated PAC IR operations. They
remain unsupported by translation: emitting the IR does not execute PAC or
provide an x86_64 equivalent. Authenticated branch, call, and return forms
terminate a decode block when `stop_at_branch` is enabled.

## Validation

Run the host suite with:

```sh
make test
```

The decoder tests cover classification, operand extraction, source bounds,
unsupported-instruction recording, branch stopping, and the return IR
operation. The decoder is not a complete AArch64/ARM64E implementation.
ARM64E subtype and Mach-O slice identification are provided by
`Formats/MachO/AARCH64MachO`; this is not a Mach-O loader.

The EDK2 X64 DEBUG and RELEASE builds were validated with GCC in an isolated
workspace; both produced a PE32+ EFI application. This verifies compilation
and linking, not execution on UEFI firmware. Memory, SIMD, system
instructions, a reference interpreter, native x86_64 code generation, PAC
execution, and Mach-O loading remain unimplemented or unverified. PAC decoder
and IR host coverage is tracked by `make arm64e-test`.
