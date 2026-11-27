# Armito

Armito is a modular project for translating and preparing operations across processor architectures. Its current foundations include an AArch64 decoder, a universal intermediate representation, CPU state models, and a Core lifecycle that coordinates modules.

## Repository layout

- `Boot/` — firmware entry and boot handoff.
- `Core/` — module lifecycle, configuration, dispatch, and coordination.
- `CPU/` — source-architecture CPU state and architecture-specific behavior.
- `Decoder/` — instruction recognition, validation, and operand decoding.
- `IR/` — the universal intermediate representation.
- `CPUToCPU/`, `GPUToGPU/`, and `JIT/` — destination and translation areas under development.
- `Tests/` — host tests grouped by subsystem; AArch64 fixtures are in `Tests/Fixtures/AARCH64/`.
- `Docs/` — architecture, compatibility, status, decisions, and roadmap documentation.

## Host tests

Run the host suite from the repository root:

```sh
make test
```

The catalog-specific structural and differential checks can also be run with:

```sh
make catalog-test
```

Catalog data and generation metadata remain with the AArch64 decoder. Test-only samples are kept under `Tests/Fixtures/`.

## Documentation

- [Documentation index](Docs/README.md)
- [Current status](Docs/STATUS.md)
- [Architecture](Docs/ARCH.md)
- [Compatibility](Docs/COMPATIBILITY.md)
- [Roadmap](Docs/ROADMAP.md)
- [Known failures and limitations](Docs/FAILS.md)
- [Decisions](Docs/DECISIONS.md)

Recognition and architectural validation do not imply that every catalogued instruction has IR translation or an execution backend. See the status and compatibility documents for implemented coverage.

## License

Copyright © 2026 SoulDotPrime.

Armito is licensed under the GNU General Public License v2.0 only (GPL-2.0-only).

Armito is an original project developed by SoulDotPrime. Independent forks are encouraged to clearly identify themselves as unofficial derivatives and preserve the original attribution.

See "LICENSE" (LICENSE) for the full license terms.
